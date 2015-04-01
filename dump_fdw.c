/* ========================================================================= */
/* Copyright (c) 2015 MeetMe, Inc. All Rights Reserved.                      */
/* ========================================================================= */

/**
 * @file      dump_fdw.c
 * @brief     Foreign Data Wrapper for Querying PostgreSQL Dump Files
 * @author    Jonah H. Harris <jonah.harris@meetme.com>
 * @date      2015
 * @copyright The New BSD License
 *
 * NOTE XXX FIX
 *
 *  THIS IS A MIMIMALLY VIABLE PRODUCT AND SHOULD NOT BE USED IN PRODUCTION!
 *
 *  Most buffers are hard-coded with an initial allocation and there isn't
 *  any bounds checking. So, basically, any row > 16MB is going to have an
 *  issue. This will be fixed later, but it works for the MVP as required
 *  for use.
 *
 *  THERE ARE PROBABLY MEMORY LEAKS AND SEGFAULTS AHEAD - THIS IS A PROTOTYPE
 */

#include "postgres.h"

#include <stdio.h>
#include <inttypes.h>
#include <zlib.h>

#include "funcapi.h"
#include "commands/defrem.h"
#include "access/htup_details.h"
#include "access/reloptions.h"
#include "foreign/fdwapi.h"
#include "catalog/pg_foreign_table.h"
#include "foreign/foreign.h"
#include "foreign/fdwapi.h"
#include "optimizer/pathnode.h"
#include "miscadmin.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "utils/builtins.h"
#include "utils/rel.h"

#include "csv_parser.h"

PG_MODULE_MAGIC;

#define ZLIB_IN_SIZE      4096
#define ZLIB_OUT_SIZE     4096
#define MIN(x, y) ((x < y) ? y : x)
#define MAX(x, y) ((x > y) ? y : x)

/**
 * A chained string
 */
typedef struct string_t {
  size_t            length;           /**< length of this segment (in bytes) */
  struct string_t  *next;                   /**< pointer to the next segment */
  char              data[1];                      /**< data for this segment */
} string_t;

/**
 * A field in the TSV
 */
typedef struct field_t {
  string_t             *data;                   /**< the data for this field */
  int                   row;                  /**< the row number in the TSV */
  int                   col;                  /**< the col number in the row */
  struct field_t       *next;                 /**< pointer to the next field */
} field_t;

/**
 * Stashed in fdw_private
 */
typedef struct {
  char                 *file_name;   /**< the path and name of the dump file */
  char                 *schema_name;         /**< the namespace of the table */
  char                 *relation_name;                /**< the table to read */
  BlockNumber           pages;         /**< storage size (in pages) estimate */
  double                ntuples;                   /**< cardinality estimate */
} DumpFdwPlanState;

/**
 * The stateful structure used during query execution.
 */
typedef struct DumpFdwExecutionState {
  char                 *file_name;   /**< the path and name of the dump file */
  char                 *schema_name;         /**< the namespace of the table */
  char                 *relation_name;                /**< the table to read */
  off_t                 offset;               /**< file offset to table data */
  int                   fd;                        /**< dump file descriptor */
  csv_parser_t          parser;                              /**< CSV parser */
  csv_parser_settings_t settings;                   /**< CSV parser settings */
  int                   tuple_count;         /**< cardinality of tuple store */
  bool                  at_eof;         /**< have we hit the end of the data */
  size_t                buflen;    /**< length of compressed data (in bytes) */
  z_streamp             zp;                                 /**< zlib stream */
  char                 *out;                   /**< decompressed data buffer */
  char                 *buf;                     /**< compressed data buffer */
  uint8_t              *tbuf;               /**< scratch buffer for row data */
  uint8_t              *tptr;       /**< pointer to offset in scratch buffer */
  uint8_t              *tend;                 /**< end of the scratch buffer */
  field_t              *head;       /**< head of tuple attribute linked-list */
  field_t              *tail;       /**< tail of tuple attribute linked-list */
  AttInMetadata        *attinmeta;          /**< relation attribute metadata */
  uint8_t              *single_tuple_buffer; /**< buffer for building tuples */
  Tuplestorestate      *tupstore;    /**< store used for decompressed tuples */
  TupleTableSlot       *ts;     /**< temporary slot for use with tuple store */
} DumpFdwExecutionState;

/*
 * SQL functions
 */
extern Datum dump_fdw_handler(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(dump_fdw_handler);
extern Datum dump_fdw_validator(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(dump_fdw_validator);

/* callback functions */
static void dumpGetForeignRelSize (PlannerInfo *, RelOptInfo *, Oid);
static void dumpGetForeignPaths (PlannerInfo *, RelOptInfo *, Oid);
static ForeignScan *dumpGetForeignPlan (PlannerInfo *, RelOptInfo *, Oid,
  ForeignPath *, List *, List *);
static void dumpBeginForeignScan (ForeignScanState *, int);
static TupleTableSlot *dumpIterateForeignScan (ForeignScanState *);
static void dumpReScanForeignScan (ForeignScanState *);
static void dumpEndForeignScan (ForeignScanState *);
static void dumpExplainForeignScan (ForeignScanState *, struct ExplainState *);
static bool dumpAnalyzeForeignTable (Relation, AcquireSampleRowsFunc *,
  BlockNumber *);
string_t *new_string (DumpFdwExecutionState *, const char *, size_t);
field_t *new_field (DumpFdwExecutionState *, const char *, size_t, int, int);
static void dumpGetOptions (Oid, char **, char **, char **);
static inline int read_int (int);
static inline char *read_str (int, bool);
static inline size_t pgdump_read (int, char **, size_t *);
static void read_toc (DumpFdwExecutionState *);
int field_cb (csv_parser_t *, const char *, size_t, int, int);

/* ========================================================================= */
/* -- EXPORTED FUNCTIONS --------------------------------------------------- */
/* ========================================================================= */

Datum
dump_fdw_handler (
  PG_FUNCTION_ARGS
) {
  FdwRoutine *fdwroutine = makeNode(FdwRoutine);

  /* Required callbacks */
  fdwroutine->GetForeignRelSize = dumpGetForeignRelSize;
  fdwroutine->GetForeignPaths = dumpGetForeignPaths;
  fdwroutine->GetForeignPlan = dumpGetForeignPlan;
  fdwroutine->BeginForeignScan = dumpBeginForeignScan;
  fdwroutine->IterateForeignScan = dumpIterateForeignScan;
  fdwroutine->ReScanForeignScan = dumpReScanForeignScan;
  fdwroutine->EndForeignScan = dumpEndForeignScan;

  /* Optional (modification-related) callbacks */
  fdwroutine->AddForeignUpdateTargets = NULL;
  fdwroutine->PlanForeignModify = NULL;
  fdwroutine->BeginForeignModify = NULL;
  fdwroutine->ExecForeignInsert = NULL;
  fdwroutine->ExecForeignUpdate = NULL;
  fdwroutine->ExecForeignDelete = NULL;
  fdwroutine->EndForeignModify = NULL;

  /* EXPLAIN-related callbacks */
  fdwroutine->ExplainForeignScan = dumpExplainForeignScan;
  fdwroutine->ExplainForeignModify = NULL;

  /* ANALYZE-related callbacks */
  fdwroutine->AnalyzeForeignTable = dumpAnalyzeForeignTable;

  PG_RETURN_POINTER(fdwroutine);

} /* dump_fdw_handler() */

/* ------------------------------------------------------------------------- */

Datum
dump_fdw_validator (
  PG_FUNCTION_ARGS
) {
  List         *options_list = untransformRelOptions(PG_GETARG_DATUM(0));
  Oid           catalog = PG_GETARG_OID(1);
  char         *file_name = NULL;
  char         *schema_name = NULL;
  char         *relation_name = NULL;
  ListCell     *cell;

  PG_RETURN_VOID();

  /* Only permit superusers to set the options on a dump_fdw table */
  if (ForeignTableRelationId == catalog
      && false == superuser()) {
    ereport(ERROR,
      (errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
        errmsg("only a superuser can set options of a dump_fdw table")));
  }

  foreach(cell, options_list) {
    DefElem    *def = (DefElem *) lfirst(cell);

    if (0 == strcmp(def->defname, "file_name")) {
      if (NULL != file_name) {
        ereport(ERROR,
          (errcode(ERRCODE_SYNTAX_ERROR),
            errmsg("conflicting or redundant options")));
      }
      file_name = defGetString(def);
    } else if (0 == strcmp(def->defname, "schema_name")) {
      if (NULL != schema_name) {
        ereport(ERROR,
          (errcode(ERRCODE_SYNTAX_ERROR),
            errmsg("conflicting or redundant options")));
      }
      schema_name = defGetString(def);
    } else if (0 == strcmp(def->defname, "relation_name")) {
      if (NULL != relation_name) {
        ereport(ERROR,
          (errcode(ERRCODE_SYNTAX_ERROR),
            errmsg("conflicting or redundant options")));
      }
      relation_name = defGetString(def);
    } else {
      ereport(ERROR,
        (errcode(ERRCODE_FDW_INVALID_OPTION_NAME),
          errmsg("unsupported option \"%s\"", def->defname)));
    }
  }

  /*
   * file_name option is required for dump_fdw foreign tables.
   */
  if (!(ForeignTableRelationId == catalog
        && NULL != file_name
        && NULL != schema_name
        && NULL != relation_name)) {
    ereport(ERROR,
      (errcode(ERRCODE_FDW_DYNAMIC_PARAMETER_VALUE_NEEDED),
        errmsg("file_name, schema_name, and relation_name options are"
               " required for dump_fdw foreign tables")));
  }

  /*
   * NOTE XXX FIX
   *
   * Rather than continuing here, we should probably validate the dump file
   * as containing the specified relation. As this will require a little
   * refactoring, it's not part of the MVP.
   */

  PG_RETURN_VOID();

} /* dump_fdw_validator */

/* ========================================================================= */
/* -- STATIC FUNCTIONS ----------------------------------------------------- */
/* ========================================================================= */

static void
dumpGetOptions (
  Oid                   foreigntableid,
  char                **file_name,
  char                **schema_name,
  char                **relation_name
) {
  ForeignTable         *table;
  ForeignServer        *server;
  ForeignDataWrapper   *wrapper;
  List                 *options;
  ListCell             *lc;
  ListCell             *prev;

  table = GetForeignTable(foreigntableid);
  server = GetForeignServer(table->serverid);
  wrapper = GetForeignDataWrapper(server->fdwid);

  options = NIL;
  options = list_concat(options, wrapper->options);
  options = list_concat(options, server->options);
  options = list_concat(options, table->options);

  /*
   * Separate out the file, schema, and relation names.
   */
  *file_name = NULL;
  *schema_name = NULL;
  *relation_name = NULL;
  prev = NULL;
  foreach(lc, options)
  {
    DefElem    *def = (DefElem *) lfirst(lc);

    if (0 == strcmp(def->defname, "file_name")) {
      *file_name = defGetString(def);
    } else if (0 == strcmp(def->defname, "schema_name")) {
      *schema_name = defGetString(def);
    } else if (0 == strcmp(def->defname, "relation_name")) {
      *relation_name = defGetString(def);
    }
    prev = lc;
  }

  /* The validator should have prevented this but, check again, just in case */
  if (NULL == *file_name) {
    elog(ERROR, "file_name is required for dump_fdw foreign tables");
  }
  if (NULL == *schema_name) {
    elog(ERROR, "schema_name is required for dump_fdw foreign tables");
  }
  if (NULL == *relation_name) {
    elog(ERROR, "relation_name is required for dump_fdw foreign tables");
  }
} /* dumpGetOptions() */

/* ------------------------------------------------------------------------- */

static void
dumpGetForeignRelSize (
  PlannerInfo          *root,
  RelOptInfo           *baserel,
  Oid                   foreigntableid
) {
  DumpFdwPlanState *fdw_private;

  elog(LOG, "%s is currently unimplemented", __func__);

  baserel->rows = 0;

  fdw_private = palloc0(sizeof(DumpFdwPlanState));
  baserel->fdw_private = (void *) fdw_private;
} /* dumpGetForeignRelSize() */

/* ------------------------------------------------------------------------- */

static void
dumpGetForeignPaths (
  PlannerInfo          *root,
  RelOptInfo           *baserel,
  Oid                   foreigntableid
) {
  Cost startup_cost;
  Cost total_cost;

  elog(LOG, "%s is currently unimplemented", __func__);

  startup_cost = 0;
  total_cost = startup_cost + baserel->rows;

  /* Create a ForeignPath node and add it as only possible path */
  add_path(baserel, (Path *)
       create_foreignscan_path(root, baserel,
                   baserel->rows,
                   startup_cost,
                   total_cost,
                   NIL,    /* no pathkeys */
                   NULL,    /* no outer rel either */
                   NIL));    /* no fdw_private data */

} /* dumpGetForeignPaths() */

/* ------------------------------------------------------------------------- */

static ForeignScan *
dumpGetForeignPlan (
  PlannerInfo          *root,
  RelOptInfo           *baserel,
  Oid                   foreigntableid,
  ForeignPath          *best_path,
  List                 *tlist,
  List                 *scan_clauses
) {
  Index scan_relid = baserel->relid;

  elog(LOG, "%s is currently unimplemented", __func__);

  scan_clauses = extract_actual_clauses(scan_clauses, false);

  /* Create the ForeignScan node */
  return make_foreignscan(tlist,
              scan_clauses,
              scan_relid,
              NIL,  /* no expressions to evaluate */
              NIL);    /* no private state either */

} /* dumpGetForeignPlan() */

/* ------------------------------------------------------------------------- */

static void
dumpBeginForeignScan (
  ForeignScanState     *node,
  int                   eflags
) {
  DumpFdwExecutionState *festate;

  festate = (DumpFdwExecutionState *) palloc(sizeof(DumpFdwExecutionState));

  dumpGetOptions(RelationGetRelid(node->ss.ss_currentRelation),
    &festate->file_name, &festate->schema_name, &festate->relation_name);

  festate->tupstore = tuplestore_begin_heap(true, false, 32768);
  festate->single_tuple_buffer = palloc(1024 * 1024 * 16);

  festate->zp = (z_streamp) palloc(sizeof(z_stream));
  festate->zp->zalloc = Z_NULL;
  festate->zp->zfree = Z_NULL;
  festate->zp->opaque = Z_NULL;

  festate->buf = palloc(ZLIB_IN_SIZE);
  festate->buflen = ZLIB_IN_SIZE;

  festate->out = palloc(ZLIB_OUT_SIZE + 1);

  festate->settings.delimiter = '\t';
  festate->settings.field_cb = field_cb;

  csv_parser_init(&festate->parser);
  festate->parser.data = festate;
  festate->tbuf = palloc(1024 * 1024 * 32);
  festate->tptr = festate->tbuf;
  festate->tend = festate->tbuf + (1024 * 1024 * 32);

  festate->head = NULL;
  festate->tail = NULL;
  festate->at_eof = false;
  festate->tuple_count = 0;
  festate->offset = 0;

  festate->fd = open(festate->file_name, O_RDONLY);
  if (0 > festate->fd) {
    elog(ERROR, "Failed to open file (%s)", festate->file_name);
  }

  read_toc(festate);

  if (0 > lseek(festate->fd, festate->offset + 6, SEEK_SET)) {
    elog(ERROR, "Failed to seek to offset (%s)", festate->file_name);
  }

  if (Z_OK != inflateInit(festate->zp)) {
    elog(ERROR, "could not initialize compression library: %s",
      festate->zp->msg);
  }

  festate->attinmeta =
    TupleDescGetAttInMetadata(node->ss.ss_ScanTupleSlot->tts_tupleDescriptor);
  festate->ts =
    MakeSingleTupleTableSlot(node->ss.ss_ScanTupleSlot->tts_tupleDescriptor);

  node->fdw_state = (void *) festate;

} /* dumpBeginForeignScan() */

/* ------------------------------------------------------------------------- */

static TupleTableSlot *
dumpIterateForeignScan (
  ForeignScanState     *node
) {
  DumpFdwExecutionState  *festate = (DumpFdwExecutionState *) node->fdw_state;
  TupleTableSlot         *slot = node->ss.ss_ScanTupleSlot;
  TupleDesc               tupleDescriptor = slot->tts_tupleDescriptor;
  Datum                  *columnValues = slot->tts_values;
  bool                   *columnNulls = slot->tts_isnull;
  int                     columnCount = tupleDescriptor->natts;

  /* initialize all values for this row to null */
  memset(columnValues, 0, columnCount * sizeof(Datum));
  memset(columnNulls, true, columnCount * sizeof(bool));
  ExecClearTuple(slot);

  /*
   * Loop until we either reach a full tuple or EOF.
   */
  while (!(festate->at_eof || (0 != festate->tuple_count))) {
    int res = Z_OK;
    size_t cnt = 0;
    while ((cnt = pgdump_read(festate->fd, &festate->buf, &festate->buflen))) {
      festate->zp->next_in = (void *) festate->buf;
      festate->zp->avail_in = cnt;
      while (festate->zp->avail_in > 0) {
        festate->zp->next_out = (void *) festate->out;
        festate->zp->avail_out = ZLIB_OUT_SIZE;

        res = inflate(festate->zp, 0);
        if (Z_OK != res && Z_STREAM_END != res) {
          elog(ERROR, "could not uncompress data: %s", festate->zp->msg);
        }

        festate->out[ZLIB_OUT_SIZE - festate->zp->avail_out] = '\0';
        csv_parser_execute(&festate->parser, &festate->settings, festate->out,
          (ZLIB_OUT_SIZE - festate->zp->avail_out));
      }
      if (0 != festate->tuple_count) {
        goto post_loop;
      }
    }

    festate->at_eof = true;
    festate->zp->next_in = NULL;
    festate->zp->avail_in = 0;

    while (Z_STREAM_END != res) {
      festate->zp->next_out = (void *) festate->out;
      festate->zp->avail_out = ZLIB_OUT_SIZE;

      res = inflate(festate->zp, 0);
      if (Z_OK != res && Z_STREAM_END != res) {
        elog(ERROR, "could not uncompress data: %s", festate->zp->msg);
      }

      festate->out[ZLIB_OUT_SIZE - festate->zp->avail_out] = '\0';
      csv_parser_execute(&festate->parser, &festate->settings, festate->out,
        (ZLIB_OUT_SIZE - festate->zp->avail_out));
    }

    if (Z_OK != inflateEnd(festate->zp)) {
      elog(ERROR, "could not close compression library: %s",
        festate->zp->msg);
    }
  }

post_loop:

  if (0 != festate->tuple_count) {
    TupleTableSlot *ts = festate->ts;
    if (true == tuplestore_gettupleslot(festate->tupstore, true, false, ts)) {
      heap_deform_tuple(ts->tts_tuple, ts->tts_tupleDescriptor,
        columnValues, columnNulls);
      ExecStoreVirtualTuple(slot);
    } else {
      elog(ERROR, "Issue getting tuple from store");
    }
    --festate->tuple_count;
    if (0 == festate->tuple_count) {
      tuplestore_trim(festate->tupstore);
    }
  }

  /* then return the slot */
  return slot;

} /* dumpIterateForeignScan() */

/* ------------------------------------------------------------------------- */

static void
dumpReScanForeignScan (
  ForeignScanState     *node
) {
  DumpFdwExecutionState  *festate = (DumpFdwExecutionState *) node->fdw_state;

  festate->tptr = festate->tbuf;
  festate->head = NULL;
  festate->tail = NULL;
  festate->at_eof = false;
  festate->tuple_count = 0;
  tuplestore_clear(festate->tupstore);
  csv_parser_init(&festate->parser);
  festate->parser.data = festate;

  if (0 > lseek(festate->fd, festate->offset + 6, SEEK_SET)) {
    elog(ERROR, "Failed to seek to offset (%s)", festate->file_name);
  }

  if (Z_OK != inflateInit(festate->zp)) {
    elog(ERROR, "could not initialize compression library: %s",
      festate->zp->msg);
  }

} /* dumpReScanForeignScan() */

/* ------------------------------------------------------------------------- */

static void
dumpEndForeignScan (
  ForeignScanState     *node
) {
  DumpFdwExecutionState  *festate = (DumpFdwExecutionState *) node->fdw_state;

  pfree(festate->file_name);
  pfree(festate->schema_name);
  pfree(festate->relation_name);
  pfree(festate->single_tuple_buffer);
  pfree(festate->zp);
  pfree(festate->buf);
  pfree(festate->out);
  pfree(festate->tbuf);

  close(festate->fd);

  ExecDropSingleTupleTableSlot(festate->ts);
  tuplestore_end(festate->tupstore);

  pfree(festate);

} /* dumpEndForeignScan() */

/* ------------------------------------------------------------------------- */

static void
dumpExplainForeignScan (
  ForeignScanState     *node,
  struct ExplainState  *es
) {
  elog(LOG, "%s is currently unimplemented", __func__);
} /* dumpExplainForeignScan() */

/* ------------------------------------------------------------------------- */

static bool
dumpAnalyzeForeignTable (
  Relation                relation,
  AcquireSampleRowsFunc  *func,
  BlockNumber            *totalpages
) {
  elog(LOG, "%s is currently unimplemented", __func__);

  return false;

} /* dumpAnalyzeForeignTable() */

/* ------------------------------------------------------------------------- */

static inline int
read_int (
  int                   fd
) {
  uint8_t sign = 0;
  read(fd, &sign, 1);
  unsigned val = 0;
  read(fd, &val, 4);
  return (0 == sign) ? val : (-val);
}

/* ------------------------------------------------------------------------- */

static inline char *
read_str (
  int                   fd,
  bool                  skip
) {
  int len = read_int(fd);
  if (-1 == len) {
    return NULL;
  } else {
    if (skip) {
      lseek(fd, len, SEEK_CUR);
      return NULL;
    }
    char *str = palloc0(len + 1);
    read(fd, str, len);
    return str;
  }
}

/* ------------------------------------------------------------------------- */

static inline size_t
pgdump_read (
  int                   fd,
  char                **buf,
  size_t               *buflen
) {
  int    blkLen;

  /* Read length */
  blkLen = read_int(fd);
  if (0 == blkLen) {
    return 0;
  }

  /* If the caller's buffer is not large enough, allocate a bigger one */
  if (blkLen > *buflen)
  {
    elog(WARNING, "having to re-alloc buf");
    pfree(*buf);
    *buf = (char *) palloc(blkLen);
    *buflen = blkLen;
  }

  /* exits app on read errors */
  read(fd, *buf, blkLen);

  return blkLen;

} /* pgdump_read() */

/* ------------------------------------------------------------------------- */

static void
read_toc (
  DumpFdwExecutionState  *festate
) {
  char buf[80] = { 0 };
  uint8_t version_major;
  uint8_t version_minor;
  uint8_t version_revision;
  uint8_t size_of_int;
  uint8_t size_of_offset;
  uint8_t format;
  int     compression;

  if (5 != read(festate->fd, buf, 5)) {
    elog(ERROR, "Failed to read dump file header");
  }
  if (0 != memcmp(buf, "PGDMP", 5)) {
    elog(ERROR, "This doesn't look like a Postgres dump file");
  }

  if (6 != read(festate->fd, buf, 6)) {
    elog(ERROR, "Failed to read dump file header");
  }

  sscanf(&buf[0], "%c", &version_major);
  sscanf(&buf[1], "%c", &version_minor);
  sscanf(&buf[2], "%c", &version_revision);
  sscanf(&buf[3], "%c", &size_of_int);
  sscanf(&buf[4], "%c", &size_of_offset);
  sscanf(&buf[5], "%c", &format);

  /* It wouldn't be hard to support much else, but this is an MVP */
  if (!(1 <= version_major
      && 10 <= version_minor
      && 4 == size_of_int
      && 8 == size_of_offset
      && 1 == format)) {
    elog(ERROR, "Unsupported Postgres dump file format");
  }

  compression = read_int(festate->fd);
  if (-1 != compression) {
    elog(ERROR, "Unsupported Postgres dump compression");
  }

  (void) read_int(festate->fd); /* tm_sec */
  (void) read_int(festate->fd); /* tm_min */
  (void) read_int(festate->fd); /* tm_hour */
  (void) read_int(festate->fd); /* tm_mday */
  (void) read_int(festate->fd); /* tm_mon */
  (void) read_int(festate->fd); /* tm_year */
  (void) read_int(festate->fd); /* tm_isdst */

  (void) read_str(festate->fd, true); /* dbname */
  (void) read_str(festate->fd, true); /* remote_version */
  (void) read_str(festate->fd, true); /* version */

  int toc_entry_count = read_int(festate->fd);
  int ii;
  for (ii = 0; ii < toc_entry_count; ++ii) {
    (void) read_int(festate->fd); /* dump_id */
    size_t dumper = read_int(festate->fd);
    if (!(dumper == 0 || dumper == 1)) {
      elog(ERROR, "Unsupported Postgres dumper");
    }

    (void) read_str(festate->fd, true); /* table_oid */
    (void) read_str(festate->fd, true); /* oid */
    char *tag = read_str(festate->fd, false);

    (void) read_str(festate->fd, true); /* desc */
    int section = read_int(festate->fd); /* section */

    (void) read_str(festate->fd, true); /* definition */
    (void) read_str(festate->fd, true); /* drop statement */
    (void) read_str(festate->fd, true); /* copy statement */

    char *namespace = read_str(festate->fd, false);

    (void) read_str(festate->fd, true); /* tablespace */
    (void) read_str(festate->fd, true); /* owner */
    (void) read_str(festate->fd, true); /* with oids */

    char *deps = read_str(festate->fd, false);
    while (NULL != deps) {
      pfree(deps);
      deps = read_str(festate->fd, false);
    }

    if (1 != read(festate->fd, buf, 1)) {
      elog(ERROR, "Failed to read offset setting");
    }
    uint8_t offset_was_set = 0;
    uint64_t offset = 0;
    sscanf(&buf[0], "%c", &offset_was_set);
    if (offset_was_set) {
      if (8 != read(festate->fd, &offset, 8)) {
        elog(ERROR, "Failed to read offset");
      }
    }

    if (tag
        && namespace
        && 3 == section
        && 0 == memcmp(festate->schema_name, namespace,
          MIN(strlen(namespace), strlen(festate->schema_name)))
        && 0 == memcmp(festate->relation_name, tag,
          MIN(strlen(tag), strlen(festate->relation_name)))) {
      festate->offset = offset;
      break;
    }
  }

  if (0 == festate->offset) {
    elog(ERROR, "Relation \"%s.%s\" does not exist in dump file.",
      festate->schema_name, festate->relation_name);
  }

} /* read_toc() */

/* ------------------------------------------------------------------------- */

string_t *
new_string (
  DumpFdwExecutionState  *festate,
  const char             *data,
  size_t                  length
) {
  string_t *str = (string_t *) festate->tptr;
  str->length = length;
  str->next = NULL;
  memcpy(str->data, data, length);
  festate->tptr += sizeof(size_t) + sizeof(string_t *) + length;

  return str;

} /* new_string() */

/* ------------------------------------------------------------------------- */

field_t *
new_field (
  DumpFdwExecutionState  *festate,
  const char             *data,
  size_t                  length,
  int                     row,
  int                     col
) {
  field_t *field = (field_t *) festate->tptr;
  memset(field, 0, sizeof(field_t));
  field->row = row;
  field->col = col;
  field->next = NULL;
  festate->tptr += sizeof(field_t);
  field->data = new_string(festate, data, length);

  return field;

} /* new_field() */

/* ------------------------------------------------------------------------- */

int
field_cb (
  csv_parser_t         *parser,
  const char           *data,
  size_t                length,
  int                   row,
  int                   col
) {
  DumpFdwExecutionState  *festate = parser->data;
  field_t                *field;

  if (festate->tptr >= festate->tend) {
    elog(PANIC, "if we haven't crashed by now, we should have!");
  }
  if (!festate->tail) {
    /* This is the a new row */
    field = new_field(festate, data, length, row, col);
    festate->tail = field;
    festate->head = field;
  } else {
    if (festate->tail->row == row && festate->tail->col == col) {
      /* This is the same field as the last callback */
      string_t *str = new_string(festate, data, length);
      festate->tail->data->next = str;
    } else if (festate->tail->row == row) {
      /* This is the a new field in the same row as the last callback */
      field = new_field(festate, data, length, row, col);
      festate->tail->next = field;
      festate->tail = field;
    } else {
      /* This is a new row */
      char *cstr[1600] = { NULL };
      char *ptr = (char *) festate->single_tuple_buffer;
      int ii = 0;
      field_t *tfld = festate->head;
      while (tfld) {
        cstr[ii] = ptr;
        string_t *str = tfld->data;
        while (NULL != str) {
          memmove(ptr, str->data, str->length);
          ptr += str->length;
          str = str->next;
        }
        *ptr = '\0';
        ++ptr;
        ++ii;
        tfld = tfld->next;
      }
      HeapTuple ret_tuple = BuildTupleFromCStrings(festate->attinmeta, cstr);
      tuplestore_puttuple(festate->tupstore, ret_tuple);
      heap_freetuple(ret_tuple);
      ++festate->tuple_count;
      festate->tptr = festate->tbuf;

      field = new_field(festate, data, length, row, col);
      festate->tail = field;
      festate->head = field;
    }
  }

  return 0;

} /* field_cb() */
/* vi: set et sw=2 ts=2: */


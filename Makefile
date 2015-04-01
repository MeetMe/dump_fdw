# contrib/dump_fdw/Makefile

MODULE_big = dump_fdw

EXTENSION = dump_fdw
DATA = dump_fdw--1.0.sql

SHLIB_LINK = -lz
OBJS = dump_fdw.o csv_parser.o
REGRESS = dump_fdw

EXTRA_CLEAN = sql/dump_fdw.sql expected/dump_fdw.out

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/dump_fdw
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

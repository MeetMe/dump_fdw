Postgres Dump Foreign Data Wrapper
==================================

This extension implements a foreign data wrapper for PostgreSQL capable of
querying data directly from files in PostgreSQL's custom dump format. This can
be used to restore row-specific data.

**Note.** This has only been tested with PostgreSQL 9.1/9.2/9.3/9.4 and is
currently a prototype. **DO NOT RUN IT ON A PRODUCTION SERVER**.


Introduction
------------

This extension uses the PostgreSQL Custom Dump Format and brings the
following benefits:

* Analytics - Perform queries directly from your backups.
* Selective Restore - Restore only the rows you need directly from your backups.


Building
--------

Use the following commands to compile and install dump_fdw:

    # To compile
    USE_PGXS=1 make
    USE_PGXS=1 make install

    # To run the test
    USE_PGXS=1 make installcheck


Example
-------

As an example, we demonstrate querying data from the BookTown database
(provided by Command Prompt, Inc.):

Log into Postgres, and run the following commands to install the FDW:

```SQL
-- load extension first time after install
CREATE EXTENSION dump_fdw;

-- create server object
CREATE SERVER dump_server FOREIGN DATA WRAPPER dump_fdw;
```

Next, we create our foreign tables mapped to the pg_dump file:

```SQL
-- create foreign table(s)
CREATE FOREIGN TABLE authors (
  id            INTEGER,
  last_name     TEXT,
  first_name    TEXT
) SERVER dump_server
  OPTIONS (
    file_name '/home/jharris/data/booktown.dmp',
    schema_name 'public',
    relation_name 'authors'
  )
;

CREATE FOREIGN TABLE books (
  id            INTEGER,
  title         TEXT,
  author_id     INTEGER,
  subject_id    INTEGER
) SERVER dump_server
  OPTIONS (
    file_name '/home/jharris/data/booktown.dmp',
    schema_name 'public',
    relation_name 'books'
  )
;
```

Finally, let's run an example SQL query on the dumped tables:

```SQL
-- query the data
SELECT *
 FROM books b
 JOIN ONLY authors a
      ON (a.id = b.author_id)
ORDER BY b.id;
```

Uninstalling dump_fdw
-----------------------

Then you can drop the extension:

    postgres=# DROP EXTENSION dump_fdw CASCADE;

Finally, to uninstall the extension you can run the following command in the
extension's source code directory. This will clean up all the files copied during
the installation:

    $ USE_PGXS=1 make uninstall


Copyright
---------

Foreign Data Wrapper

* Copyright (c) 2015 MeetMe, Inc.

CSV Parser

* Copyright (C) 2013 Tadas Vilkeliskis <vilkeliskis.t@gmail.com>


License
-------

The New BSD License. See LICENSE


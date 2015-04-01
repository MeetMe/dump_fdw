/* contrib/dump_fdw/dump_fdw--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION dump_fdw" to load this file. \quit

CREATE FUNCTION dump_fdw_handler()
RETURNS fdw_handler
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FUNCTION dump_fdw_validator(text[], oid)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FOREIGN DATA WRAPPER dump_fdw
  HANDLER dump_fdw_handler
  VALIDATOR dump_fdw_validator;

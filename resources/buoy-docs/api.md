---
title: "odbckit API contracts"
summary: "Usage and behavioral contracts for odbckit."
category: "Reference"
symbols: ["OdbcException", "OdbcRowSet", "OdbcDatabase"]
related: ["index.md", "getting-started.md", "typed-api.md"]
---

# ODBCKit 0.1.0

`Import odbckit` provides an ODBC 3 backend for Buoy's `Database` and `RowSet`.
It prepares SQL and binds parameters with `SQLBindParameter`; parameter values
never become SQL text. A small C bridge uses only the public ODBC API.

Copy `packages/odbckit-0.1.0.buipkg` into a consumer's `packages/` directory.
The included native bridge is Linux x86_64. Install a compatible system ODBC
driver manager and the database's ODBC driver separately. Importing this package
does not install drivers, create a DSN, open a connection or prompt the user.

See the compiled SQLite ODBC example and
complete runtime consumer. With an installed
MySQL ODBC driver the connection may target MySQL; this package is **not** a
standalone native MySQL backend and MySQL execution has not been tested.

## API

- `New OdbcDatabase()`, `Connect(connectionString)`, `Disconnect()`,
  `IsConnected()`: the driver owns connection-string syntax. Connections are
  noninteractive (`SQL_DRIVER_NOPROMPT`). Disconnect is idempotent; reconnecting
  discards the old connection. `IsConnected` reports local handle state and is
  not a network health check.
- `Query(sql)` / `Query(sql, params As Array Of DbValue)` return a `RowSet`.
  `QueryOdbc` has the same overloads returning `OdbcRowSet`.
- `Execute(sql[, params])` closes the result and returns the affected row count.
  A driver may report `-1` when this count is unknown.
- `OdbcRowSet` implements the standard zero-based `ColumnCount`, `ColumnName`,
  `Value(index/name)`, `HasMoreRows`, `NextRow`, and `Close` methods. `RowCount`
  reports buffered rows; `AffectedRows` reports the driver's statement count.
  A fresh result is already positioned on its first row. Names match exactly,
  including case; duplicate matching names reject rather than picking a column.
- `IdentityQuery` is an explicit SQL string. `LastInsertId()` executes it and
  requires exactly one row and one integer column. It throws if unconfigured;
  ODBC has no portable identity query. For SQLite, a tested setting is
  `SELECT CAST(last_insert_rowid() AS BIGINT)`. Configure this **before** using
  inherited `Insert` with auto-increment records: otherwise the insert may
  succeed before identity retrieval throws. Identity SQL and connection strings
  are trusted application configuration, never user input.
- `BeginOdbcTransaction`, `CommitOdbcTransaction`, `RollbackOdbcTransaction`
  use driver-manager autocommit and `SQLEndTran`, independent of SQL dialect.
  Nested transactions and commit/rollback without an active ODBC transaction
  reject. Disconnect attempts rollback before releasing the connection; driver failures
  during cleanup cannot be reported from destruction.
  On a failed commit/rollback or autocommit restoration, stop using the
  connection and reconnect before proceeding because transaction state may be
  uncertain.
- `OdbcException` extends `DatabaseException`, exposing `SqlState` and inherited
  native `ErrorCode`. Messages contain the operation and SQLSTATE, excluding
  driver diagnostic text to avoid echoing credentials or SQL parameter values.

The inherited typed record layer (`Query Of T`, `Insert`, `Update`) works through
`Database`. Its transaction conveniences issue SQL such as `BEGIN TRANSACTION`;
these are available only when the selected database accepts that SQL dialect.
Use the explicit ODBC transaction methods for driver-independent transactions,
and do not mix the two transaction styles on one connection. Buoy's inherited
async methods retain the toolchain's current synchronous behavior.

## Types and text contract

Bindings accept `DbValue` NULL, signed 64-bit integer, Double, Boolean and String.
ODBC integer columns become integers, floating columns Doubles, `SQL_BIT` Boolean,
and other nonbinary columns Strings. DECIMAL/NUMERIC, dates and timestamps stay
text, avoiding an implicit decimal-to-Double precision loss. That text is whatever
the driver renders: SQLite ODBC 0.99991 reports `DECIMAL(20,4)` as `SQL_VARCHAR`
and returns `12345678901234.1234` as `12345678901234.1`, digits SQLite never
stored. ODBCKit cannot restore them. Integer and `SQL_BIT` columns are read as
text and parsed strictly; a stored `1.5` or `'abc'` in an INTEGER-declared SQLite
column rejects instead of becoming `1` or NULL. Actual SQL column
metadata comes from the driver; an expression's result type can differ from the
underlying column's type. Binary columns reject explicitly. Output parameters,
BLOB streaming, multiple result sets, reusable prepared statement objects and
query cancellation are outside this version. Only the first result is read;
a driver may execute additional SQL statements even though later results are
not exposed.

This release uses ODBC's **narrow character API**. Use drivers configured to send
and receive UTF-8 through that API; the package does not change process locale or
set a driver's encoding options. Returned column names and received text bytes
are validated as UTF-8. Invalid received UTF-8, embedded NUL reported within a
text cell, truncation, and execution/fetch/conversion warnings reject. A rejected
execution warning does **not** mean the statement had no effect: in autocommit mode
the driver has already run it, so do not blindly retry DML after such an error.
A searched UPDATE/DELETE that matches no rows returns 0 affected rows whether the
driver reports success or `SQL_NO_DATA`. This does
not establish Unicode correctness for Windows ANSI-codepage drivers or any
untested driver. Wide-character ODBC support remains a separate follow-up.

Text must be NUL-free end to end. A driver can lose data before ODBCKit sees it:
SQLite ODBC 0.99991 reports `CAST(X'610062' AS TEXT)` as one byte `a`, even with
`SQL_C_BINARY` retrieval. Its reported length already excludes the suffix, so
ODBCKit cannot detect this truncation. The native test records this driver
boundary explicitly. Likewise the current Buoy runtime may discard NUL during
String construction. Do not use this text API to preserve arbitrary bytes or
NUL-containing database text; it cannot guarantee that loss will be detected.

## Bounds and ownership

Limits are 4096 parameters, 1 MiB SQL, 32767 connection-string bytes, 1 MiB per text
parameter/cell, 32 MiB combined text parameters, 1024 columns, 1024 UTF-8 bytes
per column name, and 100000 result rows. The native cell array and combined
result text each have a separate 32 MiB ceiling; metadata, retained parameters,
one temporary text buffer, allocator overhead and driver allocations are extra.
These are package buffer limits, not a process memory limit: an ODBC driver may
buffer the entire server result internally. Parameter/string validation also
uses temporary Buoy byte buffers.

Results are completely buffered before `Query` returns, and all native statements
are then finalized. Results remain readable after database disconnect/destruction.
`Close` is idempotent and the destructor closes automatically. A failure returns
no partial result. Use each database/result on one thread or synchronize all
access externally; no background worker or concurrent-use guarantee is provided.
Connections must be configured with driver-specific connection/query timeouts
where needed; this version does not impose execution deadlines.

## Build and verify

On Debian/Ubuntu the host prerequisites are a C compiler, `unixodbc-dev` and
`libsqliteodbc`. These are system prerequisites, not bundled dependencies.

```sh
python3 tools/build_odbckit.py
```

Build receipts bind the native binary to the current bridge, Buoy binding module
and builder source hashes. Sealing rejects a missing or stale receipt or changed
binary, and verifies the module/binary/receipt again inside the finished archive.
The receipt ships under `resources/native-build/`. Use the builder to refresh
the bridge and its receipt together.

`BUOY`, `BPM_PACKAGER`, `CC` and `ODBC_SQLITE_DRIVER` override the compiler,
packager, native compiler and absolute SQLite ODBC driver library path. The
builder packages only the bridge built for the current host; rebuilding on
another platform produces that platform's archive rather than silently including
stale native slices. macOS requires unixODBC development files and `pkg-config`;
the Windows builder expects a GCC-compatible compiler and system `odbc32`.

Native execution is verified with Buoy 0.72, unixODBC 2.3.12 and SQLite ODBC
0.99991 on Linux x86_64. Other targets have compiler checks only. Their native
bridge binaries and real driver behavior still need target-host validation.

[Typed API](typed-api.md) · [Getting started](getting-started.md)

Builder context: run shared `tools/` commands from the assembled
[BuoyBay workspace](https://github.com/BuoyBay/workspace-tools). The scripts live
in [devtools](https://github.com/BuoyBay/devtools), alongside the shared build inputs.

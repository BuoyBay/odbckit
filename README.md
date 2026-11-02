# ODBC Database and RowSet

`Import odbckit` provides an ODBC 3 backend for the `Database` and `RowSet` API.
It prepares SQL and binds parameters with `SQLBindParameter`; parameter values
never become SQL text. A small C bridge uses only the public ODBC API.

Copy `packages/odbckit-0.1.0.buipkg` into a consumer's `packages/` directory.
The included native bridge is Linux x86_64. Install a compatible system ODBC
driver manager and the database's ODBC driver separately. Importing this package
does not install drivers, create a DSN, open a connection or prompt the user.

## Install

From a Buoy consumer project with BPM installed:

```sh
bpm add odbckit --git https://github.com/BuoyBay/odbckit.git --rev refs/heads/main
```

BPM records the resolved commit in its lockfile. Keep that lockfile with the
consumer project for reproducible installs.

## API at a glance

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

## Build and API reference

Source is in `modules/`, with native/runtime inputs where required. Use the
[package guide](docs/API.md) for backend-specific build prerequisites, ownership
rules and supported targets. Rebuilding a native backend requires that target's
compiler and SDK; sealing an archive alone does not rebuild the backend.

## Local Buoy setup

On your own machine, install Buoy and run:

```sh
buoy ai init --both
```

## Contributing

[Contributing](CONTRIBUTING.md) · [License](LICENSE)

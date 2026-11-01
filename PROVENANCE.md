# ODBCKit provenance

The Buoy module, native bridge and tests are independent implementations in this
workspace. No upstream database client or driver source is copied or vendored.
The sealed package contains our bridge and module, not unixODBC, SQLite ODBC,
MySQL client libraries, database drivers or their headers.

The bridge follows the public ODBC 3 API, using platform-installed `sql.h` and
`sqlext.h` headers when built. Primary API references:

- [ODBC API reference](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/odbc-api-reference)
- [SQLBindParameter](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlbindparameter-function)
- [SQLGetData](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetdata-function)
- [Connecting with SQLDriverConnect](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/connecting-with-sqldriverconnect)
- [SQLEndTran](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlendtran-function)
- [unixODBC](https://www.unixodbc.org/)
- [SQLite ODBC driver](https://www.ch-werner.de/sqliteodbc/)

Local validation uses Debian's unixODBC 2.3.12 and SQLite ODBC 0.99991 installed
as system prerequisites. Their code is not included in this package. Runtime
licensing and redistribution obligations for a selected system driver remain
with the distributor of that driver.

The native harness deliberately records SQLite ODBC's pre-API embedded-NUL
truncation. This is a driver behavior observation, not a Buoy compiler/runtime
bug, and it is not represented as an upstream Buoy issue.

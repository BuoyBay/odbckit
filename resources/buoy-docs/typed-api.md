---
title: "odbckit typed API"
summary: "Source-derived public signatures, properties, parameters, defaults, and return values."
category: "Reference"
order: 21
types: ["OdbcDatabase", "OdbcException", "OdbcRowSet"]
dependencies: []
symbols: ["OdbcDatabase", "OdbcException", "OdbcRowSet"]
related: ["api.md", "getting-started.md", "index.md"]
---

# odbckit typed API

Generated from the package source declarations. Platform guards are shown where present; inherited members are documented on the base type. See the [API contracts](api.md) for behavior and limitations.

## OdbcException

`Public Class OdbcException Inherits DatabaseException`

### Properties

| Name | Type | Read-Only | Shared | Default | Description |
| --- | --- | --- | --- | --- | --- |
| `SqlState` | `String` | No | No | — | — |

### Constructor

`Public Sub Constructor(message As String, state As String, nativeCode As Integer)`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `message` | `String` | `ByVal` | Required |
| `state` | `String` | `ByVal` | Required |
| `nativeCode` | `Integer` | `ByVal` | Required |

Returns: no value.

## OdbcRowSet

`Public Class OdbcRowSet Inherits RowSet`

Buffered result. Independent of its connection after Query returns.

### Constructor

`Public Sub Constructor(nativeHandle As Ptr)`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `nativeHandle` | `Ptr` | `ByVal` | Required |

Returns: no value.

### RowCount

`Public Function RowCount() As Integer`

No parameters.

#### Returns

| Type |
| --- |
| `Integer` |

### AffectedRows

`Public Function AffectedRows() As Integer`

No parameters.

#### Returns

| Type |
| --- |
| `Integer` |

### HasMoreRows

`Public Overrides Function HasMoreRows() As Boolean`

No parameters.

#### Returns

| Type |
| --- |
| `Boolean` |

### NextRow

`Public Overrides Sub NextRow()`

No parameters.

Returns: no value.

### ColumnCount

`Public Overrides Function ColumnCount() As Integer`

No parameters.

#### Returns

| Type |
| --- |
| `Integer` |

### ColumnName

`Public Overrides Function ColumnName(index As Integer) As String`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `index` | `Integer` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `String` |

### Value

`Public Overrides Function Value(column As String) As DbValue`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `column` | `String` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `DbValue` |

### Value (overload 2)

`Public Overrides Function Value(index As Integer) As DbValue`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `index` | `Integer` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `DbValue` |

### Close

`Public Overrides Sub Close()`

No parameters.

Returns: no value.

### Destructor

`Public Sub Destructor()`

No parameters.

Returns: no value.

## OdbcDatabase

`Public Class OdbcDatabase Inherits Database`

Public ODBC 3 backend. The installed driver owns connection-string semantics.

### Properties

| Name | Type | Read-Only | Shared | Default | Description |
| --- | --- | --- | --- | --- | --- |
| `IdentityQuery` | `String` | No | No | `""` | — |

### Constructor

`Public Sub Constructor()`

No parameters.

Returns: no value.

### Connect

`Public Overrides Sub Connect(connectionString As String)`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `connectionString` | `String` | `ByVal` | Required |

Returns: no value.

### Disconnect

`Public Overrides Sub Disconnect()`

No parameters.

Returns: no value.

### IsConnected

`Public Overrides Function IsConnected() As Boolean`

No parameters.

#### Returns

| Type |
| --- |
| `Boolean` |

### QueryOdbc

`Public Function QueryOdbc(sql As String) As OdbcRowSet`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `sql` | `String` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `OdbcRowSet` |

### QueryOdbc (overload 2)

`Public Function QueryOdbc(sql As String, params As Array Of DbValue) As OdbcRowSet`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `sql` | `String` | `ByVal` | Required |
| `params` | `Array Of DbValue` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `OdbcRowSet` |

### Query

`Public Overrides Function Query(sql As String) As RowSet`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `sql` | `String` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `RowSet` |

### Query (overload 2)

`Public Overrides Function Query(sql As String, params As Array Of DbValue) As RowSet`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `sql` | `String` | `ByVal` | Required |
| `params` | `Array Of DbValue` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `RowSet` |

### Execute

`Public Function Execute(sql As String, params As Array Of DbValue) As Integer`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `sql` | `String` | `ByVal` | Required |
| `params` | `Array Of DbValue` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `Integer` |

### Execute (overload 2)

`Public Function Execute(sql As String) As Integer`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `sql` | `String` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `Integer` |

### LastInsertId

`Public Overrides Function LastInsertId() As Integer`

No parameters.

#### Returns

| Type |
| --- |
| `Integer` |

### BeginOdbcTransaction

`Public Sub BeginOdbcTransaction()`

No parameters.

Returns: no value.

### CommitOdbcTransaction

`Public Sub CommitOdbcTransaction()`

No parameters.

Returns: no value.

### RollbackOdbcTransaction

`Public Sub RollbackOdbcTransaction()`

No parameters.

Returns: no value.

### Destructor

`Public Sub Destructor()`

No parameters.

Returns: no value.

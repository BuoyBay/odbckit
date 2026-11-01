/* ODBCKit public-ODBC bridge. No Buoy runtime internals. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
_Static_assert(sizeof(void *) == 8, "ODBCKit supports 64-bit desktop targets");
_Static_assert(sizeof(SQLLEN) == sizeof(void *), "ODBC SQLLEN must match pointer width");
_Static_assert(sizeof(SQLULEN) == sizeof(void *), "ODBC SQLULEN must match pointer width");
_Static_assert(sizeof(SQLBIGINT) == sizeof(int64_t), "ODBC SQLBIGINT must be signed 64-bit");
#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API __attribute__((visibility("default")))
#endif
#define MAX_PARAMS 4096
#define MAX_COLUMNS 1024
#define MAX_ROWS 100000
#define MAX_CELL 1048576
#define MAX_TOTAL 33554432

typedef struct { SQLHENV env; SQLHDBC dbc; int connected; int transaction; char error[160]; char state[6]; int32_t code; } DB;
typedef struct { int kind; int64_t integer; double number; char *text; SQLLEN length; } Cell;
typedef struct { SQLHSTMT stmt; DB *db; Cell *params; int count; int columns; char **names; SQLSMALLINT *types; Cell *cells; size_t rows, capacity, total, param_total; int64_t affected; } Result;
static int good(SQLRETURN rc) { return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO; }
static void fail(DB *d, const char *operation, SQLSMALLINT kind, SQLHANDLE handle) {
    SQLCHAR state[6] = "HY000", ignored[2]; SQLINTEGER native = 0; SQLSMALLINT length = 0;
    if (handle) SQLGetDiagRec(kind, handle, 1, state, &native, ignored, 2, &length);
    memcpy(d->state, state, 5); d->state[5] = 0; d->code = native;
    snprintf(d->error, sizeof(d->error), "ODBC %s failed (SQLSTATE %.5s)", operation, d->state);
}
static void local_fail(DB *d, const char *message) { snprintf(d->error,sizeof(d->error),"%s",message); memcpy(d->state,"HY000",6); d->code=0; }
static int valid_utf8(const unsigned char *s,size_t n) {
    size_t i=0;
    while(i<n) {
        unsigned c=s[i++]; if(c<0x80) { if(c==0) return 0; continue; }
        unsigned count, value, minimum;
        if(c>=0xc2 && c<=0xdf) { count=1; value=c&31; minimum=0x80; }
        else if(c>=0xe0 && c<=0xef) { count=2; value=c&15; minimum=0x800; }
        else if(c>=0xf0 && c<=0xf4) { count=3; value=c&7; minimum=0x10000; }
        else return 0;
        if(count>n-i) return 0;
        while(count--) { unsigned next=s[i++]; if((next&0xc0)!=0x80) return 0; value=(value<<6)|(next&63); }
        if(value<minimum || value>0x10ffff || (value>=0xd800 && value<=0xdfff)) return 0;
    }
    return 1;
}
static char *copy_text(const char *s) { size_t n=strlen(s)+1; char *p=malloc(n); if(p) memcpy(p,s,n); return p; }
API void *bok_new(void) { return calloc(1,sizeof(DB)); }
API const char *bok_error(DB *d) { return d ? d->error : "ODBC allocation failed"; }
API const char *bok_state(DB *d) { return d ? d->state : "HY001"; }
API int32_t bok_code(DB *d) { return d ? d->code : 0; }
API int32_t bok_connected(DB *d) { return d && d->connected; }
API void bok_disconnect(DB *d) {
    if(!d) return;
    if(d->dbc) {
        if(d->connected) {
            /* ODBC forbids disconnect with a pending transaction. Best-effort
               rollback also covers transactions started through SQL text. */
            SQLEndTran(SQL_HANDLE_DBC,d->dbc,SQL_ROLLBACK);
            SQLDisconnect(d->dbc);
        }
        SQLFreeHandle(SQL_HANDLE_DBC,d->dbc);
    }
    if(d->env) SQLFreeHandle(SQL_HANDLE_ENV,d->env);
    d->dbc=0; d->env=0; d->connected=0; d->transaction=0;
}
API void bok_free(DB *d) { if(d) { bok_disconnect(d); free(d); } }
API int32_t bok_connect(DB *d, const char *connection) {
    if(!d) return 0;
    bok_disconnect(d);
    if(!good(SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&d->env))) { local_fail(d,"ODBC environment allocation failed"); goto bad; }
    if(!good(SQLSetEnvAttr(d->env,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0))) { fail(d,"environment setup",SQL_HANDLE_ENV,d->env); goto bad; }
    if(!good(SQLAllocHandle(SQL_HANDLE_DBC,d->env,&d->dbc))) { fail(d,"connection allocation",SQL_HANDLE_ENV,d->env); goto bad; }
    if(!good(SQLDriverConnect(d->dbc,0,(SQLCHAR *)connection,SQL_NTS,0,0,0,SQL_DRIVER_NOPROMPT))) { fail(d,"connect",SQL_HANDLE_DBC,d->dbc); goto bad; }
    d->connected=1; return 1;
bad: bok_disconnect(d); return 0;
}
API void bok_result_free(Result *r) {
    if(!r) return;
    if(r->stmt) SQLFreeHandle(SQL_HANDLE_STMT,r->stmt);
    if(r->params) for(int i=0;i<r->count;i++) free(r->params[i].text);
    for(int i=0;i<r->columns;i++) free(r->names ? r->names[i] : 0);
    for(size_t i=0;i<r->rows*(size_t)r->columns;i++) free(r->cells[i].text);
    free(r->params); free(r->names); free(r->types); free(r->cells); free(r);
}
API void *bok_prepare(DB *d, const char *sql, int32_t count) {
    if(!d || !d->connected) { if(d) local_fail(d,"ODBC connection is closed"); return 0; }
    if(count<0 || count>MAX_PARAMS) { local_fail(d,"ODBC parameter limit exceeded"); return 0; }
    Result *r=calloc(1,sizeof(*r)); if(!r) { local_fail(d,"ODBC allocation failed"); return 0; }
    r->db=d; r->count=count;
    if(count && !(r->params=calloc((size_t)count,sizeof(Cell)))) { local_fail(d,"ODBC allocation failed"); goto bad; }
    if(!good(SQLAllocHandle(SQL_HANDLE_STMT,d->dbc,&r->stmt))) { fail(d,"statement allocation",SQL_HANDLE_DBC,d->dbc); goto bad; }
    if(!good(SQLPrepare(r->stmt,(SQLCHAR*)sql,SQL_NTS))) { fail(d,"prepare",SQL_HANDLE_STMT,r->stmt); goto bad; }
    SQLSMALLINT expected=0;
    if(!good(SQLNumParams(r->stmt,&expected))) { fail(d,"parameter count",SQL_HANDLE_STMT,r->stmt); goto bad; }
    if(expected!=count) { local_fail(d,"ODBC parameter count does not match placeholders"); goto bad; }
    return r;
bad: bok_result_free(r); return 0;
}
API int32_t bok_bind(Result *r, int32_t index, int32_t kind, int64_t integer, double number, const char *text) {
    if(!r || index<0 || index>=r->count) return 0;
    Cell *p=&r->params[index]; p->kind=kind; p->integer=integer; p->number=number;
    SQLSMALLINT ct=SQL_C_CHAR, st=SQL_VARCHAR; SQLPOINTER ptr=0; SQLULEN size=1; SQLLEN buflen=0;
    if(kind==0) p->length=SQL_NULL_DATA;
    else if(kind==1) { ct=SQL_C_SBIGINT; st=SQL_BIGINT; ptr=&p->integer; p->length=0; size=19; }
    else if(kind==2) { ct=SQL_C_DOUBLE; st=SQL_DOUBLE; ptr=&p->number; p->length=0; size=15; }
    else if(kind==3) { size=strlen(text); if(size>MAX_CELL) { local_fail(r->db,"ODBC text parameter exceeds 1 MiB"); return 0; } if(r->param_total+(size_t)size>MAX_TOTAL) { local_fail(r->db,"ODBC parameter text budget exceeded"); return 0; } r->param_total+=(size_t)size; p->text=copy_text(text); if(!p->text) { local_fail(r->db,"ODBC allocation failed"); return 0; } ptr=p->text; p->length=(SQLLEN)size; buflen=(SQLLEN)size+1; if(!size) size=1; }
    else if(kind==4) { ct=SQL_C_SBIGINT; st=SQL_BIT; ptr=&p->integer; p->length=0; }
    else { local_fail(r->db,"ODBC unsupported parameter kind"); return 0; }
    if(!good(SQLBindParameter(r->stmt,(SQLUSMALLINT)(index+1),SQL_PARAM_INPUT,ct,st,size,0,ptr,buflen,&p->length))) { fail(r->db,"bind",SQL_HANDLE_STMT,r->stmt); return 0; }
    return 1;
}
static int value_kind(SQLSMALLINT type) {
    switch(type) {
    case SQL_TINYINT: case SQL_SMALLINT: case SQL_INTEGER: case SQL_BIGINT: return 1;
    case SQL_REAL: case SQL_FLOAT: case SQL_DOUBLE: return 2;
    case SQL_BIT: return 4;
    case SQL_BINARY: case SQL_VARBINARY: case SQL_LONGVARBINARY: return -1;
    default: return 3;
    }
}
API int32_t bok_execute(Result *r) {
    DB *d=r->db; SQLSMALLINT cols=0;
    /* SQL_NO_DATA is successful execution of a searched UPDATE/DELETE
       that matched no rows; its row count still needs to be returned. */
    SQLRETURN executed=SQLExecute(r->stmt);
    if(executed!=SQL_SUCCESS && executed!=SQL_NO_DATA) { fail(d,"execute",SQL_HANDLE_STMT,r->stmt); return 0; }
    if(executed==SQL_NO_DATA) {
        r->affected=0; r->columns=0;
        SQLFreeHandle(SQL_HANDLE_STMT,r->stmt); r->stmt=0;
        return 1;
    }
    SQLLEN affected=0; if(!good(SQLRowCount(r->stmt,&affected))) { fail(d,"row count",SQL_HANDLE_STMT,r->stmt); return 0; } r->affected=(int64_t)affected;
    if(!good(SQLNumResultCols(r->stmt,&cols))) { fail(d,"column count",SQL_HANDLE_STMT,r->stmt); return 0; }
    if(cols<0 || cols>MAX_COLUMNS) { local_fail(d,"ODBC column limit exceeded"); return 0; }
    r->columns=cols;
    if(cols) { r->names=calloc((size_t)cols,sizeof(char*)); r->types=calloc((size_t)cols,sizeof(SQLSMALLINT)); if(!r->names || !r->types) { local_fail(d,"ODBC allocation failed"); return 0; } }
    for(int i=0;i<cols;i++) {
        SQLCHAR name[1025]; SQLSMALLINT n=0,scale=0,nullable=0; SQLULEN size=0;
        SQLRETURN rc=SQLDescribeCol(r->stmt,(SQLUSMALLINT)(i+1),name,sizeof(name),&n,&r->types[i],&size,&scale,&nullable);
        if(rc!=SQL_SUCCESS || n<0 || n>=1025) { fail(d,"column description",SQL_HANDLE_STMT,r->stmt); return 0; }
        if(value_kind(r->types[i])<0) { local_fail(d,"ODBC binary columns require an explicit binary API (unsupported)"); return 0; }
        if(!valid_utf8(name,(size_t)n)) { local_fail(d,"ODBC column name is not NUL-free UTF-8"); return 0; }
        r->names[i]=copy_text((char*)name); if(!r->names[i]) { local_fail(d,"ODBC allocation failed"); return 0; }
    }
    while(cols) {
        SQLRETURN rc=SQLFetch(r->stmt); if(rc==SQL_NO_DATA) break;
        if(rc!=SQL_SUCCESS) { fail(d,"fetch",SQL_HANDLE_STMT,r->stmt); return 0; }
        if(r->rows>=MAX_ROWS || (r->rows+1)*(size_t)cols > MAX_TOTAL/sizeof(Cell)) { local_fail(d,"ODBC result row/cell budget exceeded"); return 0; }
        if(r->rows==r->capacity) {
            size_t cap=r->capacity ? r->capacity*2 : 16; if(cap>MAX_ROWS) cap=MAX_ROWS;
            if(cap*(size_t)cols>MAX_TOTAL/sizeof(Cell)) cap=MAX_TOTAL/sizeof(Cell)/(size_t)cols;
            Cell *p=realloc(r->cells,cap*(size_t)cols*sizeof(Cell)); if(!p) { local_fail(d,"ODBC allocation failed"); return 0; }
            r->cells=p; r->capacity=cap;
        }
        Cell *row=r->cells+r->rows*(size_t)cols; memset(row,0,(size_t)cols*sizeof(Cell)); r->rows++;
        for(int i=0;i<cols;i++) {
            Cell *c=&row[i]; c->kind=value_kind(r->types[i]); SQLLEN length=0;
            SQLSMALLINT type=SQL_C_CHAR; SQLPOINTER ptr=0; SQLLEN cap=0;
            char integer_text[64]={0};
            if(c->kind==1 || c->kind==4) { ptr=integer_text; cap=sizeof(integer_text); }
            else if(c->kind==2) { type=SQL_C_DOUBLE; ptr=&c->number; cap=sizeof(c->number); }
            else { c->text=malloc(MAX_CELL+1); if(!c->text) { local_fail(d,"ODBC allocation failed"); return 0; } c->text[0]=0; ptr=c->text; cap=MAX_CELL+1; }
            rc=SQLGetData(r->stmt,(SQLUSMALLINT)(i+1),type,ptr,cap,&length);
            if(rc!=SQL_SUCCESS) { if(rc==SQL_SUCCESS_WITH_INFO) local_fail(d,"ODBC cell conversion warning or text exceeds 1 MiB (rejected)"); else fail(d,"read column",SQL_HANDLE_STMT,r->stmt); return 0; }
            if(length==SQL_NULL_DATA) { c->kind=0; free(c->text); c->text=0; }
            else if(c->kind==1 || c->kind==4) {
                if(length<=0 || length>=(SQLLEN)sizeof(integer_text) || integer_text[length]!=0 || memchr(integer_text,0,(size_t)length)) { local_fail(d,"ODBC invalid integer text"); return 0; }
                size_t start=(integer_text[0]=='+' || integer_text[0]=='-') ? 1u : 0u;
                if(start==(size_t)length) { local_fail(d,"ODBC invalid integer text"); return 0; }
                for(size_t j=start;j<(size_t)length;j++) if(integer_text[j]<'0' || integer_text[j]>'9') { local_fail(d,"ODBC lossy integer conversion rejected"); return 0; }
                errno=0; char *end=0; long long value=strtoll(integer_text,&end,10);
                if(errno==ERANGE || !end || *end || (c->kind==4 && value!=0 && value!=1)) { local_fail(d,"ODBC integer conversion outside supported range"); return 0; }
                c->integer=(int64_t)value;
            }
            else if(c->kind==3) {
                if(length<0 || length>MAX_CELL || c->text[length]!=0 || memchr(c->text,0,(size_t)length)!=0) { local_fail(d,"ODBC invalid or NUL-containing text"); return 0; }
                if(!valid_utf8((unsigned char*)c->text,(size_t)length)) { local_fail(d,"ODBC result text is not UTF-8; use a UTF-8 narrow-API driver"); return 0; }
                if(r->total+(size_t)length>MAX_TOTAL) { local_fail(d,"ODBC result text budget exceeded"); return 0; }
                r->total+=(size_t)length;
                char *small=realloc(c->text,(size_t)length+1); if(small) c->text=small;
            }
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT,r->stmt); r->stmt=0; return 1;
}
API int64_t bok_rows(Result *r) { return (int64_t)r->rows; }
API int32_t bok_columns(Result *r) { return r->columns; }
API const char *bok_name(Result *r,int32_t col) { return r->names[col]; }
API int64_t bok_affected(Result *r) { return r->affected; }
API int32_t bok_kind(Result *r,int64_t row,int32_t col) { return r->cells[(size_t)row*(size_t)r->columns+(size_t)col].kind; }
API int64_t bok_integer(Result *r,int64_t row,int32_t col) { return r->cells[(size_t)row*(size_t)r->columns+(size_t)col].integer; }
API double bok_number(Result *r,int64_t row,int32_t col) { return r->cells[(size_t)row*(size_t)r->columns+(size_t)col].number; }
API const char *bok_text(Result *r,int64_t row,int32_t col) { return r->cells[(size_t)row*(size_t)r->columns+(size_t)col].text; }
API int32_t bok_transaction(DB *d,int32_t operation) {
    if(!d || !d->connected) { if(d) local_fail(d,"ODBC connection is closed"); return 0; }
    SQLRETURN rc;
    if((operation==0 && d->transaction) || (operation!=0 && !d->transaction)) { local_fail(d,"ODBC transaction state does not permit this operation"); return 0; }
    if(operation==0) rc=SQLSetConnectAttr(d->dbc,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
    else {
        rc=SQLEndTran(SQL_HANDLE_DBC,d->dbc,operation==1 ? SQL_COMMIT : SQL_ROLLBACK);
        if(good(rc)) rc=SQLSetConnectAttr(d->dbc,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
    }
    if(!good(rc)) { fail(d,"transaction",SQL_HANDLE_DBC,d->dbc); return 0; } d->transaction=(operation==0); return 1;
}

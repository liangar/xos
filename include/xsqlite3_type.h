#ifndef _XSQLITE3_TYPE_H_
#define _XSQLITE3_TYPE_H_

#include <xdatatype.h>

#define XSQL_OK         0
#define XSQL_SUCCESS    0
#define XSQL_WARNING    1
#define XSQL_NO_DATA    2
#define XSQL_EXECUTING	3
#define XSQL_HAS_DATA	4

#define XSQL_ERROR		-1
#define XSQL_ERROR_PARAM	-10001

#define XSQL_COMMIT     true
#define XSQL_ROLLBACK   false

#define XSQL_DEFAULT	0
#define XSQL_STATIC		1
#define XSQL_DYNAMIC	2

#endif // _XSQLITE3_TYPE_H_
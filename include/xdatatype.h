#pragma once

#define XSQL_ERROR		-1
#define XSQL_ERROR_PARAM	-10001

#define XSQL_OK         0
#define XSQL_SUCCESS    0
#define XSQL_WARNING    1
#define XSQL_NO_DATA    2
#define XSQL_EXECUTING  3
#define XSQL_HAS_DATA	4

#define XSQL_DEFAULT	0
#define XSQL_STATIC		1
#define XSQL_DYNAMIC	2


#define XSQL_COMMIT     1
#define XSQL_ROLLBACK   0

/* 系列函数bind: 绑定数据到参数或列
 * 函数bind(int nParmClass, ...):
 *  从nParmClass开始是可变的参数, 由前导参数种类分节.
 *  在XPARM_IN节:
 *    XPARM_LONG   后接一个long *变量.
 *    XPARM_DOUBLE 后接一个double *变量.
 *    XPARM_DATE   后接一个char *描述串变量,格式为'YYYY-MM-DD hh:mm:ss\0'
 *    XPARM_STR    后接一个char *变量,后跟上一个表示缓冲区长度的long变量
 *  在XPARM_OUT和XPARM_IO以及XPARM_COL节:
 *    XPARM_LONG   后接一个long *变量.
 *    XPARM_DOUBLE 后接一个double *变量.
 *    XPARM_DATE   后接一个char *描述串变量,格式为'YYYY-MM-DD hh:mm:ss\0',长度为32.
 *    XPARM_STR    后接一个char *变量, 后跟上一个表示缓冲区长度的long变量.
 *    或上XPARM_IND的类型后还要接一个long *变量用来指示或接收前一变量是否为空值以及值的长度.
 *  调用例子:
 *    long lID;
 *    char s[100];
 *    long lInd = 100;
 *    char t[32];
 *    bind("select a, b from t where tid = ?",
 *          XPARM_COL|XPARM_NSTR,  s,  &lInd,
 *                    XPARM_DATE,  t,
 *          XPARM_IN |XPARM_LONG,  lID,
 *          XPARM_END);
 */

#ifndef XPARM_CLASS_MASK
const unsigned int
	XPARM_CLASS_MASK= 0xE0,
	XPARM_SAME		= 0x00,
	XPARM_COL		= 0xa0,
	XPARM_IN		= 0x20,
	XPARM_OUT		= 0x40,
	XPARM_IO		= 0x60,
	XPARM_END		= 0x80,

    XPARM_SPEC_MASK = 0x0700,
    XPARM_TRANSID   = 0x0100,
    XPARM_STATUS    = 0x0200,
	XPARM_FORMAT	= 0x0300,
	XPARM_IGNORE	= 0x0400,

	XPARM_IND		= 0x10,

	XPARM_TYPE_MASK	= 0x0f,
	XPARM_MIN		= 0x00,
	XPARM_BOOL		= 0x00,
	XPARM_NBOOL		= XPARM_IND | XPARM_BOOL,
	XPARM_CHAR		= 0x01,
	XPARM_LSTR		= 0x01,
	XPARM_NCHAR		= XPARM_IND | XPARM_CHAR,
	XPARM_NLSTR		= XPARM_IND | XPARM_LSTR,
	XPARM_SHORT		= 0x02,
	XPARM_NSHORT	= XPARM_IND | XPARM_SHORT,
	XPARM_LONG		= 0x03,
	XPARM_NLONG		= XPARM_IND | XPARM_LONG,
	XPARM_FLOAT		= 0x04,
	XPARM_NFLOAT	= XPARM_IND | XPARM_FLOAT,
	XPARM_DOUBLE	= 0x05,
	XPARM_NDOUBLE	= XPARM_IND | XPARM_DOUBLE,
	XPARM_DATETIME	= 0x06,
	XPARM_NDATETIME	= XPARM_IND | XPARM_DATETIME,
	XPARM_STR		= 0x07,
	XPARM_NSTR		= XPARM_IND | XPARM_STR,
	XPARM_BIN		= 0x08,
	XPARM_NBIN		= XPARM_IND | XPARM_BIN,
	XPARM_DATE		= 0x09,
	XPARM_NDATE		= XPARM_IND | XPARM_DATE,
	XPARM_TIME		= 0x0a,
	XPARM_NTIME		= XPARM_IND | XPARM_TIME,
	XPARM_INT		= 0x0b,
	XPARM_DATESTR	= 0x0c,
	XPARM_POINT		= 0x0d,
	XPARM_FUNCTION	= 0x0e,
	XPARM_UNKNOWN	= 0x0f,
	XPARM_MAX		= 0x0c;

const int
	XPARM_IND_SPECIAL = 0,
	XPARM_IND_SELF	= 1,
	XPARM_IND_NULL	= 2;

#define XPARM_HAS_IND(x)            ((x) & XPARM_IND)
#define XPARM_TYPE(x)               ((x) & XPARM_TYPE_MASK)
#define XPARM_CLASS(x)              ((x) & XPARM_CLASS_MASK)
#define XPARM_TYPE_VALID(x)         ((XPARM_TYPE(x) <= XPARM_MAX))
#define XPARM_MAKE_TYPE(xclass, x)  ((xclass) | (x))
#define XPARM_SPECIAL(x)            ((x) & XPARM_SPEC_MASK)

#endif

#define XPARM_DATETIME_LEN	24
#define XPARM_DATE_LEN		12
#define XPARM_TIME_LEN		16

const int
		XPARM_NULL = -1,
		XPARM_NTS = -3;

typedef struct tagXParm{    // 参数描述
	unsigned int type;      // 数据类型
	long size;              // 长度,用在输入参数
	union {
		long* pind;             // 指示,用在输出参数或列
		bool isnull;
	};
	union {                // 值
		short   * pshort;
        int     * pi;
		long	* pl;
		float   * pf;
        double  * pd;
        char    * ps;
        void    * pv;
    };
} XParm, * LPXParm;

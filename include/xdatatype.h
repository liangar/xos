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

/* ϵ�к���bind: �����ݵ���������
 * ����bind(int nParmClass, ...):
 *  ��nParmClass��ʼ�ǿɱ�Ĳ���, ��ǰ����������ֽ�.
 *  ��XPARM_IN��:
 *    XPARM_LONG   ���һ��long *����.
 *    XPARM_DOUBLE ���һ��double *����.
 *    XPARM_DATE   ���һ��char *����������,��ʽΪ'YYYY-MM-DD hh:mm:ss\0'
 *    XPARM_STR    ���һ��char *����,�����һ����ʾ���������ȵ�long����
 *  ��XPARM_OUT��XPARM_IO�Լ�XPARM_COL��:
 *    XPARM_LONG   ���һ��long *����.
 *    XPARM_DOUBLE ���һ��double *����.
 *    XPARM_DATE   ���һ��char *����������,��ʽΪ'YYYY-MM-DD hh:mm:ss\0',����Ϊ32.
 *    XPARM_STR    ���һ��char *����, �����һ����ʾ���������ȵ�long����.
 *    ����XPARM_IND�����ͺ�Ҫ��һ��long *��������ָʾ�����ǰһ�����Ƿ�Ϊ��ֵ�Լ�ֵ�ĳ���.
 *  ��������:
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

typedef struct tagXParm{    // ��������
	unsigned int type;      // ��������
	long size;              // ����,�����������
	union {
		long* pind;             // ָʾ,���������������
		bool isnull;
	};
	union {                // ֵ
		short   * pshort;
        int     * pi;
		long	* pl;
		float   * pf;
        double  * pd;
        char    * ps;
        void    * pv;
    };
} XParm, * LPXParm;

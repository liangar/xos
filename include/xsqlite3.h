#ifndef _X_SQLITE3_H_
#define _X_SQLITE3_H_

#include <sqlite3.h>

#include <xsqlite3_type.h>

#define XDB_LOCK_DEFERRED	0
#define XDB_LOCK_IMMEDIATE	1
#define XDB_LOCK_EXCLUSIVE	2

// create and decraate sqlite3 env
int xsqlite3_open(bool ispooling = false);
int xsqlite3_close(void);

// for app to build parameters and columns structure
int buildxparamscols(XParm *pparms, XParm * pcols, int nparmclass, ...);

class xsqlite3{
public:
    xsqlite3(const char * szconnectionstring = 0);
    ~xsqlite3();

    // open and close
    int open(const char * szconnectionstring = 0);
    int close(bool bforce = true);
    int isopen(void);

    // for transaction
	int trans_begin(int locktype = XDB_LOCK_DEFERRED);
    int trans_end(bool isok = true);

	// set isolation, must sure there are no open transaction
	int setisolation(int type);

	void getconnect_string(char * d);
	void getconnect_string(char ** d);

	void set_lastsql(const char * psqlstring);

	int geterror(char * p, int maxlen);
	int errcode(void);

	void setlock(xsys_mutex* lock){
		m_db_lock = lock;
	}

	long	m_lastTime;
	char	m_lastsql[4096];

    sqlite3 *m_pdb;

protected:
    void getconnectinfo(char * pdsn, char * puid, char * ppwd, char * pstr);

protected:
    char *   m_pconnection;
    xsys_mutex* m_db_lock;
};

class xsql3{
public:
    // 构造析构
    xsql3(xsqlite3 * pdb = 0);
    ~xsql3();
    void setdbconnection(xsqlite3 * pdb);
	xsqlite3 * getdbconnection()  {  return m_pdb;  }
    int isopen(void);

    // 执行语句控制
	int preexec   (const char * psql);                      // without parameter
    int preexec   (const char * psql, int nparmclass, ...); // with many parameters
    int directexec(const char * psql, int nparmclass, ...);
    int directexec(const char * psql);
	int geta	  (const char * psql, int nparmclass, ...);
	int geta      (void);
    int exec      (void);                       // folowing the preexecsql()
    int endexec   (void);
	
    // 绑定参数或列
	// bind single parameter
    int bindparameters(void);                   // bind parameters (use m_pparams)
	int bindaparameter(XParm * pparam, int pos);
    int bindparameters(XParm * pparam);         // bind parameters
    int bindparameters(XParm * pparam, int startpos, int n);

	// bind
    int bindcols(int nparamclass, ...);             // bind cols (use m_pcols)
    int bind    (int nparamclass, ...);             // bind parameters and cols

    // 取数据
    int fetch(void);
    int resetcursor(void);

    int getcol(char * pbuf, int length, int ncol);
    int getcol(long * pbuf, int ncol);
    int getcol(double * pbuf, int ncol);

    // 列表数据库项目
    // get tables or views name list, type = 'TABLE,VIEW]'
	int gettables (char * ptables, int maxlen);
    
	// get special table's columns name list
    int getcolumns(char * pcolumns, char * ptable, int maxlen);
    int getcolumns(char * pcolumns, XParm * pcols, char * ptable, int maxlen, int maxnum);
	int getcolumns_sorted(char * pcolumns, XParm * pcols, char * ptable, int maxlen, int maxnum);

    int gettablerows(int & rows, char * ptablename);

    int changes(int & rows);
    //
    int getresultcolumns(int & cols);
	int getcolumnname(char * pname, int maxlen, int pos);

	// 出错信息
	int geterror(char * msg, int msglength);
	int errcode(void);

protected:
    int open(void);
    int close(void);

	int vbindcols(int nparmclass, va_list v);
	int vbind    (int nparmclass, va_list v);

protected:
    xsqlite3 * m_pdb;
    sqlite3_stmt * m_pstmt;

    XParm *  m_pparams, * m_pcols;
    int m_nparams, m_ncols;
};

typedef xsqlite3 *		LPxdatabase;
typedef xsql3 *			LPxsql;

#endif // _X_SQLITE3_H_
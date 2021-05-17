#pragma once

#include <libpq-fe.h>

class xpg_conn{
public:
    xpg_conn()
		: db_(0)
		, ver_(0)
		, auto_(false)
		, in_trans_(false)
	{
	}

    ~xpg_conn()
	{
		close();
	}

    // open and close
    int  open(const char * szconnectionstring);
    int  close(bool isok = true);
    bool isopen(void)  {  return (db_ != 0);  }

	// set isolation, must sure there are no open transaction
	int setisolation(int type);

	// for get info
	int get_ver(void)   {  return ver_;  }
	const char * get_dbname(void){  return PQdb(db_);  }
	const char * get_error(void)  {  return PQerrorMessage(db_);  }

    // for transaction
	void auto_commit(bool autoon)  {  auto_ = autoon;  }
	int  trans_begin(void);
    int  trans_end(bool isok);

protected:
    PGconn	*db_;	/// 数据库连接
	int 	ver_;	/// 数据库版本

	bool	auto_;	/// 是否自动提交
	bool	in_trans_;	/// 是否在事务中
};


/***
 *
 */
class xpg_sql{
public:
    // 构造析构
    xpg_sql();
    ~xpg_sql();
	
    int open(xpg_conn * db);
    int close(void);

	xpg_conn * db_conn()  {  return db_;  }
    int isopen(void);	  {  return (db_ != 0);  }

    // 执行语句控制
    int preexec   (const char * psql, int nparmclass, ...); // with many parameters
    int directexec(const char * psql);
    int directexec(const char * psql, int nparmclass, ...);
	int geta	  (const char * psql, int nparmclass, ...);
	int geta	  (void);
    int exec      (void);	// folowing the preexecsql()
    int endexec   (void);
	
    // 绑定参数或列
	// bind single parameter
	int bindaparameter(XParm * pparam, int pos);
    int bindparameters(XParm * pparam);         // bind parameters
    int bindparameters(XParm * pparam, int startpos, int n);

	// bind single col
    int bindacol(XParm * pcol, int pos);
    int bindcols(XParm * pcols);                // bind cols
    int bindcols(XParm * pcols, int startpos, int n);

    int bind(int nparamclass, ...);             // bind parameters and cols

    // 取数据
    int fetch(void);

	const char * get_col(int i);
    int 	get_col(char * v, int length, int i);
    int 	get_col(int i);
    double	get_col(int i);

    // 取执行结果属性
	int get_rows	(void)	{  return PQcmdTuples(res_);  }
    int res_rows	(void)	{  return PQntuples(res_);  }
    int res_columns (void)	{  return PQnfields(res_);  }
	const char * column_name(int i)  {  return PQfname(res_, i);  }

	// 出错信息
	int 		get_error(char * msg, int msglength);
	const char *get_error(void)  {  return PQresultErrorMessage(res_);  }

protected:
	int vbind(int nparmclass, va_list v);
    int bindparameters(void);                   // bind parameters (use m_pparams)
    int bindcols(void);                         // bind cols (use m_pcols)

protected:
	int irow_;	/// 游标h行位置
	int rows_;  /// 结果集的行数
	PGresult * res_;	/// 执行结果
    xpg_conn * db_;		/// 数据库连接

    prms_types_, * pcols_;	/// 绑定的参数和列
	int param_count_, col_count_;	/// 绑定的参数和列的数量
};

class pg_mng : public xsql{
public:
    // 列表数据库项目
    // get tables or views name list, type = 'TABLE,VIEW]'
	int gettables (char * ptables, const char * schema, const char * ptypes, int maxlen);
    int gettables (char * ptables, const char * ptypes, int maxlen);
    
	// get special table's columns name list
    int getcolumns(char * pcolumns, char * ptable, int maxlen);
    int getcolumns(char * pcolumns, XParm * pcols, char * ptable, int maxlen, int maxnum);
	int getcolumns_sorted(char * pcolumns, XParm * pcols, char * ptable, int maxlen, int maxnum);

	int getprimarykey(char * pkeycolumns, char * ptable, int maxlen);
	int getforeignkey(char * ppks, char * pfks, char * ppktable, char * pfktable, int maxpklen, int maxfklen);

    int gettablerows(long & rows, char * ptablename);
}

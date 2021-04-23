#pragma once

#include <libpq-fe.h>
#include <xdatatype.h>
#include <stdint.h>

#ifndef htobe32
#define htobe32 htonf
#endif // !htobe32

#ifndef htobe64
#define htobe64 htond
#endif // !htobe32

class xpg_conn;
class xpg_sql;

class xpg_conn{
	friend class xpg_sql;

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


union xparam_data{
	short	s;
	int 	i;
	long	l;
	uint32_t u;
	uint64_t ll;
	float	f;
	double	d;
	void *	p;
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
    bool isopen(void)	  {  return (db_ != 0);  }

    // 执行语句控制
    int preexec (const char * psql, int parm_class, ...); // with many parameters
    int exec	(const char * psql);
    int exec	(const char * psql, int parm_class, ...);
	int geta	(const char * psql, int parm_class, ...);
	int geta	(void);
    int exec    (void);	// folowing the preexecsql()
    int end_exec(void);

    // 取数据
    int fetch(void);

	const char * get_col(int i);
    int 	get_col(char * v, int length, int i);
    int 	get_col_i(int i);
    double	get_col_d(int i);

    // 取执行结果属性
	int get_rows(void);
    int res_rows	(void)	{  return rows_;  }
    int res_columns (void)	{  return cols_;  }
	const char* column_name(int i);

	// 出错信息
	int 		get_error(char * msg, int msglength);
	const char *get_error(void)  {  return PQresultErrorMessage(res_);  }

protected:
	int check_exec(void);
	int vbind(int nparmclass, va_list v);
	int bind_a_parameter(int i);
    int bind_parameters(void);		// bind parameters (use m_pparams)

protected:
	int irow_;	/// 游标h行位置
	int rows_;  /// 结果集的行数
	int cols_;

	PGresult * res_;	/// 执行结果
    xpg_conn * db_;		/// 数据库连接

    XParm * pcols_;	/// 绑定的参数和列
	XParm * params_;/// 绑定参数

	/// 以下假设参数不多于16
	int prms_types_[16], prms_lens_[16];
	xparam_data prms_temp_vs_[16];	/// 中间转换值存储
	char *prms_v_[16];

    p_oid_s p_oid;
    Oid prms_oid[16];  //

	int param_count_, col_count_;	/// 绑定的参数和列的数量
};

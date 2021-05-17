#include "xpgdb.h"

struct p_oid_s{
    Oid oid_short;		// int2;
    Oid oid_int;		// int4;
	Oid oid_long;       // int8;
    Oid oid_float;      // float4;
    Oid oid_double;     // float8;
    Oid oid_text;       // text;
};

static p_oid_s p_oid = {21, 23, 20, 700, 701, 25};
static bool oid_set = false;
static bool b64bit = false;

static void set_oid_s(xpg_conn *db)
{
	xpg_sql sql;

	sql.open(db);
	int oid;
	char typname[32];

    int typ_col = 0, oid_col = 0;

	printf("will get oids(int size=%ld)\n", sizeof(oid));

	int r = sql.exec(
		"SELECT oid, typname FROM pg_type WHERE typname IN ('int2','int4','int8','float4','float8','text')",
		XPARM_COL | XPARM_LONG, &oid,
					XPARM_STR , &typname, sizeof(typname),
		XPARM_END
	);

	int size_long = sizeof(long);	// get long length
	b64bit = (size_long == 8);
    while (XSQL_NO_DATA != sql.fetch()) {
		printf("oid=%d, typname=%s\n", oid, typname);
        switch (typname[0]) {
            case 'i': {
                if (typname[3] == '2')
                    p_oid.oid_short = oid;
                else if (typname[3] == '4'){
                    p_oid.oid_int = oid;
					if (!b64bit)	// long is 32bit
						p_oid.oid_long = oid;
				}else
					if (b64bit)	// long is 64bit
						p_oid.oid_long = oid;
            }break;
            case 'f': {
                if (typname[5] == '4')
                    p_oid.oid_float = oid;
                else
                    p_oid.oid_double = oid;
            }break;

            case 't': p_oid.oid_text = oid;  break;
            default: break;
        }

    }
    sql.end_exec();
	oid_set = true;
	printf("long is %d bytes, oid=%d, int's oid=%d\n", size_long, p_oid.oid_long, p_oid.oid_int);
}

static int check_code(ExecStatusType n)
{
    switch (n){
    case PGRES_EMPTY_QUERY:
    case PGRES_BAD_RESPONSE:
	case PGRES_FATAL_ERROR: 	return XSQL_ERROR;

    case PGRES_COMMAND_OK:
	case PGRES_TUPLES_OK:		return XSQL_OK;

	case PGRES_NONFATAL_ERROR:	return XSQL_WARNING;
    case PGRES_COPY_BOTH:		return XSQL_EXECUTING;
    }

    return -100;
}

static int parm_col_count(int nparm, va_list vlist, int& col_count, int& parm_count)
{
	int nclass, ntype;
	bool bind;

	col_count = parm_count = 0;
	nclass = XPARM_IN;

	while ((ntype = XPARM_CLASS(nparm)) != XPARM_END){
		if (ntype != XPARM_SAME) nclass = ntype; // 检测参数种类是否改变

		if (nclass == XPARM_COL)
			col_count++;
		else if (nclass == XPARM_IN || nclass == XPARM_OUT || nclass == XPARM_IO)
			parm_count++;
		else
            return -1;

		bind = XPARM_HAS_IND(nparm);
		ntype = XPARM_TYPE(nparm);

		if (!XPARM_TYPE_VALID(ntype))
            return -3;

        va_arg(vlist, void *);

		if (bind)
	        va_arg(vlist, long *);
		else{
			if (ntype == XPARM_STR || ntype == XPARM_BIN || ntype == XPARM_LSTR)
				va_arg(vlist, long);  // IO参数还需一个表示接收缓冲区最大长度的long变量.
		}

		nparm = va_arg(vlist, int);         // 下一个参数
	}
    return 0;
}

// 使用void*指针指向参数 并填写size
static int buildxparms(int nparm, va_list vlist, XParm *pcol, XParm *pparm)
{
	int nclass, ntype;
	bool bind;
	XParm * p;

	nclass = XPARM_IN;

	while ((ntype = XPARM_CLASS(nparm)) != XPARM_END){
		if(ntype != XPARM_SAME) nclass = ntype; // 检测参数种类是否改变

		bind  = XPARM_HAS_IND(nparm);
		ntype = XPARM_TYPE(nparm);

		if (nclass == XPARM_COL)
			p = pcol++;
		else
			p = pparm++;

        p->pv = va_arg(vlist, void *);
		if (bind){
	        p->pind = va_arg(vlist, long *);
			p->size = *(p->pind);
		}else{
			p->pind = 0;
			if (ntype == XPARM_STR || ntype == XPARM_BIN || ntype == XPARM_LSTR)
				p->size = va_arg(vlist, long);  // IO参数还需一个表示接收缓冲区最大长度的long变量.

			if (nclass == XPARM_IN &&
				(ntype == XPARM_STR ||
				 ntype == XPARM_LSTR||
				 ntype == XPARM_BIN ||
				 ntype == XPARM_DATETIME ||
				 ntype == XPARM_DATE ||
				 ntype == XPARM_TIME))
				p->pind = (long *)XPARM_NTS;

			switch(ntype){
				case XPARM_SHORT:	p->size = sizeof(short);break;
				case XPARM_LONG :	p->size = sizeof(long); break;
				case XPARM_INT  :   p->size = sizeof(int);	break;
				case XPARM_FLOAT:   p->size = sizeof(float);break;
				case XPARM_DOUBLE:  p->size = sizeof(double);break;

				case XPARM_DATETIME:p->size = XPARM_DATETIME_LEN;  break;
				case XPARM_DATE:	p->size = XPARM_DATE_LEN;  break;
				case XPARM_TIME:	p->size = XPARM_TIME_LEN;  break;
			}
		}
		p->type = XPARM_MAKE_TYPE(nclass, nparm);

        nparm = va_arg(vlist, int);             // 下一个参数
	}

	if (pcol)  pcol->type  = XPARM_END;
	if (pparm) pparm->type = XPARM_END;

    return 0;
}

int xpg_conn::open(const char * connect_string)
{
	if (connect_string == 0)
		return -1;

	close(true);

	db_ = PQconnectdb(connect_string);
	if (PQstatus(db_) == CONNECTION_BAD){
		db_ = 0;
		return -2;
	}

	ver_ = PQserverVersion(db_);
	printf("ver = %d\n", ver_);

	if (!oid_set)
		set_oid_s(this);

	return 0;
}

int xpg_conn::close(bool isok)
{
	if (db_){
		if (in_trans_)
			trans_end(isok);

		PQfinish(db_);
		db_ = 0;
		ver_ = 0;
		auto_ = in_trans_ = false;
	}

	return 0;
}

//事务开始
int xpg_conn::trans_begin(void)
{
	if (!isopen())
		return -1;

	if (in_trans_)
		return 0;

	PGresult *res = PQexec(db_, "BEGIN");
	ExecStatusType r = PQresultStatus(res);
	PQclear(res);
	if (r != PGRES_COMMAND_OK)
		return -1;

	in_trans_ = true;

	return 0;
}

//事务结束 参数为false则回滚事务
int xpg_conn::trans_end(bool isok)
{
	if (!in_trans_)
		return 0;

	PGresult *res;
	if (isok)
		res = PQexec(db_, "COMMIT");
	else
		res = PQexec(db_, "ROLLBACK");
	ExecStatusType r = PQresultStatus(res);
	PQclear(res);

	if (r != PGRES_COMMAND_OK)
		return -1;

	in_trans_ = false;

	return 0;
}

xpg_sql::xpg_sql()
    : db_(NULL)
	, res_(NULL)
    , irow_(-1)
	, param_count_(0)
	, col_count_(0)
	, pcols_(NULL)
	, params_(NULL)
{
	rows_ = cols_ = 0;
}

xpg_sql::~xpg_sql()
{
	close();
}


// 将db_指向一个xpg_conn对象
int xpg_sql::open(xpg_conn * pdb)
{
    close();
	db_ = pdb;

	return 0;
}

//将db指向NULL
int xpg_sql::close(void)
{
	int r = end_exec();
	db_ = 0;

	return r;
}

//清除语句结果
int xpg_sql::end_exec(void)
{
	if (res_){
		PQclear(res_);  res_ = 0;
	}
	irow_ = -1;

    if (params_){
        free(params_);  params_ = 0;  param_count_ = 0;
    }
    if (pcols_){
        free(pcols_);  pcols_ = 0; col_count_ = 0;
    }

	return 0;
}

//判断执行语句的类型
int xpg_sql::check_exec(void)
{
	ExecStatusType st = PQresultStatus(res_);
    int r = check_code(st);
	if (st == PGRES_COMMAND_OK)
		;
	else if (st == PGRES_TUPLES_OK){
		rows_ = PQntuples(res_);
		cols_ = PQnfields(res_);
	}

	return r;
}

//prepare执行一条sql语句
int xpg_sql::preexec(const char * psql, int parm_class, ...)
{
	end_exec();

	va_list vparms;
	va_start(vparms, parm_class);
    va_end(vparms);

    int r = vbind(parm_class, vparms);

	res_ = PQprepare(db_->db_, "", psql, param_count_, prms_oid);

	ExecStatusType st = PQresultStatus(res_);

    return check_code(st);
}

//prepare执行一条sql语句
int xpg_sql::exec(const char * psql, int parm_class, ...)
{
	end_exec();

	va_list vparms;
	va_start(vparms, parm_class);
    int r = vbind(parm_class, vparms);
    va_end(vparms);

	if (r == XSQL_OK) {
		res_ = PQprepare(db_->db_, "", psql, param_count_, prms_oid);
		ExecStatusType st = PQresultStatus(res_);
		r = check_code(st);
	}

    if (r != XSQL_OK)  return r;

	PQclear(res_);  res_ = NULL;

    return exec();
}

//链接存在则执行一条sql语句
int xpg_sql::exec(const char * psql)
{
    if (!isopen())
        return -1;

	end_exec();

	res_ = PQexec(db_->db_, psql);

    return check_exec();
}

//执行一次prepare过的语句
int xpg_sql::exec()
{
	// 将 parm 中参数值放入temp表中
	int i;
	for (i = 0; i < param_count_; i++){
		xparam_data * p = prms_temp_vs_ + i;
		switch(prms_types_[i])
		{
			// 普通类型的值,存储于temp数组中
			case XPARM_SHORT:  p->s = htons(u_short(*(params_[i].pshort))); break;
			case XPARM_INT  :  p->i = htonl(uint32_t(*(params_[i].pi)));	break;
			case XPARM_LONG :  p->l = (b64bit? htobe64(*((uint64_t*)params_[i].pd)) : htonl(uint32_t(*(params_[i].pl))));  break;
			case XPARM_FLOAT:  p->u = htobe32(*((uint32_t*)params_[i].pf)); break;
			case XPARM_DOUBLE: p->ll= htobe64(*((uint64_t*)params_[i].pd)); break;
		}
	}

	// 调用执行函数
	if (res_) {
		PQclear(res_);  res_ = NULL;
	}
	res_ = PQexecPrepared(db_->db_, "", param_count_, prms_v_, prms_lens_, prms_format_, 0);

	// 校验结果
	return check_exec();
}


//绑定列表并获取值
int xpg_sql::geta(const char * psql, int parm_class, ...)
{
	va_list vparms;
	va_start(vparms, parm_class);
    int r = vbind(parm_class, vparms);
    va_end(vparms);

	if (r == XSQL_OK) {
		res_ = PQprepare(db_->db_, "", psql, param_count_, prms_oid);
		ExecStatusType st = PQresultStatus(res_);
		r = check_code(st);
	}

	if (r != XSQL_OK)  return r;

	PQclear(res_);  res_ = NULL;

    return geta();
}

//获取值到列表
int xpg_sql::geta(void)
{
	int r = exec();

	if (r == XSQL_OK){
		r = fetch();
	}

	if (r == XSQL_OK)
		end_exec();
	return r;
}

//绑定一个参数
int xpg_sql::bind_a_parameter(int i) // bind single parameter
{
	if (i >= param_count_)
		return -1;

	XParm * p = params_ + i;

	unsigned short nclass, ntype;

	nclass = XPARM_CLASS(p->type);
    if (nclass != XPARM_IN && nclass != XPARM_OUT && nclass != XPARM_IO)
		return -2;

	prms_types_[i] = ntype = XPARM_TYPE(p->type);
	
    switch(ntype)
    {
		// 普通类型的值,存储于temp数组中
        case XPARM_SHORT: prms_oid[i] = p_oid.oid_short; break;
		case XPARM_INT  : prms_oid[i] = p_oid.oid_int;   break;
		case XPARM_LONG : prms_oid[i] = p_oid.oid_long;  break;
        case XPARM_FLOAT: prms_oid[i] = p_oid.oid_float; break;
        case XPARM_DOUBLE:prms_oid[i] = p_oid.oid_double;break;

		// 字符串的值,不用转存
		case XPARM_DATETIME:              // format: 'YYYY-MM-DD hh:mm:ss.000'
		case XPARM_DATE:
		case XPARM_TIME:
		case XPARM_STR:
			prms_v_[i] = (char *)(p->pv);
			prms_types_[i] = 0;
			prms_oid[i] = p_oid.oid_text;
			prms_format_[i]= 0;
			break;

		// 不能识别的类型
		default:  return -3;
    }
	
	if (prms_oid[i] != p_oid.oid_text){
		prms_v_[i] = (char *)(prms_temp_vs_+i);
		prms_format_[i]= 1;
		prms_lens_[i] = p->size;
	}
	
	printf("%d: size=%d, oid=%d, format=%d\n", i, int(p->size), int(prms_oid[i]), prms_format_[i]);

    return 0;
}

//绑定参数
int xpg_sql::bind_parameters(void)
{
    if (!isopen())  return -1;

	for (int i = 0; i < param_count_; i++)
        if (bind_a_parameter(i))  return -(i+1);

    return 0;
}

//分配空间并绑定列和参数
int xpg_sql::vbind(int parm_class, va_list v)
{
    int ncolcnt, nparmcnt;

	va_list d;
	va_copy(d, v);

    int r = parm_col_count(parm_class, v, ncolcnt, nparmcnt);
	if (r < 0)
		return r;

    if (ncolcnt > 0){
        if (pcols_ != NULL){
			free(pcols_);
			pcols_ = NULL;
		}
		pcols_ = (XParm *)calloc(ncolcnt + 1, sizeof(XParm));
        if (pcols_ == NULL)
            return -1;
    }

    if (nparmcnt > 0){
        if (params_ != NULL){  free(params_);  params_ = NULL;  }
        if ((params_ = (XParm *)calloc(nparmcnt + 1, sizeof(XParm))) == NULL)
            return -2;
    }

	col_count_  = ncolcnt;
	param_count_= nparmcnt;

	printf("col = %d, parm = %d, sizeof(XParm)=%d\n", ncolcnt, nparmcnt, int(sizeof(XParm)));

    r = buildxparms(parm_class, d, pcols_, params_);

    if (nparmcnt > 0)
        if (bind_parameters())  return -3;

    return 0;
}

//将游标加1 如果有绑定列则给绑定列取值
int xpg_sql::fetch(void)
{
	++irow_;

	// 检查是否走到头
	if (irow_ >= rows_)
		return XSQL_NO_DATA;

	// 如果有绑定列，则给绑定列取值
	int i;
	XParm * p = pcols_;
	for (i = 0; i < col_count_ && i < cols_; i++, p++){
		const char * pv = PQgetvalue(res_, irow_, i);

		switch(XPARM_TYPE(p->type))
		{
			// 普通类型的值,存储于temp数组中
			case XPARM_SHORT:	*(p->pshort) = short(atoi(pv));  break;
			case XPARM_LONG :
			case XPARM_INT  :   *(p->pi) = atoi(pv);  break;
			case XPARM_FLOAT:   *(p->pf) = float(atof(pv));  break;
			case XPARM_DOUBLE:  *(p->pd) = atof(pv);  break;

			// 字符串的值,不用转存
			case XPARM_DATETIME:              // format: 'YYYY-MM-DD hh:mm:ss.000'
			case XPARM_DATE:
			case XPARM_TIME:
			case XPARM_STR:		strncpy((char *)p->ps, pv, p->size);  break;
		}
	}

	return XSQL_OK;
}

//获取字符串数据到buf 返回长度
int xpg_sql::get_col(char * v, int length, int icol)
{
	if (v == 0 || length < 2 || icol < 0)
		return -1;

	*v = 0;
	const char *pv = get_col(icol);
	if (pv){
		strncpy(v, pv, length);  v[length-1] = 0;
	}

    return strlen(v);
}
//获取字符串数据 返回指针
const char * xpg_sql::get_col(int icol)
{
	if (res_ == 0 || irow_ < 0 || icol < 0 || PQgetisnull(res_, irow_, icol))
		return 0;

	return PQgetvalue(res_, irow_, icol);
}

//获取整形数据 返回整形值
int xpg_sql::get_col_i(int icol)
{
	const char * pv = get_col(icol);
	if (pv == 0 || *pv == 0)
		return 0;

    return atoi(pv);
}

//获取浮点数据 返回浮点值
double xpg_sql::get_col_d(int icol)
{
	const char * pv = get_col(icol);
	if (pv == 0 || *pv == 0)
		return 0;

    return atof(pv);
}

//获取错误信息 返回信息长度
int xpg_sql::get_error(char *msg, int max_len)
{
	if (msg == 0 || max_len < 2)
		return 0;

	const char * p = get_error();
	if (p && *p){
		strncpy(msg, p, max_len-1);  msg[max_len-1] = 0;
	}else
		*msg = 0;

	return strlen(msg);
}

const char* xpg_sql::get_error(void)
{
	return PQresultErrorMessage(res_);
}

int xpg_sql::get_rows(void) { return atoi(PQcmdTuples(res_)); }
const char* xpg_sql::column_name(int i) { return PQfname(res_, i); }

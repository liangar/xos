#include <xsys.h>
#include <l_str.h>

#include <xsqlite3.h>

static int henv = 0;

static int buildxparms(int nparm, va_list vlist, XParm *pcol, XParm *pparm);
static int getparmcnts(int nparm, va_list vlist, int& ncolcnt, int& nparmcnt);
static int checkcode(int n)
{
    switch (n){
    case SQLITE_OK:			    return XSQL_OK;
    case SQLITE_ROW:            return XSQL_HAS_DATA;
    case SQLITE_DONE:           return XSQL_OK;
    }
    return XSQL_ERROR;
}

xsqlite3::xsqlite3(const char * szconnectionstring)
    : m_pdb(0)
    , m_pconnection(0)
	, m_lastTime(0)
{
	m_db_lock = 0;
	if (szconnectionstring != NULL){
        int l = strlen(szconnectionstring) + 1;
        if ((m_pconnection = (char *)malloc(l)) == NULL)  return;
        strcpy(m_pconnection, szconnectionstring);
    }
	m_lastsql[0] = 0;
}

xsqlite3::~xsqlite3()
{
    close();
}

void xsqlite3::getconnect_string(char * d)
{
	if (m_pconnection)
		strcpy(d, m_pconnection);
	else
		*d = '\0';
}

void xsqlite3::getconnect_string(char ** d)
{
	*d = 0;
	if (m_pconnection){
		*d = new char[strlen(m_pconnection) + 1];
		strcpy(*d, m_pconnection);
	}
}

// open and close
int xsqlite3::open(const char * szconnectionstring)
{
    if (szconnectionstring == NULL && m_pconnection == NULL)
        return XSQL_ERROR_PARAM;

	if (m_pdb != 0 && close() != 0)
			return XSQL_ERROR;

	// update m_pconnection
	{
        int l = strlen(szconnectionstring) + 1;
        m_pconnection = (char *)realloc(m_pconnection, l);
        strcpy(m_pconnection, szconnectionstring);
    }

    sqlite3 * pdb;
    int r = sqlite3_open(m_pconnection, &pdb);
    if (r == SQLITE_OK)
        m_pdb = pdb;

	return r;
}

int xsqlite3::close(bool bforce)
{
	int r = SQLITE_OK;
    
    if (m_pdb){
        r =  sqlite3_close(m_pdb);		// sqlite3_close_v2
		if (r != SQLITE_OK && bforce){
            return checkcode(r);
		}
		if (r == SQLITE_OK){
            m_pdb = 0;
            if (m_pconnection){
                free(m_pconnection);  m_pconnection = 0;
        	}
		}
    }
    return r;
}

int xsqlite3::isopen(void)  {  return m_pdb != 0;  }

static const char * g_locktype[] = {
    "BEGIN", "BEGIN IMMEDIATE", "EXCLUSIVE"
};

int xsqlite3::trans_begin(int locktype)
{
    if (locktype < 0 || locktype > 2)
        locktype = 0;

    int r = sqlite3_exec(m_pdb, g_locktype[locktype], 0, 0, 0);
	int i = checkcode(r);
	return i;
}

int xsqlite3::trans_end(bool isok)
{
	int n;
    if (m_pdb == NULL)  return XSQL_ERROR;

	if (m_db_lock)
		m_db_lock->lock(300);

	if (isok)
		n = sqlite3_exec(m_pdb, "COMMIT", 0, 0, 0);
	else
		n = sqlite3_exec(m_pdb, "ROLLBACK",0,0, 0);

	if (m_db_lock)
		m_db_lock->unlock();

    int r = checkcode(n);

	return r;
}

void xsqlite3::set_lastsql(const char * psqlstring)
{
	m_lastTime = long(time(0));
	strncpy(m_lastsql, psqlstring, sizeof(m_lastsql));
	m_lastsql[sizeof(m_lastsql)-1] = 0;
}

xsql3::xsql3(xsqlite3 * pdb)
    : m_pstmt(0)
    , m_pparams(NULL)
    , m_pcols(NULL)
    , m_nparams(0)
    , m_ncols(0) 
{
    m_pdb = pdb;
}

xsql3::~xsql3()
{
    endexec();
}

void xsql3::setdbconnection(xsqlite3 * pdb)
{
    endexec();
	m_pdb = pdb;
}

int xsql3::isopen(void)
{
    return m_pstmt != 0;
}

int xsql3::open(void)
{
    if (m_pdb == NULL)  return -1;
    if (!m_pdb->isopen() && m_pdb->open())  return -2;

    if (isopen()){
		close();
	}
    
	return XSQL_OK;
}

int xsql3::close(void)
{
    int r = endexec();

	return r;
}

int xsql3::preexec(const char * psql)
{
	if (open())
        return -1;

    sqlite3_stmt * pstmt;
    int r = sqlite3_prepare_v2(m_pdb->m_pdb, psql, -1, &pstmt, NULL);
	if (r == SQLITE_OK){
        m_pstmt = pstmt;
	}else
		printf("preexec:sqlite3_prepare_v2(%d): %s\n", r, psql);

    m_pdb->set_lastsql(psql);

    return checkcode(r);
}

// folowing the preexecsql()
int xsql3::exec(void)
{
    // bind parameter values
	int r = bindparameters();
	if (r)
		return r;
    
    // call step to execute    
    return checkcode(sqlite3_step(m_pstmt));
}

int xsql3::endexec(void)
{
    int r = SQLITE_OK;
    if (m_pstmt != 0){
        int r = sqlite3_finalize(m_pstmt); 
        if (r != SQLITE_OK){
			return r;
		}
        m_pstmt = 0;
    }
    if (m_pparams != NULL){
        free(m_pparams);  m_pparams = NULL;  m_nparams = 0;
    }
    if (m_pcols != NULL){
        free(m_pcols);  m_pcols = NULL;  m_ncols = 0;
    }

    return r;
}

int xsql3::preexec(const char * psql, int nparmclass, ...)
{
    int n = preexec(psql);
    if (n != XSQL_OK)  return n;

	va_list vparms;
	va_start(vparms, nparmclass);
    n = vbind(nparmclass, vparms);
    va_end(vparms);

    return n;
}

int xsql3::directexec(const char * psql, int nparmclass, ...)
{
    int r = preexec(psql);
    if (r != XSQL_OK)  return r;

	va_list vparms;
	va_start(vparms, nparmclass);
    r = vbind(nparmclass, vparms);
    va_end(vparms);

    if (r != XSQL_OK)  return r;

    return exec();
}

int xsql3::directexec(const char * psql)
{
    if (open())
        return -1;

    int r = preexec(psql);
    if (r != XSQL_OK)  return r;

    r = exec();

	if (r == XSQL_OK)
		endexec();

    return r;
}

int xsql3::geta(const char * psql, int nparmclass, ...)
{
    int r = preexec(psql);
    if (r != XSQL_OK)  return r;

	va_list vparms;
	va_start(vparms, nparmclass);
    r = vbind(nparmclass, vparms);
    va_end(vparms);

    if (r != XSQL_OK)  return r;

	return geta();
}

int xsql3::geta(void)
{
	int n = bindparameters();
	if (n == XSQL_OK){
		n = fetch();
		if (n == XSQL_HAS_DATA){
			endexec();
			return XSQL_OK;
		}
	}
	return n;
}

int xsql3::bindaparameter(XParm * p, int i) // bind single parameter
{
    if (m_pstmt == 0)  return -1;

    if (XPARM_CLASS(p->type) != XPARM_IN)
        return -2;

	int r;

    switch(XPARM_TYPE(p->type))
    {
    case XPARM_LONG:
    case XPARM_INT:
        r = sqlite3_bind_int(m_pstmt, i, *(p->pi));
        break;

    case XPARM_LSTR: // XPARM_CHAR:
    case XPARM_DATETIME:              // format: 'YYYY-MM-DD hh:mm:ss'
	case XPARM_DATE:
	case XPARM_TIME:
    case XPARM_STR:
        r = sqlite3_bind_text(m_pstmt, i, p->ps, -1, SQLITE_STATIC);
        break;

    case XPARM_DOUBLE:
		r = sqlite3_bind_double(m_pstmt, i, *(p->pd));
        break;

    case XPARM_BIN:
        r = sqlite3_bind_text(m_pstmt, i, p->ps, -1, SQLITE_STATIC);
        break;

    default:  return -2;
    }

    return r;
}

int xsql3::bindparameters(void)
{
    return bindparameters(m_pparams);
}

int xsql3::bindparameters(XParm * pparams)
{
    if (m_pstmt == 0)  return -1;
	if (pparams == 0)  return XSQL_OK;

	for (int i = 1; pparams->type != XPARM_END; i++, pparams++)
        if (bindaparameter(pparams, i))  return -i;
    return XSQL_OK;
}

int xsql3::bindparameters(XParm * pparam, int startpos, int n)
{
    if (m_pstmt == 0 || startpos < 1)  return -1;

    for (int i = 0; i < n; i++, pparam++)
        if (bindaparameter(pparam, startpos + i))  return -(startpos + i);
    return XSQL_OK;
}

int xsql3::bind(int nparmclass, ...)
{
	va_list vparms;
	va_start(vparms, nparmclass);
    int r = vbind(nparmclass, vparms);
    va_end(vparms);
    return r;
}

int xsql3::bindcols(int nparmclass, ...)
{
	va_list vparms;
	va_start(vparms, nparmclass);
    int r = vbindcols(nparmclass, vparms);
    va_end(vparms);
    return r;
}

int xsql3::vbind(int nparmclass, va_list v)
{
    int ncolcnt, nparmcnt;

    getparmcnts(nparmclass, v, ncolcnt, nparmcnt);
    if (ncolcnt > 0){
        if (m_pcols != NULL)  delete[] m_pcols;
        if ((m_pcols = new XParm[ncolcnt + 1]) == NULL)
            return -1;
    }
    if (nparmcnt > 0){
        if (m_pparams != NULL)  delete[] m_pparams;
        if ((m_pparams = new XParm[nparmcnt + 1]) == NULL)
            return -2;
    }
    buildxparms(nparmclass, v, m_pcols, m_pparams);
        
    m_nparams = nparmcnt;  m_ncols = ncolcnt;

    return 0;
}

int xsql3::vbindcols(int nparmclass, va_list v)
{
    int ncolcnt, nparmcnt;

    getparmcnts(nparmclass, v, ncolcnt, nparmcnt);
    if (ncolcnt > 0){
        if (m_pcols != NULL)  delete[] m_pcols;
        if ((m_pcols = new XParm[ncolcnt + 1]) == NULL)
            return -1;
    }
    buildxparms(nparmclass, v, m_pcols, NULL);
    m_ncols = ncolcnt;

    return 0;
}

int xsql3::fetch(void)
{
    int r;

    if (m_pstmt == 0)  return -1;
	r = sqlite3_step(m_pstmt);
    if (r != SQLITE_ROW){
        if (r == SQLITE_DONE)
            return XSQL_NO_DATA;
        return checkcode(r);
    }

    int i;
    LPXParm p;
    for (i = 0, p = m_pcols; i < m_ncols; i++, p++){
        r = sqlite3_column_type(m_pstmt, i);
        p->isnull = (r == SQLITE_NULL);

        switch (XPARM_TYPE(p->type)){
        case XPARM_LONG:
        case XPARM_INT:
            if (p->isnull)
                *(p->pi) = 0;
            else
                *(p->pi) = sqlite3_column_int(m_pstmt, i);
            break;
    
        case XPARM_LSTR: // XPARM_CHAR:
        case XPARM_DATETIME:              // format: 'YYYY-MM-DD hh:mm:ss'
    	case XPARM_DATE:
    	case XPARM_TIME:
        case XPARM_STR:
            if (p->isnull)
                *(p->ps) = 0;
            else
                strncpy(p->ps, (const char *)sqlite3_column_text(m_pstmt, i), p->size);
            break;
    
        case XPARM_DOUBLE:
            if (p->isnull)
                *(p->pd) = 0;
            else
        		*(p->pd) = sqlite3_column_double(m_pstmt, i);
            break;
    
        case XPARM_BIN:
            if (p->isnull)
                *(p->ps) = 0;
            else
                r = sqlite3_bind_text(m_pstmt, i, p->ps, -1, SQLITE_STATIC);
            break;
        }
    }
    return XSQL_HAS_DATA; 
}

int xsql3::resetcursor(void)
{
	return sqlite3_reset(m_pstmt);
}

int xsql3::getcol(char * pbuf, int length, int ncol)
{
	int r = sqlite3_column_type(m_pstmt, ncol);
    if (r != SQLITE_NULL)
        strncpy(pbuf, (const char *)sqlite3_column_text(m_pstmt, ncol), length);
    else
        *pbuf = 0;

    return r;
}

int xsql3::getcol(long * pbuf, int ncol)
{
    int r = sqlite3_column_type(m_pstmt, ncol);
    if (r != SQLITE_NULL){
        *pbuf = sqlite3_column_int(m_pstmt, ncol); 
    }else
        *pbuf = 0;
    return r;
}

int xsql3::getcol(double * pbuf, int ncol)
{
    int r = sqlite3_column_type(m_pstmt, ncol);
    if (r != SQLITE_NULL){
        *pbuf = sqlite3_column_double(m_pstmt, ncol); 
    }else
        *pbuf = 0;
    return r;
}

int xsql3::gettables(char *ptables, int maxlen)
{
    if (open())  return -1;

	char tableName[64];

    int r = preexec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name",
        XPARM_COL | XPARM_STR, tableName, sizeof(tableName),
        XPARM_END        
    );
    
    if (r != XSQL_OK)
        return r;

    int i, n;
    for (i = n = 0; n < maxlen && fetch() == XSQL_OK; ){
        int l = strlen(tableName);
		if (l == 0 || n + l + 1 >= maxlen)  break;

        memcpy(ptables + n, tableName, l+1);
        n += l + 1;

		i++;
    }
	ptables[n] = '\0';
    endexec();

	return i;
}

int xsql3::getcolumns(char * pcolumns, char * ptable, int maxlen)
{
    if (open())  return -1;

    int n;
    int irow;
    char s[128];
	sprintf(s, "PRAGMA table_info(%s)", ptable);
	n = directexec(s,
        XPARM_COL | XPARM_INT, &irow,
                    XPARM_STR, s, sizeof(s),
        XPARM_END
    );

    int i = 0;
	if (n == XSQL_OK) {
		while (fetch() == XSQL_OK && n < maxlen - int(strlen(s)) - 1){
			strcpy(pcolumns+n, s);
			n += strlen(pcolumns+n) + 1;
            i++;
		}
		pcolumns[n] = '\0';
	}
	endexec();
	return i;
}

int xsql3::getcolumns(char * pcolumns, XParm * pcols, char * ptable, int maxlen, int maxnum)
{
    if (open())  return -1;

    int n;
    int irow;
    char s[128];
    char t[16];
	sprintf(s, "PRAGMA table_info(%s)", ptable);
	n = directexec(s,
        XPARM_COL | XPARM_INT, &irow,
                    XPARM_STR, s, sizeof(s),
                    XPARM_STR, t, sizeof(t),
        XPARM_END
    );

    int i = 0;
	if (n == XSQL_OK) {
		while (fetch() == XSQL_OK && n < maxlen - int(strlen(s)) - 1){
            memset(pcols+i, 0, sizeof(XParm));
            pcols[i].ps = pcolumns+n;
            
			strcpy(pcolumns+n, s);
			n += strlen(pcolumns+n) + 1;

            switch (t[0]){
            case 'i':  pcols[i].type = XPARM_INT;  break;
			case 'v':
            case 't':  pcols[i].type = XPARM_STR; break;
			case 'n':
            case 'f':  pcols[i].type = XPARM_DOUBLE;  break;
            case 'b':  pcols[i].type = XPARM_BIN;  break;    
            }
 
            i++;
		}
		pcolumns[n] = '\0';
	}
	endexec();
	return i;
}

int xsql3::geterror(char *msg, int msglength)
{
	*msg = '\0';

    if (m_pstmt == 0){
        strncpy(msg, "the sql statement is not open.", msglength);
        msg[msglength - 1] = '\0';
        return 0;
    }

	m_pdb->geterror(msg, msglength);
    return XSQL_OK;
}

int xsql3::errcode(void)
{
	return m_pdb->errcode();
}

static int getparmcnts(int nparm, va_list vlist, int& ncolcnt, int& nparmcnt)
{
	unsigned int nclass, ntype;
	bool bind;

	ncolcnt = nparmcnt = 0;
	nclass = XPARM_IN;

	while ((ntype = XPARM_CLASS(nparm)) != XPARM_END){
		if(ntype != XPARM_SAME) nclass = ntype; // 检测参数种类是否改变

		if (nclass == XPARM_COL)
			ncolcnt++;
		else if (nclass == XPARM_IN || nclass == XPARM_OUT || nclass == XPARM_IO)
			nparmcnt++;
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

static int buildxparms(int nparm, va_list vlist, XParm *pcol, XParm *pparm)
{
	unsigned int nclass, ntype;
	XParm tmp;
	XParm * p;

	nclass = XPARM_IN;

	while ((ntype = XPARM_CLASS(nparm)) != XPARM_END){
		if(ntype != XPARM_SAME) nclass = ntype; // 检测参数种类是否改变

		ntype = XPARM_TYPE(nparm);

		if (nclass == XPARM_COL){
			if (pcol != NULL)
				p = pcol++;
			else
				p = &tmp;
		}else{
			if (pparm != NULL)
				p = pparm++;
			else
				p = &tmp;
		}

        p->pv = va_arg(vlist, void *);
		if (ntype == XPARM_STR || ntype == XPARM_BIN || ntype == XPARM_LSTR)
			p->size = va_arg(vlist, long);  // IO参数还需一个表示接收缓冲区最大长度的long变量.
		p->type = XPARM_MAKE_TYPE(nclass, nparm);

        nparm = va_arg(vlist, int);             // 下一个参数
	}

	if (pcol)  pcol->type = XPARM_END;
	if (pparm) pparm->type = XPARM_END;

    return 0;
}

int buildxparamscols(XParm *pparms, XParm * pcols, int nparmclass, ...)
{
    if (pparms == NULL)  return -1;

    int ncolcnt, nparmcnt;
    va_list vparms;
    va_start(vparms, nparmclass);
    getparmcnts(nparmclass, vparms, ncolcnt, nparmcnt);
    if ((ncolcnt != 0 && pcols == NULL) ||
	    (nparmcnt!= 0 && pparms== NULL)){
        va_end(vparms);  return -2;
    }
    buildxparms(nparmclass, vparms, pcols, pparms);
    va_end(vparms);
    return 0;
}

static int colcompare(const void *p0, const void *p1)
{
	return stricmp((LPXParm(p0))->ps, (LPXParm(p1))->ps);
}

int xsql3::getcolumns_sorted(char *pcolumns, XParm *pcols, char *ptable, int maxlen, int maxnum)
{
	int i = getcolumns(pcolumns, pcols, ptable, maxlen, maxnum);
	if (i > 0)
		qsort(pcols, i, sizeof(XParm), colcompare);
	return i;
}

int xsql3::gettablerows(int & rows, char * ptablename)
{
	char sql[128];

    sprintf(sql, "SELECT COUNT(*) FROM %s", ptablename);
	return geta(sql, XPARM_COL|XPARM_LONG, &rows, XPARM_END);
}

int xsql3::getresultcolumns(int & cols)
{
    cols = sqlite3_data_count(m_pstmt);
    return (cols? XSQL_OK : XSQL_NO_DATA);
}

int xsqlite3::geterror(char *p, int maxlen)
{
    *p = 0;
    strncpy(p, sqlite3_errmsg(m_pdb), maxlen);  p[maxlen-1] = 0;
    return XSQL_OK;
}

int xsqlite3::errcode(void)
{
	return sqlite3_errcode(m_pdb);
}

int xsql3::getcolumnname(char *pname, int maxlen, int pos)
{
    const char * v;
	v = sqlite3_column_name(m_pstmt, pos);
    if (v)
    	strncpy(pname, v, maxlen);
    return (v? XSQL_OK : -1);
}

int xsqlite3_open(bool ispooling)
{
    if (henv)
        return SQLITE_OK;

    int r = sqlite3_initialize();
    if (r == SQLITE_OK)
        henv = 1;
    return r;
}

int xsqlite3_close(void)
{
    if (henv == 0)
        return SQLITE_OK;
    return sqlite3_shutdown();
}

#include <xsqlite3_pool.h>

#include <l_str.h>
#include <ss_service.h>

xsqlite3_pool::xsqlite3_pool()
	: m_maxnum(6)
	, m_maxfreetime(600)
	, m_idle(600)
	, m_maxlifetime(1800)
{
	set_null();
}

xsqlite3_pool::~xsqlite3_pool()
{
	down();
}

void xsqlite3_pool::set_null(void)
{
	m_dsn[0] = 0;
	m_lastmsg[0] = 0;
}

int xsqlite3_pool::init(
	const char * dsn,
	int maxnum, long maxfreetime, long idle, long maxlifetime)
{
	strncpy(m_dsn, dsn, sizeof(m_dsn));
	m_dsn[sizeof(m_dsn)-1] = 0;

	m_maxnum = maxnum;
	m_maxfreetime = maxfreetime;
	m_maxlifetime = maxlifetime;
	m_idle = idle;

	// 时间不能太小
	if (m_maxfreetime < 60)
		m_maxfreetime = 60;
	if (m_maxlifetime < 120)
		m_maxlifetime = m_maxfreetime * 2;
	if (m_idle < 30)
		m_idle = 30;

	//默认6个连接
	if (m_maxnum < 3)
		m_maxnum = 6;
	
	m_list.init(m_maxnum, m_maxnum / 2);

	m_db_lock.init();

	m_mutex.init();
	m_estop.init(0);

	//线程管理连接池
	m_monitor.init(schedule, this);

	m_lastmsg[0] = 0;

	return 1;
}

void xsqlite3_pool::down(void)
{
	m_estop.set();
	m_monitor.down();

	m_mutex.down();
	m_estop.down();

	for (int i = 0; i < m_list.m_count; i++){
		m_list[i].pdb->trans_end(false);
		m_list[i].pdb->close();
		delete m_list[i].pdb;
	}

	//modify by Freeman 2016-09-30 Must be after trans_end
	m_db_lock.down();

	m_list.free_all();

	set_null();
}

xsqlite3 * xsqlite3_pool::open(void)
{
	static const char szFunctionName[] = "xsqlite3_pool::open";

	if (m_mutex.lock(3000) != 0){
		sprintf(m_lastmsg, "%s: lock(3000) path error", szFunctionName);
		return 0;
	}

	m_lastmsg[0] = 0;
	xsqlite3 * pdb = 0;	// store return xsqlite3

	int i;
	for (i = 0; i < m_list.m_count; i++){
		xdb_control * pcon = m_list.get(i);

		if (pcon->isused == false){
			pdb = pcon->pdb;
			if (pdb != 0 && !pdb->isopen()){
				int r = pdb->open(m_dsn);
				if (r != XSQL_OK){
					sprintf(m_lastmsg, "%s: cannot open database connection.", szFunctionName);
					pdb = 0;
				}
			}
			if (pdb){
				pdb->setlock(&m_db_lock);
				pdb->trans_begin();
				sprintf(m_lastmsg, "%s: use (%d).", szFunctionName, i);
				pcon->isused = true;
			}
			break;
		}
	}

	if (i == m_list.m_count){
		// 产生新连接
		pdb = new xsqlite3;
		int r = pdb->open(m_dsn);

		if (r != XSQL_OK){
			sprintf(m_lastmsg, "%s: cannot open database connection.", szFunctionName);
			delete pdb;
			pdb = 0;
		}else{
			pdb->setlock(&m_db_lock);
			pdb->trans_begin();
			if (m_list.m_count < m_maxnum){
				xdb_control con;
				memset(&con, 0, sizeof(con));
				con.pdb = pdb;
				con.isused = true;
				con.createtime = long(time(0));
				m_list.add(&con);
				sprintf(m_lastmsg, "%s: add to pool ok(%d).", szFunctionName, m_list.m_count);
			}else
				sprintf(m_lastmsg, "%s: ok(%d).", szFunctionName, m_list.m_count);
		}
		WriteToEventLog(m_lastmsg);
	}

	m_mutex.unlock();

	if (pdb){
		pdb->m_lastsql[0] = 0;
		pdb->m_lastTime = long(time(0));
	}
	return pdb;
}

void xsqlite3_pool::close(xsqlite3 * pdb, bool isok)
{
	static const char szFunctionName[] = "xsqlite3_pool::close";

	if (pdb == 0)  return;

	pdb->trans_end(isok);

	int i;
	for (i = 0; i < m_list.m_count; i++){
		xdb_control * pcon = m_list.get(i);
		if (pcon->pdb == pdb){
			if (!isok){
				if (pcon->pdb){
					pcon->pdb->m_lastTime = long(time(0));
					pcon->pdb->close();
				}
			}
			m_list[i].isused = false;
			sprintf(m_lastmsg, "%s: push back to pool ok(%d).", szFunctionName, i);
			break;
		}
	}
	if (i == m_list.m_count){
		if (pdb){
			int r = pdb->close();
			delete pdb;
			sprintf(m_lastmsg, "%s: free db ok(%d).", szFunctionName, r);
		}
	}
}

unsigned int xsqlite3_pool::schedule(void * data)
{
	static const char szFunctionName[] = "xsqlite3_pool::schedule";
	xsqlite3_pool * p = (xsqlite3_pool*)data;

	int t = 0;
	while (p->m_estop.wait(p->m_idle) == ERR_TIMEOUT){
		if (p->m_mutex.lock(3000))
			continue;

		for (int i = 0; i < p->m_list.m_count; i++){
			xdb_control * pcon = p->m_list.get(i);
			if (pcon->isused || pcon->pdb == 0)
				continue;
			if (!pcon->pdb->isopen())
				continue;

			t = int(time(0));
			if (t  >= pcon->pdb->m_lastTime + p->m_maxfreetime ||
				t  >= pcon->createtime + p->m_maxlifetime)
			{
				pcon->pdb->trans_end(false);
				pcon->pdb->close();
				if (t  >= pcon->pdb->m_lastTime + p->m_maxfreetime)
					pcon->pdb->m_lastTime = t;
				if (t  >= pcon->createtime + p->m_maxlifetime)
					pcon->createtime = t;
				WriteToEventLog("%s: 到时间释放(%d)", szFunctionName, i);
			}
		}
		p->m_mutex.unlock();
	}

	return 0;
}

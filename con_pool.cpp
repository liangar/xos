#include <con_pool.h>

#include <l_str.h>
#include <ss_service.h>

con_pool::con_pool()
	: m_maxnum(6)
	, m_maxlifetime(1800)
	, m_maxfreetime(600)
	, m_concount(0)
	, m_idle(600)
	, m_numno(0)
{
	set_null();
}

con_pool::~con_pool()
{
	down();
}

void con_pool::set_null(void)
{
	m_service_name[0] = m_uid[0] = m_pwd[0] = 0;
	m_lastmsg[0] = 0;
}

int con_pool::init(
	const char * service_name,
	const char * uid,
	const char * pwd,
	int maxnum, long maxfreetime, long maxlifetime, long idle)
{
	strncpy(m_service_name, service_name, sizeof(m_service_name));
	strncpy(m_uid, uid, sizeof(m_uid));
	strncpy(m_pwd, pwd, sizeof(m_pwd));

	m_maxnum = maxnum;
	m_maxfreetime = maxfreetime;
	m_maxlifetime = maxlifetime;
	m_idle = idle;

	if (m_maxnum < 3)
		m_maxnum = 6; //默认6个连接
	
	m_list.init(m_maxnum, m_maxnum / 2);

	m_mutex.init();
	m_estop.init(0);

	//线程管理连接池
	m_monitor.init(schedule, this);

	m_lastmsg[0] = 0;

	return 1;
}

int con_pool::init(const char * dsn, int maxnum, long maxfreetime, long maxlifetime, long idle)
{
	char server[64], uid[64], pwd[64], t[64];
	server[0] = '\0';  uid[0] = '\0';  pwd[0] = '\0';
	
	char * pdsn = new char[strlen(dsn) + 1];
	strcpy(pdsn, dsn);
	
	const char * p = pdsn;
	while(p && *p){
		p = getaword(t, p, '=');
		if (stricmp(t, "server") == 0 || stricmp(t, "dsn") == 0){
			p = getaword(server, p, ';');
		}else if (stricmp(t, "uid") == 0 || stricmp(t, "user") == 0) {
			p = getaword(uid, p, ';');
		}else if (stricmp(t, "pwd") == 0 || stricmp(t, "password") == 0) {
			p = getaword(pwd, p, ';');
		}
	}
	delete[] pdsn;	
	
	return init(server, uid, pwd, maxnum, maxfreetime, maxlifetime, idle);
}

void con_pool::down(void)
{
	m_estop.set();
	m_monitor.down();

	m_mutex.down();
	m_estop.down();

	for (int i = 0; i < m_list.m_count; i++){
		m_list[i].con->close();
		delete m_list[i].con;
	}

	m_list.free_all();

	set_null();
}

connection * con_pool::open()
{
	static const char szFunctionName[] = "con_pool::open";

	if (m_mutex.lock(10) != 0){
		sprintf(m_lastmsg, "%s: lock(10) path error", szFunctionName);
		WriteToEventLog("%s: %s", szFunctionName, m_lastmsg);
		return 0;
	}

	m_lastmsg[0] = 0;
	connection * pdb = 0;	// store return connection

	int i;
	try{
		for (i = 0; i < m_list.m_count; i++){
			con_control * pcon = m_list.get(i);

			if (pcon->isused == false)
			{
				pdb = pcon->con;
				if (!pdb->is_opened)
				{
					pdb->open(m_service_name, m_uid, m_pwd);
					pcon->createtime = long(time(0));
				}
				pcon->isused = true;
				break;
			}
		}
		if (i == m_list.m_count){
			// 产生新连接
			pdb = new connection;
			pdb->open(m_service_name, m_uid, m_pwd);

			if (m_list.m_count < m_maxnum){
				con_control con;
				memset(&con, 0, sizeof(con));
				con.con = pdb;
				con.isused = true;
				con.createtime = long(time(0));
				m_list.add(&con);
				sprintf(m_lastmsg, "%s: add to pool ok(%d).", szFunctionName, m_list.m_count);
			}else
				sprintf(m_lastmsg, "%s: ok(%d).", szFunctionName, m_list.m_count);
		}
	}catch (oralib::error &e) {
		sprintf(m_lastmsg, "%s: %s", szFunctionName, (e.details()).c_str());
		if (i == m_list.m_count && i > 0 && pdb){
			delete pdb;
			pdb = 0;
		}
	}
	WriteToEventLog("%s: %s", szFunctionName, m_lastmsg);

	m_mutex.unlock();

	return pdb;
}

void con_pool::close(connection * con, bool isok)
{
	if (con == 0)
		return;

	int i;
	for (i = 0; i < m_list.m_count; i++){
		con_control * pcon = m_list.get(i);
		if (pcon->con == con){
			if (!isok){
				pcon->con->rollback();
				pcon->con->close();
			}
			m_list[i].isused = false;
			break;
		}
	}
	if (i == m_list.m_count){
		con->close();
		delete con;
	}
}

unsigned int con_pool::schedule(void * data)
{
	static const char szFunctionName[] = "con_pool";

	con_pool * p = (con_pool*)data;

	int t = 0;
	while (p->m_estop.wait(p->m_idle) == ERR_TIMEOUT){
		if (p->m_mutex.lock(10))
			continue;

		for (int i = 0; i < p->m_list.m_count; i++){
			con_control * pcon = p->m_list.get(i);
			if (pcon->isused)
				continue;

			if (t  >= pcon->con->lastTime + p->m_maxfreetime ||
				t  >= pcon->createtime + p->m_maxlifetime)
			{
				pcon->con->close();

				if (t  >= pcon->createtime + p->m_maxlifetime)
					pcon->createtime = t;
				WriteToEventLog("%s: 到时间释放(%d)", szFunctionName, i);
			}
		}
		p->m_mutex.unlock();
	}

	return 0;
}

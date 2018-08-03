#include <xsys.h>

#include <ss_service.h>

#include <xwork_server.h>

void xwork_server::init_props(void)
{
	getnowtime(m_createtime);
	m_laststart[0] = '\0';
	m_cmd[0] = m_parms[0] = '\0';

	m_heartbeat = 0;
	m_run_times = 0;

	m_works = 0;
	m_idle = 30;

	m_laststarttime = 0;

	m_requests = m_successes = m_failes = m_waitinges = 0;

	m_exceptiontimes = 0;
	m_starttimes = 0;
	m_phworker = 0;
	m_pprop_lock = 0;

	m_isrun = false;
}

xwork_server::xwork_server()
{
	init_props();
	set_attributes("xwork_server", 5, 1);
}

xwork_server::xwork_server(const char * name, int nmaxexception, int works)
{
	init_props();
	set_attributes(name, nmaxexception, works);
}

xwork_server::~xwork_server()
{
	close();
}

void xwork_server::set_attributes(const char * name, int nmaxexception, int works)
{
	strncpy(m_name, name, sizeof(m_name));	m_name[sizeof(m_name) - 1] = '\0';
	m_maxexception = nmaxexception;
	m_works = works;
}

bool xwork_server::open(void)
{
	if (m_hnotify.init(1) != 0)
		return false;

	m_laststart[0] = '\0';
	m_cmd[0] = m_parms[0] = '\0';

	m_pprop_lock = new xsys_mutex;
	m_pprop_lock->init();

	m_phworker = new xsys_thread[m_works];
	m_exceptiontimes = 0;
	m_heartbeat = long(time(0));
	m_run_times = 0;

	if (m_phworker == 0){
		m_pprop_lock->down();
		delete m_pprop_lock;  m_pprop_lock = 0;
		return false;
	}

	return true;
}

bool xwork_server::close(int secs)
{
	if (m_phworker == 0){
		return true;
	}

	stop();

	m_hnotify.down();

	delete[] m_phworker;
	m_phworker = 0;

	if (m_pprop_lock){
		m_pprop_lock->down();  delete m_pprop_lock;  m_pprop_lock = 0;
	}

	return true;
}

void xwork_server::getinfo(char * msg, int maxlen)
{
	if (msg == 0 || maxlen == 0) {
		return;
	}
	char b[256];
	char h[32];
	time2simple(h, m_heartbeat);
	sprintf(b,
		"name            : %s\n"
		"workers         : %d\n"
		"create time     : %s\n"
		"run times       : %d\n"
		"last start time : %s\n"
		"exception limit : %d\n"
		"exception times : %d\n"
		"heart beat      : %s"
		,
		m_name,
		m_works,
		m_createtime,
		m_starttimes,
		m_laststart,
		m_maxexception,
		m_exceptiontimes,
		h
	);
	strncpy(msg, b, maxlen);
}

bool xwork_server::start(void)
{
	static const char szFunctionName[] = "xwork_server::start";

    WriteToEventLog("%s[%d] : begin.", szFunctionName, m_works);

	bool isok = false;
	strcpy(m_cmd, "start");		/// add cmd = start (2011-12-27)
	m_isrun = true;
	for (int i = 0; i < m_works; i++){
		sprintf(m_parms, "%d", i);	/// set parm = thread seqno. (2011-12-27)
		int r = m_phworker[i].init(master, this);
		if (r < 0){
			WriteToEventLog("%s : 不能建立第%d个工作线程, error=[%d]", m_name, i, r);
		}else{
			isok = true;
			m_starttimes++;
			m_heartbeat = m_laststarttime = long(time(0));
			time2string(m_laststart, m_laststarttime);
		}
		xsys_sleep(1);	/// 启动时间至少相隔 1s，保证线程可以在下一次启动之前，得到 cpu 时间
	}
    WriteToEventLog("%s : end(%s).", szFunctionName, (isok?"ok" : "error"));
	return isok;
}

bool xwork_server::stop(int seconds)
{
	static const char szFunctionName[] = "xwork_server::stop";

    WriteToEventLog("%s[%d, %s] : start.", szFunctionName, seconds, m_name);

	m_isrun = false;

	notify_stop();
	xsys_sleep_ms(300);
	if (m_phworker){
		for (int i = 0; i < m_works; i++){
			int r = m_phworker[i].down(seconds);
			if (r != 0){
				m_exceptiontimes++;
			}
		}
	}

    WriteToEventLog("%s[%d, %s] : end.", szFunctionName, seconds, m_name);

	return true;
}

bool xwork_server::restart(int secs)
{
	static const char szFunctionName[] = "xwork_server::restart";
	if (!stop(secs)){
	    WriteToEventLog("%s[%d] : 停止服务不成功.", szFunctionName, secs);
		return false;
	}
	return start();
}

void xwork_server::notify(const char * cmd, const char * parms)
{
	if (lock_prop(2) < 0)	///
		return;

	if (cmd){
		strncpy(m_cmd, cmd, sizeof(m_cmd)-1);  m_cmd[sizeof(m_cmd)-1] = '\0';
	}else
		m_cmd[0] = '\0';

	if (parms){
		strncpy(m_parms, parms, sizeof(m_parms)-1);  m_parms[sizeof(m_parms)-1] = '\0';
	}else
		m_parms[0] = '\0';

	m_hnotify.set();

	unlock_prop();
}

void xwork_server::notify_stop(void)
{
	lock_prop(1);
	m_isrun = false;
	unlock_prop();

	notify("stop");
}

unsigned int xwork_server::master(void * data)
{
	static const char szFunctionName[] = "xwork_server::master";
	
	xwork_server * q = (xwork_server *)data; 

	WriteToEventLog("%s : in.", q->m_name);
	while (q->m_isrun){
		try{
			q->m_run_times++;
			q->run();

			if (!q->m_isrun || stricmp(q->m_cmd, "stop") == 0)
				break;

			if (q->m_idle == 0)
				q->m_idle = 30;

			int r = q->m_hnotify.wait(q->m_idle);

			if (r == ERR_TIMEOUT){
				q->lock_prop(1);
				q->m_cmd[0] = 0;
				q->unlock_prop();
			}else
				if (stricmp(q->m_cmd, "stop") == 0)
					break;

			q->lock_prop(1);
			if (r != ERR_TIMEOUT){
				q->m_hnotify.reset();
			}
			q->m_heartbeat = long(time(0));
			/// 打印运行情况LOG
			char h[32];
	        time2simple(h, q->m_heartbeat);
			q->unlock_prop();

			WriteToEventLog("%s: 服务执行情况[%s]第[%d]次运行[%s]", szFunctionName,q->m_name,q->m_run_times,h);
		}catch(...){
			q->m_exceptiontimes++;
			WriteToEventLog("%s : 执行异常,次数:%d", q->m_name, q->m_exceptiontimes);
			xsys_sleep(3);
		}
		if (q->m_exceptiontimes >= q->m_maxexception){
			break;
		}
	}
	WriteToEventLog("%s : stop.", q->m_name);
	return 0;
}

void xwork_server::clear_notify(void)
{
	if (lock_prop(1) < 0)
		return;
	m_hnotify.reset();
	m_cmd[0] = 0;
	m_parms[0] = 0;
	unlock_prop();
}

int xwork_server::lock_prop(int seconds)
{
	if (m_pprop_lock == 0)  return -1;

	return m_pprop_lock->lock(seconds*1000);
}

void xwork_server::unlock_prop(void)
{
	if (m_pprop_lock)
		m_pprop_lock->unlock();
}


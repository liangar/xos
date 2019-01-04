#include <xsys_udp_server.h>

#define XSYS_UDP_SESSION_TIMEOUT(i) 	((m_session_ttl < m_heartbeat - m_psessions[i].last_recv_time) || \
			(15 < m_heartbeat - m_psessions[i].last_recv_time && \
			 m_psessions[i].last_recv_time == m_psessions[i].createTime \
			))

xsys_udp_server::xsys_udp_server()
	: xwork_server("XUDP Server", 5, 1)
	, m_listen_port(0)
	, m_session_ttl(300)
	, m_psessions(0)
	, m_session_count(0)
	, m_precv_buf(0)
	, m_recv_len(0)
{
	m_serverURL[0] = 0;
	m_pused_index = NULL;
	m_used_count = 0;
}

xsys_udp_server::xsys_udp_server(const char * name, int nmaxexception)
	: xwork_server(name, nmaxexception, 1)	// 启动两个服务 0: accept/recv, 1: send
	, m_listen_port(0)
	, m_session_ttl(300)
	, m_psessions(0)
	, m_session_count(0)
	, m_precv_buf(0)
	, m_recv_len(0)
{
	m_pused_index = NULL;
	m_used_count = 0;
	m_serverURL[0] = 0;
	m_has_new_cmd = false;
}

static int calc_addr_crc(SYS_INET_ADDR * addr)
{
	int * p = (int *)addr;
	int i, s = 0;
	for (i = 0; i + sizeof(int) <= sizeof(SYS_INET_ADDR); i += sizeof(int)){
		s ^= *p;
		++p;
	}
	return s;
}

// 关闭已用标记的第i_used个会话，返回实际的序号
int xsys_udp_server::session_close_used(int i_used)
{
	if (i_used < 0 || i_used >= m_used_count)
		return -1;

	int i = m_pused_index[i_used];

	WriteToEventLog("Close i_used=%d, i = %d, used count = %d", i_used, i, m_used_count);
	session_close(i);

	--m_used_count;
	if (m_used_count > i_used)	// 用于防护意外
		memmove(m_pused_index + i_used, m_pused_index+i_used+1, (m_used_count-i_used)*sizeof(int));

	return i;
}

int xsys_udp_server::session_close_used_by_i(int i_session)
{
	if (i_session < 0 || i_session >= m_session_count)
		return i_session;

	int i;
	for (i = m_used_count-1; i >= 0 && m_pused_index[i] != i_session; i--)
		;

	if (i >= 0)
		return session_close_used(i);

	WriteToEventLog("使用队列中没找到，直接关闭<%d>", i_session);

	session_close(i_session);

	return i_session;
}

/*
void xsys_udp_server::timeout_check(void)
{
	static const char szFunctionName[] = "xsys_udp_server::timeout_check";

	int i;

	lock_prop(2);	///

	int n = m_used_count;
	for (i = m_used_count-1; i >= 0; i--){
		int j = m_pused_index[i];

		if (XSYS_UDP_SESSION_TIMEOUT(j)){
			WriteToEventLog("%s: client[%d/%d] timeout[%d/20](%d - %d)", szFunctionName, i, j, m_session_ttl, int(m_psessions[j].createTime), m_heartbeat);
			// 由于close之后，m_used_count会减1，且之后的会前移，故此i保持不变
			session_close_used(i);
		}
	}
	if (n > 0 && m_used_count < 1)
		m_recv_queue.clear();

	unlock_prop();
}
//*/

int xsys_udp_server::opened_find(int * crc, SYS_INET_ADDR * addr)
{
	static const char szFunctionName[] = "xsys_udp_server::opened_find";
	int i, n = -1;

	if (addr && crc)
		*crc = calc_addr_crc(addr);
	else
		if (crc)
			*crc = 0;

	for (i = m_used_count-1; i >= 0; i--){
		int j = m_pused_index[i];
		// 用CRC和addr进行比较查找
		if (addr && crc && m_psessions[j].addr_crc == *crc &&
			memcmp(&m_psessions[j].addr, addr, sizeof(SYS_INET_ADDR)) == 0)
		{
			n = j;
			continue;
		}
	}

	return n;
}

bool xsys_udp_server::open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len)
{
	max_sessions += 4 + max_sessions / 8;

	if (max_sessions > 1024)
		max_sessions = 1024;
	else{
		if (max_sessions < 2)
			max_sessions = 4;
		else
			max_sessions += 4;
	}

	m_recv_queue.init(max( 4, (recv_len+1023) / 1024 * (max_sessions / 2) + 2), max(max_sessions, 4));
	m_listen_port = listen_port;
	m_session_ttl = ttl;

	m_recv_len = recv_len;
	if (m_recv_len < 1024)
		m_recv_len = 1024;
	m_precv_buf = (char *)calloc(m_recv_len+4, 1);

	m_psessions = (struct xudp_session *)calloc(max_sessions, sizeof(struct xudp_session));
	m_pused_index = (int *)calloc(max_sessions, sizeof(int));
	m_session_count = max_sessions;

	xudp_session * p = m_psessions;

	m_send_len = max(send_len, 32);
	m_send_queue.init(max(4, send_len/1024*(max_sessions/5+2)), max(max_sessions/2+1, 4));
	m_close_requests.init(4, max_sessions*8/1024+4);

	WriteToEventLog("xsys_udp_server::open: TTL=%d, max session=%d, recv_len=%d, send_len=%d",
		m_session_ttl, max_sessions, m_recv_len, m_send_len
	);

	m_sock_lock.init();
	m_plisten_sock = new xsys_socket;

	return xwork_server::open();
}

bool xsys_udp_server::open(const char * url,int ttl, int recv_len)
{
	strncpy(m_serverURL, url, sizeof(m_serverURL)-1);  m_serverURL[sizeof(m_serverURL)-1] = 0;
	return open(0, ttl, 32, recv_len, 2048);
}

static unsigned int run_send_thread(void * pudp_server)
{
	try{
		xsys_udp_server * pserver = (xsys_udp_server *)pudp_server;
		pserver->send_server();
	}catch(...){
		WriteToEventLog("xsys_udp_server::send_server: an exception occured.");
	}
	return 0;
}

static unsigned int run_msg_thread(void * pudp_server)
{
	try{
		xsys_udp_server * pserver = (xsys_udp_server *)pudp_server;
		pserver->msg_server();
	}catch(...){
		WriteToEventLog("xsys_udp_server::msg_server: an exception occured.");
	}
	return 0;
}

void xsys_udp_server::run(void)
{
	static const char szFunctionName[] = "xsys_udp_server::run";

	WriteToEventLog("%s : in.\n", szFunctionName);

	if (m_plisten_sock == NULL)
		m_plisten_sock = new xsys_socket;

	if (!m_plisten_sock->isopen() && m_plisten_sock->udp_listen(m_listen_port) != 0){
		WriteToEventLog("%s : 创建消息监听(%d)失败，退出运行", szFunctionName, m_listen_port);
		return;
	}

	WriteToEventLog("%s: %s\n", szFunctionName, m_plisten_sock->get_self_ip(m_lastmsg));

	// 启动发送线程
	if (m_send_thread.m_thread == SYS_INVALID_THREAD)
		m_send_thread.init(run_send_thread, this);
	if (m_msg_thread.m_thread == SYS_INVALID_THREAD)
		m_msg_thread.init(run_msg_thread, this);

	// wait the other worker in ready
	xsys_sleep_ms(100);

	int idle_count = 0;
	long time_check_time = long(time(0));
	while (m_isrun && m_plisten_sock->isopen()) {

		// 处理关闭通知消息
		while (!m_close_requests.isempty()){
			// get event
			int i = -1;
			*((time_t *)m_precv_buf) = 0;
			int n = m_close_requests.get_free((long *)&i, m_precv_buf);

			// check
			if (n != sizeof(time_t)){
				WriteToEventLog("%s: 关闭%d无效, len=%d", szFunctionName, i, n);
				continue;
			}

			// deal
			time_t t = *((time_t *)m_precv_buf);
			if (t <= m_psessions[i].createTime){
				WriteToEventLog("%s: 忽略过期关闭%d, %d <= %d", szFunctionName, i, t, m_psessions[i].createTime);
				continue;
			}

			WriteToEventLog("%s: get close %d request", szFunctionName, i);
			lock_prop(2);
			session_close_used_by_i(i);
			unlock_prop();
		}

		m_heartbeat = long(time(0));

		/// 检查所有空闲的session，并调用处理方法
		lock_prop(1);
		{
			int n = m_used_count;
			int i;
			for (i = m_used_count-1; i >= 0; i--){
				int isession = m_pused_index[i];

				if (XSYS_UDP_SESSION_TIMEOUT(isession)){
					WriteToEventLog("%s: client[%d/%d] timeout[%d/20](%d - %d)", szFunctionName, i, isession, m_session_ttl, int(m_psessions[isession].createTime), m_heartbeat);
					// 由于close之后，m_used_count会减1，且之后的会前移，故此i保持不变
					session_close_used(i);
				}else if (m_psessions[isession].idle_secs > 0 &&
					m_heartbeat >= m_psessions[isession].last_trans_time + m_psessions[isession].idle_secs)
				{
					char parms[32];
					sprintf(parms, "idle:%d", isession);
					notify_do_cmd(parms);
				}
			}
			if (n > 0 && m_used_count < 1)
				m_recv_queue.clear();
		}
		unlock_prop();

		int crc;

		SYS_INET_ADDR from_addr;
		ZeroData(from_addr);
		memset(m_precv_buf, 0, m_recv_len);
		int len = m_plisten_sock->recv_from(m_precv_buf, m_recv_len-1, &from_addr, 2000);

		m_heartbeat = long(time(0));

		if (len == ERR_TIMEOUT){
			++idle_count;
			if ((idle_count & 0x00FF) == 0){
				/*
				m_sock_lock.lock(1500);
				m_plisten_sock->shutdown();
				delete m_plisten_sock;
				m_plisten_sock = new xsys_socket;
				int r = m_plisten_sock->udp_listen(m_listen_port);
				m_sock_lock.unlock();
				WriteToEventLog("%s: reopen(%d) %d -> %d", szFunctionName, m_listen_port, idle_count, r);
				//*/
			}
			continue;
		}

		if (len <= 0){
			m_sock_lock.lock(1500);
			m_plisten_sock->close();
			delete m_plisten_sock;  m_plisten_sock = NULL;
			WriteToEventLog(
				"%s: recv return = %d(parms len = %d)",
				szFunctionName, len, m_recv_len
			);

//			this->send(-1, "rest");

//			xsys_sleep_ms(500);
			m_plisten_sock = new xsys_socket;
			m_plisten_sock->udp_listen(m_listen_port);
			m_sock_lock.unlock();

			continue;
//*/
//			m_idle = 5;
//			break;
		}

		{
			char ip_port[64];
			SysInetRevNToA(from_addr, ip_port, MAX_IP_LEN);
			WriteToEventLog(
				"peer ip: %s(used count = %d/%d)\n",
				ip_port, m_used_count, (m_used_count > 0)?m_pused_index[m_used_count-1]:0
			);
		}

		// 找session，并检查timeout的会话
		int i = opened_find(&crc, &from_addr);

		if (i == -1){
			WriteToEventLog("新建会话:%04X, time = %d", crc, m_heartbeat);

			lock_prop(1);
			if (m_used_count > 0){
				if (m_session_count == m_used_count){
					WriteToEventLog("强制最早建立的会话过期，used = %d", m_used_count);
					i = session_close_used(0);
				}else{
					// 从最后的开始找
					i = m_pused_index[m_used_count-1] + 1;
					if (i < 1 || i >= m_session_count){
						if (i < 1 || i > m_session_count)
							WriteToEventLog("%s: 超出范围出错[%d]=%d", szFunctionName, --m_used_count, i);
						i = 0;
					}
					int n;
					for (n = 0; n < m_session_count; n++){
						if (!session_isopen(i))
							break;
						++i;
						if (i == m_session_count)
							i = 0;
					}
				}
			}else{
				m_used_count = 0;  i = 0;
			}
			// 至此，已经找到free的i，并且实施使用
			session_open(i);
			m_pused_index[m_used_count++] = i;
			memcpy(&m_psessions[i].addr, &from_addr, sizeof(SYS_INET_ADDR));
			m_psessions[i].addr_crc = crc;

			unlock_prop();
		}

		// receive data
		m_precv_buf[len] = '\0';
		int r = session_recv(i, len);	// 将数据存入session的临时缓存

		if (r >= len){
			m_psessions[i].recv_state = XTS_RECVING;
			// 检查接收状态
			// 返回首个完整包的应有长度，0 < 为非协议包
			r = calc_msg_len(i);
			WriteToEventLog("%s: client[%d, len=%d], msg_len=%d, time = %d", szFunctionName, i, len, r, m_heartbeat);
		}else{
			WriteToEventLog("%s: client[%d, len=%d] save error, r=%d, time = %d", szFunctionName, i, len, r, m_heartbeat);
			r = -1;
			m_psessions[i].recv_state = XTS_RECV_ERROR;
		}

		if (r <= 0){
			// 无效或无用数据，则释放
			write_buf_log("xsys_udp_server recved INVALID data", (unsigned char *)m_precv_buf, len);
			m_psessions[i].recv_state = XTS_RECV_ERROR;
		}else{
			if (r <= m_psessions[i].recv_len)
				m_psessions[i].recv_state = XTS_RECVED;
			else
				m_psessions[i].recv_state = XTS_RECVING;
		}

		switch (m_psessions[i].recv_state){
		case XTS_RECVED:
			WriteToEventLog("%s: %d - recved a packet data(%d/%d/%d)",
				szFunctionName, i,
				r, len, m_psessions[i].recv_len
			);
			// 将收到的完整包逐条发到request消息队列中
			while (r <= m_psessions[i].recv_len){
				// 发布
				if ((len = session_recv_to_request(i, r)) <= 0)
					break;
				r = calc_msg_len(i);
			}
			WriteToEventLog("%s: %d - send packet to recver", szFunctionName, i);
			xsys_sleep_ms(3);

			break;
		case XTS_RECV_ERROR:
			WriteToEventLog("%s: %d - recv error", szFunctionName, i);
			session_recv_reset(i);
			break;
		case XTS_RECV_EXCEPT:
		case XTS_SESSION_END:
			session_close(i);
			break;
		}
	}

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_udp_server::msg_server(void)
{
	static const char szFunctionName[] = "xsys_udp_server::msg_server";

	WriteToEventLog("%s : in", szFunctionName);

	char * ptemp_buf = (char *)calloc(1, m_recv_len+4);

	int idle = 120 * 1000;

	while (m_isrun){
		int isession = -1;

		m_recv_queue.set_timeout_ms(idle);

		int r;

		/// 取出数据
		memset(ptemp_buf, 0, m_recv_len);
		int len = m_recv_queue.get_free((long *)(&isession), ptemp_buf);

		try{
			if (len != ERR_TIMEOUT){
				if (len <= 0){
					WriteToEventLog("%s : len = %d, no data", szFunctionName, len);
					continue;
				}

				r = 0;
				if (isession < 0 || isession >= m_session_count){
					// 系统通知执行，或者出错
					if (isession == -1){
						if (strcmp(ptemp_buf, "stop") == 0)
							break;

						if (strncmp(ptemp_buf, "idle:", 5) == 0){
							// set session
							isession = atoi(ptemp_buf+5);
							r = do_idle(isession);
						}else if (strcmp(ptemp_buf, "cmds") == 0 || len < 1){
							WriteToEventLog("%s: load cmds", szFunctionName);
							r = do_cmd();
						}else{
							WriteToEventLog("%s: load cmds for(%s)", szFunctionName, ptemp_buf);
							r = do_cmd(ptemp_buf);
						}
					}else
						WriteToEventLog("%s: <%d>, len = %d, 超出会话界限,忽略", szFunctionName, isession, len);

					if (r == XSYS_IP_FATAL)
						notify_close_session(isession, true);

					continue;
				}

				/// 调用处理函数进行消息处理
				r = do_msg(isession, ptemp_buf, len);
				if (r == XSYS_IP_FATAL)
					notify_close_session(isession, true);

				WriteToEventLog("%s: <%d>处理%d长的数据，返回%d", szFunctionName, isession, len, r);
			}
		}catch(...){
			WriteToEventLog("%s: 出现执行异常", szFunctionName);
		}
	}

	::free(ptemp_buf);

	WriteToEventLog("%s : out", szFunctionName);
}

void xsys_udp_server::send_server(void)
{
	static const char szFunctionName[] = "xsys_udp_server::send_server";

	WriteToEventLog("%s : in.\n", szFunctionName);

	char * ptemp_buf = (char *)calloc(1, m_send_len+1);

	m_send_queue.set_timeout_ms(120*1000);

	// create a socket for send
	// xsys_socket sock;

	while (m_isrun){
		int i_session = -1;

		// bind to the local port
		// if (!sock.isopen() && sock.udp_listen(m_listen_port) != 0){
//		if (!m_plisten_sock || !m_plisten_sock->isopen()){
			// sock.m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			// int zero = 0;
			// setsockopt(sock.m_sock, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero));

			// WriteToEventLog("%s: open send sock error.", szFunctionName);
			// sock.close();
			// xsys_sleep_ms(800);
//			continue;
//		}

		int len = m_send_queue.get_free((long *)&i_session, ptemp_buf);

		if (!m_isrun)
			break;

		if (i_session == -1 && len == 4){
			WriteToEventLog("%s: get cmd[%s]", szFunctionName, ptemp_buf);
			if (strcmp(ptemp_buf, "stop") == 0)
				break;
			// sock.close();
			xsys_sleep_ms(500);
			continue;
		}

		if (len == ERR_TIMEOUT){
			continue;
		}

		if (i_session < 0 || i_session >= m_session_count){
			WriteToEventLog("%s: <%d>, len = %d, 超出会话界限,忽略", szFunctionName, i_session, len);
			continue;
		}

		write_buf_log(szFunctionName, (unsigned char *)ptemp_buf, len);

		// find session
		int i = i_session, t = int(time(0));

		if (i < 0 || i > m_session_count || !PUDPSESSION_ISOPEN(m_psessions+i)){
			WriteToEventLog("%s: 出错,试图在未打开的会话上发送(%d)", szFunctionName, i);
		}else{
			// send ...
			/*
			int trytimes = 0;
			while (m_plisten_sock == NULL && trytimes < 5){
				xsys_sleep_ms(300);  trytimes++;
			}
			if (m_plisten_sock == NULL){
				WriteToEventLog("%s: socket 没开，不能发送", szFunctionName);
				continue;
			}
			//*/

			m_sock_lock.lock(1500);
			int r = 0;
			if (m_plisten_sock)
				r = m_plisten_sock->sendto(ptemp_buf, len, &m_psessions[i].addr, 3);
			m_sock_lock.unlock();

			// int r = sock.sendto(ptemp_buf, len, &m_psessions[i].addr, 3);
			if (r <= 0){
				WriteToEventLog("%s: 发送错误(%d)(return = %d, time = %d)", szFunctionName, i, r, t);
				notify_close_session(i);
				// sock.close();
				xsys_sleep_ms(500);
			}else{
				WriteToEventLog("%s: the %dth session sent (ret = %d, len=%d, time = %d)", szFunctionName, i, r, len, t);

				lock_prop(1);
				m_psessions[i].last_trans_time = t;
				unlock_prop();

				on_sent(i, r);
			}
		}
	}

//	if (sock.isopen())
//		sock.close();

	::free(ptemp_buf);

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_udp_server::close_all_session(void)
{
	if (m_psessions){
		for (int i = m_session_count-1; i > 0 ; i--) {
			if (PUDPSESSION_ISOPEN(m_psessions+i)) {
				session_close(i);
			}
		}
	}
}

bool xsys_udp_server::stop(int secs)
{
	static const char szFunctionName[] = "xsys_udp_server::stop";

	WriteToEventLog("%s: in", szFunctionName);
	m_isrun = false;

	// notify send/recv thread stop
	m_send_queue.put(-1, "stop", 4);
	m_recv_queue.put(-1, "stop", 4);
	xsys_sleep_ms(10);

	if (m_plisten_sock)
		m_plisten_sock->close();

	bool isok = xwork_server::stop(secs);

	xsys_sleep_ms(100);

	m_send_thread.down();
	m_msg_thread.down();

	if (m_plisten_sock){
		m_plisten_sock->close();
		delete m_plisten_sock;
		m_plisten_sock = NULL;
	}

	xsys_sleep_ms(100);

	WriteToEventLog("%s: out(%d)", szFunctionName, isok);

	return isok;
}

bool xsys_udp_server::close(int secs)
{
	static const char szFunctionName[] = "xsys_udp_server::close";

	WriteToEventLog("%s[%d] : in", szFunctionName, secs);

	bool isok = xwork_server::close(secs);	// 调用 stop

	m_sock_lock.down();

	m_recv_queue.down();
	m_send_queue.down();

	xsys_sleep_ms(10);

	m_close_requests.down();

	if (m_psessions){
		close_all_session();
		::free(m_psessions);  m_psessions = 0;
	}

	if (m_pused_index){
		::free(m_pused_index);  m_pused_index = 0;
	}

	if (m_precv_buf){
		free(m_precv_buf);  m_precv_buf = 0;
	}

	WriteToEventLog("%s[%d] : out(%d)", szFunctionName, secs, isok);

	return isok;
}

void xsys_udp_server::session_open(int i)
{
	xudp_session * psession = m_psessions + i;

	memset(psession, 0, sizeof(xudp_session));

	psession->createTime =
		psession->last_recv_time  =
		psession->last_trans_time = m_heartbeat;

	// 如果不加，会是0，此数或许会被应用程序用到，故此设置为-1
	psession->peerid = -1;
	psession->running_cmdid = -1;
}

void xsys_udp_server::session_close(xudp_session * psession)
{
	static const char szFunctionName[] = "xsys_udp_server::session_close";

	if (psession->precv_buf){
		free(psession->precv_buf);
	}

	memset(psession, 0, sizeof(xudp_session));

	psession->recv_state = XTS_SESSION_END;
	psession->peerid = -1;
}

int xsys_udp_server::notify_close_session(int i, bool need_shift)
{
	time_t now = time(0);
	if (need_shift)
		++now;
	int r = m_close_requests.put(i, (char *)&now, sizeof(now));

	WriteToEventLog("udp_server notify_close_session: %d(r=%d) force=%d", i, r, (need_shift?1:0));

	if (need_shift && (i % 4) == 0)
		xsys_sleep_ms(50);

	return r;
}

bool xsys_udp_server::session_isopen(int i)
{
	if (i < 0 || i >= m_session_count)
		return false;

	xudp_session * psession = m_psessions + i;
	return PUDPSESSION_ISOPEN(psession);
}

void xsys_udp_server::session_close(int i)
{
	static const char szFunctionName[] = "xsys_udp_server::session_close";

	if (i < 0 || i >= m_session_count){
		WriteToEventLog("%s: i = %d is out range[0~%d]", szFunctionName, i, m_session_count);
		return;
	}

	WriteToEventLog("%s: i = %d will be closed", szFunctionName, i);

	xudp_session * psession = m_psessions + i;
	if (!PUDPSESSION_ISOPEN(psession)){
		WriteToEventLog("%s: i = %d is not opened", szFunctionName, i);
	}else{
		psession->recv_state = XTS_SESSION_END;
		on_closed(i);
	}

	session_close(psession);

	WriteToEventLog("%s: i = %d is closed", szFunctionName, i);
}

void xsys_udp_server::session_recv_reset(int i)
{
	xudp_session * psession = m_psessions + i;
	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = 0;
	}
	psession->recv_buflen = psession->recv_len = 0;
	psession->recv_state = XTS_RECV_READY;
}

// 将数据发布到request队列
int xsys_udp_server::session_recv_to_request(int i, int len)
{
	if (len < 1 || i < 0 || i > m_session_count-1){
		WriteToEventLog("session_recv_to_request(%d, %d): 参数出错", i, len);
		return 0;
	}

	xudp_session * psession = m_psessions + i;

	if (len > psession->recv_len)
		len = psession->recv_len;

	int r;

	if (PUDPSESSION_ISOPEN(psession))
		r = m_recv_queue.put(i, psession->precv_buf, len);
	else{
		// 正常情况下，有接收，会话应该不会被关闭
		r = 0;
		WriteToEventLog("会话%d已关闭，不发送", i);
	}

	psession->recv_len -= len;
	if (psession->recv_len <= 0){
		psession->recv_len = 0;
		psession->recv_state = XTS_RECV_READY;
	}else{
		char * p = psession->precv_buf;
		memmove(p, p+len, psession->recv_len);
	}

	WriteToEventLog("session_recv_to_queue: %d - send %d bytes to recver", i, r);

	return r;
}

int xsys_udp_server::session_recv(int i, int len)
{
	xudp_session * psession = m_psessions + i;

	lock_prop(1);
	m_psessions[i].last_trans_time = m_psessions[i].last_recv_time = m_heartbeat;
	unlock_prop();

	if (psession->recv_len < 0)
		psession->recv_len = 0;
	int l = psession->recv_len + len;
	if (psession->recv_buflen < l+1){
		void * p;
		p = realloc(psession->precv_buf, l+1);

		if (!p)
			return -1;

		psession->precv_buf = (char *)p;
		psession->recv_buflen = l+1;
	}
	memcpy(psession->precv_buf+psession->recv_len, m_precv_buf, len);
	psession->precv_buf[l] = 0;
	psession->recv_len = l;
	return l;
}

void xsys_udp_server::set_idle(int isession, int idle_secs)
{
	if (!PUDPSESSION_ISOPEN(m_psessions+isession))
		return;

	m_psessions[isession].idle_secs = idle_secs;
}

int xsys_udp_server::send(int isession, const char * s, int len)
{
	static const char szFunctionName[] = "xsys_udp_server::send";

	if (isession >= 0 && isession < m_session_count &&
		!PUDPSESSION_ISOPEN(m_psessions+isession))
	{
		WriteToEventLog("%s: 出错,试图在未打开的会话(%d)上发送(len=%d)", szFunctionName, isession, len);
		write_buf_log(szFunctionName, (unsigned char *)s, len);
		return -1;
	}
	return m_send_queue.put(isession, s, len);
}

int xsys_udp_server::send(int isession, const char * s)
{
	return send(isession, s, strlen(s));
}

void xsys_udp_server::notify_do_cmd(const char * cmd)
{
	if (!m_msg_thread.isopen())
		return;

	if (cmd == 0){
		m_recv_queue.put(-1, "cmds", 4);
		sign_new_cmd();
	}else
		m_recv_queue.put(-1, cmd, strlen(cmd));
}

void xsys_udp_server::sign_new_cmd(bool has_new)
{
	lock_prop(1);
	m_has_new_cmd = has_new;
	unlock_prop();
}

bool xsys_udp_server::has_new_cmd(void)
{
	bool b;
	lock_prop(1);
	b = m_has_new_cmd;
	unlock_prop();
	
	return b;
}

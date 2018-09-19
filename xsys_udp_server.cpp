#include <xsys_udp_server.h>

#define XSYS_UDP_SESSION_TIMEOUT(i) 	((m_session_ttl < m_heartbeat - m_psessions[i].last_trans_time) || \
			(20 < m_heartbeat - m_psessions[i].last_trans_time && \
			 m_psessions[i].last_trans_time == m_psessions[i].createTime \
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
	: xwork_server(name, nmaxexception, 1)	// ������������ 0: accept/recv, 1: send
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
	for (i = 0; i < sizeof(SYS_INET_ADDR); i += sizeof(int)){
		s ^= *p;
		++p;
	}
	return s;
}

// �ر����ñ�ǵĵ�i_used���Ự������ʵ�ʵ����
int xsys_udp_server::session_close_used(int i_used)
{
	if (i_used < 0 || i_used >= m_used_count)
		i_used = 0;

	int i = m_pused_index[i_used];

	WriteToEventLog("Close i_used=%d, i = %d, used count = %d", i_used, i, m_used_count);
	session_close(i);

	--m_used_count;
	if (m_used_count > i_used)	// ���ڷ�������
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

	WriteToEventLog("ʹ�ö�����û�ҵ���ֱ�ӹر�<%d>", i_session);

	session_close(i_session);

	return i_session;
}

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
			// ����close֮��m_used_count���1����֮��Ļ�ǰ�ƣ��ʴ�i���ֲ���
			session_close_used(i);
		}
	}
	if (n > 0 && m_used_count < 1)
		m_recv_queue.clear();

	unlock_prop();
}

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
		// ��CRC��addr���бȽϲ���
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
	m_precv_buf = (char *)calloc(m_recv_len+1, 1);

	m_psessions = (struct xudp_session *)calloc(max_sessions, sizeof(struct xudp_session));
	m_pused_index = (int *)calloc(max_sessions, sizeof(int));
	m_session_count = max_sessions;

	xudp_session * p = m_psessions;

	m_send_len = max(send_len, 32);
	m_send_queue.init(max(4, send_len/1024*(max_sessions/5+2)), max(max_sessions/2+1, 4));
	m_close_requests.init(4, max_sessions/2+1);

	WriteToEventLog("xsys_udp_server::open: TTL=%d, max session=%d, recv_len=%d, send_len=%d",
		m_session_ttl, max_sessions, m_recv_len, m_send_len
	);

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

	if (!m_listen_sock.isopen() && m_listen_sock.udp_listen(m_listen_port) != 0){
		WriteToEventLog("%s : ������Ϣ����(%d)ʧ�ܣ��˳�����", szFunctionName, m_listen_port);
		return;
	}

	WriteToEventLog("%s: %s\n", szFunctionName, m_listen_sock.get_self_ip(m_lastmsg));

	// ���������߳�
	m_send_thread.init(run_send_thread, this);
	m_msg_thread.init(run_msg_thread, this);

	// wait the other worker in ready
	xsys_sleep_ms(100);

	int	session_i = 0;		/// ѭ��������ʼλ��

	int conn_amount = 0, n, idle_times = 0;
	while (m_isrun && m_listen_sock.isopen()) {

		while (!m_close_requests.isempty()){
			int i = -1;
			int n = m_close_requests.get_free((long *)&i, m_precv_buf);
			if (n == 0){
				WriteToEventLog("%s: get close %d request", szFunctionName, i);
				lock_prop(2);
				session_close_used_by_i(i);
				unlock_prop();
			}
			xsys_sleep_ms(10);
		}

		m_heartbeat = long(time(0));

		int i;
		/// ������г�ʱ��session�������ô�����
		for (i = 0; i < m_used_count; i++){
			int isession = m_pused_index[i];
			if (m_psessions[isession].idle_secs > 0 &&
				m_heartbeat >= m_psessions[isession].last_trans_time + m_psessions[isession].idle_secs)
			{
				char parms[32];
				sprintf(parms, "idle:%d", isession);
				notify_do_cmd(parms);
			}
		}

		int crc;

		SYS_INET_ADDR from_addr;
		ZeroData(from_addr);
		int len = m_listen_sock.recv_from(m_precv_buf, m_recv_len-1, &from_addr, 2000);

		m_heartbeat = long(time(0));

		if (len == ERR_TIMEOUT || len == 0){
			++idle_times;

			if (idle_times >= 30){
				idle_times = 0;
				timeout_check();	// ֻ�Ǽ��timeout���رչ�������
			}

			continue;
		}

		if (len < 0)
			break;

		{
			char ip_port[64];
			SysInetRevNToA(from_addr, ip_port, MAX_IP_LEN);
			WriteToEventLog(
				"peer ip: %s(used count = %d/%d)\n",
				ip_port, m_used_count, (m_used_count > 0)?m_pused_index[m_used_count-1]:0
			);
		}

		// ��session�������timeout�ĻỰ
		i = opened_find(&crc, &from_addr);

		if (i == -1){
			WriteToEventLog("�½��Ự:%04X, time = %d", crc, m_heartbeat);
			if (m_used_count > 0){
				if (m_session_count == m_used_count){
					WriteToEventLog("ǿ�����罨���ĻỰ���ڣ�used = %d", m_used_count);
					i = session_close_used(0);
				}else{
					// �����Ŀ�ʼ��
					i = m_pused_index[m_used_count-1] + 1;
					if (i == m_session_count)
						i = 0;
					for (n = 0; n < m_session_count; n++){
						if (!session_isopen(i) || XSYS_UDP_SESSION_TIMEOUT(i))
							break;
						++i;
						if (i == m_session_count)
							i = 0;
					}
				}
			}else{
				m_used_count = 0;  i = 0;
			}
			// ���ˣ��Ѿ��ҵ�free��i������ʵʩʹ��
			if (session_isopen(i) && XSYS_UDP_SESSION_TIMEOUT(i)){
				lock_prop(2);	///
				session_close_used_by_i(i);
				unlock_prop();
			}
			session_open(i);

			lock_prop(2);	///
			m_pused_index[m_used_count++] = i;
			unlock_prop();

			memcpy(&m_psessions[i].addr, &from_addr, sizeof(SYS_INET_ADDR));
			m_psessions[i].addr_crc = crc;
		}

		// receive data
		m_precv_buf[len] = '\0';
		int r = session_recv(i, len);	// �����ݴ���session����ʱ����
		if (r >= len){
			m_psessions[i].recv_state = XTS_RECVING;
			// ������״̬
			// �����׸���������Ӧ�г��ȣ�0 < Ϊ��Э���
			r = calc_msg_len(i);
			WriteToEventLog("%s: client[%d, len=%d], msg_len=%d, time = %d", szFunctionName, i, len, r, m_heartbeat);
		}else{
			WriteToEventLog("%s: client[%d, len=%d] save error, r=%d, time = %d", szFunctionName, i, len, r, m_heartbeat);
			r = -1;
			m_psessions[i].recv_state = XTS_RECV_ERROR;
		}

		if (r <= 0){
			// ��Ч���������ݣ����ͷ�
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
			// ���յ�����������������request��Ϣ������
			while (r <= m_psessions[i].recv_len){
				// ����
				if ((n = session_recv_to_request(i, r)) <= 0)
					break;
				r = calc_msg_len(i);
			}
			WriteToEventLog("%s: %d - send packet to recver", szFunctionName, i);

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

	char * ptemp_buf = (char *)calloc(1, m_recv_len+1);

	int idle = 120 * 1000;

	while (m_isrun){
		int isession = -1;

		m_recv_queue.set_timeout_ms(idle);

		int r;

		/// ȡ������
		int len = m_recv_queue.get_free((long *)(&isession), ptemp_buf);

		try{
			if (len != ERR_TIMEOUT){
				if (len <= 0){
					WriteToEventLog("%s : len = %d, no data", szFunctionName, len);
					continue;
				}

				r = 0;
				if (isession < 0 || isession >= m_session_count){
					// ϵͳִ֪ͨ�У����߳���
					if (isession == -1){
						if (strcmp(ptemp_buf, "stop") == 0)
							break;

						if (strncmp(ptemp_buf, "idle:", 5) == 0){
							r = do_idle(atoi(ptemp_buf+5));
						}else if (strcmp(ptemp_buf, "cmds") == 0 || len < 1){
							WriteToEventLog("%s: do_cmd -> %d", szFunctionName, isession);
							r = do_cmd();
						}else{
							WriteToEventLog("%s: do_cmd(%s) -> %d", szFunctionName, ptemp_buf, isession);
							r = do_cmd(ptemp_buf);
						}
					}else
						WriteToEventLog("%s: <%d>, len = %d, �����Ự����,����", szFunctionName, isession, len);

					if (r == XSYS_IP_FATAL)
						notify_close_session(isession, false);

					continue;
				}

				/// ���ô�����������Ϣ����
				r = do_msg(isession, ptemp_buf, len);
				if (r == XSYS_IP_FATAL)
					notify_close_session(isession, false);

				WriteToEventLog("%s: <%d>����%d�������ݣ�����%d", szFunctionName, isession, len, r);
			}
		}catch(...){
			WriteToEventLog("%s: ����ִ���쳣", szFunctionName);
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
	xsys_socket sock;

	while (m_isrun && m_listen_sock.isopen()){
		int i_session = -1;

		// bind to the local port
		if (!sock.isopen())
			sock.udp_listen(m_listen_port);

		int len = m_send_queue.get_free((long *)&i_session, ptemp_buf);

		if (!m_isrun || (i_session == -1 && len == 4))
			break;

		if (len < 0){
			continue;
		}

		if (i_session < 0 || i_session >= m_session_count){
			WriteToEventLog("%s: <%d>, len = %d, �����Ự����,����", szFunctionName, i_session, len);
			continue;
		}

		write_buf_log(szFunctionName, (unsigned char *)ptemp_buf, len);

		// find session
		int i = i_session, t = int(time(0));

		if (i < 0 || i > m_session_count || !PUDPSESSION_ISOPEN(m_psessions+i)){
			WriteToEventLog("%s: ����,��ͼ��δ�򿪵ĻỰ�Ϸ���(%d)", szFunctionName, i);
		}else{
			// send ...
			int r = sock.sendto(ptemp_buf, len, &m_psessions[i].addr, 3);
			if (r <= 0){
				WriteToEventLog("%s: ���ʹ���(%d)(return = %d, time = %d)", szFunctionName, i, r, t);
				notify_close_session(i);
				sock.close();
			}else{
				WriteToEventLog("%s: the %dth session sent (ret = %d, len=%d, time = %d)", szFunctionName, i, r, len, t);
				on_sent(i, r);
			}
		}
	}

	sock.close();

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

	m_listen_sock.close();

	xsys_sleep_ms(100);

	// notify send/recv thread stop
	m_send_queue.put(-1, "stop", 4);
	xsys_sleep_ms(100);

	m_recv_queue.put(-1, "stop", 4);
	xsys_sleep_ms(100);

	m_send_thread.down();
	m_msg_thread.down();

	bool isok = xwork_server::stop(secs);

	WriteToEventLog("%s: out(%d)", szFunctionName, isok);

	return isok;
}

bool xsys_udp_server::close(int secs)
{
	static const char szFunctionName[] = "xsys_udp_server::close";

	WriteToEventLog("%s[%d] : in", szFunctionName, secs);

	bool isok = xwork_server::close(secs);	// ���� stop

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

	psession->createTime = time(0);
	psession->last_trans_time = long(psession->createTime);

	// ������ӣ�����0����������ᱻӦ�ó����õ����ʴ�����Ϊ-1
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
	int r = m_close_requests.put(i, (char *)&i, 0);

	WriteToEventLog("udp_server notify_close_session: %d(r = %d)", i, r);

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

// �����ݷ�����request����
int xsys_udp_server::session_recv_to_request(int i, int len)
{
	xudp_session * psession = m_psessions + i;

	if (len > psession->recv_len)
		len = psession->recv_len;

	int r = m_recv_queue.put(i, psession->precv_buf, len);

	psession->recv_len -= len;
	if (psession->recv_len <= 0){
		psession->recv_len = 0;
		psession->recv_state = XTS_RECV_READY;
	}else{
		char * p = psession->precv_buf;
		memmove(p, p+len, psession->recv_len);
	}

	xsys_sleep_ms(3);
	WriteToEventLog("session_recv_to_queue: %d - send %d bytes to recver", i, r);

	return r;
}

int xsys_udp_server::session_recv(int i, int len)
{
	xudp_session * psession = m_psessions + i;

	psession->last_trans_time = m_heartbeat;

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

	m_psessions[isession].last_trans_time = long(time(0));
	m_psessions[isession].idle_secs = idle_secs;
}

int xsys_udp_server::send(int isession, const char * s, int len)
{
	static const char szFunctionName[] = "xsys_udp_server::send";

	if (!PUDPSESSION_ISOPEN(m_psessions+isession)){
		WriteToEventLog("%s: ����,��ͼ��δ�򿪵ĻỰ(%d)�Ϸ���(len=%d)\n%s", szFunctionName, isession, len, s);
		return -1;
	}
	m_psessions[isession].last_trans_time = long(time(0));
	return m_send_queue.put(isession, s, len);
}

int xsys_udp_server::send(int isession, const char * s)
{
	return m_send_queue.put(isession, s, strlen(s));
}

void xsys_udp_server::notify_do_cmd(const char * cmd)
{
	if (!m_msg_thread.isopen())
		return;

	if (cmd == 0){
		m_recv_queue.put(-1, "cmds", 4);
		m_has_new_cmd = true;
	}else
		m_recv_queue.put(-1, cmd, strlen(cmd));
}

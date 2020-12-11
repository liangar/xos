#define FD_SETSIZE 512

#include <xsys_tcp_server2.h>

#define XSYS_TCP_SESSION_TIMEOUT(i) 	((m_session_ttl < m_heartbeat - m_psessions[i].last_recv_time) || \
			(40 < m_heartbeat - m_psessions[i].last_recv_time && \
			 m_psessions[i].last_recv_time == m_psessions[i].createTime \
			))
// (m_session_ttl < m_heartbeat - m_psessions[i].last_recv_time)

unsigned int xsys_tcp_server2::run_send_thread(void * ptcp_server)
{
	try{
		xsys_tcp_server2 * pserver = (xsys_tcp_server2 *)ptcp_server;
		pserver->send_server();
	}catch(...){
		WriteToEventLog("XTCP2::send_server: an exception occured.");
	}
	return 0;
}

unsigned int xsys_tcp_server2::run_msg_thread(void * ptcp_server)
{
	try{
		xsys_tcp_server2 * pserver = (xsys_tcp_server2 *)ptcp_server;
		pserver->msg_server();
	}catch(...){
		WriteToEventLog("XTCP2::msg_server: an exception occured.");
	}
	return 0;
}

xsys_tcp_server2::xsys_tcp_server2()
	: xwork_server("XTCP2 Server", 5, 1)
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

xsys_tcp_server2::xsys_tcp_server2(const char * name, int nmaxexception)
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
}

// �ر����ñ�ǵĵ�i_used���Ự������ʵ�ʵ����
int xsys_tcp_server2::session_close_used(int i_used)
{
	if (i_used < 0 || i_used >= m_used_count)
		i_used = 0;

	int i = m_pused_index[i_used];

	int used_count = m_used_count;
	WriteToEventLog("XTCP2: Close i_used=%d, <%d>, used_count=%d", i_used, i, used_count);
	session_close(i);

	--used_count;

	if (used_count > i_used)	// ���ڷ�������
		memmove(m_pused_index + i_used, m_pused_index+i_used+1, (used_count-i_used)*sizeof(int));
	m_used_count = used_count;

	return i;
}

int xsys_tcp_server2::session_close_used_by_i(int i_session)
{
	if (!XTCP2SESSION_INRANGE(i_session))
		return -1;

	int i;
	for (i = m_used_count-1; i >= 0 && m_pused_index[i] != i_session; i--)
		;

	if (i >= 0)
		return session_close_used(i);

	WriteToEventLog("ʹ�ö�����û�ҵ�,ֱ�ӹر�<%d>", i_session);

	session_close(i_session);

	return i_session;
}

void xsys_tcp_server2::timeout_check(void)
{
	static const char szFunctionName[] = "XTCP2::timeout_check";

	int i;

	for (i = 0; i < m_used_count && m_used_count > 0; i++){
		int j = m_pused_index[i];

		if (XSYS_TCP_SESSION_TIMEOUT(j)){
			WriteToEventLog("%s: client[%d] timeout[%d/20](%d - %d)", szFunctionName, j, m_session_ttl, int(m_psessions[j].createTime), m_heartbeat);
			session_close_used(i);
		}
	}
}

bool xsys_tcp_server2::open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len)
{
	// ���֧��1K�Ự
	if (max_sessions > 1024)
		max_sessions = 1024;

	m_listen_port = listen_port;
	m_session_ttl = ttl;
	m_session_idle = ttl / 4;
	if (m_session_idle < 3)
		m_session_idle = 3;

	m_recv_len = recv_len;	// ������ݰ��ĳ��ȣ��ɾ���ֵ�趨
	if (m_recv_len < 1024)
		m_recv_len = 1024;

	// �����Ự���Ļ���������
	m_recv_queue.init(max(4, (m_recv_len+1023) / 1024 * (max_sessions / 2) + 2), max(max_sessions, 4));

	m_precv_buf = (char *)calloc(m_recv_len+1, 1);

	// �п��ܶ��δ�ܾ��������ӣ��ʶ��4��
	max_sessions += 4;
	m_psessions =  (struct xtcp2_session *)calloc(max_sessions, sizeof(struct xtcp2_session));
	m_session_count = max_sessions;
	m_pused_index = (int *)calloc(max_sessions, sizeof(int));

	m_send_len = max(send_len, 32);
	m_send_queue.init(max(4, (send_len+1023)/1024*(max_sessions/5+2)), max(max_sessions/2+1, 4));
	m_close_requests.init(4, max_sessions/2+1);

	WriteToEventLog("XTCP2::open: TTL=%d, max session=%d, recv_len=%d, send_len=%d",
		m_session_ttl, max_sessions, m_recv_len, m_send_len
	);

	return xwork_server::open();
}

bool xsys_tcp_server2::open(const char * url,int ttl, int recv_len)
{
	strncpy(m_serverURL, url, sizeof(m_serverURL)-1);  m_serverURL[sizeof(m_serverURL)-1] = 0;
	return open(0, ttl, 32, recv_len, 2048);
}

void xsys_tcp_server2::run(void)
{
	static const char szFunctionName[] = "XTCP2::run";
	WriteToEventLog("%s : in.\n", szFunctionName);

	// ���������߳�
	if (m_send_thread.m_thread == SYS_INVALID_THREAD)
		m_send_thread.init(run_send_thread, this);

	if (m_msg_thread.m_thread == SYS_INVALID_THREAD)
		m_msg_thread.init(run_msg_thread, this);

	m_close_requests.set_timeout_ms(5);

	xsys_sleep_ms(500);
	// wait the other worker in ready
	// xsys_sleep_ms(100);

	int	session_i = 0;		/// ѭ��������ʼλ��

	int i, conn_amount = 0, n, idle_times = 0, trytimes = 0, error_recv = 0;
	int not_full_packet_count = 0;

	if (!m_listen_sock.isopen() && m_listen_sock.listen(m_listen_port) != 0){
		WriteToEventLog("%s : ������Ϣ����(%d)ʧ�ܣ��˳�����", szFunctionName, m_listen_port);
		return;
	}

	fd_set fdsr;
	memset(&fdsr, 0, sizeof(fdsr));

	struct timeval tv;

	int s_used_count = 0;

	while (m_isrun && trytimes < 3 && m_listen_sock.isopen()) {
		int used_count = m_used_count;
		if (used_count != s_used_count){
			WriteToEventLog("%s: ��������, %d, %d", szFunctionName, used_count, s_used_count);
			m_used_count = used_count = s_used_count;
		}

		// ���ر���������
		while (!m_close_requests.isempty()){
			i = -1;
			n = m_close_requests.get_free((long *)&i, m_precv_buf);
			if (n == 0){
				WriteToEventLog("%s: get close %d request", szFunctionName, i);
				lock_prop(2);
				if (session_close_used_by_i(i) >= 0)
					s_used_count--;
				unlock_prop();
			}
			// xsys_sleep_ms(10);
		}

		m_heartbeat = long(time(0));

		// check timeout or idle
		lock_prop(1);
		{
			n = s_used_count;
			int i;
			for (i = s_used_count-1; i >= 0; i--){
				int isession = m_pused_index[i];

				if (XSYS_TCP_SESSION_TIMEOUT(isession)){
					WriteToEventLog("%s: client[%d/%d] timeout[%d/20](%d - %d - %d)", szFunctionName, i, isession, m_session_ttl, int(m_psessions[isession].createTime), m_psessions[isession].last_recv_time, m_heartbeat);
					session_close_used(i);
					s_used_count--;
				}else if (m_psessions[isession].idle_secs > 0 &&
					m_heartbeat >= m_psessions[isession].last_trans_time + m_psessions[isession].idle_secs)
				{
					char parms[32];
					sprintf(parms, "idle:%d", isession);
					notify_do_cmd(parms);
				}
			}
			if (n > 0 && s_used_count < 1){
				m_recv_queue.clear();
			}
		}
		unlock_prop();

		if (error_recv > 0){
			/*
			m_listen_sock.close();
			xsys_sleep_ms(5);
			m_listen_sock.listen(m_listen_port);
			//*/
			error_recv = 0;
		}

		// get network event
		FD_ZERO(&fdsr);
		FD_SET(m_listen_sock.m_sock, &fdsr);
		tv.tv_sec  = 10;
		tv.tv_usec = 0;

		// add active connection to fd set
		n = m_listen_sock.m_sock;
		for (i = 0; i < s_used_count; i++) {
			int i_session = m_pused_index[i];
			if (XSYS_VALID_SOCK(m_psessions[i_session].sock)) {
				FD_SET(m_psessions[i_session].sock, &fdsr);

				if (n < int(m_psessions[i_session].sock))
					n = int(m_psessions[i_session].sock);
			}
		}
		WriteToEventLog("%s: 1 - %d", szFunctionName, ++n);

		m_heartbeat = long(time(0));

		int ret = ::select(n, &fdsr, NULL, NULL, &tv);

		if (ret < 0) {
			WriteToEventLog("%s: select error(%d), loop end", szFunctionName, ret);
			++trytimes;
			m_listen_sock.close();
			m_idle = 3;
			break;
		}

		trytimes = 0;

		if (!m_isrun)
			break;

		m_heartbeat = long(time(0));

		if (ret == 0) {
			++idle_times;

			/*
			if (idle_times >= 0){
				// timeout_check();	// ֻ�Ǽ��timeout���رչ�������
				idle_times = 0;
			}
			*/
			continue;
		}

		// ��session�������timeout�ĻỰ
		int j;
		for (j = s_used_count-1; j >= 0; j--){
			i = m_pused_index[j];
			if (!PSESSION_ISOPEN(m_psessions+i)){
				WriteToEventLog("%s: ERROR used[%d] session(%d) is not open!", szFunctionName, j, i);
				session_close_used(j);
				s_used_count--;
				continue;
			}

			if (!FD_ISSET(m_psessions[i].sock, &fdsr)){
				/*
				if (XSYS_TCP_SESSION_TIMEOUT(i)){
					WriteToEventLog("%s: <%d> timeout[%d](%d - %d)", szFunctionName, i, m_session_ttl, int(m_psessions[i].createTime), m_heartbeat);

					session_close_used(j);
					s_used_count--;
				}else if (m_psessions[i].idle_secs > 0 &&
					m_heartbeat >= m_psessions[i].last_trans_time + m_psessions[i].idle_secs)
				{
					char parms[32];
					sprintf(parms, "idle:%d", i);
					notify_do_cmd(parms);
				}
				//*/

				continue;
			}

			SYS_SOCKET sock = m_psessions[i].sock;
			int len = // ::recv(sock, m_precv_buf, m_recv_len - 1, 0);
				SysRecvData(sock, m_precv_buf, m_recv_len - 1, 300);
			FD_CLR(sock, &fdsr);

			if (len < 0) {        // client close
				WriteToEventLog("%s: client<%d> close, recv return [%d]", szFunctionName, i, len);
				session_close_used(j);
				s_used_count--;
				++error_recv;
				continue;
			}

			if (len == 0){
				WriteToEventLog("%s: client<%d> recved 0 bytes, will be closed", szFunctionName, i);
				session_close_used(j);
				s_used_count--;
				++error_recv;
				continue;
			}

			write_buf_log(szFunctionName, (unsigned char *)m_precv_buf, len);

			// receive data
			m_psessions[i].recv_sum += len;	// increase the sum
			m_precv_buf[len] = '\0';
			WriteToEventLog("%s: client[%d/sock=%d, len=%d]", szFunctionName, i, sock, len);
			int r;
			m_psessions[i].recv_state = XTS_RECVING;
			m_psessions[i].last_trans_time = m_psessions[i].last_recv_time = m_heartbeat;

			if (m_psessions[i].recv_len == 0 && calc_msg_len(m_precv_buf, len) == len){
				// ��������������ֱ�����봦�����
				r = m_recv_queue.put(i, m_precv_buf, len);
				m_psessions[i].recv_state = XTS_RECV_READY;
				WriteToEventLog("%s: <%d/%d> - send to recv_queue", szFunctionName, i, m_psessions[i].sock);
				xsys_sleep_ms(5);
				continue;
			}

			// ��Ҫ���ϴν��պϲ���������
			session_recv(i, len);	// �����ݴ���session����ʱ����

			// ������״̬
			// �����׸���������Ӧ�г��ȣ�0 < Ϊ��Э���
			r = calc_msg_len(i);
			WriteToEventLog("%s: %d <- %d/%d bytes recved", szFunctionName,
				m_psessions[i].sock,
				m_psessions[i].recv_len, r
			);
			if (r <= 0){
				// ��Ч���������ݣ����ͷ�
				m_psessions[i].recv_state = XTS_RECV_ERROR;
			}else{
				if (r <= m_psessions[i].recv_len)
					m_psessions[i].recv_state = XTS_RECVED;
				else{
					if ((not_full_packet_count & 0x0F) == 0){
						write_buf_log("��������", (unsigned char *)m_precv_buf, len);
						++not_full_packet_count;
					}
				}
			}

			switch (m_psessions[i].recv_state){
			case XTS_RECVED:
				WriteToEventLog("%s: <%d/%d> - recved a packet data(len = %d)", szFunctionName,
					i, m_psessions[i].sock, r
				);
				// ���յ�����������������request��Ϣ������
				while (r <= m_psessions[i].recv_len){
					// ����
					if ((n = session_recv_to_queue(i, r)) <= 0)
						break;
					r = calc_msg_len(i);
				}
				WriteToEventLog("%s: <%d/%d> - send to recv_queue", szFunctionName, i, m_psessions[i].sock);

				break;
			case XTS_RECV_ERROR:
				WriteToEventLog("%s: %d/%d - recv error(%d)", szFunctionName, i, m_psessions[i].sock, r);
				write_buf_log(szFunctionName, (unsigned char *)m_psessions[i].precv_buf, m_psessions[i].recv_len);
				i = session_close_used(j);
				s_used_count--;
				break;

			case XTS_RECV_EXCEPT:
			case XTS_SESSION_END:
				i = session_close_used(j);
				s_used_count--;
				break;
			}
		}

		if (FD_ISSET(m_listen_sock.m_sock, &fdsr)) {
			// find a free session
			int k = session_i;
			for (i = 0; i < m_session_count; i++){
				if (!(PSESSION_ISOPEN(m_psessions+k)))
					break;
				++k;
				if (k == m_session_count)
					k = 0;
			}
			SYS_SOCKET sock = SYS_INVALID_SOCKET;
			n = m_listen_sock.accept(sock);
			if (n != 0) {
				break;
			}

			if (i == m_session_count){
				WriteToEventLog("%s: max connections(%d) arrive.", szFunctionName, i);
				::close(sock);
				continue;
			}

			m_heartbeat = long(time(0));
			/*
			int active = 1;
			n = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char const*)&active, sizeof(active));
			//*/
			m_psessions[k].sock = sock;
			session_open(k);
			m_pused_index[s_used_count++] = k;
			m_used_count++;
			char ip[32];
			ip[0] = 0;
			xsys_socket s;
			s.m_sock = sock;  s.get_peer_ip(ip);  s.m_sock = SYS_INVALID_SOCKET;
			WriteToEventLog("%s: new client(%d/%d) %d %s", szFunctionName, k, s_used_count, m_heartbeat, ip);

			session_i = k+1;
			if (session_i == m_session_count)
				session_i = 0;
		}
	}

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_tcp_server2::msg_server(void)
{
	static const char szFunctionName[] = "XTCP2::msg_server";

	WriteToEventLog("%s : in/%d", szFunctionName, m_recv_len);

	char * ptemp_buf = (char *)calloc(1, m_recv_len+1);
	if (ptemp_buf == nullptr)
		return;

	m_recv_queue.set_timeout_ms(120*1000);
	while (m_isrun){
		int isession = -1;

		/// ȡ������
		int len = m_recv_queue.get_free((long *)(&isession), ptemp_buf);
		if (!m_isrun || (isession == -1 && len == 4 && memcmp(ptemp_buf, "stop", 4) == 0))
			break;

		if (len == ERR_TIMEOUT)
			continue;

		if (len <= 0){
			WriteToEventLog("%s : len = %d, no data", szFunctionName, len);
			continue;
		}

		int r;
		if (!XTCP2SESSION_INRANGE(isession)){
			if (isession == -1){
				if (strcmp(ptemp_buf, "stop") == 0)
					break;

				if (strncmp(ptemp_buf, "idle:", 5) == 0){
					// set session
					isession = atoi(ptemp_buf+5);
					r = do_idle(isession);

					if (r == XSYS_IP_FATAL)
						notify_close_session(isession);
				}
			}else{
				WriteToEventLog("%s: <%d>, len = %d, �����Ự����,����", szFunctionName, isession, len);
			}

			continue;
		}

		/// ���ô�����������Ϣ����
		r = do_msg(isession, ptemp_buf, len);
		WriteToEventLog("%s : <%d/%d>����%d�������ݣ�����%d", szFunctionName, isession, m_psessions[isession].sock, len, r);

		if (r == XSYS_IP_FATAL)
			notify_close_session(isession);
	}

	::free(ptemp_buf);

	WriteToEventLog("%s : out", szFunctionName);
}

void xsys_tcp_server2::send_server(void)
{
	static const char szFunctionName[] = "XTCP2::send_server";

	WriteToEventLog("%s : in.\n", szFunctionName);

	char * ptemp_buf = (char *)calloc(1, m_send_len+1);

	m_send_queue.set_timeout_ms(120*1000);

	xsys_sleep(1);

	while (m_isrun){
		int i_session = -1;

		int len = m_send_queue.get_free((long *)&i_session, ptemp_buf);

		if (!m_isrun || (i_session == -1 && len == 4))
			break;

		if (len <= 0){
			continue;
		}

		if (!XTCP2SESSION_INRANGE(i_session)){
			WriteToEventLog("%s: <%d>, len = %d, �����Ự����,����", szFunctionName, i_session, len);
			continue;
		}

		// find session
		int i = i_session;
		long t = long(time(0));
		if (i < 0 || i > m_session_count || !PSESSION_ISOPEN(m_psessions+i)){
			WriteToEventLog("%s: ����,��ͼ��δ�򿪵ĻỰ�Ϸ���(%d)", szFunctionName, i);
			write_buf_log(szFunctionName, (unsigned char *)ptemp_buf, len);
		}else{
			SYS_SOCKET sock = m_psessions[i].sock;
			int r = SysSendData(sock, ptemp_buf, len, 300);

			if (r <= 0){
				WriteToEventLog("%s: ���ʹ���(%d)(return = %d)", szFunctionName, i, r);
				notify_close_session(i);
			}else{
				WriteToEventLog("%s: the %dth session sent (ret = %d, len=%d)", szFunctionName, i, r, len);

				lock_prop(1);
				m_psessions[i].last_trans_time = t;
				m_psessions[i].sent_len = r;
				unlock_prop();

				on_sent(i, r);
			}
		}
	}

	::free(ptemp_buf);

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_tcp_server2::close_all_session(void)
{
	if (m_psessions){
		for (int i = m_session_count-1; i > 0 ; i--) {
			if (PSESSION_ISOPEN(m_psessions+i)) {
				session_close(i);
			}
		}
	}
}

bool xsys_tcp_server2::stop(int secs)
{
	static const char szFunctionName[] = "XTCP2::stop";

	WriteToEventLog("%s: in", szFunctionName);
	
	if (!m_isrun)
		return true;

	m_isrun = false;

	m_listen_sock.close();
	xsys_sleep_ms(10);

	// notify recver stop
	notify_do_cmd("stop");

	// notify sender thread stop
	this->send(-1,"stop",4);

	xsys_sleep_ms(100);

	m_send_thread.down();
	m_msg_thread.down();

	bool isok = xwork_server::stop(secs);

	WriteToEventLog("%s: out(%d)", szFunctionName, isok);

	return isok;
}

bool xsys_tcp_server2::close(int secs)
{
	static const char szFunctionName[] = "XTCP2::close";

	WriteToEventLog("%s[%d] : in", szFunctionName, secs);

	bool isok = stop(secs);

	isok = xwork_server::close(secs);	// ���� stop

	if (m_psessions){
		close_all_session();
		::free(m_psessions);  m_psessions = 0;
	}

	m_recv_queue.down();
	m_send_queue.down();
	m_close_requests.down();

	if (m_pused_index){
		::free(m_pused_index);  m_pused_index = 0;
	}

	if (m_precv_buf){
		free(m_precv_buf);  m_precv_buf = 0;
	}

	WriteToEventLog("%s[%d] : out(%d)", szFunctionName, secs, isok);

	return isok;
}

void xsys_tcp_server2::session_open(int i)
{
	xtcp2_session * psession = m_psessions + i;

	SYS_SOCKET sock = psession->sock;
	memset(psession, 0, sizeof(xtcp2_session));
	psession->sock = sock;

	psession->idle_secs = m_session_idle;

	psession->createTime =
		psession->last_recv_time  =
		psession->last_trans_time = m_heartbeat-1;

	psession->peerid = -1;
	psession->running_cmdid = -1;
}

int xsys_tcp_server2::notify_close_session(int i)
{
	int r = m_close_requests.put(i, (char *)&i, 0);

	WriteToEventLog("notify_close_session: %d(r = %d)", i, r);

	if ((i % 4) == 0)
		xsys_sleep_ms(50);

	return r;
}

void xsys_tcp_server2::session_close(xtcp2_session * psession)
{
	static const char szFunctionName[] = "XTCP2::session_close";

	SYS_SOCKET sock = psession->sock;

	if (XSYS_VALID_SOCK(sock)){
		SysCloseSocket(sock);
	}

	if (psession->precv_buf){
		::free(psession->precv_buf);
	}

	memset(psession, 0, sizeof(xtcp2_session));
	psession->sock = SYS_INVALID_SOCKET;

	psession->recv_state = XTS_SESSION_END;
	psession->peerid = -1;
	psession->running_cmdid = -1;
}

bool xsys_tcp_server2::session_isopen(int i)
{
	if (!XTCP2SESSION_INRANGE(i))
		return false;

	xtcp2_session * psession = m_psessions + i;
	return PSESSION_ISOPEN(psession);
}

void xsys_tcp_server2::session_close(int i)
{
	static const char szFunctionName[] = "XTCP2::session_close";

	if (!XTCP2SESSION_INRANGE(i)){
		WriteToEventLog("%s: i = %d is out range[0~%d]", szFunctionName, i, m_session_count);
		return;
	}

	xtcp2_session * psession = m_psessions + i;
	WriteToEventLog("%s: i = %d(%d, %d) will be closed", szFunctionName, i, psession->recv_sum, m_heartbeat-psession->createTime);
	if (!PSESSION_ISOPEN(psession)){
		WriteToEventLog("%s: i = %d is not opened", szFunctionName, i);
	}else{
		psession->recv_state = XTS_SESSION_END;
		on_closed(i);
	}

	session_close(psession);

	WriteToEventLog("%s: i = %d is closed", szFunctionName, i);
}

void xsys_tcp_server2::session_recv_reset(int i)
{
	xtcp2_session * psession = m_psessions + i;
	if (psession->precv_buf){
		::free(psession->precv_buf);  psession->precv_buf = 0;
	}
	psession->recv_buflen = psession->recv_len = 0;
	psession->recv_state = XTS_RECV_READY;
}

// �����ݷ�����request����
int xsys_tcp_server2::session_recv_to_queue(int i, int len)
{
	if (len < 1 || i < 0 || i > m_session_count-1){
		WriteToEventLog("session_recv_to_request(%d, %d): ��������", i, len);
		return 0;
	}

	xtcp2_session * psession = m_psessions + i;

	if (!PSESSION_ISOPEN(psession)){
		WriteToEventLog("�Ự%d�ѹرգ�������", i);
		return 0;
	}

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

	WriteToEventLog("session_recv_to_request: %d - send %d -> do_msg", i, r);

	return r;
}

int xsys_tcp_server2::session_recv(int i, int len)
{
	xtcp2_session * psession = m_psessions + i;

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

int xsys_tcp_server2::send(int isession, const char * s, int len)
{
	static const char szFunctionName[] = "XTCP2::send";

	if ((isession != -1 || len != 4) &&
		(!XTCP2SESSION_INRANGE(isession) || !PSESSION_ISOPEN(m_psessions+isession))
	)
	{
		WriteToEventLog("%s: ����,��ͼ��δ�򿪵ĻỰ(%d)�Ϸ���(len=%d)\n", szFunctionName, isession, len);
		write_buf_log(szFunctionName, (unsigned char *)s, len);
		return -1;
	}
	return m_send_queue.put(isession, s, len);
}

int xsys_tcp_server2::send(int isession, const char * s)
{
	return send(isession, s, strlen(s));
}

bool xsys_tcp_server2::on_opened(int i)
{
	return true;
}

bool xsys_tcp_server2::on_closed(int i)
{
	return true;
}

void xsys_tcp_server2::notify_do_cmd(const char* cmd)
{
	if (!m_msg_thread.isopen())
		return;

	if (cmd == 0) {
		m_recv_queue.put(-1, "cmds", 4);
	}
	else
		m_recv_queue.put(-1, cmd, strlen(cmd));
}

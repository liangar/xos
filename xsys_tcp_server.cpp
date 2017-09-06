#include <xsys_tcp_server.h>

#define PSESSION_ISOPEN(p)	((p) != 0 && XSYS_VALID_SOCK((p)->sock.m_sock))

xsys_tcp_server::xsys_tcp_server()
	: xwork_server("XTCP Server", 5, 2)
	, m_listen_port(0)
	, m_session_ttl(300)
	, m_psessions(0)
	, m_session_count(0)
	, m_precv_buf(0)
	, m_recv_len(0)
	, m_session_init_len(0)
{
	m_serverURL[0] = 0;
}

xsys_tcp_server::xsys_tcp_server(const char * name, int nmaxexception)
	: xwork_server(name, nmaxexception, 2)	// 启动两个服务 0: accept/recv, 1: send
	, m_listen_port(0)
	, m_session_ttl(300)
	, m_psessions(0)
	, m_session_count(0)
	, m_precv_buf(0)
	, m_recv_len(0)
	, m_session_init_len(0)
{
	m_serverURL[0] = 0;
}

bool xsys_tcp_server::open(int listen_port, int ttl, int max_sessions, int recv_len)
{
	if (max_sessions > 1024)
		max_sessions = 1024;

	m_requests.init((recv_len / 1024 + 1) * max_sessions, max_sessions);
	m_listen_port = listen_port;
	m_session_ttl = ttl;

	m_recv_len = recv_len;
	if (m_recv_len < 1024)
		m_recv_len = 1024;

	m_precv_buf = (char *)calloc(m_recv_len+1, 1);

	m_psessions = new xtcp_session[max_sessions];
	m_session_count = max_sessions;

	m_sends.init(4, max_sessions);
	m_close_requests.init(4, max_sessions);

	xtcp_session * p = m_psessions;
	m_session_init_len = sizeof(xtcp_session) - ((char *)p->peerip - (char *)p);

	m_lock.init();

	return xwork_server::open();
}

bool xsys_tcp_server::open(const char * url,int ttl, int recv_len)
{
	strncpy(m_serverURL, url, sizeof(m_serverURL)-1);  m_serverURL[sizeof(m_serverURL)-1] = 0;
	return open(0, ttl, 32, recv_len);
}


void xsys_tcp_server::run(void)
{
	static const char szFunctionName[] = "xsys_tcp_server::run";

	// 如果是启动命令是 1, 则启动 send
	if (m_run_times % 2 == 1){
		send_server();
		return;
	}

	if (m_listen_port == 0){
		recv_server();  return;
	}

	WriteToEventLog("%s : in(sessions = %d).\n", szFunctionName, m_session_count);

	int trytimes = 0;
	int	session_i = 0;		/// 循环查找起始位置
	while (m_isrun && trytimes < 3)
	{
		if (!m_listen_sock.isopen() && m_listen_sock.listen(m_listen_port) != 0){
			WriteToEventLog("%s : 创建消息监听(%d)失败.", szFunctionName, m_listen_port);
			break;
		}

		fd_set fdsr;
		struct timeval tv;

		int i, conn_amount = 0, n;
		while (m_isrun && m_listen_sock.isopen() && trytimes < 3) {
			// initialize file descriptor set
	//		WriteToEventLog("%s: 1", szFunctionName);
			// 检查关闭连接请求
			while (!m_close_requests.isempty()){
				i = -1;
				n = m_close_requests.get_free((long *)&i, m_precv_buf);
				if (n == 0){
					WriteToEventLog("%s: get close %d request", szFunctionName, i);
					session_close(i);
				}
				xsys_sleep_ms(10);
			}

			n = m_listen_sock.m_sock;
			FD_ZERO(&fdsr);
			FD_SET(m_listen_sock.m_sock, &fdsr);
	//		WriteToEventLog("%s: 2", szFunctionName);

			m_heartbeat = long(time(0));

			// timeout setting
			tv.tv_sec =  30;
			tv.tv_usec = 0;

			// add active connection to fd set
			for (i = 0; i < m_session_count; i++) {
				if (m_psessions[i].sock.isopen()) {
					FD_SET(m_psessions[i].sock.m_sock, &fdsr);
					if (n < int(m_psessions[i].sock.m_sock))
						n = int(m_psessions[i].sock.m_sock);
				}
			}
			WriteToEventLog("%s: 3 - %d", szFunctionName, ++n);

			int ret = ::select(n+1, &fdsr, NULL, NULL, &tv);

			m_heartbeat = long(time(0));

	//		WriteToEventLog("%s: 4", szFunctionName);
			if (ret < 0) {
				WriteToEventLog("%s: select error(%d), try times=%d", szFunctionName, ret, ++trytimes);
				xsys_sleep_ms(100);
				continue;
			}

			trytimes = 0;

			if (!m_isrun)
				break;

			if (ret == 0) {
				xsys_sleep_ms(100);
				continue;
			}

			// check every fd in the set
	//		WriteToEventLog("%s: 5", szFunctionName);
			for (i = 0; i < m_session_count; i++) {
				if (!m_psessions[i].sock.isopen())
					continue;

				if (!FD_ISSET(m_psessions[i].sock.m_sock, &fdsr)){
					if ((m_session_ttl < m_heartbeat - m_psessions[i].last_trans_time && m_psessions[i].last_trans_time > 0) ||
						(20 < m_heartbeat - m_psessions[i].createTime && m_psessions[i].last_trans_time == 0))
					{
						WriteToEventLog("%s: client[%d] timeout[%d/20](%d/%d - %d)", szFunctionName, i, m_session_ttl, int(m_psessions[i].createTime), int(m_psessions[i].last_trans_time), m_heartbeat);

						session_close(i);
					}
					continue;
				}

				ret = m_psessions[i].sock.recv(m_precv_buf, m_recv_len-1, 20);

				FD_CLR(m_psessions[i].sock.m_sock, &fdsr);
				if (ret < 0) {        // client close
					WriteToEventLog("%s: client[%d] close, recv return [%d]", szFunctionName, i, ret);
					session_close(i);
					continue;
				}

				if (ret == 0){
					WriteToEventLog("%s: warning, client[%d] recv return [%d]", szFunctionName, i, ret);
					session_close(i);
					continue;
				}

				// receive data
				m_precv_buf[ret] = '\0';
				WriteToEventLog("%s: client[%d, len=%d]", szFunctionName, i, ret);
				session_recv(i, ret);
				m_psessions[i].recv_state = XTS_RECVING;
				check_recv_state(i);

				switch (m_psessions[i].recv_state){
				case XTS_RECVED:
					WriteToEventLog("%s: %d - recved a packet data", szFunctionName, i);
					if (!on_recved(i) || (n = session_recv_to_request(i)) >= 0)
						session_recv_reset(i);
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

			// check whether a new connection comes
			if (FD_ISSET(m_listen_sock.m_sock, &fdsr)) {
				// find a free session
				int j = session_i;
				for (i = 0; i < m_session_count; i++){
					if (!(m_psessions[j].sock.isopen()))
						break;
					++j;
					if (j == m_session_count)
						j = 0;
				}
				if (i == m_session_count){
					WriteToEventLog("%s: max connections(%d) arrive.", szFunctionName, i);
					continue;
				}

				n = m_listen_sock.accept(m_psessions[j].sock);
				if (n != 0) {
					break;
				}

				session_open(j);
				if (!on_connected(j)){
					WriteToEventLog("%s: on_connected(%d) failed.", szFunctionName, j);
					session_close(j);
				}else{
					session_i = j+1;
					if (session_i == m_session_count)
						session_i = 0;
				}
			}
		}
		// close_all_session();
		m_listen_sock.close();
	}

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_tcp_server::send_server(void)
{
	static const char szFunctionName[] = "xsys_tcp_server::send_server";

	WriteToEventLog("%s : in.\n", szFunctionName);
	xseq_buf_use use;
	while (m_isrun){
		int r = m_sends.get(&use);
		m_heartbeat = long(time(0));

		if (r < 0){
			if (strcmp(m_cmd, "stop") == 0)
				break;
			continue;
		}

		// find session
		int i = use.id;
		if (i < 0 || i > m_session_count || !m_psessions[i].sock.isopen()){
			WriteToEventLog("%s: 出错,试图在未打开的会话上发送(%d len=%d)\n%s", szFunctionName, i, use.len, use.p);
		}else{
			r = m_psessions[i].sock.send_all(use.p, use.len);
			if (r <= 0){
				WriteToEventLog("%s: 发送错误(%d sock=%d)(return = %d)", szFunctionName, i, m_psessions[i].sock.m_sock, r);
				notify_close_session(i);
			}else{
				m_psessions[i].send_len = use.len;
				m_psessions[i].sent_len = r;
				check_send_state(i);
			}
		}
		m_sends.free_use(use);
		// send packet
	}
	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_tcp_server::recv_server(void)
{
	static const char szFunctionName[] = "xsys_tcp_server::recv_server";

	WriteToEventLog("%s : in.\n", szFunctionName);

	int ret, i;

	for (i = 0; m_isrun;){
		if (!m_psessions[i].sock.isopen()){
			if (strcmp(m_parms, "stop") == 0)
				break;
			ret = m_psessions[i].sock.connect(m_serverURL, 0, m_session_ttl);
			if (ret)
				break;

			session_open(i);
			if (!on_connected(i)){
				WriteToEventLog("%s: on_connected(i = %d) failed.", szFunctionName, i);
				session_close(i);
			}
		}

		m_heartbeat = long(time(0));
		ret = m_psessions[i].sock.recv(m_precv_buf, m_recv_len-1, m_idle);
		if (ret == 0 || ret == ERR_TIMEOUT)
			continue;

		if (ret < 0) {        // client close
			WriteToEventLog("%s: client[%d] will be closed, for recv return [%d]\n", szFunctionName, i, ret);
			session_close(i);
			continue;
		}

		// receive data
		WriteToEventLog("%s: client[%d] send:%s\n", szFunctionName, i, m_precv_buf);
		session_recv(i, ret);
		check_recv_state(i);

		switch (m_psessions[i].recv_state){
		case XTS_RECVED:
			if (!on_recved(i) || (ret = session_recv_to_request(i)) >= 0)
				session_recv_reset(i);
			break;

		case XTS_RECV_ERROR:
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

void xsys_tcp_server::close_all_session(void)
{
	if (m_psessions){
		for (int i = m_session_count-1; i > 0 ; i--) {
			if (m_psessions[i].sock.m_sock != SYS_INVALID_SOCKET) {
				session_close(i);
			}
		}
	}
}

bool xsys_tcp_server::stop(int secs)
{
	static const char szFunctionName[] = "xsys_tcp_server::stop";

	WriteToEventLog("%s: in", szFunctionName);
	m_isrun = false;

	m_listen_sock.close();
	bool isok = xwork_server::stop(secs);

	if (m_psessions){
		close_all_session();
		delete[] m_psessions;  m_psessions = 0;
	}

	m_requests.down();
	m_sends.down();
	m_close_requests.down();

	WriteToEventLog("%s: out(%d)", szFunctionName, isok);

	return isok;
}

bool xsys_tcp_server::close(int secs)
{
	static const char szFunctionName[] = "xsys_tcp_server::close";

	WriteToEventLog("%s[%d] : in", szFunctionName, secs);

	bool isok = xwork_server::close(secs);

	if (m_precv_buf){
		free(m_precv_buf);  m_precv_buf = 0;
	}

	m_lock.down();

	WriteToEventLog("%s[%d] : out(%d)", szFunctionName, secs, isok);

	return isok;
}

void xsys_tcp_server::session_open(int i)
{
	xtcp_session * psession = m_psessions + i;

	memset(psession->peerip, 0, m_session_init_len);

	psession->createTime = time(0);
	psession->last_trans_time = 0;
	psession->sock.get_peer_ip(psession->peerip);
	psession->sock.get_self_ip(psession->servip);
	psession->peerid = -1;
}

int xsys_tcp_server::notify_close_session(int i)
{
	int r = m_close_requests.put(i, (char *)&i, 0);

	WriteToEventLog("notify_close_session: %d(r = %d)", i, r);

	if ((i % 4) == 0)
		xsys_sleep_ms(50);

	return r;
}

void xsys_tcp_server::session_close(xtcp_session * psession)
{
	static const char szFunctionName[] = "xsys_tcp_server::session_close";

	m_lock.lock(3);
	if (psession->sock.isopen())
		psession->sock.close();

	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = 0;
	}
	if (psession->psend_buf){
		free(psession->psend_buf);  psession->psend_buf = 0;
	}

	memset(psession->peerip, 0, m_session_init_len);
	psession->peerid = -1;

	psession->recv_state = XTS_SESSION_END;
	psession->send_state = XTS_SESSION_END;

	m_lock.unlock();
}

bool xsys_tcp_server::session_isopen(int i)
{
	if (i < 0 || i >= m_session_count)
		return false;

	xtcp_session * psession = m_psessions + i;
	return psession->sock.isopen();
}

void xsys_tcp_server::session_close(int i)
{
	static const char szFunctionName[] = "xsys_tcp_server::session_close";

	if (i < 0 || i >= m_session_count){
		WriteToEventLog("%s: i = %d is out range[0~%d]", szFunctionName, i, m_session_count);
		return;
	}

	WriteToEventLog("%s: i = %d will be closed", szFunctionName, i);

	xtcp_session * psession = m_psessions + i;
	if (!psession->sock.isopen()){
		WriteToEventLog("%s: i = %d is not opened", szFunctionName, i);
		return;
	}

	m_lock.lock(3);
	psession->recv_state = XTS_SESSION_END;
	psession->send_state = XTS_SESSION_END;
	m_lock.unlock();

	on_closed(i);

	session_close(psession);

	WriteToEventLog("%s: i = %d is closed", szFunctionName, i);
}

void xsys_tcp_server::session_close_by_sock(int sock)
{
	if (sock == SYS_INVALID_SOCKET)
		return;

	for (int i = 0; i < m_session_count; i++){
		if (m_psessions[i].sock.m_sock == (unsigned int)sock){
			session_close(i);
			return;
		}
	}
}

void xsys_tcp_server::session_recv_reset(int i)
{
	xtcp_session * psession = m_psessions + i;
	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = 0;
	}
	psession->recv_buflen = psession->recv_len = psession->recv_next_len = 0;
	psession->recv_state = XTS_RECV_READY;
}

int xsys_tcp_server::session_recv_to_request(int i)
{
	xtcp_session * psession = m_psessions + i;

	int r = m_requests.put(i, psession->precv_buf, psession->recv_len);
	xsys_sleep_ms(100);
	WriteToEventLog("session_recv_to_request: %d - send packet to recver", i);
	return r;
}

int xsys_tcp_server::session_recv(int i, int len)
{
	xtcp_session * psession = m_psessions + i;

	psession->last_trans_time = m_heartbeat;

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

int xsys_tcp_server::send(int isession, const char * s, int len)
{
	static const char szFunctionName[] = "xsys_tcp_server::send";

	if (!m_psessions[isession].sock.isopen()){
		WriteToEventLog("%s: 出错,试图在未打开的会话(%d)上发送(len=%d)\n%s", szFunctionName, isession, len, s);
		return -1;
	}
	m_psessions[isession].last_trans_time = long(time(0));
	return m_sends.put(isession, s, len);
}

int xsys_tcp_server::send(int isession, const char * s)
{
	return m_sends.put(isession, s);
}

int xsys_tcp_server::send_byid(int peerid, const char * s, int len)
{
	int i = find_session_byid(peerid);
	if (i < 0)
		return i;
	return send(i, s, len);
}

int xsys_tcp_server::send_byid(int peerid, const char * s)
{
	return send_byid(peerid, s, strlen(s));
}

int xsys_tcp_server::find_session_byid(int peerid)
{
	for (int i = 0; i < m_session_count; i++){
		if (m_psessions[i].peerid == peerid && m_psessions[i].sock.isopen())
			return i;
	}
	return -1;
}

#include <xsys_tcp_server2.h>

xsys_tcp_server2::xsys_tcp_server2()
	: xwork_server("XTCP2 Server", 5, 2)
	, m_listen_port(0)
	, m_session_ttl(300)
	, m_psessions(0)
	, m_session_count(0)
	, m_precv_buf(0)
	, m_recv_len(0)
{
	m_serverURL[0] = 0;
}

xsys_tcp_server2::xsys_tcp_server2(const char * name, int nmaxexception)
	: xwork_server(name, nmaxexception, 2)	// 启动两个服务 0: accept/recv, 1: send
	, m_listen_port(0)
	, m_session_ttl(300)
	, m_psessions(0)
	, m_session_count(0)
	, m_precv_buf(0)
	, m_recv_len(0)
{
	m_serverURL[0] = 0;
}

// 关闭已用标记的第i_used个会话，返回实际的序号
int xsys_tcp_server2::session_close_used(int i_used)
{
	if (i_used < 0 || i_used >= m_used_count)
		i_used = 0;

	int i = m_pused_index[i_used];

	WriteToEventLog("Close i_used=%d, i = %d", i_used, i);
	session_close(i);

	memmove(m_pused_index + i_used, m_pused_index+i_used+1, m_used_count-i_used-1);
	--m_used_count;
	
	return i;
}

void xsys_tcp_server2::timeout_check(void)
{
	static const char szFunctionName[] = "xsys_tcp_server2::timeout_check";

	int i;

	for (i = m_used_count-1; i >= 0; i--){
		int j = m_pused_index[i];

		if ((m_session_ttl < m_heartbeat - m_psessions[j].last_trans_time) ||
			(20 < m_heartbeat - m_psessions[j].last_trans_time && m_psessions[j].last_trans_time == m_psessions[j].createTime))
		{
			WriteToEventLog("%s: client[%d] timeout[%d/20](%d - %d)", szFunctionName, j, m_session_ttl, int(m_psessions[j].createTime), m_heartbeat);
			session_close_used(i);
			++i;
		}
	}
}

bool xsys_tcp_server2::open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len)
{
	// 最多支持1K会话
	if (max_sessions > 1024)
		max_sessions = 1024;

	m_listen_port = listen_port;
	m_session_ttl = ttl;
	m_recv_len = recv_len;	// 最大数据包的长度，由经验值设定
	if (m_recv_len < 1024)
		m_recv_len = 1024;

	// 打开最多会话数的缓冲区长度
	m_recv_queue.init((m_recv_len / 1024) * max_sessions + 2, max_sessions);

	m_precv_buf = (char *)calloc(m_recv_len+1, 1);

	m_psessions = new xtcp2_session[max_sessions];
	m_session_count = max_sessions;

	m_send_queue.init((send_len+1023)/1024*(max_sessions/5+2), max_sessions/2+1);
	m_close_requests.init(4, max_sessions/2+1);

	m_lock.init();

	return xwork_server::open();
}

bool xsys_tcp_server2::open(const char * url,int ttl, int recv_len)
{
	strncpy(m_serverURL, url, sizeof(m_serverURL)-1);  m_serverURL[sizeof(m_serverURL)-1] = 0;
	return open(0, ttl, 32, recv_len, 2);
}

static unsigned int run_send_thread(void * ptcp_server)
{
	try{
		xsys_tcp_server2 * pserver = (xsys_tcp_server2 *)ptcp_server;
		pserver->send_server();
	}catch(...){
		WriteToEventLog("xsys_tcp_server2::send_server: an exception occured.");
	}
	return 0;
}

static unsigned int run_msg_thread(void * ptcp_server)
{
	try{
		xsys_tcp_server2 * pserver = (xsys_tcp_server2 *)ptcp_server;
		pserver->msg_server();
	}catch(...){
		WriteToEventLog("xsys_tcp_server2::msg_server: an exception occured.");
	}
	return 0;
}

void xsys_tcp_server2::run(void)
{
	static const char szFunctionName[] = "xsys_tcp_server2::run";

	WriteToEventLog("%s : in.\n", szFunctionName);

	if (!m_listen_sock.isopen() && m_listen_sock.listen(m_listen_port) != 0){
		WriteToEventLog("%s : 创建消息监听(%d)失败，退出运行", szFunctionName, m_listen_port);
		return;
	}

	// 启动发送线程
	m_send_thread.init(run_send_thread, this);
	m_msg_thread.init(run_msg_thread, this);

	int	session_i = 0;		/// 循环查找起始位置

	fd_set fdsr;
	struct timeval tv;

	int i, conn_amount = 0, n, idle_times = 0, trytimes = 0;

	xsys_socket xsock;
	while (m_isrun && trytimes < 3 && m_listen_sock.isopen()) {
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

		// get network event
		FD_ZERO(&fdsr);
		FD_SET(m_listen_sock.m_sock, &fdsr);
		tv.tv_sec  = 30;
		tv.tv_usec = 0;

		// add active connection to fd set
		n = m_listen_sock.m_sock;
		for (i = 0; i < m_session_count; i++) {
			if (XSYS_VALID_SOCK(m_psessions[i].sock)) {
				FD_SET(m_psessions[i].sock, &fdsr);
				if (n < int(m_psessions[i].sock))
					n = int(m_psessions[i].sock);
			}
		}
		WriteToEventLog("%s: 1 - %d", szFunctionName, ++n);

		m_heartbeat = long(time(0));

		int ret = ::select(n, &fdsr, NULL, NULL, &tv);

		if (ret < 0) {
			WriteToEventLog("%s: select error(%d), loop end", szFunctionName, ret);
			++trytimes;
			break;
		}
		trytimes = 0;
		
		if (!m_isrun)
			break;

		if (ret == 0) {
			continue;
		}

		if (ret == ERR_TIMEOUT){
			++idle_times;
			
			if (idle_times >= 10){
				timeout_check();	// 只是检查timeout，关闭过期连接
				idle_times = 0;
			}
			continue;
		}

		if (ret < 0)
			break;

		// 找session，并检查timeout的会话
		int j;
		for (j = m_used_count-1; j >= 0; j--){
			i = m_pused_index[j];
			if (!PSESSION_ISOPEN(m_psessions+i))
				continue;

			if (!FD_ISSET(m_psessions[i].sock, &fdsr)){
				if ((m_session_ttl < m_heartbeat - m_psessions[i].last_trans_time) ||
					(20 < m_heartbeat - m_psessions[i].last_trans_time &&
					 m_psessions[i].last_trans_time == m_psessions[i].createTime
					))
				{
					WriteToEventLog("%s: client[%d] timeout[%d/20](%d - %d)", szFunctionName, i, m_session_ttl, int(m_psessions[i].createTime), m_heartbeat);
					
					session_close_used(j);
					++j;
				}
				continue;
			}

			xsock.m_sock = m_psessions[i].sock;
			int len = xsock.recv(m_precv_buf, m_recv_len-1, 10);
			xsock.m_sock = SYS_INVALID_SOCKET;

			// receive data
			m_precv_buf[len] = '\0';
			WriteToEventLog("%s: client[%d, len=%d]", szFunctionName, i, len);
			session_recv(i, len);	// 将数据存入session的临时缓存
			m_psessions[i].recv_state = XTS_RECVING;
			
			// 检查接收状态
			// 返回首个完整包的应有长度，0 < 为非协议包
			int r = calc_msg_len(i);
			if (r <= 0){
				// 无效或无用数据，则释放
				m_psessions[i].recv_state = XTS_RECV_ERROR;
			}else{
				if (r <= m_psessions[i].recv_len)
					m_psessions[i].recv_state = XTS_RECVED;
				else
					m_psessions[i].recv_state = XTS_RECVING;
			}
		
			switch (m_psessions[i].recv_state){
			case XTS_RECVED:
				WriteToEventLog("%s: %d - recved a packet data", szFunctionName, i);
				// 将收到的完整包逐条发到request消息队列中
				while (r <= m_psessions[i].recv_len){
					// 发布
					if ((n = session_recv_to_queue(i)) <= 0)
						break;
					m_psessions[i].recv_len -= n;
					if (m_psessions[i].recv_len <= 0){
						m_psessions[i].recv_len = 0;
						m_psessions[i].recv_state = XTS_RECV_READY;
						break;
					}
					char * p = m_psessions[i].precv_buf;
					memmove(p, p+n, m_psessions[i].recv_len);
					r = calc_msg_len(i);
				}
				WriteToEventLog("%s: %d - send packet to recver", szFunctionName, i);

				break;
			case XTS_RECV_ERROR:
				WriteToEventLog("%s: %d - recv error", szFunctionName, i);
				i = session_close_used(j);
				break;

			case XTS_RECV_EXCEPT:
			case XTS_SESSION_END:
				i = session_close_used(j);
				break;
			}
		}
		
		if (FD_ISSET(m_listen_sock.m_sock, &fdsr)) {
			// find a free session
			int j = session_i;
			for (i = 0; i < m_session_count; i++){
				if (!(PSESSION_ISOPEN(m_psessions+j)))
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
			m_pused_index[m_used_count++] = j;

			session_i = j+1;
			if (session_i == m_session_count)
				session_i = 0;
		}
	}

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_tcp_server2::msg_server(void)
{
	static const char szFunctionName[] = "xsys_tcp_server2::send_server";

	WriteToEventLog("%s : in", szFunctionName);
	
	char * ptemp_buf = (char *)calloc(1, m_recv_len+1);

	while (m_isrun){
		int isession = -1;

		/// 取出数据
		int len = m_recv_queue.get_free((long *)(&isession), ptemp_buf);

		/// 调用处理函数进行消息处理
		int r = do_msg(isession, ptemp_buf, len);

		WriteToEventLog("%s : do_msg 处理%d长的数据，返回%d", szFunctionName, len, r);
	}

	::free(ptemp_buf);
	
	WriteToEventLog("%s : out", szFunctionName);
}

void xsys_tcp_server2::send_server(void)
{
	static const char szFunctionName[] = "xsys_tcp_server2::send_server";

	WriteToEventLog("%s : in.\n", szFunctionName);
	xseq_buf_use use;
	
	xsys_socket xsock;

	while (m_isrun && m_listen_sock.isopen()){
		int r = m_send_queue.get(&use);
		m_heartbeat = long(time(0));

		if (r < 0){
			continue;
		}

		// find session
		int i = use.id;
		if (i < 0 || i > m_session_count || !PSESSION_ISOPEN(m_psessions+i)){
			WriteToEventLog("%s: 出错,试图在未打开的会话上发送(%d)", szFunctionName, i);
			write_buf_log(szFunctionName, (unsigned char *)use.p, use.len);
		}else{
			xsock.m_sock = m_psessions[i].sock;
			r = xsock.send(use.p, use.len, 1);
			xsock.m_sock = SYS_INVALID_SOCKET;
			
			if (r <= 0){
				WriteToEventLog("%s: 发送错误(%d)(return = %d)", szFunctionName, i, r);
				notify_close_session(i);
			}else{
				WriteToEventLog("%s: the %dth session sent (ret = %d, len=%d)", szFunctionName, i, r, use.len);
				on_sent(i, r);
			}
		}
		m_send_queue.free_use(use);
	}
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
	static const char szFunctionName[] = "xsys_tcp_server2::stop";

	WriteToEventLog("%s: in", szFunctionName);
	m_isrun = false;

	m_listen_sock.close();

	m_send_thread.down();
	m_msg_thread.down();

	bool isok = xwork_server::stop(secs);

	if (m_psessions){
		close_all_session();
		delete[] m_psessions;  m_psessions = 0;
	}

	m_recv_queue.down();
	m_send_queue.down();
	m_close_requests.down();

	WriteToEventLog("%s: out(%d)", szFunctionName, isok);

	return isok;
}

bool xsys_tcp_server2::close(int secs)
{
	static const char szFunctionName[] = "xsys_tcp_server2::close";

	WriteToEventLog("%s[%d] : in", szFunctionName, secs);

	bool isok = this->stop(secs);

	isok = xwork_server::close(secs);
	
	if (m_precv_buf){
		free(m_precv_buf);  m_precv_buf = 0;
	}

	m_lock.down();

	WriteToEventLog("%s[%d] : out(%d)", szFunctionName, secs, isok);

	return isok;
}

void xsys_tcp_server2::session_open(int i)
{
	xtcp2_session * psession = m_psessions + i;

	memset(psession, 0, sizeof(xtcp2_session));

	psession->createTime = time(0);
	psession->last_trans_time = long(psession->createTime);
	psession->peerid = -1;
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
	static const char szFunctionName[] = "xsys_tcp_server2::session_close";

	m_lock.lock(3000);

	xsys_socket xsock;
	xsock.m_sock = psession->sock;

	if (xsock.isopen()){
		xsock.close();
		psession->sock = SYS_INVALID_SOCKET;
	}

	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = 0;
	}

	memset(psession, 0, sizeof(xtcp2_session));;

	psession->recv_state = XTS_SESSION_END;

	m_lock.unlock();
}

bool xsys_tcp_server2::session_isopen(int i)
{
	if (i < 0 || i >= m_session_count)
		return false;

	xtcp2_session * psession = m_psessions + i;
	return PSESSION_ISOPEN(psession);
}

void xsys_tcp_server2::session_close(int i)
{
	static const char szFunctionName[] = "xsys_tcp_server2::session_close";

	if (i < 0 || i >= m_session_count){
		WriteToEventLog("%s: i = %d is out range[0~%d]", szFunctionName, i, m_session_count);
		return;
	}

	WriteToEventLog("%s: i = %d will be closed", szFunctionName, i);

	xtcp2_session * psession = m_psessions + i;
	if (!PSESSION_ISOPEN(psession)){
		WriteToEventLog("%s: i = %d is not opened", szFunctionName, i);
		return;
	}

	m_lock.lock(3000);
	psession->recv_state = XTS_SESSION_END;
	m_lock.unlock();

	on_closed(i);

	session_close(psession);

	WriteToEventLog("%s: i = %d is closed", szFunctionName, i);
}

void xsys_tcp_server2::session_recv_reset(int i)
{
	xtcp2_session * psession = m_psessions + i;
	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = 0;
	}
	psession->recv_buflen = psession->recv_len = 0;
	psession->recv_state = XTS_RECV_READY;
}

// 将数据发布到request队列
int xsys_tcp_server2::session_recv_to_queue(int i)
{
	xtcp2_session * psession = m_psessions + i;

	int r = m_recv_queue.put(i, psession->precv_buf, psession->recv_len);
	xsys_sleep_ms(3);
	WriteToEventLog("session_recv_to_queue: %d - send packet to recver", i);
	return r;
}

int xsys_tcp_server2::session_recv(int i, int len)
{
	xtcp2_session * psession = m_psessions + i;

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

int xsys_tcp_server2::send(int isession, const char * s, int len)
{
	static const char szFunctionName[] = "xsys_tcp_server2::send";

	if (!PSESSION_ISOPEN(m_psessions+isession)){
		WriteToEventLog("%s: 出错,试图在未打开的会话(%d)上发送(len=%d)\n%s", szFunctionName, isession, len, s);
		return -1;
	}
	m_psessions[isession].last_trans_time = long(time(0));
	return m_send_queue.put(isession, s, len);
}

int xsys_tcp_server2::send(int isession, const char * s)
{
	return m_send_queue.put(isession, s);
}

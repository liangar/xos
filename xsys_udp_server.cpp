#include <xsys_udp_server.h>

xsys_udp_server::xsys_udp_server()
	: xwork_server("XUDP Server", 5, 2)
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

// 关闭已用标记的第i_used个会话，返回实际的序号
int xsys_udp_server::session_close_used(int i_used)
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

void xsys_udp_server::timeout_check(void)
{
	static const char szFunctionName[] = "xsys_udp_server::timeout_check";

	int i;

	for (i = 0; i < m_used_count && m_used_count > 0; i++){
		int j = m_pused_index[i];

		if ((m_session_ttl < m_heartbeat - m_psessions[j].last_trans_time) ||
			(20 < m_heartbeat - m_psessions[j].last_trans_time && m_psessions[j].last_trans_time == m_psessions[j].createTime))
		{
			WriteToEventLog("%s: client[%d] timeout[%d/20](%d - %d)", szFunctionName, j, m_session_ttl, int(m_psessions[j].createTime), m_heartbeat);
			session_close_used(i);
		}
	}
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
		// 用CRC和addr进行比较查找
		if (addr && m_psessions[j].addr_crc == *crc &&
			memcmp(&m_psessions[j].addr, addr, sizeof(SYS_INET_ADDR)) == 0)
		{
			n = j;
			continue;
		}

		if ((m_session_ttl < m_heartbeat - m_psessions[j].last_trans_time) ||
			(20 < m_heartbeat - m_psessions[j].last_trans_time && m_psessions[j].last_trans_time == m_psessions[j].createTime))
		{
			WriteToEventLog("%s: client[%d] timeout[%d/20](%d - %d)", szFunctionName, j, m_session_ttl, int(m_psessions[j].createTime), m_heartbeat);
			session_close(j);
			memmove(m_psessions+i, m_psessions+i+1, m_used_count-i);
			--m_used_count;
		}
	}

	return n;
}

bool xsys_udp_server::open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len)
{
	if (max_sessions > 1024)
		max_sessions = 1024;
	else{
		if (max_sessions < 2)
			max_sessions = 4;
		else
			max_sessions += 4;
	}

	m_recv_queue.init((recv_len / 1024 + 1) * max_sessions, max_sessions);
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
		WriteToEventLog("%s : 创建消息监听(%d)失败，退出运行", szFunctionName, m_listen_port);
		return;
	}

	WriteToEventLog("%s: %s\n", szFunctionName, m_listen_sock.get_self_ip(m_lastmsg));

	// 启动发送线程
	m_send_thread.init(run_send_thread, this);
	m_msg_thread.init(run_msg_thread, this);

	// wait the other worker in ready
	xsys_sleep_ms(100);

	int	session_i = 0;		/// 循环查找起始位置

	int conn_amount = 0, n, idle_times = 0;
	while (m_isrun && m_listen_sock.isopen()) {

		m_heartbeat = long(time(0));

		int crc;

		SYS_INET_ADDR from_addr;
		ZeroData(from_addr);
		int len = m_listen_sock.recv_from(m_precv_buf, m_recv_len-1, &from_addr, 2000);

		if (len == ERR_TIMEOUT || len == 0){
			++idle_times;
			
			if (idle_times >= 30){
				timeout_check();	// 只是检查timeout，关闭过期连接
			}
			continue;
		}

		if (len < 0)
			break;

		{
			char ip_port[64];
			SysInetRevNToA(from_addr, ip_port, MAX_IP_LEN);
			WriteToEventLog("peer ip: %s\n", ip_port);
		}

		// 找session，并检查timeout的会话
		int i = opened_find(&crc, &from_addr);

		if (i == -1){
			WriteToEventLog("新建会话:%04X", crc);
			if (m_used_count > 0){
				if (m_session_count == m_used_count){
					WriteToEventLog("强制最早建立的会话过期");
					i = session_close_used(0);
				}else{
					// 从最后的开始找
					for (i = m_pused_index[m_used_count-1]+1, n = 0; n < m_session_count; n++){
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
		}

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
			WriteToEventLog("%s: %d - recved a packet data", szFunctionName, i);
			// 将收到的完整包逐条发到request消息队列中
			while (r <= m_psessions[i].recv_len){
				// 发布
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
	static const char szFunctionName[] = "xsys_udp_server::send_server";

	WriteToEventLog("%s : in", szFunctionName);
	
	char * ptemp_buf = (char *)calloc(1, m_recv_len+1);

	int idle = 120 * 1000;

	while (m_isrun){
		int isession = -1;

		m_recv_queue.set_timeout_ms(idle);

		/// 取出数据
		int len = m_recv_queue.get_free((long *)(&isession), ptemp_buf);

		if (len != ERR_TIMEOUT){
			if (len <= 0){
				WriteToEventLog("%s : len = %d, no data", szFunctionName, len);
				continue;
			}

			if (isession < 0 || isession >= m_session_count){
				WriteToEventLog("%s: <%d>, len = %d, 超出会话界限,忽略", szFunctionName, isession, len);
				continue;
			}
		}

		int r;
		
		if (len != ERR_TIMEOUT){
			/// 调用处理函数进行消息处理
			r = do_msg(isession, ptemp_buf, len);

			WriteToEventLog("%s : <%d>处理%d长的数据，返回%d", szFunctionName, isession, len, r);
		}
		
		/// 检查所有超时的session，并调用处理方法
		int i;
		int lnow = time(0);
		for (i = 0; i < m_used_count; i++){
			isession = m_pused_index[i];
			if (m_psessions[isession].idle_secs > 0 &&
				lnow >= m_psessions[isession].last_trans_time + m_psessions[isession].idle_secs)
			{
				do_idle(isession);
				m_psessions[isession].last_trans_time = time(0);
			}
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

	while (m_isrun && m_listen_sock.isopen()){
		int i_session = -1;

		int len = m_send_queue.get_free((long *)&i_session, ptemp_buf);

		if (!m_isrun || (i_session == -1 && len == 4))
			break;

		if (len < 0){
			continue;
		}

		if (i_session < 0 || i_session >= m_session_count){
			WriteToEventLog("%s: <%d>, len = %d, 超出会话界限,忽略", szFunctionName, i_session, len);
			continue;
		}

		// find session
		int i = i_session;
		if (i < 0 || i > m_session_count || !PUDPSESSION_ISOPEN(m_psessions+i)){
			WriteToEventLog("%s: 出错,试图在未打开的会话上发送(%d)", szFunctionName, i);
			write_buf_log(szFunctionName, (unsigned char *)ptemp_buf, len);
		}else{
			int r = m_listen_sock.sendto(ptemp_buf, len, &m_psessions[i].addr, 1000);
			if (r <= 0){
				WriteToEventLog("%s: 发送错误(%d)(return = %d)", szFunctionName, i, r);
				notify_close_session(i);
			}else{
				WriteToEventLog("%s: the %dth session sent (ret = %d, len=%d)", szFunctionName, i, r, len);
				on_sent(i, r);
			}
		}
	}

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

	bool isok = xwork_server::close(secs);	// 调用 stop

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
	psession->peerid = -1;
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

int xsys_udp_server::notify_close_session(int i)
{
	int r = m_close_requests.put(i, (char *)&i, 0);

	WriteToEventLog("notify_close_session: %d(r = %d)", i, r);

	if ((i % 4) == 0)
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
		WriteToEventLog("%s: 出错,试图在未打开的会话(%d)上发送(len=%d)\n%s", szFunctionName, isession, len, s);
		return -1;
	}
	m_psessions[isession].last_trans_time = long(time(0));
	return m_send_queue.put(isession, s, len);
}

int xsys_udp_server::send(int isession, const char * s)
{
	return m_send_queue.put(isession, s, strlen(s));
}

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
}

xsys_udp_server::xsys_udp_server(const char * name, int nmaxexception)
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
			memcmp(&m_psessions[j].addr, addr, sizeof(SYS_INET_ADDR)))
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
			++i;
		}
	}

	return n;
}

bool xsys_udp_server::open(int listen_port, int ttl, int max_sessions, int recv_len)
{
	if (max_sessions > 1024)
		max_sessions = 1024;
	
	m_requests.init((recv_len / 1024 + 1) * max_sessions, max_sessions);
	m_listen_port = listen_port;
	m_session_ttl = ttl;

	m_recv_len = recv_len;
	if (m_recv_len < 1024)
		m_recv_len = 1024;
	m_precv_buf = (char *)malloc(m_recv_len+1);
	memset(m_precv_buf,0,m_recv_len);

	m_psessions = new xudp_session[max_sessions];
	m_session_count = max_sessions;

	m_sends.init(4096,max_sessions);
	xudp_session * p = m_psessions;

	m_lock.init();

	return xwork_server::open();
}

bool xsys_udp_server::open(const char * url,int ttl, int recv_len)
{
	strncpy(m_serverURL, url, sizeof(m_serverURL)-1);  m_serverURL[sizeof(m_serverURL)-1] = 0;
	return open(0, ttl, 32, recv_len);
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

	// 启动发送线程
	m_send_thread.init(run_send_thread, this);
	m_msg_thread.init(run_msg_thread, this);

	int	session_i = 0;		/// 循环查找起始位置

	int i, conn_amount = 0, n, idle_times = 0;
	while (m_isrun && m_listen_sock.isopen()) {

		m_heartbeat = long(time(0));

		int crc;

		SYS_INET_ADDR from_addr;
		ZeroData(from_addr);
		int len = m_listen_sock.recv_from(m_precv_buf, m_recv_len-1, &from_addr, 1000);

		if (len == ERR_TIMEOUT || len == 0){
			++idle_times;
			
			if (idle_times >= 30){
				opened_find(&crc, NULL);	// 只是检查timeout，关闭过期连接
			}
			continue;
		}

		if (len < 0)
			break;

		// 找session，并检查timeout的会话
		int i = opened_find(&crc, &from_addr);
		
		if (i == -1){
			WriteToEventLog("新建会话:%40X", crc);
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
			// 至此，已经找到free的i，并且实施使用
			session_open(i);
			m_pused_index[m_used_count++] = i;
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
				if ((n = session_recv_to_request(i)) <= 0)
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
			session_recv_reset(i);
			break;
		case XTS_RECV_EXCEPT:
		case XTS_SESSION_END:
			session_close(i);
			break;
		}
	}
	close_all_session();
	m_listen_sock.close();

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_udp_server::msg_server(void)
{
	static const char szFunctionName[] = "xsys_udp_server::send_server";

	WriteToEventLog("%s : in", szFunctionName);
	
	char * ptemp_buf = (char *)calloc(1, m_recv_len+1);

	while (m_isrun){
		int isession = -1;

		/// 取出数据
		int len = m_requests.get_free((long *)(&isession), ptemp_buf);

		/// 调用处理函数进行消息处理
		int r = do_msg(isession, ptemp_buf, len);

		WriteToEventLog("%s : do_msg 处理%d长的数据，返回%d", szFunctionName, len, r);
	}

	::free(ptemp_buf);
	
	WriteToEventLog("%s : out", szFunctionName);
}

void xsys_udp_server::send_server(void)
{
	static const char szFunctionName[] = "xsys_udp_server::send_server";

	WriteToEventLog("%s : in.\n", szFunctionName);
	xseq_buf_use use;
	while (m_isrun && m_listen_sock.isopen()){
		int r = m_sends.get(&use);
		m_heartbeat = long(time(0));

		if (r < 0){
			if (strcmp(m_cmd, "stop") == 0)
				break;
			continue;
		}

		// find session
		int i = use.id;
		if (i < 0 || i > m_session_count || !PSESSION_ISOPEN(m_psessions+i)){
			WriteToEventLog("%s: 出错,试图在未打开的会话上发送(%d)", szFunctionName, i);
			write_buf_log(szFunctionName, (unsigned char *)use.p, use.len);
		}else{
			r = m_listen_sock.sendto(use.p, use.len, &m_psessions[i].addr, 1000);
			if (r <= 0){
				WriteToEventLog("%s: 发送错误(%d)(return = %d)", szFunctionName, i, r);
				session_close(i);
			}else{
				WriteToEventLog("%s: the %dth session sent (ret = %d, len=%d)", szFunctionName, i, r, use.len);
				on_sent(i, r);
			}
		}
		m_sends.free_use(use);
	}
	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_udp_server::close_all_session(void)
{
	if (m_psessions){
		for (int i = m_session_count-1; i > 0 ; i--) {
			if (PSESSION_ISOPEN(m_psessions+i)) {
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

	m_send_thread.down();
	m_msg_thread.down();

	bool isok = xwork_server::stop(secs);

	if (m_psessions){
		close_all_session();
		delete[] m_psessions;  m_psessions = 0;
	}

	m_requests.down();
	m_sends.down();

	WriteToEventLog("%s: out(%d)", szFunctionName, isok);

	return isok;
}

bool xsys_udp_server::close(int secs)
{
	static const char szFunctionName[] = "xsys_udp_server::close";

	WriteToEventLog("%s[%d] : in", szFunctionName, secs);

	bool isok = stop(secs);

	isok = xwork_server::close(secs);
	
	if (m_precv_buf){
		free(m_precv_buf);  m_precv_buf = 0;
	}

	m_lock.down();

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

	m_lock.lock(3000);

	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = 0;
	}

	memset(psession, 0, sizeof(xudp_session));;

	psession->recv_state = XTS_SESSION_END;

	m_lock.unlock();
}

bool xsys_udp_server::session_isopen(int i)
{
	if (i < 0 || i >= m_session_count)
		return false;

	xudp_session * psession = m_psessions + i;
	return PSESSION_ISOPEN(psession);
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
int xsys_udp_server::session_recv_to_request(int i)
{
	xudp_session * psession = m_psessions + i;

	int r = m_requests.put(i, psession->precv_buf, psession->recv_len);
	xsys_sleep_ms(3);
	WriteToEventLog("session_recv_to_request: %d - send packet to recver", i);
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

int xsys_udp_server::send(int isession, const char * s, int len)
{
	static const char szFunctionName[] = "xsys_udp_server::send";

	if (!PSESSION_ISOPEN(m_psessions+isession)){
		WriteToEventLog("%s: 出错,试图在未打开的会话(%d)上发送(len=%d)\n%s", szFunctionName, isession, len, s);
		return -1;
	}
	m_psessions[isession].last_trans_time = long(time(0));
	return m_sends.put(isession, s, len);
}

int xsys_udp_server::send(int isession, const char * s)
{
	return m_sends.put(isession, s);
}

#include <xsys_udp_server.h>

#define XSYS_UDP_SESSION_TIMEOUT(i) (m_session_ttl < m_heartbeat - s_[i].last_recv_time)

/* #define XSYS_UDP_SESSION_TIMEOUT(i) 	((m_session_ttl < m_heartbeat - s_[i].last_recv_time) || \
			(15 < m_heartbeat - s_[i].last_recv_time && \
			 s_[i].last_recv_time == s_[i].createTime \
			))
*/

#define XSYS_UDP_RELAY_TIME 	(120 * 1000)

static unsigned int run_send_thread(void * pudp_server)
{
	try{
		xsys_udp_server * pserver = (xsys_udp_server *)pudp_server;
		pserver->send_server();
	}catch(...){
		WriteToEventLog("x_udp_svr::send_server: an exception occured.");
	}
	return 0;
}

static unsigned int run_msg_thread(void * pudp_server)
{
	try{
		xsys_udp_server * pserver = (xsys_udp_server *)pudp_server;
		pserver->msg_server();
	}catch(...){
		WriteToEventLog("x_udp_svr::msg_server: an exception occured.");
	}
	return 0;
}

static unsigned int run_relay_thread(void * pudp_server)
{
	try{
		xsys_udp_server * p = (xsys_udp_server *)pudp_server;
		p->relay_server();
	}catch(...){
		WriteToEventLog("x_udp_svr::msg_server: an exception occured.");
	}
	return 0;
}

xsys_udp_server::xsys_udp_server()
	: xwork_server("XUDP Server", 5, 1)
	, port_(0)
	, m_session_ttl(300)
	, s_(nullptr)
	, m_precv_buf(0)
	, m_recv_len(0)
	, m_session_idle(10)
	, m_plisten_sock(nullptr)
	, has_new_cmd_(false)
{
	m_serverURL[0] = 0;
	m_send_len = 0;
}

xsys_udp_server::xsys_udp_server(const char * name, int nmaxexception)
	: xwork_server(name, nmaxexception, 1)	// 启动两个服务 0: accept/recv, 1: send
	, port_(0)
	, m_session_ttl(300)
	, s_(nullptr)
	, m_precv_buf(0)
	, m_recv_len(0)
	, m_session_idle(10)
	, m_plisten_sock(nullptr)
	, has_new_cmd_(false)
{
	m_serverURL[0] = 0;
	m_send_len = 0;
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

//*
void xsys_udp_server::timeout_check(void)
{
	static const char szFunctionName[] = "xsys_udp2::timeout_check";

	int i, i_s, count = s_q_.count_, i_post;
	for (i = 0, i_s = s_q_.first(); i < count; i++, i_s = i_post){
		i_post = s_q_.post(i_s);
		if (XSYS_UDP_SESSION_TIMEOUT(i_s)){
			WriteToEventLog("%s: client[%d] timeout[%d/20](%d - %d)",
				szFunctionName, i_s, m_session_ttl, int(s_[i_s].createTime), m_heartbeat
			);
			// 由于close之后，m_used_count会减1，且之后的会前移，故此i保持不变
			session_close(i_s);
		}
	}
	if (s_q_.count_ < 1)
		m_recv_queue.clear();
}

/*
 * 查找匹配的会话
 * 返回是否是转发，以及校验字
 */
int xsys_udp_server::opened_find(bool &for_trans, int * crc, SYS_INET_ADDR * addr)
{
	static const char szFunctionName[] = "x_udp_svr::opened_find";

	if (!addr || !crc)
		return -1;

	for_trans = false;

	*crc = calc_addr_crc(addr);

	int i, i_s;
	for (i = 0, i_s = s_q_.first(); i < s_q_.count_; i++, i_s = s_q_.post(i_s)){
		// 用CRC和addr进行比较查找
		if (s_[i_s].addr_crc == *crc &&
			memcmp(&s_[i_s].addr, addr, sizeof(SYS_INET_ADDR)) == 0)
		{
			for_trans = XSYS_VALID_SOCK(s_[i_s].target_sock);
			return i_s;
		}
	}

	return -1;
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

	m_recv_queue.init(
		(recv_len+1023) / 1024 * (max(16, (max_sessions / 8) + 8)),	// 最少16,最多大约1/4
		max(max_sessions/2+16, 4)
	);

	port_ = listen_port;
	m_session_ttl = ttl;
	m_session_idle = ttl / 4;
	if (m_session_idle < 3)
		m_session_idle = 3;

	m_recv_len = recv_len;
	if (m_recv_len < 1024)
		m_recv_len = 1024;
	m_precv_buf = (char *)calloc(m_recv_len+4, 1);

	s_q_.open(max_sessions);	// 4K = 4*1024 指令缓冲
	s_ = s_q_.get_recs(); // 取得指令数组地址
	int i;
	for (i = 0; i < s_q_.amount_; i++){
		s_[i].i = i;
		s_[i].recv_state = XTS_SESSION_END;
	}

	/*
	s_ = (struct xudp_session *)calloc(max_sessions, sizeof(struct xudp_session));
	// set all is free
	int i;
	for (i = 0; i < max_sessions; i++)
		s_[i].recv_state = XTS_SESSION_END;

	m_pused_index = (int *)calloc(max_sessions, sizeof(int));
	m_session_count = max_sessions;

	xudp_session * p = s_;
	*/

	m_send_len = max(send_len, 32);
	m_send_queue.init(
		(send_len+1023)/1024 * max(8, (max_sessions/16+4)),
		max(max_sessions/16+4, 16)
	);
	m_close_requests.init(8, max_sessions/64+16);

	m_relay_queue.init(
		(recv_len+1023) / 1024 * (max(16, (max_sessions / 8) + 8)),	// 最少16,最多大约1/4
		max(max_sessions/2+16, 4)
	);

	WriteToEventLog("x_udp_svr::open: TTL=%d, max session=%d, recv_len=%d, send_len=%d",
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

void xsys_udp_server::run(void)
{
	static const char szFunctionName[] = "x_udp_svr::run";

	WriteToEventLog("%s : in.\n", szFunctionName);

	if (m_plisten_sock == nullptr)
		m_plisten_sock = new xsys_socket;

	if (!m_plisten_sock->isopen() && m_plisten_sock->udp_listen(port_) != 0){
		WriteToEventLog("%s : 创建消息监听(%d)失败，退出运行", szFunctionName, port_);
		return;
	}

	WriteToEventLog("%s: %s\n", szFunctionName, m_plisten_sock->get_self_ip(m_lastmsg));

	// 启动发送线程
	if (m_relay_thread.m_thread == SYS_INVALID_THREAD)
		m_relay_thread.init(run_relay_thread, this);
	if (m_send_thread.m_thread == SYS_INVALID_THREAD)
		m_send_thread.init(run_send_thread, this);
	if (m_msg_thread.m_thread == SYS_INVALID_THREAD)
		m_msg_thread.init(run_msg_thread, this);

	// wait the other worker in ready
	xsys_sleep_ms(3000);

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
			if (!XUDPSESSION_INRANGE(i)) {
				WriteToEventLog("%s: 关闭无效会话(%d)", szFunctionName, i);
				continue;
			}else if (!PUDPSESSION_ISOPEN(s_+i) || t < s_[i].createTime) {
				WriteToEventLog("%s: 忽略过期关闭%d, %d <= %d", szFunctionName, i, t, s_[i].createTime);
				continue;
			}

			WriteToEventLog("%s: get close %d request", szFunctionName, i);
			// lock_prop(2);
			session_close(i);
			// unlock_prop();
		}

		m_heartbeat = long(time(0));

		/// 检查所有空闲的session，并调用处理方法
		// lock_prop(1);
		if (m_heartbeat - time_check_time > 6 && s_q_.count_){
			int i, i_s, count = s_q_.count_, i_post;
			for (i = 0, i_s = s_q_.first(); i < count; i++, i_s = i_post){
				i_post = s_q_.post(i_s);
				if (XSYS_UDP_SESSION_TIMEOUT(i_s)){
					WriteToEventLog("%s: client[%d/%d] timeout[%d](%d, %d, %d)", szFunctionName, i, i_s, m_session_ttl, int(s_[i_s].createTime), int(s_[i_s].last_recv_time), m_heartbeat);
					if (session_close(i_s) == 1)
						s_q_.del(i_s);
				}else if (s_[i_s].idle_secs > 0 &&
					m_heartbeat >= s_[i_s].last_trans_time + s_[i_s].idle_secs)
				{
					char parms[32];
					sprintf(parms, "idle:%d", i_s);
					notify_do_cmd(parms);
				}
			}
			if (s_q_.count_ < 1)
				m_recv_queue.clear();
			
			time_check_time = m_heartbeat;
		}
		// unlock_prop();

		int crc = 0;

		SYS_INET_ADDR from_addr;
		ZeroData(from_addr);
		memset(m_precv_buf, 0, m_recv_len);
		int len = m_plisten_sock->recv_from(m_precv_buf, m_recv_len-1, &from_addr, 2200);

		m_heartbeat = long(time(0));

		if (len == ERR_TIMEOUT){
			++idle_count;
			if ((idle_count & 0x00FF) == 0){
				/*
				m_sock_lock.lock(1500);
				m_plisten_sock->shutdown();
				delete m_plisten_sock;
				m_plisten_sock = new xsys_socket;
				int r = m_plisten_sock->udp_listen(port_);
				m_sock_lock.unlock();
				WriteToEventLog("%s: reopen(%d) %d -> %d", szFunctionName, port_, idle_count, r);
				//*/
			}
			continue;
		}

		// error occured
		if (len <= 0){
			m_sock_lock.lock(1500);
			m_plisten_sock->close();
			delete m_plisten_sock;  m_plisten_sock = NULL;
			WriteToEventLog(
				"%s: recv return = %d(parms len = %d)",
				szFunctionName, len, m_recv_len
			);

//			xsys_sleep_ms(500);
			m_plisten_sock = new xsys_socket;
			m_plisten_sock->udp_listen(port_);
			m_sock_lock.unlock();

			continue;
		}

		// 找session
		bool bfor_relay = false;
		int i = opened_find(bfor_relay, &crc, &from_addr);

		{
			char ip_port[64];
			SysInetRevNToA(from_addr, ip_port, MAX_IP_LEN);
			WriteToEventLog(
				"peer ip: %s(used count = %d) crc=%08X\n",
				ip_port, s_q_.count_, (unsigned int)crc
			);
		}

		if (i == -1){
			WriteToEventLog("新建会话:%04X, time = %d", crc, m_heartbeat);

			if (s_q_.is_full()){
				WriteToEventLog("强制最早建立的会话过期，used = %d", s_q_.count_);
				i = s_q_.first();
				session_close(i);
			}

			// find a free session
			i = s_q_.get();

			// 至此，已经找到free的i，并且实施使用
			session_open(i);
			memcpy(&s_[i].addr, &from_addr, sizeof(SYS_INET_ADDR));
			s_[i].addr_crc = crc;
			s_q_.add(i);
		}else{
			s_[i].last_trans_time = s_[i].last_recv_time = m_heartbeat;
		}
		s_[i].recv_count++;

		// receive data
		m_precv_buf[len] = '\0';
		int r;

		m_heartbeat = time(0);
		/// 判断转发, 首次接收时判断
		if (s_[i].recv_count <= 1 && !bfor_relay){
			const char * prelay_url = get_target_url(i, m_precv_buf, len);
			if (prelay_url){
				xsys_socket sock;
				r = sock.connect(prelay_url, 0, 2, false);
				if (r >= 0){
					char b[64];
					WriteToEventLog("relay peer ip: %s", sock.get_peer_ip(b));
					WriteToEventLog("relay owner: %s\n", sock.get_self_ip(b));
					// 设置转发
					s_[i].target_sock = sock.m_sock;
					sock.m_sock = SYS_INVALID_SOCKET;
					bfor_relay = true;
				}else
					WriteToEventLog("建立转发(%s)失败", prelay_url);
			}
		}

		write_buf_hex_log(szFunctionName, (unsigned char *)m_precv_buf, len);

		/// 判断直接发送
		if (s_[i].recv_len == 0 && (r = calc_msg_len(i, m_precv_buf, len)) == len){
			// 接收了完整包，直接送入处理队列
			if (bfor_relay){
				SysSendData(s_[i].target_sock, m_precv_buf, len, 500);
				r = m_relay_queue.put(i, m_precv_buf, len);
				WriteToEventLog("%s: <%d> - send to relay_queue", szFunctionName, i);
			}else{
				r = m_recv_queue.put(i, m_precv_buf, len);
				WriteToEventLog("%s: <%d> - send(%d/%d) to recv_queue", szFunctionName, i, r, len);
			}
			s_[i].recv_state = XTS_RECV_READY;
			std::this_thread::yield();
			continue;
		}

		/// 分包到达的处理
		r = session_recv(i, len);	// 将数据存入session的临时缓存

		if (r >= len){
			s_[i].recv_state = XTS_RECVING;
			// 检查接收状态
			// 返回首个完整包的应有长度，0 < 为非协议包
			r = calc_msg_len(i);
			WriteToEventLog("%s: client[%d, len=%d], msg_len=%d, time = %d", szFunctionName, i, len, r, m_heartbeat);
		}else{
			WriteToEventLog("%s: client[%d, len=%d] save error, r=%d, time = %d", szFunctionName, i, len, r, m_heartbeat);
			r = -1;
			s_[i].recv_state = XTS_RECV_ERROR;
		}

		if (r <= 0){
			// 无效或无用数据，则释放
			write_buf_log("x_udp_svr recved INVALID data", (unsigned char *)m_precv_buf, len);
			s_[i].recv_state = XTS_RECV_ERROR;
		}else{
			if (r <= s_[i].recv_len)
				s_[i].recv_state = XTS_RECVED;
			else
				s_[i].recv_state = XTS_RECVING;
		}

		switch (s_[i].recv_state){
		case XTS_RECVED:
			WriteToEventLog("%s: %d - recved a packet data(%d/%d/%d)",
				szFunctionName, i,
				r, len, s_[i].recv_len
			);
			// 将收到的完整包逐条发到request消息队列中
			while (r > 0 && r <= s_[i].recv_len){
				// 发布
				if ((len = session_recv_to_request(i, r)) <= 0)
					break;
				r = calc_msg_len(i);
			}
			WriteToEventLog("%s: %d - send packet to recver", szFunctionName, i);
			xsys_sleep_ms(5);

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
	static const char szFunctionName[] = "x_udp_svr::msg_server";

	WriteToEventLog("%s : in", szFunctionName);

	char * ptemp_buf = (char *)calloc(1, m_recv_len+4);
	if (ptemp_buf == nullptr)
		return;

	int idle = 120 * 1000;

	while (m_isrun){
		int isession = -1;

		m_recv_queue.set_timeout_ms(idle);

		int r;

		/// 取出数据
		memset(ptemp_buf, 0, m_recv_len);
		int len = m_recv_queue.get_free((long *)(&isession), ptemp_buf);

		WriteToEventLog("%s : len = %d - %d", szFunctionName, len, int(time(0)));
		try{
			if (len != ERR_TIMEOUT){
				if (len <= 0)
					continue;

				r = 0;
				if (!XUDPSESSION_INRANGE(isession)){
					// 系统通知执行，或者出错
					if (isession == -1){
						if (strcmp(ptemp_buf, "stop") == 0)
							break;

						if (strncmp(ptemp_buf, "idle:", 5) == 0){
							// set session
							isession = atoi(ptemp_buf+5);
							r = do_idle(isession);
							s_[isession].last_trans_time = m_heartbeat;
						}else if (strcmp(ptemp_buf, "cmds") == 0 || len < 1){
							WriteToEventLog("%s: load cmds", szFunctionName);
							r = do_cmd();
						}else{
							WriteToEventLog("%s: load cmds for(%s)", szFunctionName, ptemp_buf);
							r = do_cmd(ptemp_buf);
						}
					}else
						WriteToEventLog("%s: <%d>, len = %d, 超出会话界限,忽略", szFunctionName, isession, len);

					if (r == XSYS_IP_FATAL){
						if (isession < 0)
							idle = 120 * 1000;
						else
							notify_close_session(isession, false);
					}

					continue;
				}

				/// 调用处理函数进行消息处理
				r = do_msg(isession, ptemp_buf, len);
				if (r == XSYS_IP_FATAL)
					notify_close_session(isession, false);

				WriteToEventLog("%s: <%d>处理%d长的数据，返回%d - %d", szFunctionName, isession, len, r, int(time(0)));
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
	static const char szFunctionName[] = "x_udp_svr::send_server";

	char * ptemp_buf = (char *)calloc(1, m_send_len+1);
	if (ptemp_buf == nullptr)
		return;

	WriteToEventLog("%s : in.\n", szFunctionName);

	m_send_queue.set_timeout_ms(120*1000);

	// create a socket for send
	// xsys_socket sock;

	while (m_isrun){
		int i_session = -1;

		// bind to the local port
		// if (!sock.isopen() && sock.udp_listen(port_) != 0){
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
			// xsys_sleep_ms(50);
			continue;
		}

		if (len == ERR_TIMEOUT){
			continue;
		}

		if (!XUDPSESSION_INRANGE(i_session)){
			WriteToEventLog("%s: <%d>, len = %d, 超出会话界限,忽略", szFunctionName, i_session, len);
			continue;
		}

		write_buf_log(szFunctionName, (unsigned char *)ptemp_buf, len);

		// find session
		int i = i_session, t = int(time(0));

		if (!XUDPSESSION_INRANGE(i) || !PUDPSESSION_ISOPEN(s_+i)){
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

			int r = 0;
			if (m_plisten_sock)
				r = m_plisten_sock->sendto(ptemp_buf, len, &s_[i].addr, 10);

			// int r = sock.sendto(ptemp_buf, len, &s_[i].addr, 3);
			if (r <= 0){
				WriteToEventLog("%s: 发送错误(%d)(return = %d, time = %d)", szFunctionName, i, r, t);
				notify_close_session(i);
				// sock.close();
//				xsys_sleep_ms(500);
			}else{
				WriteToEventLog("%s: the %dth session sent (ret = %d, len=%d, time = %d)", szFunctionName, i, r, len, t);

				s_[i].last_trans_time = t;

				on_sent(i, r);
			}
		}
	}

//	if (sock.isopen())
//		sock.close();

	::free(ptemp_buf);

	WriteToEventLog("%s : Exit.", szFunctionName);
}

void xsys_udp_server::relay_server(void)
{
	static const char szFunctionName[] = "x_udp_relay_svr";

	WriteToEventLog("%s: in", szFunctionName);

	int idle = XSYS_UDP_RELAY_TIME;

	char* ptemp_buf = (char*)calloc(1, m_recv_len + 1);

	class {
	public:
		xlist_s<int>		indexs;	// 记录session引用位置
		xlist_s<SYS_SOCKET> socks;	// 记录转发socket

		void add_session(int isession, SYS_SOCKET sock)
		{
			int i;
			for (i = 0; i < indexs.m_count; i++)
				if (isession == indexs[i])
					break;
			if (i == indexs.m_count) {
				indexs.add(isession);
				socks.add(sock);
				WriteToEventLog("SS add(%d): %d, %d", i, isession, sock);
			}
		}
		void del_session(int isession)
		{
			int i;
			for (i = 0; i < indexs.m_count; i++)
				if (isession == indexs[i]) {
					del_session_i(i);
					break;
				}
		}
		void del_session_i(int i)
		{
			if (i >= 0 && i < indexs.m_count) {
				WriteToEventLog("SS del(%d): %d, %d", i, indexs[i], socks[i]);
				indexs.del(i);
				socks.del(i);
			}
		}
		void clear_invalid(xudp_session* s, int count)
		{
			int i;
			for (i = 0; i < indexs.m_count;) {
				int isession = indexs[i];
				if (isession >= count || isession < 0 || !XSYS_VALID_SOCK(s[isession].target_sock)) {
					indexs.del(i);
					socks.del(i);
				}
				else
					i++;
			}
		}
		void clear_all(void)
		{
			indexs.free_all();  socks.free_all();
		}
	} SS;

	int r;
	while (m_isrun){
		int isession = -1;

		m_relay_queue.set_timeout_ms(idle);

		/// 取出数据,转发
		int i, len;
		do{
			// memset(ptemp_buf, 0, m_recv_len);
			len = m_relay_queue.get_free((long *)(&isession), ptemp_buf);

			if (isession == -1 && len == 4){
				WriteToEventLog("%s: get cmd[%s]", szFunctionName, ptemp_buf);
				if (strcmp(ptemp_buf, "stop") == 0)
					break;
				// sock.close();
				xsys_sleep_ms(100);
				continue;
			}

			if (len > 0 && isession >= 0){
				// 转发
				SYS_SOCKET sock = s_[isession].target_sock;

				if (!XSYS_VALID_SOCK(sock)){
					SS.del_session(isession);
					WriteToEventLog("%s: no relay, [%d]'s relay-sock is invalid(%u)",
						szFunctionName, isession, (unsigned int)(sock)
					);
				}else{
					xsys_socket s;
					s.m_sock = sock;
					char b[32];
					WriteToEventLog("-> %s", s.get_peer_ip(b));
					WriteToEventLog("<- %s", s.get_self_ip(b));
					// r = SysSendData(sock, ptemp_buf, len, 100);
					r = len;
					s.m_sock = SYS_INVALID_SOCKET;
					if (r != len){
						WriteToEventLog("%s: relay [%d]'s %d bytes parcel failed, r = %d",
							szFunctionName, isession, len, r
						);
						SS.del_session(isession);
					}else{
						// 转发成功,加入
						SS.add_session(isession, sock);
					}
				}
				write_buf_log(szFunctionName, (unsigned char *)ptemp_buf, len);
				m_relay_queue.set_timeout_ms(10);
			}
		}while(len > 0);

		if (!m_isrun)
			break;

		long tnow = int(time(0));

		lock_prop(2);
		SS.clear_invalid(s_, s_q_.amount_);
		unlock_prop();

		if (SS.indexs.m_count < 1)
		{
			idle = XSYS_UDP_RELAY_TIME;
			continue;
		}

		// idle = min(s_expires_secs - 60 - (tnow - s_last_auth_time), s_idle);
		// idle = max(idle, 5);
		idle = 2;

		// WriteToEventLog("will wait %d secs, socks=%d\n", idle, SS.socks.m_count);

		if (SS.socks.m_count < 1)  continue;

		// 接收数据
		int n;
		fd_set fdsr;
		FD_ZERO(&fdsr);
		for (i = n = 0; i < SS.socks.m_count; i++){
			FD_SET(SS.socks[i], &fdsr);
			if (n < int(SS.socks[i]))
				n = int(SS.socks[i]);
		}

		if (n == 0){
			WriteToEventLog("%s: no valid socket", szFunctionName);
			continue;
		}

		// struct timeval tv;
		// tv.tv_sec  = 2;
		// tv.tv_usec = 0;
		r = SysSelect(n+1, &fdsr, NULL, NULL, 1900);
		if (r <= 0){
			if (r < 0 && r != ERR_TIMEOUT)
				WriteToEventLog("%s: select ret: %d", szFunctionName, r);
			continue;
		}

		for (i = n = 0; i < SS.socks.m_count; i++){
			SYS_SOCKET s = SS.socks[i];
			if (!XSYS_VALID_SOCK(s))
				continue;

			++n;

			if (!FD_ISSET(s, &fdsr))
				continue;

			len = SysRecvData(s, ptemp_buf, m_recv_len, 300);
			FD_CLR(s, &fdsr);

			if (len < 0){
				SS.del_session_i(i);
				continue;
			}

			tnow = int(time(0));

			lock_prop(1);
			s_[SS.indexs[i]].last_trans_time = int(time(0));
			unlock_prop();
			r = send(SS.indexs[i], ptemp_buf, len);
			// write_buf_log(szFunctionName, (unsigned char *)ptemp_buf, len);
		}
	}

	::free(ptemp_buf);
	WriteToEventLog("%s: out", szFunctionName);
}

void xsys_udp_server::close_all_session(void)
{
	int i, i_s, i_post, count = s_q_.count_;
	for (i = 0, i_s = s_q_.first(); i < count; i++, i_s = i_post){
		i_post = s_q_.post(i_s);
		if (PUDPSESSION_ISOPEN(s_+i_s))
			session_close(i_s);
	}
}

bool xsys_udp_server::stop(int secs)
{
	static const char szFunctionName[] = "x_udp_svr::stop";

	WriteToEventLog("%s: in", szFunctionName);
	m_isrun = false;

	// notify send/recv thread stop
	m_recv_queue.put(-1, "stop", 4);
	m_send_queue.put(-1, "stop", 4);
	m_relay_queue.put(-1, "stop", 4);
	xsys_sleep_ms(10);

	if (m_plisten_sock)
		m_plisten_sock->close();

	bool isok = xwork_server::stop(secs);

	xsys_sleep_ms(100);

	m_msg_thread.down();
	m_send_thread.down();
	m_relay_thread.down();

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
	static const char szFunctionName[] = "x_udp_svr::close";

	WriteToEventLog("%s[%d] : in", szFunctionName, secs);

	bool isok = xwork_server::close(secs);	// 调用 stop

	m_sock_lock.down();

	m_recv_queue.down();
	m_send_queue.down();
	m_relay_queue.down();

	xsys_sleep_ms(10);

	m_close_requests.down();

	if (s_){
		close_all_session();
		s_q_.close();
		s_ = nullptr;
	}

	/*
	if (m_pused_index){
		::free(m_pused_index);  m_pused_index = 0;
	}
	*/

	if (m_precv_buf){
		free(m_precv_buf);  m_precv_buf = 0;
	}

	WriteToEventLog("%s[%d] : out(%d)", szFunctionName, secs, isok);

	return isok;
}

void xsys_udp_server::session_open(int i)
{
	xudp_session * psession = s_ + i;

	if (psession->precv_buf){
		free(psession->precv_buf);
	}

	memset(psession, 0, sizeof(xudp_session));

	psession->idle_secs = m_session_idle;

	psession->createTime =
		psession->last_recv_time  =
		psession->last_trans_time = m_heartbeat;
	int r = on_open(i);
}

int xsys_udp_server::notify_close_session(int i, bool need_shift)
{
	time_t now = time(0);
//	if (need_shift)
//		++now;
	int r = m_close_requests.put(i, (char *)&now, sizeof(now));

	WriteToEventLog("x_udp_svr notify_close_session: %d(r=%d) force=%d", i, r, (need_shift?1:0));

//	if (need_shift && (i % 4) == 0)
//		xsys_sleep_ms(5);

	return r;
}

bool xsys_udp_server::session_isopen(int i)
{
	if (!XUDPSESSION_INRANGE(i))
		return false;

	xudp_session * psession = s_ + i;
	return PUDPSESSION_ISOPEN(psession);
}

int xsys_udp_server::session_close(int i)
{
	static const char szFunctionName[] = "x_udp_svr::session_close";

	if (!XUDPSESSION_INRANGE(i)){
		WriteToEventLog("%s: i = %d is out range[0~%d]", szFunctionName, i, s_q_.count_);
		return -1;
	}

	xudp_session * psession = s_ + i;
	if (!PUDPSESSION_ISOPEN(psession)){
		WriteToEventLog("%s: i = %d is not opened", szFunctionName, i);
		return 1;
	}
	
	WriteToEventLog("%s: i = %d will be closed", szFunctionName, i);
	on_closed(i);
	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = nullptr;
	}
	psession->recv_state = XTS_SESSION_END;
	WriteToEventLog("%s: i = %d is closed", szFunctionName, i);
	s_q_.del(i);

	return 0;
}

void xsys_udp_server::session_recv_reset(int i)
{
	xudp_session * psession = s_ + i;
	if (psession->precv_buf){
		free(psession->precv_buf);  psession->precv_buf = 0;
	}
	psession->recv_buflen = psession->recv_len = 0;
	psession->recv_state = XTS_RECV_READY;
}

// 将数据发布到request队列
int xsys_udp_server::session_recv_to_request(int i, int len)
{
	if (len < 1 || !XUDPSESSION_INRANGE(i)){
		WriteToEventLog("session_recv_to_request(%d, %d): 参数出错", i, len);
		return 0;
	}

	xudp_session * psession = s_ + i;

	if (len > psession->recv_len)
		len = psession->recv_len;

	int r;

	if (PUDPSESSION_ISOPEN(psession)){
		if (XSYS_VALID_SOCK(psession->target_sock))
			r = m_relay_queue.put(i, psession->precv_buf, len);
		else
			r = m_recv_queue.put(i, psession->precv_buf, len);
	}else{
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
	xudp_session * psession = s_ + i;

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

void xsys_udp_server::set_session_idle(int isession, int idle_secs)
{
	if (!PUDPSESSION_ISOPEN(s_+isession))
		return;

	s_[isession].idle_secs = idle_secs;
}

void xsys_udp_server::set_idle(int idle_secs)
{
	if (idle_secs > m_session_ttl)
		m_idle = m_session_ttl + 3;
	else if (idle_secs < 3)
		m_idle = 3;
	else
		m_idle = idle_secs;
}

int xsys_udp_server::send(int isession, const char * s, int len)
{
	static const char szFunctionName[] = "x_udp_svr::send";

	if (!XUDPSESSION_INRANGE(isession) || !PUDPSESSION_ISOPEN(s_+isession))
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

const char * xsys_udp_server::get_target_url(int isession, char * pbuf, int len)
{
	return NULL;
}
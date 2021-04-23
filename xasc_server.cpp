#include <xasc_server.h>

#define XASC_TCP_SESSION_TIMEOUT(i) ((session_ttl_ < heartbeat_ - sesns_[i].last_recv_time) || \
			(40 < heartbeat_ - sesns_[i].last_recv_time && \
			 sesns_[i].last_recv_time == sesns_[i].create_time \
			))

#define XASC_SESSION_VALID(i)		(i >= 0 && i < max_sessions_)

xasc_server::xasc_server(const char * name)
	: listen_port_(0)
	, session_ttl_(300)
	, sesns_(0)
	, max_sessions_(0)
	, most_len_(0)
	, send_len_(0)
{
	is_run_ = false;
	if (name == nullptr) {
		name = "XASC_SVR";
	}

	strcpy(name_, name);
}

bool xasc_server::open(int listen_port, int max_sessions, int ttl, int most_len, int max_len, int send_len)
{
	listen_port_ = listen_port;
	max_sessions_ = max(max_sessions, 4) + 4;	// 有可能多出未能决定的连接，多给4个
	most_len_ = most_len;
	max_len_ = max_len;
	send_len_ = send_len;
	session_ttl_ = ttl;
	idle_ = max(ttl / 4, 5);

	int ret = iom_.open(listen_port_, max_sessions_, most_len, max_len);

	if (ret < 0){
		log_print("%s: asc_io open error(%d)", __FUNCTION__, ret);
		return false;
	}

	// 消息处理所用消息队列
	int recv_queue_kbytes = (most_len_ * max_sessions / 2 + max_len + 1023) / 1024;
	recv_queue_.init(max(4, recv_queue_kbytes), max(max_sessions, 4));
	send_queue_.init(max(4, (send_len+1023)/1024*(max_sessions/5+2)), max(max_sessions/2+1, 4));
	close_requests_.init(4, max_sessions_/2+1);

	sesn_q_.open(max_sessions_);
	sesns_ = sesn_q_.get_recs();
	int i;
	for (i = 0; i < max_sessions_; i++)
		sesns_[i].i = i;

	log_print("XASC::open: port=%d, TTL=%d, max session=%d, recv_len=%d, send_len=%d",
		listen_port_, session_ttl_, max_sessions_, max_len_, send_len_
	);

	return true;
}

bool xasc_server::close(int secs)
{
	log_print("%s[%d] : in", __FUNCTION__, secs);

	bool isok = stop(secs);

	sesn_q_.close();

	recv_queue_.done();
	send_queue_.done();
	close_requests_.done();

	log_print("%s[%d] : out(%d)", __FUNCTION__, secs, isok);

	return isok;
}

bool xasc_server::start(void)
{
	is_run_ = true;

	th_sender_ = std::thread([this]{ sender(); });
	th_actor_  = std::thread([this]{ actor();  });

	xsleep_ms(300);

	th_master_ = std::thread([this]{ master(); });

	return true;
}

bool xasc_server::stop(int secs)
{
	log_print("%s: in", __FUNCTION__);

	if (!is_run_)
		return true;

	is_run_ = false;

	// 关闭尚未处理的会话
	session_close(iom_.get_accept_s());

	// 关闭 I/O 处理
	iom_.close();
	xsleep_ms(300);

	// notify recver stop
	notify_do_cmd(-1, "stop");

	// notify sender thread stop
	this->send(-1,"stop",4);

	xsleep_ms(100);

	if (th_master_.joinable())
		th_master_.join();
	if (th_actor_.joinable())
		th_actor_.join();
	if (th_sender_.joinable())
		th_sender_.join();

	session_close_all();

	log_print("%s: out", __FUNCTION__);

	return true;
}

// 所有会话的超时最小值
int xasc_server::timeout_secs(void)
{
	int secs = 99999999;

	int i, i_sesn = sesn_q_.used_;
	for (i = 0; i < sesn_q_.count_; i++, i_sesn = sesn_q_.indexs_[i_sesn].i_post) {
		auto s = sesns_ + i_sesn;

		int x = heartbeat_ - s->last_recv_time;	// 现在-最近通讯时间
		// 取超时时间，若是第一个包，用空闲时间，不然用超时时间
		int d = (s->recv_amount < 1)? idle_ : session_ttl_;
		d -= x;	// 超时时间
		if (d < 0)
			session_close(s);
		else{
			if (secs > d)
				secs = d;
			x = heartbeat_ - max(s->last_trans_time, s->last_recv_time);
			if (x > idle_)
				notify_do_cmd(s->i, "idle");
		}
	}

	return secs;
}

void xasc_server::master(void)
{
	log_print("%s : in.\n", __FUNCTION__);

	int i;
	int not_full_packet_count = 0;

	xasc_session * psession = nullptr;
	int ret = sesn_q_.get(&psession);
	iom_.add_fd(psession);

	char* precv_buf = (char*)calloc(most_len_+4, 1);
	while (is_run_ && iom_.isopen()) {

		// 检查关闭连接请求
		while (!close_requests_.isempty()){
			int n = close_requests_.get((long *)&i, precv_buf);
			if (n == 0){
				log_print("%s: get close %d request", __FUNCTION__, i);
			}
		}

		heartbeat_ = int(time(0));

		// check timeout or idle
		int idle = timeout_secs();
		int bytes;
		ret = iom_.process_event(&psession, &bytes, idle * 1000);

		if (ret == XASC_OP_EXIT){
			log_print("%s: get exit command", __FUNCTION__);
			break;
		}

		if (ret == XASC_ERROR){
			log_print("%s: asc_io error, stop running", __FUNCTION__);
			break;
		}

		if (ret == XASC_TIMEOUT){
			log_print("%s: timeout", __FUNCTION__);
			// do timeout
			continue;
		}

		if (ret == XASC_OP_CLOSE){
			log_print("%s: close %d session", __FUNCTION__, psession->i);
			session_close(psession);
			continue;
		}

		if (ret == XASC_OP_ACCEPT){
			session_open(psession);
			log_print("%s: accept %d session", __FUNCTION__, psession->i);

			// 再取一个新的 session 交 asc_io 处理
			ret = sesn_q_.get(&psession);
			ret = iom_.add_fd(psession);
			continue;
		}

		// 到此，只剩下接收事件 (XASC_OP_RECV)
		// 此时，session 中已经存有接收到的数据，以及接收到的总长度
		unsigned char *precv_buf = (unsigned char *)psession->precv_buf;
		int  len = psession->recv_bytes;
		precv_buf[len] = 0;
		log_print_buf(__FUNCTION__, precv_buf, len);

		psession->last_trans_time = psession->last_recv_time = heartbeat_;

		// 取完整包长
		if ((ret = calc_pkg_len(precv_buf, len)) < 0){
			// < 0, 为无效或非法数据，关闭会话
			session_close(psession);
			continue;
		}

		if (ret <= len){
			// 接收了完整包，直接送入处理队列
			session_recv_to_queue(psession);
			log_print("%s: <%d/%d> - send to recv_queue(%d)", __FUNCTION__, psession->i, psession->sock, ret);
		}else
			log_print("不完整包(%d/%d)", len, ret);

		if (ret > psession->recv_size) {
			char* p = (char *)realloc(psession->precv_buf, ret+4);
			if (p) {
				psession->precv_buf = p;
				psession->recv_size = ret;
			}
		}

		// 发出期望包长
		iom_.post_recv(psession, ret-len);
	}

	if (precv_buf){
		free(precv_buf);  precv_buf = 0;
	}

	log_print("%s : Exit.", __FUNCTION__);
}

int xasc_server::calc_pkg_len(int i)
{
	xasc_session * s = sesns_ + i;
	return calc_pkg_len((const unsigned char *)s->precv_buf, s->recv_bytes);
}

void xasc_server::actor(void)
{
	log_print("%s : in/%d", __FUNCTION__, most_len_);

	char * ptemp_buf = (char *)calloc(1, max_len_+1);
	if (ptemp_buf == nullptr)
		return;

	int idle = 120*1000;
	while (is_run_){
		int isession = -1;

		/// 取出数据
		int len = recv_queue_.get((long *)(&isession), ptemp_buf, idle);

		if (!is_run_ || (isession == -1 && len == 4 && memcmp(ptemp_buf, "stop", 4) == 0))
			break;

		// timeout
		if (len <= 0){
			log_print("%s : len = %d, no data", __FUNCTION__, len);
			continue;
		}

		int r;
		if (!XASC_SESSION_VALID(isession)){
			if (isession == -1){
				if (strcmp(ptemp_buf, "stop") == 0)
					break;

				if (strncmp(ptemp_buf, "idle:", 5) == 0){
					// set session
					isession = atoi(ptemp_buf+5);
					r = do_idle(isession);
					if (r == XASC_ERROR)
						notify_close_session(isession);
				}
			}else{
				log_print("%s: <%d>, len = %d, 超出会话界限,忽略", __FUNCTION__, isession, len);
			}

			continue;
		}

		/// 调用处理函数进行消息处理
		r = do_msg(isession, ptemp_buf, len);
		log_print("%s : <%d/%d>处理%d长的数据，返回%d", __FUNCTION__, isession, sesns_[isession].sock, len, r);

		if (r == XASC_ERROR)
			notify_close_session(isession);
	}

	::free(ptemp_buf);

	log_print("%s : out", __FUNCTION__);
}

void xasc_server::sender(void)
{
	log_print("%s : in.\n", __FUNCTION__);

	char * ptemp_buf = (char *)calloc(1, send_len_+1);

	while (is_run_){
		int i_session = -1;

		int len = send_queue_.get((long *)&i_session, ptemp_buf, 120*1000);

		if (!is_run_ || (i_session == -1 && len == 4))
			break;

		if (len <= 0){
			continue;
		}

		if (!XASC_SESSION_VALID(i_session)){
			log_print("%s: <%d>, len = %d, 超出会话界限,忽略", __FUNCTION__, i_session, len);
			continue;
		}

		// find session
		int i = i_session;
		if (i < 0 || i > max_sessions_ || !XSESSION_ISOPEN(sesns_+i)){
			log_print("%s: 出错,试图在未打开的会话上发送(%d)", __FUNCTION__, i);
			log_print_buf(__FUNCTION__, (unsigned char *)ptemp_buf, len);
		}else{
			int sock = sesns_[i].sock;
			int sent_len = 0, r = 0;
			for(sent_len = 0; sent_len < len; ){
				r = ::send(sock, ptemp_buf, len, 0);
				if (r < 1)
					break;

				sent_len += r;
				sesns_[i].last_trans_time = int(time(0));
				sesns_[i].sent_len = sent_len;
			}

			if (r <= 0){
				log_print("%s: 发送错误(%d)(return = %d/%d)", __FUNCTION__, i, r, sent_len);
				notify_close_session(i);
			}else{
				log_print("%s: %dth sesn sent (ret = %d, len=%d)", __FUNCTION__, i, r, len);
				on_sent(i, len);
			}
		}
	}

	::free(ptemp_buf);

	log_print("%s : Exit.", __FUNCTION__);
}

void xasc_server::close_all_session(void)
{
	if (sesns_){
		for (int i = max_sessions_-1; i > 0 ; i--) {
			xasc_session* s = sesns_ + i;
			if (XSESSION_ISOPEN(s)) {
				session_close(i);
			}
		}
	}
}

void xasc_server::session_open(xasc_session *psession)
{
	// 将准备好的 session 入列
	sesn_q_.add(psession->i, psession);

	psession->idle_secs = idle_;

	psession->create_time =
		psession->last_recv_time  =
		psession->last_trans_time = heartbeat_;

	psession->app_id = -1;
}

int xasc_server::notify_close_session(int i)
{
	int r = close_requests_.put(i, (char *)&i, 0);

	log_print("notify_close_session: %d(r = %d)", i, r);

	if ((i % 4) == 0)
		xsleep_ms(50);

	return r;
}

void xasc_server::session_close(xasc_session * psession)
{
	if (psession == nullptr)
		return;

	int sock = psession->sock;

	if (psession->state == XASC_OP_NULL)
		return;

	int r = iom_.del_fd(psession);

	if (psession->precv_buf){
		::free(psession->precv_buf);
	}

	int i = psession->i;
	memset(psession, 0, sizeof(xasc_session));
	psession->i = i;
	psession->sock = XASC_INVALID_SOCKET;

	psession->state = XASC_OP_NULL;
	psession->app_id = -1;

	i = int(psession - sesns_);
	sesn_q_.del(i);
}

// 关闭所有活动的会话
void xasc_server::session_close_all(void)
{
	int i, i_sesn = sesn_q_.used_;
	auto idx = sesn_q_.indexs_ + i_sesn;
	int count = sesn_q_.count_;
	for (i = 0; i < count; i++) {
		auto s = sesn_q_.buf_ + i_sesn;
		int i_post = idx->i_post;

		session_close(s);

		i_sesn = i_post;
		idx = sesn_q_.indexs_ + i_sesn;
	}
}


bool xasc_server::session_isopen(int i)
{
	if (!XASC_SESSION_VALID(i))
		return false;

	xasc_session * psession = sesns_ + i;
	return XSESSION_ISOPEN(psession);
}

void xasc_server::session_close(int i)
{
	if (!XASC_SESSION_VALID(i)){
		log_print("%s: i = %d is out range[0~%d]", __FUNCTION__, i, max_sessions_);
		return;
	}

	xasc_session * psession = sesns_ + i;
	log_print("%s: i = %d(%d, %d) will be closed", __FUNCTION__, i, psession->recv_amount, heartbeat_, psession->create_time);
	if (!XSESSION_ISOPEN(psession)){
		log_print("%s: i = %d is not opened", __FUNCTION__, i);
	}else{
		psession->state = XASC_OP_NULL;
		on_closed(i);
	}

	session_close(psession);

	log_print("%s: i = %d is closed", __FUNCTION__, i);
}

// 将数据发布到request队列
int xasc_server::session_recv_to_queue(xasc_session * s)
{
	if (s == nullptr || !XASC_SESSION_VALID(s->i) || s->precv_buf == nullptr || s->recv_bytes < 1){
		log_print("%s(%d, %p, %d): 参数出错", __FUNCTION__, s->i, s->precv_buf, s->recv_bytes);
		return 0;
	}

	if (!XSESSION_ISOPEN(s)){
		log_print("会话%d已关闭，不发送", s->i);
		return 0;
	}

	int r = recv_queue_.put(s->i, s->precv_buf, s->recv_bytes);

	s->pkg_amunt++;	/// 接收数据包增加
	s->recv_bytes = 0;	/// 接收清零

	log_print("%s: %d - send %d -> do_msg", __FUNCTION__, s->i, r);

	return r;
}

int xasc_server::send(int isession, const char * s, int len)
{
	if ((isession != -1 || len != 4) &&
		(!XASC_SESSION_VALID(isession) || !XSESSION_ISOPEN(sesns_+isession))
	)
	{
		log_print("%s: 出错,试图在未打开的会话(%d)上发送(len=%d)\n", __FUNCTION__, isession, len);
		log_print_buf(__FUNCTION__, (unsigned char *)s, len);
		return -1;
	}
	return send_queue_.put(isession, s, len);
}

int xasc_server::send(int isession, const char * s)
{
	return send(isession, s, strlen(s));
}

bool xasc_server::on_opened(int i)
{
	return true;
}

bool xasc_server::on_closed(int i)
{
	return true;
}

void xasc_server::notify_do_cmd(int i, const char* cmd)
{
	if (!th_actor_.joinable())
		return;

	if (cmd == 0) {
		recv_queue_.put(i, "cmds", 4);
	}
	else
		recv_queue_.put(i, cmd, strlen(cmd));
}

///////////////////////////////////////////////////////////////////////////////////
/// 以下是LOG输出处理

static FILE * s_log_file = NULL;
static bool s_for_debug  = true;
static int  s_level = 0;
static int  s_timestamp_secs  = 60;
static int  s_time = 0;

char * time2string(char * d, long t)
{
	struct tm *ltime;
	time_t tt = (time_t)t;
    ltime = localtime(&tt);
	sprintf(d, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	return d;
}

char * get_now(char *d)
{
	long t = long(time(NULL));
	time2string(d, t);
	return d;
}

bool log_open(
	const char * file_name,
	bool for_debug,
	int level,
	int timestamp_secs
)
{
	s_for_debug = for_debug;

	if (file_name == nullptr){
		s_log_file = stderr;  // fdopen(2, "w");
	}else
		s_log_file = fopen(file_name, "wb+");
	
	s_level = level;
	s_timestamp_secs = timestamp_secs;

	return s_log_file != nullptr;
}

void log_close(void)
{
	if (s_log_file != nullptr && s_log_file != stderr){
		fclose(s_log_file);  s_log_file = nullptr;
	}
}

void log_put(char const * s)
{
	if (s_for_debug){
		fputs(s, stdout);
	}
	
	if (s_log_file != nullptr){
		fputs(s, s_log_file);
	}
}

void log_put(const char c)
{
	if (s_for_debug){
		fputc(c, stdout);
	}
	
	if (s_log_file != nullptr){
		fputc(c, s_log_file);
	}
}

void log_put_line(const char * s)
{
	log_put(s);
	log_put('\n');
}

void log_print(const char * format, ...)
{
	static time_t prev_time = 0;

	time_t t = time(NULL);
	if (int(t - prev_time) > s_timestamp_secs){
		char now[32];
		time2string(now, long(t));
	
		if (s_for_debug){
			log_put(now);  log_put('|');
		}
		prev_time = t;
	}

    va_list vs;

	va_start(vs, format);
	char s[1024];
	vsnprintf_s(s, sizeof(s)-1, format, vs);  s[sizeof(s)-1] = '\0';
    va_end(vs);

	log_put_line(s);
}

void log_print_buf(const char * title, const unsigned char * buf, int len)
{
	int i, j, n;
	const unsigned char * p = buf;
	char sshow[20];  sshow[16] = '\n';  sshow[17] = '\0';
	char s[64];

	log_put(title);
	log_put(":\n     0001 0203 0405 0607 0809 0A0B 0C0D 0E0F 0123456789ABCDEF\n");
	for (i = n = 0; n < len; i++){
		int l = sprintf(s, "%3d: ", i+1);
		for (j = 0; j < 16; j++){
			static const char * pformats[] = {"%02X", "%02X "};
			static const char * pblanks [] = {"  ", "   "};
			if (n < len){
				unsigned char c;
				c = *p++;  n++;
				sshow[j] = (c > 0x20 && c <= 126)? c : ' ';
				l += sprintf(s+l, pformats[(j&0x01)], c);
			}else{
				sshow[j] = ' ';
				strcpy(s+l, pblanks[(j&0x01)]);  l += 2 + (j & 0x01);
			}
		}
		log_put(s);
		log_put(sshow);
	}
}

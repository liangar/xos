#include <xasc_iom.h>

#ifndef SOMAXCONN
#define SOMAXCONN	512
#endif

int  net_evn_init(void);
void net_evn_done(void);

int xasc_iom::open(int listen_port, int max_sessions, int most_len, int max_len)
{
	port_ = listen_port;
	max_sessions_ = max_sessions;

	most_len_ = max(most_len, 64);
	max_len_  = max(max_len, 256);
	
	int r = -1;

	do{
		if ((iom_fd_ = iom_create_fd()) < 0)
			break;

		listen_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		
		if (iom_bind_listen() < 0)
			break;

		struct sockaddr_in sin;
		memzero(sin);
		sin.sin_family = AF_INET;
		sin.sin_port = htons((unsigned short)port_);
		sin.sin_addr.s_addr = htonl(INADDR_ANY);

		// bind socket to the port & start listening
		if (bind(listen_fd_, (const sockaddr *)&sin, sizeof(sin)) < 0 ||
			listen(listen_fd_, 1) < 0)
		{
			break;
		}
		
		r = 0;
	}while(false);
	
	if (r < 0)
		close();

	return r;
}

int xasc_iom::close(void)
{
	if (iom_fd_ > 0){
#ifdef _WIN32
		if (iom_fd_ > 0) {
			PostQueuedCompletionStatus(HANDLE(iom_fd_), 0, XASC_OP_EXIT, NULL);
			xsleep_ms(300);
		}

		CloseHandle(HANDLE(iom_fd_));
#else
		::close(iom_fd_);
#endif // _WIN32

		iom_fd_ = -1;
	}
	
	if (listen_fd_ > 0){
		close_socket(listen_fd_);  listen_fd_ = -1;
	}
	
	if (accept_s_ != nullptr){
		accept_s_ = nullptr;
	}
	
	return 0;
}

#ifndef _WIN32

int  net_evn_init(void)  {  return 0;  }
void net_evn_done(void)  (  return   }

//////////////////////////////////////////////////////////////////////////////////////////
/// xasc_iom - epoll
/// https://www.cnblogs.com/xuewangkai/p/11158576.html
int xasc_iom::iom_create_fd(void)
{
	// 每次只等1个消息
	return epoll_create(1);
}

int xasc_iom::iom_bind_listen(void)
{
	if (iom_fd_ == 0){
		r = -1;  break;
	}

	struct epoll_event ev;

	ev.events = EPOLLIN;
	ev.data.ptr = &listen_fd_;

	return epoll_ctl(iom_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
}

int xasc_iom::add_fd(xasc_session * s)
{
	accept_s_ = s;
	s->state = XASC_OP_ACCEPT;

	return 0;
}

int xasc_iom::del_fd(xasc_session * s)
{
	struct epoll_event  ev;
    ev.events = 0;
    ev.data.ptr = NULL;
	
	int r = epoll_ctl(iom_fd_, EPOLL_CTL_DEL, s->sock, &ev);

	s->state = XASC_OP_NULL;

	return r;
}

int xasc_iom::iom_do_accept(void)
{
	int len = sizeof(sockaddr);
	int sock = accept(listen_fd_, &accept_s_->peer_addr, &len);
	if (sock < 0)
		return -1;

	accept_s_->create_time =
	accept_s_->last_trans_time =
	accept_s_->last_recv_time = int(time(nullptr));

	accept_s_->sock = sock;

	getsockname(sock, &accept_s_->sock_addr, &len);

	return post_recv(accept_s_);
}

int xasc_iom::post_recv(xasc_session * s)
{
	struct epoll_event ev;

	ev.events = EPOLLIN;
	ev.data.ptr = s;

	// 在 accept 操作时，是第一次，用 ADD，之后用 MOD
	int r = epoll_ctl(
		iom_fd_,
		(s->state == XASC_OP_ACCEPT)? EPOLL_CTL_ADD : EPOLL_CTL_MOD,
		s->sock,
		&ev
	);

	if (r == 0);
		s_->state = XASC_OP_RECV;
	}

	return 	r;
}

int xasc_iom::post_recv(xasc_session * s, int len)
{
	return post_recv(s);
}

int xasc_iom::process_event(xasc_session **ps, int *bytes, int timeout_ms)
{
	struct epoll_event ev;

	if (timeout_ms < 0)
		timeout_ms = XASC_TIMER_INFINITE;

	int r = epoll_wait(iom_fd_, &ev, 1, timeout_ms);
	
	// 出错
	if (r < 0)
		return r;
	
	// 无事件，超时
	if (r == 0){
		if (timeout_ms == XASC_TIMER_INFINITE)
			return XASC_ERROR;
		return XASC_TIMEOUT;
	}
	
	// 如果事件是 listen_fd_ 则是心连接到达
	if (ev.data.ptr == &listen_fd_){
		r = iom_do_accept();
		*ps = accept_s_;

		return XASC_OP_ACCEPT;
	}
	
	// 到此，只有接收到数据的事件
	*ps = (xasc_session *)ev.data.ptr;
	*bytes = r;
	
	// 接收数据
	r = ::recv(
		(*ps)->sock,
		(*ps)->precv_buf + (*ps)->recv_bytes,
		size_t((*ps)->recv_size - (*ps)->recv_bytes),
		0
	);

	// 对方已经关闭连接
	if (r == 0)
		return XASC_OP_CLOSE;
	
	// 进行实际的接收处理
	(*ps)->recv_bytes += *bytes;
	(*ps)->recv_amount += *bytes;

	return XASC_OP_RECV;
}

#else
//////////////////////////////////////////////////////////////////////////////////////////
// iocp
// accept socket bind to i/o completion port & recv the 1th data packege:
// https://docs.microsoft.com/en-us/windows/win32/api/mswsock/nf-mswsock-acceptex
// 
// https://www.codeproject.com/articles/13382/a-simple-application-using-i-o-completion-ports-an
// https://www.xuebuyuan.com/2202681.html
//
// only accept connection, donnot recv data
// https://stackoverflow.com/questions/19956186/iocp-acceptex-not-creating-completion-upon-connect
//
// another implement is wepoll:
//
#pragma comment(lib, "ws2_32.lib")

#ifndef WSAID_ACCEPTEX

typedef BOOL (PASCAL FAR * LPFN_ACCEPTEX)(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    );

#define WSAID_ACCEPTEX                                                       \
    {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}

#endif

#ifndef WSAID_DISCONNECTEX

typedef BOOL (PASCAL FAR * LPFN_DISCONNECTEX) (
    IN SOCKET s,
    IN LPOVERLAPPED lpOverlapped,
    IN DWORD  dwFlags,
    IN DWORD  dwReserved
    );

#define WSAID_DISCONNECTEX                                                   \
    {0x7fda2e11,0x8630,0x436f,{0xa0,0x31,0xf5,0x36,0xa6,0xee,0xc1,0x57}}

#endif

static u_int               osviex;
static OSVERSIONINFOEX     osvi;

static char   host_name[64];
static int    wsa_isopen = 0;  // ???ü???

static GUID ax_guid = WSAID_ACCEPTEX;
static GUID as_guid = WSAID_GETACCEPTEXSOCKADDRS;
static GUID dx_guid = WSAID_DISCONNECTEX;

static LPFN_ACCEPTEX 		pf_acceptex	    = nullptr;	// AcceptEx ????????
static LPFN_DISCONNECTEX    pf_disconnectex = nullptr;	// 
static LPFN_GETACCEPTEXSOCKADDRS  pf_getacceptexsockaddrs = nullptr;

int os_init(void)
{
	memzero(osvi);
    osvi.dwOSVersionInfoSize = sizeof(osvi);

	return 0;
}

static WSAOVERLAPPED	olOverlap;
static char 			lpOutputBuf[1024];
static int				outBufLen = 1024;
static DWORD 			dwBytes;

// ?????WSA???? / ????
// ????γ??????????
int net_evn_init(void)
{
	if (wsa_isopen < 0)
		wsa_isopen = 0;

	if (wsa_isopen > 0)
		return ++wsa_isopen;

	int len  = sizeof(host_name);
	GetComputerName(host_name, LPDWORD(&len));

	WSADATA  was_data;
	if (WSAStartup(MAKEWORD(2,2), &was_data) != 0)
		return -1;

	int r = 1;
    DWORD  bytes;
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	// get the function pointer of AcceptEx function
    if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &ax_guid, sizeof(GUID),
				 &pf_acceptex, sizeof(LPFN_ACCEPTEX), &bytes, NULL, NULL)
		== -1)
	{
		r = -2;
	}

	// get the function address of  DisconnectionEx function
    if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &dx_guid, sizeof(GUID),
                 &pf_disconnectex, sizeof(LPFN_DISCONNECTEX), &bytes,
                 NULL, NULL)
        == -1)
    {
		r = -3;
    }

	// get the function address of GetAcceptEx function
    if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &as_guid, sizeof(GUID),
                 &pf_getacceptexsockaddrs, sizeof(LPFN_GETACCEPTEXSOCKADDRS),
                 &bytes, NULL, NULL)
        == -1)
	{
		r = -4;
	}

	return (r < 0)? r : wsa_isopen;
}

void net_env_done(void)
{
	if (wsa_isopen <= 0)
		return;

    if (wsa_isopen == 1){
		pf_acceptex = nullptr;
		pf_disconnectex = nullptr;

        WSACleanup();
	}

    wsa_isopen --;
}

int close_socket(int sock)
{
	/* 如果需要用此函数，则 ovlp 必须在session中另外分配
	if (pf_disconnectex){
		OVERLAPPED ovlp;
		memzero(ovlp);
		bool isok = pf_disconnectex(SOCKET(sock), &ovlp, TF_REUSE_SOCKET, 0);
		return isok? 0 : -1;
	}
	//*/
	return closesocket(SOCKET(sock));
}

int xasc_iom::iom_create_fd(void)
{
	HANDLE fd = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if (fd == NULL)
		return -1;

	return int(fd);
}

int xasc_iom::iom_bind_listen(void)
{
	HANDLE h = CreateIoCompletionPort((HANDLE)listen_fd_, HANDLE(iom_fd_), (u_long) 0, 0);
	if (h == NULL)
		return -1;

	return 0;
}

int xasc_iom::add_fd(xasc_session *s)
{
	// 加入初始会话
	if (s->state != XASC_OP_NULL)
		return 0;

	if (s->sock <= 0)
		s->sock = int(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

	if (s->sock < 1)
		return -1;

	memzero(s->ovlp);

	int data_len = 0;
	int addr_len = sizeof (sockaddr_in) + 16;
	if (s->precv_buf == nullptr){
		s->precv_buf = (char *)calloc(most_len_+4, 1);
		if (s->precv_buf == nullptr)
			return -2;

		s->recv_size= most_len_;
	}
	s->recv_bytes = 0;

	DWORD dw_bytes;
	BOOL bRetVal = pf_acceptex(
		SOCKET(listen_fd_), SOCKET(s->sock),
		s->precv_buf,
		0,
		sizeof (sockaddr_in) + 16, sizeof (sockaddr_in) + 16, 
		&dw_bytes,
		&s->ovlp
	);

	if (bRetVal == FALSE) {
		DWORD code = WSAGetLastError();
		if (code != WSA_IO_PENDING)
			return -1;
	}

	HANDLE h2 = CreateIoCompletionPort((HANDLE) s->sock, HANDLE(iom_fd_), (u_long) s->i, 0);
	
	if (h2 == NULL)
		return -2;
	
	s->state = XASC_OP_ACCEPT;

	accept_s_ = s;

	return 0;
}

int xasc_iom::del_fd(xasc_session *s)
{
	if (s->sock > 0){
		CancelIo(HANDLE(s->sock));
		close_socket(s->sock);  s->sock = -1;
	}
	
	s->state = XASC_OP_NULL;

	return 0;
}

// 处理接收连接
int xasc_iom::iom_do_accept(void)
{
	// 取得 sock_addr, peer_addr
	// 

	/* the SO_UPDATE_ACCEPT_CONTEXT option is set on the accepted socket,
	the local address associated with the accepted socket can also be retrieved using the
	*getsockname* function. Likewise, the remote address associated with the accepted socket
	can be retrieved using the *getpeername* function.
	*/
	int r = setsockopt(
		accept_s_->sock,
		SOL_SOCKET,
		SO_UPDATE_ACCEPT_CONTEXT,
		(char *) &listen_fd_,
		sizeof(int)
	);

	if (r < 0)
		return XASC_ERROR;

	sockaddr * local, * peer;
	int local_len, peer_len;
	local = peer = nullptr;
	local_len = peer_len = 0;
	pf_getacceptexsockaddrs(
		accept_s_->precv_buf,
		0,
		sizeof (sockaddr_in) + 16, sizeof (sockaddr_in) + 16, 
		(SOCKADDR **)&local, &local_len,
		(SOCKADDR **)&peer , &peer_len
	);
	
	memzero(accept_s_->sock_addr);  memzero(accept_s_->peer_addr);
	if (local && local_len > 0)
		memcpy(&accept_s_->sock_addr, local, local_len);	// sizeof(sockaddr)
	if (peer && peer_len > 0)
		memcpy(&accept_s_->peer_addr, peer, peer_len);		// sizeof(sockaddr)

	// set time
	accept_s_->create_time =
	accept_s_->last_trans_time =
	accept_s_->last_recv_time = int(time(nullptr));

	return post_recv(accept_s_);
}

int xasc_iom::post_recv(xasc_session *s)
{
	if (s->state == XASC_OP_ACCEPT){
		s->state = XASC_OP_RECV;
		s->recv_bytes = 0;
	}

	int len = s->recv_size - s->recv_bytes;
	if (len == 0)
		len = most_len_;

	return post_recv(s, len);
}

// 发出异步 recv 操作
// 
int xasc_iom::post_recv(xasc_session *s, int len)
{
	// 检查状态和参数
	if (s->state != XASC_OP_RECV)
		return 0;

	if (len < 1)  len = 0;

	if (s->recv_bytes + len > s->recv_size){
		// 重新分配缓冲
		int buf_len = s->recv_size + len;
		char * p = (char *)realloc(s->precv_buf, buf_len + 4);	// 4 bytes 为越界保护
		if (p == nullptr)
			return -1;

		memset(p+s->recv_bytes, 0, buf_len - s->recv_bytes);		
		s->precv_buf = p;
		s->recv_size= buf_len;
	}
	
	WSABUF wsa_buf;
	wsa_buf.buf = s->precv_buf + s->recv_bytes;
	wsa_buf.len = s->recv_size - s->recv_bytes;
	memzero(s->ovlp);
	DWORD flags = 0;
	DWORD bytes = 0;
	
	int r = WSARecv(s->sock, &wsa_buf, 1, &bytes, &flags, &s->ovlp, NULL);
	DWORD code = WSAGetLastError();
	if (r == SOCKET_ERROR && code != WSA_IO_PENDING ){
		fprintf(stderr, "post_recv : WSARecv -> fail, %d.\n", code);
		return -2;
	}
	
	s->state = XASC_OP_RECV;
	
	return 0;
}

/***
 * 网络事件处理
 */
int xasc_iom::process_event(xasc_session **ps, int *bytes, int timeout_ms)
{
	// 
	DWORD key = 0, ms;
	LPOVERLAPPED ovlp = nullptr;
	*bytes = 0;

	ms = (timeout_ms > 0)? DWORD(timeout_ms) : INFINITE;
	bool ok = GetQueuedCompletionStatus(HANDLE(iom_fd_), LPDWORD(bytes), &key, &ovlp, ms);

	*ps = nullptr;

	if (!ok){
		//  timeout / error
		DWORD err = GetLastError();
		if (err == WAIT_TIMEOUT)
			return XASC_TIMEOUT;
		return XASC_ERROR;
	}

	// 空 ovlp 是针对 iocp 端口的消息, 或者是直接的 post 的消息
	// error / notify exit
	if (ovlp == nullptr){
		if (key == XASC_OP_EXIT)
			return XASC_OP_EXIT;

		return XASC_ERROR;
	}

	// ovlp 不空，则是网络操作返回，这里只有：
	// accept / recv 
	// 用 ovlp 地址，取得 session 地址
	*ps = container_of(ovlp, xasc_session, ovlp);

	// ACCEPT 操作，实测 bytes 返回0，进行 accept 处理
	if ((*ps)->state == XASC_OP_ACCEPT){
		if (iom_do_accept() < 0)
			return XASC_ERROR;
		
		return XASC_OP_ACCEPT;
	}

	// recv 返回 0 则对方 socket 关闭
	if (*bytes == 0)
		return XASC_OP_CLOSE;

	// 进行实际的接收处理
	(*ps)->recv_bytes += *bytes;
	(*ps)->recv_amount += *bytes;

	return XASC_OP_RECV;
}

#endif // _WIN32


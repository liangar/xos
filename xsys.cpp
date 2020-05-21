#include <xsys.h>
#include <l_str.h>
#include <ss_service.h>

bool bServerDebug;
static bool bsys_inited = false;

int xsys_init(bool bDebug)
{
	if (bsys_inited)
		return 0;

	bServerDebug = bDebug;
	int r = SysInitLibrary();
	if (r == 0)
		bsys_inited = true;
	return r;
}

void xsys_down(void)
{
	if (bsys_inited){
		xsys_shutdown();
		xsys_sleep_ms(100);
		xsys_cleanup();
		
		bsys_inited = false;
	}
}

xsys_event::xsys_event()
{
	m_event = SYS_INVALID_EVENT;
}


int xsys_event::init(int iManualReset)
{
	if (m_event != SYS_INVALID_EVENT){
		down();
	}

	m_event = SysCreateEvent(iManualReset);
	if (m_event == SYS_INVALID_EVENT) {
		return -1;
	}
	return 0;
}

void xsys_event::down(void)
{
	if (m_event != SYS_INVALID_EVENT){
		int r = SysCloseEvent(m_event);
		m_event = SYS_INVALID_EVENT;
	}
}

int xsys_event::wait_ms(int msec)
{
	if (m_event == SYS_INVALID_EVENT)
		return 0;

	if (msec < 0){
		msec = SYS_INFINITE_TIMEOUT;
	}
	return SysWaitEvent(m_event, msec);
}

int xsys_event::wait(int seconds)
{
	return wait_ms(seconds*1000);
}

int xsys_event::set(void)
{
	if (m_event == SYS_INVALID_EVENT)
		return 0;

	return SysSetEvent(m_event);
}

int xsys_event::reset(void)
{
	if (m_event == SYS_INVALID_EVENT)
		return 0;

	return SysResetEvent(m_event);
}

int xsys_event::trywait(void)
{
	if (m_event == SYS_INVALID_EVENT)
		return 0;

	return SysTryWaitEvent(m_event);
}

xsys_semaphore::xsys_semaphore()
{
	m_semaphore = SYS_INVALID_SEMAPHORE;
	m_psem_bind = 0;
}

int xsys_semaphore::init(int iInitCount, int iMaxCount)
{
	if (m_semaphore != SYS_INVALID_SEMAPHORE){
		down();
	}
	m_psem_bind = 0;
	m_semaphore = SysCreateSemaphore(iInitCount, iMaxCount);
	if (m_semaphore == SYS_INVALID_SEMAPHORE)
		return -1;
	return 0;
}

void xsys_semaphore::down(void)
{
	if (m_semaphore != SYS_INVALID_SEMAPHORE){
		int r = SysCloseSemaphore(m_semaphore);
		m_semaphore = SYS_INVALID_SEMAPHORE;
		m_psem_bind = 0;
	}
}

void xsys_semaphore::bindto(xsys_semaphore * psem)
{
	m_psem_bind = psem;
}

int xsys_semaphore::P(int ms)
{
	if (ms < 0){
		ms = SYS_INFINITE_TIMEOUT;
	}
	int r = SysWaitSemaphore(m_semaphore, ms);
	if (r == 0 && m_psem_bind){
		m_psem_bind->P(500);
	}

	return r;
}

int xsys_semaphore::V(int iCount)
{
	int r = SysReleaseSemaphore(m_semaphore, iCount);
	if (r == 0 && m_psem_bind){
		m_psem_bind->V(iCount);
	}

	return r;
}

int xsys_semaphore::trywait(int ms)
{
	return SysTryWaitSemaphore(m_semaphore);
}

xsys_mutex::xsys_mutex()
{
	m_mutex = SYS_INVALID_MUTEX;
}

int xsys_mutex::init(void)
{
	if (m_mutex != SYS_INVALID_MUTEX){
		down();
	}
	m_mutex = SysCreateMutex();
	if (m_mutex == SYS_INVALID_MUTEX) {
		return -1;
	}
	return 0;
}

void xsys_mutex::down(void)
{
	if (m_mutex != SYS_INVALID_MUTEX){
		SysCloseMutex(m_mutex);
		m_mutex = SYS_INVALID_MUTEX;
	}
}

int xsys_mutex::lock(int ms)
{
	if (ms < 0){
		ms = SYS_INFINITE_TIMEOUT;
	}
	return SysLockMutex(m_mutex, ms);
}

int xsys_mutex::unlock(void)
{
	return SysUnlockMutex(m_mutex);
}

int xsys_mutex::trylock(void)
{
	return SysTryLockMutex(m_mutex);
}

xsys_thread::xsys_thread()
{
	m_thread = SYS_INVALID_THREAD;
}

int xsys_thread::init(xsys_thread_function f, void *pThreadData)
{
	if (m_thread != SYS_INVALID_THREAD){
		down(10, 1);
	}

	m_thread = SysCreateThread(f, pThreadData);
	if (m_thread == SYS_INVALID_THREAD) {
		return -1;
	}
	return 0;
}

int xsys_thread::down(int seconds, int iForce)
{
	if (m_thread != SYS_INVALID_THREAD) {
        int r = wait(seconds);
		if (r){ // 没有结束
			if (iForce){
				SysCloseThread(m_thread, iForce);
				m_thread = SYS_INVALID_THREAD;
				return 1;
			}else
				return -1;
		}
		SysCloseThread(m_thread, 0);
		m_thread = SYS_INVALID_THREAD;
	}
	return 0;
}

int xsys_thread::wait(int seconds)
{
	if (seconds < 0){
		seconds = SYS_INFINITE_TIMEOUT;
	}
	if (m_thread != SYS_INVALID_THREAD) {
		return SysWaitThread(m_thread, seconds * 1000);
	}
	return 0;
}

void terminatethread(xsys_thread &hthread, int seconds, int returncode)
{
	hthread.wait(seconds);
	hthread.down();
}

xsys_module::xsys_module()
{
	m_h = SYS_INVALID_HANDLE;
}

int xsys_module::load(const char * pszFilePath)
{
	if (m_h != SYS_INVALID_HANDLE){
		down();
	}

	m_h = SysOpenModule(pszFilePath);
	if (m_h == SYS_INVALID_HANDLE) {
		return -1;
	}
	return 0;
}

void xsys_module::down(void)
{
	if (m_h != SYS_INVALID_HANDLE) {
		SysCloseModule(m_h);
		m_h = SYS_INVALID_HANDLE;
	}
}

void * xsys_module::get_symbol(const char * pszSymbol)
{
	if (m_h == SYS_INVALID_HANDLE) {
		return 0;
	}
	return SysGetSymbol(m_h, pszSymbol);
}

xsys_ls::xsys_ls()
{
	m_h = SYS_INVALID_HANDLE;
}

xsys_ls::~xsys_ls()
{
	close();
}

bool xsys_ls::open(const char *pszPath, char *pszFileName, int len)
{
	close();

	m_h = SysFirstFile(pszPath, pszFileName, len);
	if (m_h == SYS_INVALID_HANDLE) {
		return false;
	}
	return true;
}

void xsys_ls::close(void)
{
	if (m_h != SYS_INVALID_HANDLE) {
		SysFindClose(m_h);
		m_h = SYS_INVALID_HANDLE;
	}
}

int xsys_ls::ispath(void)
{
	if (m_h != SYS_INVALID_HANDLE) {
		return SysIsDirectory(m_h);
	}
	return 0;
}

unsigned long xsys_ls::get_size(void)
{
	if (m_h != SYS_INVALID_HANDLE) {
		return long(SysGetSize(m_h));
	}
	return 0;
}

bool xsys_ls::next(char * filename, int len)
{
	if (m_h == SYS_INVALID_HANDLE) {
		return false;
	}
	return (SysNextFile(m_h, filename, len) == 1);
}


static int MscGetServerAddress(char const *purl, SYS_INET_ADDR & SvrAddr, int iport)
{

	char url[MAX_HOST_NAME] = "";
	strcpy(url, purl);

	char * p;	int port;

	port = iport;
	if ((p = strchr(url, ':')) != NULL){
		*p++ = '\0';
		port = atoi(p);
		if (url[0] == '!') {
			strcpy(url, "127.0.0.1");
		}
	}

	int r;
	SysInetAnySetup(SvrAddr, AF_INET, iport);

	if ((r = SysGetHostByName((const char *)url, AF_INET, SvrAddr)) < 0)
		return r;

	SysSetAddrPort(SvrAddr, port);

	return (0);
}

xsys_socket::xsys_socket()
{
	m_sock = SYS_INVALID_SOCKET;
	m_isserver = 0;
}
xsys_socket::~xsys_socket()
{
	close();
}

int xsys_socket::listen(int nportnumber, int iconnections)
{
	SYS_SOCKET sock = SysCreateSocket(AF_INET, SOCK_STREAM, 0);

	if (sock == SYS_INVALID_SOCKET)
		return (ErrGetErrorCode());

	SYS_INET_ADDR InSvrAddr;

	SysInetAnySetup(InSvrAddr, AF_INET, nportnumber);

	if (SysBindSocket(sock, &InSvrAddr) < 0)
	{
		ErrorPush();
		SysCloseSocket(sock);
		return (ErrorPop());
	}

	SysListenSocket(sock, iconnections);

	m_sock = sock;
	m_isserver = 1;

	return 0;
}

int xsys_socket::udp_listen(int nportnumber)
{
	SYS_SOCKET sock = SysCreateSocket(AF_INET, SOCK_DGRAM, 0);

	if (sock == SYS_INVALID_SOCKET)
		return (ErrGetErrorCode());

	SYS_INET_ADDR InSvrAddr;

	SysInetAnySetup(InSvrAddr, AF_INET, nportnumber);

	if (SysBindSocket(sock, &InSvrAddr) < 0)
	{
		ErrorPush();
		SysCloseSocket(sock);
		return (ErrorPop());
	}

	m_sock = sock;
	m_isserver = 1;

	return 0;
}

int xsys_socket::accept(xsys_socket & client_sock, unsigned int timeout_ms)
{
	SYS_SOCKET s;

	int r = accept(s, timeout_ms);
	client_sock.m_sock = s;

	return r;
}

int xsys_socket::accept(SYS_SOCKET & sock, unsigned int timeout_ms)
{
	sock = SYS_INVALID_SOCKET;

	if (m_sock == SYS_INVALID_SOCKET || !m_isserver) {
		return -1;
	}

	SYS_INET_ADDR ConnAddr;
	int iConnAddrLength = sizeof(ConnAddr);

	ZeroData(ConnAddr);

	if (timeout_ms == 0)
		timeout_ms = SYS_INFINITE_TIMEOUT;

	sock = SysAccept(m_sock, &ConnAddr, timeout_ms);
	if (sock == SYS_INVALID_SOCKET) {
		return (ErrGetErrorCode());
	}

	return 0;
}

int xsys_socket::connect(const char * lphostname, int nportnumber, int timeout, bool is_tcp)
{
	SYS_INET_ADDR SvrAddr;

	if (MscGetServerAddress(lphostname, SvrAddr, nportnumber) < 0)
		return (ErrGetErrorCode());

	SYS_SOCKET sock = SysCreateSocket(AF_INET, (is_tcp? SOCK_STREAM : SOCK_DGRAM), 0);

	if (sock == SYS_INVALID_SOCKET)
		return (ErrGetErrorCode());

	EL_WriteHexString((char *)(&SvrAddr.Addr), SvrAddr.iSize);

	if (timeout < 0){
		timeout = SYS_INFINITE_TIMEOUT;
	}else
		timeout *= 1000;

	if (SysConnect(sock, &SvrAddr, timeout) < 0) {
		ErrorPush();
		SysCloseSocket(sock);
		return (ErrorPop());
	}

	m_isserver = 0;

	m_sock = sock;

	return 0;
}

void xsys_socket::close(void)
{
	if (isopen()) {
		// printf("xsys_socket::close %d\n", int(m_sock));
		SysCloseSocket(m_sock);
		m_sock = SYS_INVALID_SOCKET;
	}	// else
        // printf("xsys_socket::close null socket\n");
	m_isserver = 0;
}

int xsys_socket::shutdown(void)
{
	int r = 0;
	if (isopen()) {
		// printf("xsys_socket::close %d\n", int(m_sock));
		r = SysShutdownSocket(m_sock, SD_BOTH);
		m_sock = SYS_INVALID_SOCKET;
	}	// else
        // printf("xsys_socket::close null socket\n");
	m_isserver = 0;
	return r;
}

int xsys_socket::isopen(void)
{
	if (m_sock != SYS_INVALID_SOCKET) {
		return 1;
	}
	return 0;
}

const char * xsys_socket::get_peer_ip(char * ip)
{
	if (m_sock == SYS_INVALID_SOCKET)
		return 0;

	SYS_INET_ADDR addr;
	if (SysGetPeerInfo(m_sock, addr) < 0)
		return 0;

	return (const char *)SysInetRevNToA(addr, ip, 32);
}

const char * xsys_socket::get_self_ip(char * ip)
{
	if (m_sock == SYS_INVALID_SOCKET)
		return 0;

	SYS_INET_ADDR addr;
	if (SysGetSockInfo(m_sock, addr) < 0)
		return 0;

	return (const char *)SysInetRevNToA(addr, ip, 32);
}

int xsys_socket::recv(char * buf, int l, int timeout_ms)
{
	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}

	if (l < 1)
		return 0;

	if (timeout_ms < 0){
		timeout_ms = SYS_INFINITE_TIMEOUT;
	}

	return SysRecvData(m_sock, buf, l, timeout_ms);
}


int xsys_socket::recv_from(char * buf, int len, char * peer_ip, int timeout)
{
	SYS_INET_ADDR peer_addr;

	int r = recv_from(buf, len, &peer_addr, timeout);

	if (r > 0)
		SysInetRevNToA(peer_addr, peer_ip, MAX_IP_LEN);

	return r;
}

int xsys_socket::recv_from(char * buf, int len, SYS_INET_ADDR * pfrom_addr, int timeout)
{
	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}
	if (timeout < 0){
		timeout = SYS_INFINITE_TIMEOUT;
	}
	if (len == 0)
		return 0;

	return SysRecvDataFrom(m_sock, pfrom_addr, buf, len, timeout);
}

int xsys_socket::send(const char * buf, int l, int timeout_ms)
{
	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}
	if (timeout_ms < 0){
		timeout_ms = SYS_INFINITE_TIMEOUT;
	}

	return SysSendData(m_sock, buf, l, timeout_ms);
}

int xsys_socket::sendto(const char * buf, int len, const SYS_INET_ADDR *pto_addr, int timeout_ms)
{
	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}
	if (timeout_ms < 0)
		timeout_ms = SYS_INFINITE_TIMEOUT;

	int sent_bytes = 0;
	while (sent_bytes < len){
		int current_snd = ::sendto(m_sock, buf, len-sent_bytes, 0, (const struct sockaddr *)pto_addr->Addr, sizeof(sockaddr));
		if (current_snd < 0)
			break;
		sent_bytes += current_snd;
	}

	return sent_bytes;
}

int xsys_socket::recv_byend(char * buf, int maxlen, char * endstr, int timeout)
{
	int i = 0, r;
	memset(buf, 0, maxlen);

	if (timeout < 0){
		timeout = SYS_INFINITE_TIMEOUT;
	}
	int t0 = int(time(0)), idle = timeout;

	--maxlen;	// 保留一个字节作为结束符
	while (i < maxlen){
		if (timeout != SYS_INFINITE_TIMEOUT){
			idle = timeout - (int(time(0)) - t0);
			if (idle < 0)
				return -1;
		}

		if ((r = recv(buf + i, maxlen-i, idle)) < 0){
			return r;
		}
		if (r > 0){
			i += r;
			if (endcmp(buf, endstr) == 0){
				i -= (strlen(endstr) - 1);
				buf[i] = 0;
				return i;
			}
		}
	}
	return maxlen;
}

int xsys_socket::recv_byend(char ** p, const char * endstr, int steplen, int timeout)
{
	if (steplen < 128)
		steplen = 128;

	int i = 0, r, len = 2 * steplen;

	*p = 0;
	char * q = (char *)calloc(len, 1);

	if (timeout < 0){
		timeout = SYS_INFINITE_TIMEOUT;
	}
	int t0 = int(time(0)), idle = timeout;
	while (i < len-1){
		if (timeout != SYS_INFINITE_TIMEOUT){
			idle = timeout - (int(time(0)) - t0);
			if (idle < 0)
				return -1;
		}

		if ((r = recv(q + i, len-i-1, idle)) < 0){
			free(q);
			return r;
		}
		if (r > 0){
			i += r;
			if (endcmp(q, endstr) == 0){
				i -= (strlen(endstr) - 1);
				q[i] = 0;
				*p = q;
				return i;
			}
			if (len - i < steplen) {
				char * newq = (char *)realloc(q, len + 2*steplen);

				if (newq) {
					q = newq;
					memset(q+len, 0, 2*steplen);
					len += 2*steplen;
				}
			}
		}
	}
	*p = q;
	return len;
}

int xsys_socket::recv_all(char * buf, int l, int timeout_ms)
{
	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}

	if (l < 1)
		return 0;

	if (timeout_ms < 0){
		timeout_ms = SYS_INFINITE_TIMEOUT;
	}

	return SysRecv(m_sock, buf, l, timeout_ms);
}

int xsys_socket::send_all(const char * buf, int l, int timeout_ms)
{
	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}

	if (timeout_ms < 0){
		timeout_ms = SYS_INFINITE_TIMEOUT;
	}

	return SysSend(m_sock, buf, l , timeout_ms);
}

int xsys_socket::send_all(const char * buf)
{
	return send_all(buf, strlen(buf), 200);
}

int xsys_socket::recvblob(char * buf, int * prest_len, int max_len, int timeout)
{
	if (timeout < 0){
		timeout = SYS_INFINITE_TIMEOUT;
	}

	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}

	char n[11];

	if (recv_all(n, 10, timeout) == 10){
		n[10] = '\0';  int l = atoi(n);
		int  len = min(l, max_len-1);
		if (recv_all(buf, len, timeout) == len){
			buf[len] = '\0';
			if (prest_len)
				*prest_len = l - len;

			return len;
		}
	}
	return -1;
}

int xsys_socket::recvblob(char **buf, int * prest_len, int max_len, int timeout)
{
	if (timeout < 0){
		timeout = SYS_INFINITE_TIMEOUT;
	}

	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}

	char n[11];
	n[10] = '\0';
	if (recv_all(n, 10, timeout) != 10)  return -1;

	int l = atoi(n);
	if (l < 0)  return -2;

	int len = min(l, max_len-1);
	if ((*buf = new char[len+1]) == NULL)  return -3;

	if (recv_all(*buf, len, timeout) == l){
		(*buf)[len] = '\0';
		if (prest_len)
			*prest_len = l - len;
		return l;
	}

	return -4;
}

int xsys_socket::sendblob(const char * buf, int len, int timeout)
{
	if (timeout < 0){
		timeout = SYS_INFINITE_TIMEOUT;
	}

	if (m_sock == SYS_INVALID_SOCKET) {
		return -1;
	}

	if (len == 0){
		len = strlen(buf);
	}

	char str[11];
	sprintf(str, "%010d", len);
	if ((send_all(str, 10, timeout) == 10) &&
		(send_all(buf, len, timeout) == len))
		return len;
	return -1;
}

int xsys_cp(const char * new_name, const char * old_name)
{
	int r = 0, len = 0;
	char cmd[1024], path[256], errbuf[256];

	memset(cmd, 0x0, sizeof(cmd));
	memset(path, 0x0, sizeof(path));
	memset(errbuf, 0x0, sizeof(errbuf));

	if (new_name == NULL || new_name[0] == '\0' ||
		old_name == NULL || old_name[0] == '\0'){
		return -1;
	}

	if (SysExistFile(old_name) == 0){
		return -2;
	}

	if (!xsys_md(new_name, true)){
		return -3;
	}

#ifdef WIN32
	if (!::CopyFile(old_name, new_name, false)){
		r = -4;
	}
#else
	sprintf(cmd, "cp \"%s\" \"%s\"", old_name, new_name);
	r = SysDoCmd(cmd, path, errbuf, len);
	if (r > 0){
		sprintf(cmd, "chmod 755 \"%s\"", new_name);
		r = SysDoCmd(cmd, path, errbuf, len);
	}
#endif

	return r;
}

int xsys_cp(const char * newpath, const char * oldpath, const char * filename)
{
	char newFileName[MAX_PATH], oldFileName[MAX_PATH];

	memset(newFileName, 0x0, sizeof(newFileName));
	memset(oldFileName, 0x0, sizeof(oldFileName));

	if (newpath == NULL || newpath[0] == '\0'){
		strcpy(newFileName, filename);
	} else {
		int l = strlen(newpath);
		if (newpath[l-1] == '\\' || newpath[l-1] == '/'){
			sprintf(newFileName, "%s%s", newpath, filename);
		} else {
			sprintf(newFileName, "%s/%s", newpath, filename);
		}
	}

	if (oldpath == NULL || oldpath[0] == '\0'){
		strcpy(oldFileName, filename);
	} else {
		int l = strlen(oldpath);
		if (oldpath[l-1] == '\\' || oldpath[l-1] == '/'){
			sprintf(oldFileName, "%s%s", oldpath, filename);
		} else {
			sprintf(oldFileName, "%s/%s", oldpath, filename);
		}
	}

	return xsys_cp(newFileName, oldFileName);
}

int xsys_mv(const char * newpath, const char * oldpath, const char * filename)
{
	char newFileName[MAX_PATH], oldFileName[MAX_PATH];

	memset(newFileName, 0x0, sizeof(newFileName));
	memset(oldFileName, 0x0, sizeof(oldFileName));

	if (newpath == NULL || newpath[0] == '\0'){
		strcpy(newFileName, filename);
	} else {
		int l = strlen(newpath);
		if (newpath[l-1] == '\\' || newpath[l-1] == '/'){
			sprintf(newFileName, "%s%s", newpath, filename);
		} else {
			sprintf(newFileName, "%s/%s", newpath, filename);
		}
	}

	if (oldpath == NULL || oldpath[0] == '\0'){
		strcpy(oldFileName, filename);
	} else {
		int l = strlen(oldpath);
		if (oldpath[l-1] == '\\' || oldpath[l-1] == '/'){
			sprintf(oldFileName, "%s%s", oldpath, filename);
		} else {
			sprintf(oldFileName, "%s/%s", oldpath, filename);
		}
	}

	if (newFileName[0] == '\0' || oldFileName[0] == '\0'){
		return -1;
	}

	if (SysExistFile(oldFileName) == 0){
		return -2;
	}

	if (!xsys_md(newFileName, true)){
		return -3;
	}

	return SysMoveFile(oldFileName, newFileName);
}

int xsys_rm(const char * path, const char * filename)
{
	char szFileName[MAX_PATH];

	memset(szFileName, 0x0, sizeof(szFileName));

	if (path == NULL || path[0] == '\0'){
		strcpy(szFileName, filename);
	} else {
		int l = strlen(path);
		if (path[l-1] == '\\' || path[l-1] == '/'){
			sprintf(szFileName, "%s%s", path, filename);
		} else {
			sprintf(szFileName, "%s/%s", path, filename);
		}
	}

	if (szFileName[0] == '\0')
		return -1;

	return 	SysRemove(szFileName);
}

void xsys_clear_path(const char * path, const char * prefix)
{
	char fullname[MAX_PATH];
	xsys_ls ls;
	char filename[MAX_PATH];

	int len = 0;
	if (prefix)
		len = strlen(prefix);

	for (bool hasFile = ls.open(path, filename);
		hasFile;
		hasFile = ls.next(filename)
		)
	{
		if (strcmp(filename, ".") == 0 ||
			strcmp(filename, "..")== 0){
			continue;
		}
		if (len > 0 && strncmp(prefix, filename, len) != 0)
			continue;

		sprintf(fullname, "%s/%s", path, filename);
		if (ls.ispath()){
			xsys_rm_path(fullname);
		}else
			xsys_rm(fullname);
	}
}

int xsnprintf(char * d, int l, const char * formatstring, ...)
{
	va_list args;
	va_start(args, formatstring);
	int r = SysVSNPrintf(d, l, formatstring, args);
	va_end(args);

	return r;
}

const char * xlasterror(void)
{
	return ErrGetErrorString();
}

const char * xgeterror_string(int ierror)
{
	return ErrGetErrorString(ierror);
}

bool xsys_md(const char * pathname, bool isfile)
{
	char path[MAX_PATH];

	if (xsys_getpath(path, (char *)pathname, isfile) == 0)
		return false;

	int i = strlen(path);
	path[i+1] = 0;	// # 终止符
	for (i -= 1; i > 0 && !SysExistDir(path);){
		for (i--; path[i] != '\\' && path[i] != '/' && i > 0; i--)
			;
		if (i > 0){
			path[i] = 0;
		}
	}

	if (i == 0){
		// # 当前path不存在
		xsys_md(path);
		i = strlen(path);
	}
	// # i = 存在的结尾
	path[i++] = '/';
	while (path[i]){
		if (xsys_md(path) != 0){
			return false;
		}
		i += strlen(path+i);
		path[i++] = '/';
	}

	return true;
}

const char * xsys_ltoa(long l)
{
	char v[16];
#ifdef WIN32
	return _ltoa(l, v, 10);
#else
	sprintf(v, "%ld", l);
	return v;
#endif
}

#ifndef WIN32
/// 将串中的小写字母转换为大写字母
char *strupr(char *str)
{
	if (str == NULL)
		return NULL;

	while (*str) {
		*str = toupper(*str);
		str++;
	}

	return str;
}
#endif

#ifndef SysDoCmd
int SysDoCmd(const char * pszCmmand, const char * workpath, char * outbuf, int len);
#endif
int xsys_do_cmd(const char * cmd, const char * workpath, char * outbuf, int len)
{
	return SysDoCmd(cmd, workpath, outbuf, len);
}

int xsys_file_size(const char * fullname)
{
	SYS_FILE_INFO fi;
	if (xsys_file_info(fullname, fi) < 0)
		return -1;

	return int(fi.llSize);
}

char * xsys_getpath(char * d, char * s, bool isfile)
{
	char * p;
	if (isfile){
		p = strrchr(s, '\\');
		if (p == 0)
			p = strrchr(s, '/');
		if (p == 0){
			return 0;
		}
	}else{
		p = s + strlen(s);
	}

	// # p -> path end
	int l = int(p - s);
	if (l < 1)
		return 0;

	if (d != s)
		memmove(d, s, l);
	d[l] = 0;
	return d;
}

long get_tick_count(void)
{
#ifdef WIN32
	return long(GetTickCount());
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

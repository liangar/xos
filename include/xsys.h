/*! \file xsys.h
 *  与操作系统相关的底层类<br>
 *  主要从开源项目XMail中抽出<br>
 *  包括了事件,信号灯,临界区,线程控制,动态库操作,socket操作等<br>
 *  对常用的socket操作,加了一些方法<br>
 *  所有涉及的超时时间参数都以秒计
 */

#ifndef _XOS_H_
#define _XOS_H_

#include <xmailsys/SysInclude.h>
#include <xmailsys/SysDep.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif // MAX_PATH

#ifndef MAX_IP_LEN
#define MAX_IP_LEN 32
#endif

#ifndef MAX_HOST_NAME
#define MAX_HOST_NAME	128
#endif // MAX_HOST_NAME

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#define XSYS_VALID_SOCK(x)	((x) != 0 && (x) != SYS_INVALID_SOCKET)

int  xsys_init(bool bServerDebug = false);
void xsys_down(void);

#define xsys_cleanup    SysCleanupLibrary
#define xsys_shutdown   SysShutdownLibrary

/*! \class xsys_event
 *  事件类
 */
class xsys_event
{
public:
	xsys_event();
	~xsys_event()  {  down();  }

	//! 初始化函数
	/*!
	 * \param iManualReset 是否手工重置,0/else=否/是
	 * \return 0/else=成功/失败
	 */
	int  init(int iManualReset);
	void down(void);	/*!< 关闭函数. */

	bool isopen(void)  {  return (m_event != SYS_INVALID_EVENT);  }

	//! 等待
	/*!
	 * \param seconds 超时秒数,如果<=0,则为无限
	 * \return 0/ERR_TIMEOUT=有事件/超时
	 */
	int wait(int seconds);
	int wait_ms(int ms);
	int set(void);		/*!< 设置有事件. */
	int reset(void);	/*!< 重置. */
	int trywait(void);	/*!< 测试是否可以等待. */

	SYS_EVENT	m_event;
};

/*! \class xsys_semaphore
 *  信号量类
 */
class xsys_semaphore
{
public:
	xsys_semaphore();
	~xsys_semaphore()  {  down();  }

	//! 初始化函数
	/*!
	 * \param iInitCount 初始信号量
	 * \param iMaxCount  最多信号量
	 * \return 0/else=成功/失败
	 */
	int  init(int iInitCount, int iMaxCount);
	void down(void);			/*!< 关闭函数. */
	void bindto(xsys_semaphore * psem);

	//! 等待信号
	/*!
	 * \param ms 超时毫秒数,如果<=0,则为无限
	 * \return 0/ERR_TIMEOUT=有事件/超时
	 */
	int  P(int ms);
	int  V(int iCount = 1);		/*!< 信号增量 */

	int  trywait(int ms);	/*!< 测试是否可以等待 */

protected:
	SYS_SEMAPHORE m_semaphore;	/*!< 信号量 */
	xsys_semaphore *m_psem_bind;
};

/*! \class xsys_mutex
 *  临界区类
 */
class xsys_mutex
{
public:
	xsys_mutex();
	~xsys_mutex()  {  down();  }

	int  init(void);		/*!< 初始化 */
	void down(void);		/*!< 关闭 */

	bool isopen(void)  {  return (m_mutex != SYS_INVALID_MUTEX);  }

	//! 加锁
	/*!
	 * \param seconds 超时秒数,如果<=0,则为无限
	 * \return 0/ERR_TIMEOUT=有事件/超时
	 */
	int  lock(int ms);
	int  unlock(void);		/*!< 解锁 */
	int  trylock(void);		/*!< 测试是否可以加锁 */

protected:
	SYS_MUTEX	m_mutex;	/*!< 临界区 */
};

/*! \class xsys_thread
 *  线程类
 */
class xsys_thread
{
public:
	//! 创建
	xsys_thread();
	//! 析构
	~xsys_thread()  {  down();  }

	//! 启动
	/*! \param pThreadProc 线程执行函数
	 *  \param pThreadData 执行参数
	 *  \return 0/else=成功/失败
	 */
	int init(xsys_thread_function f, void *pThreadData);
	//! 关闭
	/*! \param seconds 等待秒数,缺省3
	 *  \param iForce 是否强行关闭,缺省是
	 *  \return 0/1/-1=成功/强行关闭/失败
	 */
	int down(int seconds = 3, int iForce = 1);

	bool isopen(void)  {  return (m_thread != SYS_INVALID_THREAD);  }
	//! 等待线程结束
	/*!
	 * \param seconds 超时秒数,如果<=0,则为无限
	 * \return 0/ERR_TIMEOUT=结束/超时
	 */
	int  wait(int seconds = 3);

	SYS_THREAD	m_thread;	/*!< 线程 */
};

void terminatethread(xsys_thread &hthread, int seconds = 3000, int returncode = 0); /*!< 停止线程 */

/*! \class xsys_module
 *  动态库类
 */
class xsys_module
{
public:
	xsys_module();
	~xsys_module()  {  down();  }

	int  load(const char * pszFilePath);
	void down(void);	/*!<  */

	void * get_symbol(const char * pszSymbol);

protected:
	SYS_HANDLE	m_h;	/*!< 动态库句柄 */
};

/*! \class xsys_ls
 *  文件目录列表类
 */
class xsys_ls
{
public:
	xsys_ls();
	~xsys_ls();

	/// 打开列表并取得第一个文件名
	/// \param pszPath 文件目录
	/// \param pszFileName 取得的文件名
	/// \return true/false = 成功/失败
	bool open(const char *pszPath, char *pszFileName, int len = MAX_PATH);
	int  ispath(void);	/// 当前文件是否是目录
	unsigned long get_size(void);	/// 当前文件大小

	/// 取得下一个文件名称
	/// \param filename 取得的文件名
	/// \return true/false = 已取得文件/没有文件
	bool next(char * filename, int len = MAX_PATH);

	void close(void);	/*!< 关闭列表 */

protected:
	SYS_HANDLE m_h;		/*!< 列表句柄 */
};

/*! \class xsys_socket
 *  TCP socket 操作类<br>
 *  所有的发送接收函数都是有超时时间设置的<br>
 *  超时时间以秒计
 */
class xsys_socket
{
public:
	xsys_socket();
	~xsys_socket();

	/// 监听
	int listen(int nportnumber, int iconnections = 1);
	int udp_listen(int nportnumber);

	/// 进入服务，接收连接，返回0为正确
	int accept(SYS_SOCKET  & client_sock, unsigned int timeout_ms = 0);
	int accept(xsys_socket & client_sock, unsigned int timeout_ms = 0);

	/// 客户端连接
	/// \param lphostname 地址
	/// \param nportnumber 端口
	/// \param timeout 超时时间(秒)
	/// \return 0/else 0/出错码
	int connect(const char * lphostname, int nportnumber = 0, int timeout = 30, bool is_tcp = true);

	void close(void);	/*!< 关闭 */
	int shutdown(void);/*!< 强行关闭 */
	int isopen(void);	/*!< 是否打开 */

	const char * get_peer_ip(char * ip);
	const char * get_self_ip(char * ip);

	//! 接收
	/*!
	 * \param buf 接收缓冲区
	 * \param l 希望接收的字节数
	 * \param timeout 超时秒数,如果<0,则为无限
	 * \return >=0/<0 接收的字节数/出错码<br>
	 *        出错码的文字解释可以通过xgeterror_string函数取得
	 */
	int recv(char * buf, int l, int timeout = SYS_INFINITE_TIMEOUT);
	int recv_from(char * buf, int len, char * peer_ip, int timeout = SYS_INFINITE_TIMEOUT);
	int recv_from(char * buf, int len, SYS_INET_ADDR * from_addr, int timeout = SYS_INFINITE_TIMEOUT);

	//! 发送
	/*!
	 * \param buf 发送缓冲区
	 * \param l 希望发送的字节数
	 * \param timeout 超时秒数,如果<=0,则为无限
	 * \return =0/<0 成功/出错码<br>
	 *        出错码的文字解释可以通过xgeterror_string函数取得
	 */
	int send(const char * buf, int l, int timeout = 10);
	int sendto(const char * buf, int l, const SYS_INET_ADDR *pto_addr, int timeout = 10);

	/// 接收并检查结束符
	/// \brief 接收,直到出错,收满,或者得到结束符为止<br>
	/// 收满情况下,没有到达结束符<br>
	/// 如果20秒内无字符到达,也出错
	/// \param buf 接收缓冲区
	/// \param maxlen 接收缓冲的字节数
	/// \param endstr 结束符
	/// \param timeout 超时秒数,如果<=0,则为无限
	/// \return >=0/<0  接收的字节数/失败
	int recv_byend(char * buf, int maxlen, char * endstr, int timeout = 20);

	/// 接收直到遇到结束符
	/// \brief 接收,直到出错,无内存分配,或者得到结束符为止<br>
	/// 内存不能再分配的情况下,没有到达结束符<br>
	/// 如果20秒内无字符到达,也出错<br>
	/// 最后,内存应该用free释放,不是new
	/// \param p 接收缓冲区指针
	/// \param endstr 结束符
	/// \param steplen 内存增长步幅
	/// \param timeout 超时秒数,如果<=0,则为无限
	/// \return >=0/<0  接收的字节数/失败
	int recv_byend(char ** p, const char * endstr, int steplen = 2048, int timeout = 20);

	/// 接收全部
	/// \param buf 接收缓冲区
	/// \param len 想接收的字节数
	/// \param timeout 超时秒数,如果<=0,则为无限
	/// \return len/<0 正确/失败
	int recv_all(char * buf, int len, int timeout = SYS_INFINITE_TIMEOUT);
	int send_all(const char * buf, int len, int timeout = 30); /*!< 发送全部 */
	int send_all(const char * buf);	/// 发送字符串

	/// 已内部协议块发送接收
	/// 块的结构:<%010d 长度><数据块>
	int recvblob(char * buf, int * prest_len = 0, int max_len = 64*1024, int timeout = SYS_INFINITE_TIMEOUT);
	int recvblob(char **buf, int * prest_len = 0, int max_len = 64*1024, int timeout = SYS_INFINITE_TIMEOUT);
	int sendblob(const char * buf, int len = 0, int timeout = 30);

	int m_isserver;		/*!< 是否是服务句柄 */
	SYS_SOCKET	m_sock;	/*!< socket句柄 */
};

#define xsys_set_breaker   SysSetBreakHandler
#define xsys_exec          SysExec

#define xsys_event_logv    SysEventLogV
#define xsys_event_log     SysEventLog
#define xsys_log_message   SysLogMessage

#define xsys_sleep         SysSleep
#define xsys_sleep_ms      SysMsSleep

inline long xsys_get_today_ms_time(void)  {  return long(SysMsTime());  }

#define xsys_file_info     SysGetFileInfo
#define xsys_file_modtime  SysSetFileModTime

#define xsys_exist_file    SysExistFile
#define xsys_exist_path    SysExistDir
#define xsys_md            SysMakeDir
#define xsys_rm            SysRemove
#define xsys_rm_path       SysRemoveDir
#define xsys_move_file     SysMoveFile

/// 取文件大小
/// \param fullname 文件全路径名称
/// \return >=0/else 文件大小/出错码
int xsys_file_size(const char * fullname);

/// 拷贝文件
/// \param new_name 新文件名称(全名)
/// \param old_name 源文件名称(全名)
/// \return 0/errorcode = 正确/出错代码
int xsys_cp(const char * new_name, const char * old_name);
int xsys_cp(const char * newpath, const char * oldpath, const char * filename);
int xsys_mv(const char * newpath, const char * oldpath, const char * filename);
int xsys_rm(const char * path, const char * filename);
void xsys_clear_path(const char * path, const char * prefix = NULL);

int xsnprintf(char * d, int l, const char * formatstring, ...);

//! 取得最近系统出错字符串
/*!
 * \return 返回出错字符串
 */
const char * xlasterror(void);
const char * xgeterror_string(int ierror);

/// 创建目录
/// \brief 如目录不存在则创建,不会破坏已有的目录文件
/// \param path 要创建的目录
/// \param isfile path参数是否是一个文件
/// \return true/false = 成功/失败
bool xsys_md(const char * path, bool isfile);

/// 将数字转成字符串
const char * xsys_ltoa(long l);

#ifndef WIN32
/// 将串中的小写字母转换为大写字母
	char *strupr(char *str);
#endif

int xsys_do_cmd(const char * cmd, const char * workpath, char * outbuf, int len);

char * xsys_getpath(char * d, char * s, bool isfile);

long get_tick_count(void);

#endif // _XOS_H_


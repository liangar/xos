#pragma once

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <mswsock.h>
	#include <stdio.h>

	int close_socket(int sock);
#else
	#include <sys/epoll.h>
	#include <stdio.h>

	#define close_socket    close
#endif

#include <xipc.h>

#ifndef memzero
#define memzero(x)	memset(&(x), 0, sizeof(x))
#endif

/// 异步操作结果
#define XASC_OK 		0
#define XASC_ERROR	   -1
#define XASC_TIMEOUT	1

/// 状态 / 异步所做操作指令
#define XASC_OP_NULL	0
#define XASC_OP_ACCEPT	10000	/// 设置比较的大的数，为了与其他区分开，不容易混淆
#define XASC_OP_RECV	10001
#define XASC_OP_CLOSE	10002
#define XASC_OP_EXIT   -10000

#define XASC_TIMER_INFINITE		INT_MAX	/// 用标准的最大整数作为永久时间
#define XASC_INVALID_SOCKET		-1

// 判断socket/session是否有效
#define XASC_VALID_SOCK(s)	(int(s) > 0)
#define XSESSION_ISOPEN(p)	((p) != nullptr && (XASC_VALID_SOCK((p)->sock)))

/**
 * \brief epoll/iocp 网络处理执行思路
 *
 * 二者都是将所有的网络连接的通讯消息，集中到一个句柄上进行通知，以便于高效处理
 * 不同之处也非常明显：
 * epoll 简洁，只需要将连接 socket 绑定到通知上即可
 * iocp 繁复，不仅连接要绑定，之后要发出异步通讯操作，iocp才会通知的此操作的处理结果
 *
 * 因此，iocp 必须为每个异步网络操作（recv/send/disconnect）安排单独的存储空间
 * 应用以 overlap 的 struct 结构缓存投递，有结果时 iocp 用它通知应用
 * 另外，每个 post 出去的异步网络操作，还需要有自己的数据缓冲空间：WSABUF
 *
 * epoll 只需要绑定时，给出一个通知项目放在 event.data 中，即可由此接收事件通知
 * 它只是发 event 通知，不发数据，因此应用无需为每个连接的每个操作建立缓冲区
 *
 * 另外，iocp 的很多函数，不能用静态连接得到，必须用 MS 提供的函数在程序加载时取
 *
 * 因此，统一处理接口，大多数是为了照顾 iocp，因为 epoll 的处理相当简单，而 iocp 复杂
 */
 
/**
 * \brief 网络环境的初始化
 * 此接口函数的设置，主要为 windows 环境
 * 当然细致的处理，对任何操作系统的不同版本都是需要的
 */
int net_evn_init(void);
void net_env_done(void);

/**
 * \brief 通讯会话结构
 * 
 */
struct xasc_session{
	int 	i;		/// id 比如在数组中的下标位置

	int 	sock;	/// 通讯所用socket,为系统分配的文件句柄

	struct sockaddr sock_addr;	/// socket 的地址
	struct sockaddr peer_addr;	/// socket 的对端地址

	time_t	create_time;	/// 建立连接的时间
	volatile time_t	last_trans_time;/// 最近处理时间，包含所有的操作
	volatile time_t	last_recv_time;	/// 最近接收时间

	volatile int idle_secs;		/// 空闲秒数，到达空闲时间，执行on_idle

	volatile int state;	/// 状态，参照 define 中的定义

	char   *precv_buf;	/// 接收缓冲区，动态分配
	int		recv_size;	/// 缓冲空间大小
	int		recv_bytes; /// 实际接收数据的字节数

	int 	recv_amount;/// 接收总字节数
	int		pkg_amunt;	/// 接收的完整数据包数

	int 	sent_len;	/// 最近发送长度

	int		pkg_amount;	/// 接收包数

#ifdef _WIN32
	OVERLAPPED	ovlp;	/// ovlp for recv
#endif

	/// 以下给应用扩展使用
	int		app_id;		/// 应用扩展 id 标识
	char   *app_data;	/// 应用扩展数据
};

/**
 * \brief 异步I/O操作管理基础类
 * 此类针对服务端
 * 所有接口函数的返回值，一般都是 0 表示成功，<0 表示失败
 */
class xasc_iom{
protected:
	int iom_fd_,	// 异步操作句柄
		listen_fd_,	// 网络监听句柄
		port_;		// 监听端口
	xasc_session * accept_s_;	// 当前处理接入会话

	int most_len_,	// 通常接收长度
		max_len_,	// 最大接收长度
		max_sessions_;	// 最大会话数

public:
	xasc_iom()
		: iom_fd_(-1)
		, listen_fd_(-1)
		, accept_s_(nullptr)
		, port_(0)
	{
		 most_len_ = 0;  max_len_ = 0;
		 max_sessions_ = 0;
	}

	/** 打开网络集中通知
	 * \param[in]  listen_port  监听端口
	 * \param[in]  max_sessions 最大会话数
	 * \param[in]  most_len  常用接收长度
	 * \param[in]  max_len   最大接收长度
	 * \return 0/<0 = 正确 / 出错码
	 */
	int open(int listen_port, int max_sessions, int most_len, int max_len);

	/// 关闭
	int close(void);

	/// 是否打开
	bool isopen(void) { return listen_fd_ > 0; }

	/** 增加一个待接收会话
	 * \param[in]  s    会话地址, 内容会被修改
	 * \return 0/<0 = 成功 / 出错码
	 */	
	int add_fd(xasc_session *s);

	/** 取得当前待接收会话
	 *  此函数在释放时特别需要处理，因为它不是在使用，又非空闲，是特殊的中间状态
	 */
	xasc_session* get_accept_s(void) { return accept_s_; }

	/** 去掉一个socket的网络通知
	 *  \param[in]  s     会话地址
	 *  \return 0/<0 = 正确 / 出错
	 */
	int del_fd(xasc_session *s);

	/** 为会话发起接收操作
	 *  发出的操作，将视缓冲大小，发出不需要重新分配空间的最大申请
	 *  在 epoll 下，只是修改事件处理状态
	 *  \param[in]  s     会话指针
	 *  \return 0/<0 = 正确 / 出错
	 */
	int post_recv(xasc_session *s);
	
	/** 为会话发起接收指定长度操作
	 *  \param[in]  s     会话地址
	 *  \param[in]  len   期望接收的数据最大长度
	 *  \return 0/<0 = 正确 / 出错
	 */
	int post_recv(xasc_session *s, int len);

	/** 等待并取得网络通知数据
	 *  \param[out]  ps     会话地址的指针
	 *  \param[out]  bytes  通讯处理的字节数
	 *  \param[in]   timeout_ms   超时毫秒数
	 *  \return >=0 / <0 = 正确, 网络操作码 / 出错码
	 */
	int process_event(xasc_session **ps, int *bytes, int timeout_ms);

protected:
	/** 创建 I/O 异步操作管理句柄
	 *  \return >0 / < 0 = 管理通知句柄 / 出错码
	 */
	virtual int iom_create_fd(void);

	/** 绑定监听句柄
	 *  \return 0 / < 0 = 正确 / 出错码
	 */
	virtual int iom_bind_listen(void);

	// 处理接收
	virtual int iom_do_accept(void);
};

#pragma once

/*!
 * \file xasc_server.cpp
 * \brief 异步通讯服务
 * 提供了简洁、高效的通讯处理框架。
 * 实际应用，一般只需要扩展2个接口：1. 协议处理:计算包预定长度, 2. 处理完整包
 *
 * 本服务是异步通讯的一个简要实现。基本思路是为信息处理的3个阶段，分别用3个线程担任
 * 通讯部分有2个：接收(包括会话管理), 发送
 * 实际处理就1个：接收包的处理
 *
 * \author liangar@qq.com
 * \version V1.0.0
 *
 */

#include <xasc_iom.h>

#include <xqueue_msg.h>
#include <xslm_deque.h>

///////////////////////////////////////////////////////////////////////////////
// log 输出函数
// 可以输出到屏幕(for_debug = true)，以及文件中
bool log_open(const char * file_name, bool for_debug = true, int level = 0, int timestamp_secs = 60);
void log_close(void);

void log_put(const char * s);
void log_put(const char c);
void log_put_line(char const * s);
void log_print(const char * format, ...);
void log_print_buf(const char * title, const unsigned char * buf, int len);

//////////////////////////////////////////////////////////////////////////////
// 会话管理队列
class xasc_sessions : public xslm_deque<xasc_session>{
protected:
	int compare(const xasc_session *t1, const xasc_session *t2){
		return (t1->i - t2->i);
	}

	void show_t(FILE * pf, const xasc_session * t)
	{
		xasc_session * p = container_of(&(t->state), xasc_session, state);
		int n = offsetof(xasc_session, state);
		fprintf(pf, "id=%d %p %p %d", t->i, p, t, p->state);
	}
};

/**
 * \brief ASIO Server 框架类
 * 结构：
 * 使用 3 种处理线程：1. 接收处理(master)，2. 事务处理(actor), 3. 发送处理(sender)
 * master 负责处理网络消息，对会话进行管理
 * actor  负责处理接收的数据（事务），它通常最为耗时
 * sender 负责发送数据包, 它最为简单
 *
 * 机理：
 * 应用扩展只需要扩展事务处理函数（do_msg），以及协议包长计算（calc_pkg_len）
 * master 使用 calc_pkg_len 判断包是否接收完毕，是的话，发给 actor
 * actor 线程主要工作是调用 do_msg 进行事务处理
 * 服务提供的 send 函数，其实是用消息队列发给 sender 服务进行发送
 *
 * 只有 master 改变会话的启用/关闭，所以会话管理无需加锁
 * actor/sender 需要关闭会话时，向 master 发关闭的请求通知
 * 目前，请求通知只用消息队列，其实在 iocp 时，可以用 iocp 通知，也可以考虑开网络连接通知
 */
class xasc_server
{
#define XASC_SESSION_INRANGE(i)	(i >= 0 && i < max_sessions_)
public:
	/// 构造
	xasc_server(const char * name = nullptr);

	virtual ~xasc_server(){};

	/** 打开服务
	 * \param listen_port  服务listen端口
	 * \param ttl          通讯超时间隔时间(s)
	 * \param max_seesions 最多在线会话数
	 * \param recv_len     接收缓冲长度，应该设为大多数通讯包的最大长度
	 * \param send_len 	发送缓冲长度, 应该设为最大包长
	 * \return 成功与否
	 */
	virtual bool open(int listen_port, int max_sessions, int ttl, int most_len, int max_len, int send_len);
	/// 关闭释放
	virtual bool close(int secs = 5);
	/// 启动服务
	virtual bool start(void);
	/// 停止服务
	virtual bool stop(int secs = 5);

	/// 取指定会话
	xasc_session * get_session(int i){
		if (!XASC_SESSION_INRANGE(i))
			return nullptr;
		return sesns_ + i;
	}

	/// 发送定长数据
	int send(int isession, const char * s, int len);
	/// 发送字符串
	int send(int isession, const char * s);

	/** 通知关闭指定会话
	 * \param  i   会话标号[0 ~ N-1]
	 * \return 通知发送的消息长度，<0/else = 出错/成功
	 */
	int  notify_close_session(int i);
	void notify_do_cmd(int i, const char* cmd);

	bool session_isopen(int i);	/// 会话是否打开

protected:
	/** 计算通讯包应有长度
	 * \param  i 指定会话位置号
	 * \return <0|0|>0 = 无效出错数据长度|无用数据|完整包数据长度
	 */
	virtual int  calc_pkg_len(int i);
	virtual int  calc_pkg_len(const unsigned char * pbuf, int len) = 0;

	virtual int  do_msg(int i, char * msg, int msg_len) = 0;
	virtual int  do_idle(int i) = 0;	/// 空闲处理

	/// 以下3个事件处理函数
	virtual bool on_opened	(int i);	/// 打开指定会话处理
	virtual bool on_closed  (int i);	/// 关闭指定会话处理
	/// 发送完成
	virtual bool on_sent	(int i, int len)  {  return true;  }

protected:
	/// 服务线程函数
	void master(void);	/// 会话管理与接收处理线程
	void actor (void);	/// 消息处理服务线程（从request队列接收数据，用do_msg处理）
	void sender(void);	/// 发送处理服务线程

	xqueue_msg	close_requests_; 	/// 关闭通知队列(master 使用)
	xqueue_msg	recv_queue_;	/// 提交数据处理(actor 使用)
	xqueue_msg	send_queue_;	/// 发送消息队列(sender 使用)

	volatile bool	is_run_;	/// 正在运行标志

	/// 计算需要等待的时间
	int  timeout_secs(void);

	void session_open (xasc_session * psession);
	void session_close(xasc_session * psession);
	void session_close(int i);	/// 关闭指定会话
	void session_close_all(void);

	/// 将会话数据发给 actor 处理
	int  session_recv_to_queue(xasc_session * s);

	void close_all_session(void);

protected:
	char name_[64];	/// 服务名
	volatile int heartbeat_; /// 心跳:服务的最近活动时间

	xasc_iom	iom_;	/// 异步 I/O 通讯管理

	xasc_sessions sesn_q_;	/// 会话队列
	xasc_session * sesns_;	/// 会话存储
	int 	max_sessions_;  /// 最大会话数

	int	listen_port_;	/// 端口
	int session_ttl_;	/// 超时时间(秒)
	int idle_;		/// 会话空闲时间
	int	most_len_;	/// 一般情况下接收的最大长度
	int	max_len_;	/// 数据包的最大长度
	int send_len_; 	/// 发送的最大长度
	
	std::thread th_master_;	/// 消息处理服务线程
	std::thread th_sender_;	/// 发送服务线程
	std::thread th_actor_;	/// 消息处理服务线程
};

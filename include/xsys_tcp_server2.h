#pragma once

#include <xwork_server.h>
#include <xsys_tcp_server.h>
#include <xlist.h>

#include <xseq_buf.h>

/*
typedef enum {
	XTS_RECV_READY = 0,
	XTS_RECVING,
	XTS_RECVED,
	XTS_RECV_ERROR,
	XTS_RECV_EXCEPT,
	XTS_SEND_READY = 1000,
	XTS_SENDING,
	XTS_SENT,
	XTS_SEND_ERROR,
	XTS_SEND_EXCEPT,
	XTS_SESSION_END = 10000,
} XTS_STATES;
//*/

struct xtcp2_session{
	SYS_SOCKET	sock;	/// 通讯地址

	int		peerid;			/// 连接对方的 id 标识, 给应用使用
	time_t	createTime;		/// 建立连接的时间
	long	last_trans_time;/// 最近通讯时间

	/// recv
	XTS_STATES	recv_state;	/// 接收状态
	char *  precv_buf;		/// 接收缓冲区
	int		recv_len;		/// 当前所用的缓冲空间
	int		recv_buflen;	/// 当前缓冲长度
	
	int 	sent_len;		/// 最近发送长度

	int		cmd_count;
	int		running_cmdid;
};

#define PSESSION_ISOPEN(p)	((p) != 0 && XSYS_VALID_SOCK((p)->sock))

class xsys_tcp_server2 : public xwork_server
{
public:
	/// 构造
	xsys_tcp_server2();

	/// 构造
	/// \param name 服务名称,长度不超过128
	/// \param dbstring 数据库连接串
	/// \param nmaxexception 最多可容忍的异常次数
	/// \param works 服务线程数
	xsys_tcp_server2(const char * name, int nmaxexception = 5);

	virtual ~xsys_tcp_server2(){};

	/// 初始化服务
	/// ttl : 通讯间隔, 超时则废弃
	/// max_seesions : 最多在线会话数 不超过 1024
	/// recv_len : 每次试图接收长度，最小 1024
	virtual bool open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len);
	virtual bool open(const char * url,int ttl, int recv_len);
	virtual bool stop(int secs = 5);
	virtual bool close(int secs = 5);	/// 关闭释放

	xseq_buf * get_recv_queue(void){
		return &m_recv_queue;
	}
	xtcp2_session * get_session(int i){
		if (i < 0 || i >= m_session_count)
			return 0;
		return m_psessions + i;
	}

	int send(int isession, const char * s, int len);
	int send(int isession, const char * s);

	int  notify_close_session(int i);

	void session_close(int i);
	bool session_isopen(int i);

protected:
	static unsigned int run_send_thread(void * ptcp_server);
	static unsigned int run_msg_thread (void * ptcp_server);

protected:
	/// 服务线程函数
	void run(void);			/// 接收处理线程
	void msg_server(void);	/// 消息处理服务线程（从request队列接收数据，用do_msg进行处理）
	virtual void send_server(void);	/// 发送处理线程

	virtual int  calc_msg_len(int i) = 0;	/// <0|0|>0 = 无效出错数据长度|无用数据|完整包数据长度
	virtual int  do_msg(int i, char * msg, int msg_len) = 0;
	virtual int  do_idle(int i) = 0;
	
	virtual bool on_opened	(int i);
	virtual bool on_closed  (int i);
	virtual bool on_sent	(int i, int len) = 0;	/// 发送完成

protected:
	void timeout_check(void);

	void session_open(int i);
	void session_close(xtcp2_session * psession);
	// 关闭已用标记的第i_used个会话，返回实际的序号
	int  session_close_used(int i_used);
	int  session_close_used_by_i(int i_session);
	
	void session_recv_reset(int i);
	int session_recv_to_queue(int i, int len);
	int session_recv(int i, int len);

	void close_all_session(void);

protected:
	xtcp2_session *	m_psessions;/// 保存各个连接(已经建立的连接)
	int 	  * m_pused_index;	/// 保存正在使用的连接
	int 		m_used_count;	/// 正在使用的数量
	int			m_session_count;

	xseq_buf	m_recv_queue;	/// 保存提交数据(循环队列)
	xseq_buf	m_send_queue;	/// 保存回复数据
	xseq_buf	m_close_requests; 	/// 关闭通知

protected:
	int			m_listen_port;	/// 端口
	char		m_serverURL[64];/// 服务端的URL地址
	xsys_socket m_listen_sock;	
	int 		m_session_ttl;	/// 超时时间(秒)
	int			m_recv_len;		/// 每次接收的最大长度
	int 		m_send_len; 	/// 每次发送最大值
	char *		m_precv_buf;	/// 接收缓冲

	xsys_thread m_send_thread;	/// 发送服务线程
	xsys_thread m_msg_thread;	/// 消息处理服务线程
};

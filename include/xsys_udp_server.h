#pragma once

#include <thread>
#include <xsys_tcp_server.h>

#define PUDPSESSION_ISOPEN(p)	((p)->recv_state != XTS_SESSION_END)
// ((p)->addr_crc != 0)

struct xudp_session{
	SYS_INET_ADDR	addr;	/// 通讯地址
	volatile int 	addr_crc;

	time_t	createTime;		/// 建立连接的时间
	volatile long	last_recv_time;	/// 最近接收时间
	volatile long	last_trans_time;/// 最近通讯时间
	volatile int 	idle_secs;		/// 空闲秒数，到达空闲时间，执行on_idle

	/// recv
	volatile XTS_STATES	recv_state;	/// 接收状态
	char *  precv_buf;		/// 接收缓冲区
	int		recv_len;		/// 当前所用的缓冲空间
	int		recv_buflen;	/// 当前缓冲长度

	int		recv_count; 	/// 总共接收的包
	
	SYS_SOCKET target_sock; /// 转发的目标地址

	union{
		volatile int	peerid;		/// 连接对方的 id 标识, 给应用使用
		volatile void * pdev;
	};
	volatile void * pcmd;
};

class xsys_udp_server : public xwork_server
{
#define XUDPSESSION_INRANGE(i)	(i >= 0 && i < m_session_count)
public:
	/// 构造
	xsys_udp_server();

	/// 构造
	/// \param name 服务名称,长度不超过128
	/// \param dbstring 数据库连接串
	/// \param nmaxexception 最多可容忍的异常次数
	/// \param works 服务线程数
	xsys_udp_server(const char * name, int nmaxexception = 5);

	virtual ~xsys_udp_server(){};

	/// 初始化服务
	/// ttl : 通讯间隔, 超时则废弃
	/// max_seesions : 最多在线会话数 不超过 1024
	/// recv_len : 每次试图接收长度，最小 1024
	bool open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len);
	bool open(const char * url,int ttl, int recv_len);
	bool stop(int secs = 5);
	bool close(int secs = 5);	/// 关闭释放

	xseq_buf * get_requests(void){
		return &m_recv_queue;
	}
	xudp_session * get_session(int i){
		if (i < 0 || i >= m_session_count)
			return 0;
		return m_psessions + i;
	}

	void set_session_idle(int isession, int idle_secs);
	void set_idle(int idle_secs);

	int send(int isession, const char * s, int len);
	int send(int isession, const char * s);

	void session_close(int i);
	bool session_isopen(int i);
	int notify_close_session(int i, bool need_shift = true);

	void notify_do_cmd(const char * cmd = 0);
	void sign_new_cmd (bool has_new = true);
	bool has_new_cmd(void);

	/// 服务线程函数
	void send_server(void);	/// 发送处理线程
	void msg_server(void);	/// 消息处理服务线程（从request队列接收数据，用do_msg进行处理）
	void relay_server(void);/// 转发处理服务

protected:
	void run(void);			/// 接收处理线程

	/// 转发函数判断
	virtual const char * get_target_url(int isession, char * pbuf, int len);

	virtual int  calc_msg_len(int isession) = 0;	/// <0|0|>0 = 无效出错数据长度|无用数据|完整包数据长度
	virtual int  calc_msg_len(int isession, char * pbuf, int len) = 0;

	virtual int  do_msg(int i, char * msg, int msg_len) = 0;	/// 处理消息
	virtual int  do_cmd(const char * cmd = 0) = 0;	/// 主动命令执行

	virtual int  on_open	(int i)  {  return 0;  }
	virtual bool on_sent	(int i, int len) = 0;	/// 发送完成
	virtual bool on_closed  (int i) = 0;
	virtual int  do_idle	(int i) = 0;

protected:
//	void timeout_check(void);
	int opened_find(bool &for_trans, int * crc, SYS_INET_ADDR * addr);

	void session_open(int i);
//	void session_close(xudp_session * psession);
	// 关闭已用标记的第i_used个会话，返回实际的序号
	int  session_close_used(int i_used);
	int session_close_used_by_i(int i_session);

	void session_recv_reset(int i);
	int session_recv_to_request(int i, int len);
	int session_recv(int i, int len);

	void close_all_session(void);

protected:
	xudp_session *	m_psessions;	/// 会话
	int 		*m_pused_index; 	/// 正在使用的连接
	volatile int m_used_count;		/// 正在使用的数量
	int			m_session_count;

	xseq_buf	m_recv_queue;	/// 保存提交数据(循环队列)
	xseq_buf	m_send_queue;	/// 保存发送数据
	xseq_buf	m_close_requests; 	/// 关闭通知
	xseq_buf	m_relay_queue;	/// 转发通知 2020-04-14

	volatile bool	m_has_new_cmd;	/// 标志：有新消息

protected:
	volatile int	m_listen_port;	/// 端口
	char		m_serverURL[64];/// 服务端的URL地址

	xsys_socket * m_plisten_sock;
	xsys_mutex	m_sock_lock;

	int 		m_session_ttl;	/// 超时时间(秒)
	int 		m_session_idle;	/// 会话空闲时间
	int			m_recv_len;		/// 每次接收的最大长度
	int 		m_send_len; 	/// 每次发送最大值
	char *		m_precv_buf;	/// 接收缓冲

	xsys_thread m_send_thread;	/// 发送服务线程
	xsys_thread m_msg_thread;	/// 消息处理服务线程
	xsys_thread m_relay_thread; /// 转发线程 2020-04-14
};

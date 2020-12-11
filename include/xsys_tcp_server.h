#pragma once

#include <xsys.h>

#include <ss_service.h>
#include <xwork_server.h>
#include <xlist.h>

#include <xseq_buf.h>

#define XSYS_IP_FATAL	-101

enum XTS_STATES {
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
	XTS_SESSION_OPENING = 10001
};

struct xtcp_session{
	xsys_socket	sock;	/// 通讯句柄
	char	peerip[32];	/// 对方ip地址
	char	servip[32];	/// 主机ip地址
	time_t	createTime;	/// 建立连接的时间
	long	last_trans_time;	/// 最近通讯时间
	int		peerid;		/// 连接对方的 id 标识, 给应用使用

	/// recv
	XTS_STATES	recv_state;	/// 接收状态
	char *  precv_buf;		/// 接收缓冲区
	int		recv_len;		/// 当前所用的缓冲空间
	int		recv_next_len;	/// 将接收数据的长度
	int		recv_buflen;	/// 当前缓冲长度

	/// send
	XTS_STATES	send_state;
	char *	psend_buf;
	int		send_len;
	int		sent_len;
};

class xsys_tcp_server : public xwork_server
{
public:
	/// 构造
	xsys_tcp_server();

	/// 构造
	/// \param name          服务名称,长度不超过128
	/// \param nmaxexception 最多可容忍的异常次数
	xsys_tcp_server(const char * name, int nmaxexception = 5);

	virtual ~xsys_tcp_server(){};

	/// 打开服务
	/// \param listen_port  服务listen端口
	/// \param ttl          通讯间隔, 超时则废弃
	/// \param max_seesions 最多在线会话数,不超过 1024(目前用select)
	/// \param recv_len     每次试图接收长度，最小 1024
	/// \return 成功与否
	bool open(int listen_port, int ttl, int max_sessions, int recv_len);
	bool open(const char * url,int ttl, int recv_len);
	bool stop(int secs = 5);
	bool close(int secs = 5);	/// 关闭释放

	void run(void);

	void send_server(void);
	void recv_server(void);

	virtual void check_recv_state(int i) = 0;
	virtual void check_send_state(int i) = 0;

	virtual bool on_connected(int i) = 0;
	virtual bool on_recved   (int i) = 0;
	virtual bool on_closed   (int i) = 0;

	xseq_buf * get_requests(void){
		return &m_requests;
	}
	xtcp_session * get_session(int i){
		if (i < 0 || i >= m_session_count)
			return 0;
		return m_psessions + i;
	}

	int send(int isession, const char * s, int len);
	int send(int isession, const char * s);

	int send_byid(int peerid, const char * s, int len);
	int send_byid(int peerid, const char * s);

	int find_session_byid(int peerid);

	int  notify_close_session(int i);
	void session_close(int i);
	bool session_isopen(int i);
protected:
	void session_open(int i);
	void session_close(xtcp_session * psession);
	void session_close_by_sock(int sock);

	void session_recv_reset(int i);
	int session_recv_to_request(int i);
	int session_recv(int i, int len);

	void close_all_session(void);

protected:
	xtcp_session *	m_psessions;	/// 保存各个连接(已经建立的连接)
	int			m_session_count;
	xseq_buf	m_requests;			/// 保存提交数据(循环队列)
	xseq_buf	m_sends;			/// 保存回复数据

	xseq_buf	m_close_requests; 	/// 关闭通知

protected:
	int			m_listen_port;
	char		m_serverURL[64];/// 服务端的URL地址
	xsys_socket m_listen_sock;	/// 端口
	int 		m_session_ttl;	/// 超时时间(秒)
	int			m_recv_len;		/// 每次接收的最大长度
	char *		m_precv_buf;	/// 接收缓冲
	int			m_session_init_len;	/// session 可以初始化的部分

	xsys_mutex	m_lock;
};

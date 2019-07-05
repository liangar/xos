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
	SYS_SOCKET	sock;	/// ͨѶ��ַ

	int		peerid;			/// ���ӶԷ��� id ��ʶ, ��Ӧ��ʹ��
	time_t	createTime;		/// �������ӵ�ʱ��
	long	last_trans_time;/// ���ͨѶʱ��

	/// recv
	XTS_STATES	recv_state;	/// ����״̬
	char *  precv_buf;		/// ���ջ�����
	int		recv_len;		/// ��ǰ���õĻ���ռ�
	int		recv_buflen;	/// ��ǰ���峤��
	
	int 	sent_len;		/// ������ͳ���

	int		cmd_count;
	int		running_cmdid;
};

#define PSESSION_ISOPEN(p)	((p) != 0 && XSYS_VALID_SOCK((p)->sock))

class xsys_tcp_server2 : public xwork_server
{
public:
	/// ����
	xsys_tcp_server2();

	/// ����
	/// \param name ��������,���Ȳ�����128
	/// \param dbstring ���ݿ����Ӵ�
	/// \param nmaxexception �������̵��쳣����
	/// \param works �����߳���
	xsys_tcp_server2(const char * name, int nmaxexception = 5);

	virtual ~xsys_tcp_server2(){};

	/// ��ʼ������
	/// ttl : ͨѶ���, ��ʱ�����
	/// max_seesions : ������߻Ự�� ������ 1024
	/// recv_len : ÿ����ͼ���ճ��ȣ���С 1024
	virtual bool open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len);
	virtual bool open(const char * url,int ttl, int recv_len);
	virtual bool stop(int secs = 5);
	virtual bool close(int secs = 5);	/// �ر��ͷ�

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
	/// �����̺߳���
	void run(void);			/// ���մ����߳�
	void msg_server(void);	/// ��Ϣ��������̣߳���request���н������ݣ���do_msg���д���
	virtual void send_server(void);	/// ���ʹ����߳�

	virtual int  calc_msg_len(int i) = 0;	/// <0|0|>0 = ��Ч�������ݳ���|��������|���������ݳ���
	virtual int  do_msg(int i, char * msg, int msg_len) = 0;
	virtual int  do_idle(int i) = 0;
	
	virtual bool on_opened	(int i);
	virtual bool on_closed  (int i);
	virtual bool on_sent	(int i, int len) = 0;	/// �������

protected:
	void timeout_check(void);

	void session_open(int i);
	void session_close(xtcp2_session * psession);
	// �ر����ñ�ǵĵ�i_used���Ự������ʵ�ʵ����
	int  session_close_used(int i_used);
	int  session_close_used_by_i(int i_session);
	
	void session_recv_reset(int i);
	int session_recv_to_queue(int i, int len);
	int session_recv(int i, int len);

	void close_all_session(void);

protected:
	xtcp2_session *	m_psessions;/// �����������(�Ѿ�����������)
	int 	  * m_pused_index;	/// ��������ʹ�õ�����
	int 		m_used_count;	/// ����ʹ�õ�����
	int			m_session_count;

	xseq_buf	m_recv_queue;	/// �����ύ����(ѭ������)
	xseq_buf	m_send_queue;	/// ����ظ�����
	xseq_buf	m_close_requests; 	/// �ر�֪ͨ

protected:
	int			m_listen_port;	/// �˿�
	char		m_serverURL[64];/// ����˵�URL��ַ
	xsys_socket m_listen_sock;	
	int 		m_session_ttl;	/// ��ʱʱ��(��)
	int			m_recv_len;		/// ÿ�ν��յ���󳤶�
	int 		m_send_len; 	/// ÿ�η������ֵ
	char *		m_precv_buf;	/// ���ջ���

	xsys_thread m_send_thread;	/// ���ͷ����߳�
	xsys_thread m_msg_thread;	/// ��Ϣ��������߳�
};

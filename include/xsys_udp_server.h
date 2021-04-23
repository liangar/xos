#pragma once

#include <thread>
#include <xsys_tcp_server.h>

#define PUDPSESSION_ISOPEN(p)	((p)->recv_state != XTS_SESSION_END)
// ((p)->addr_crc != 0)

struct xudp_session{
	SYS_INET_ADDR	addr;	/// ͨѶ��ַ
	volatile int 	addr_crc;

	time_t	createTime;		/// �������ӵ�ʱ��
	volatile long	last_recv_time;	/// �������ʱ��
	volatile long	last_trans_time;/// ���ͨѶʱ��
	volatile int 	idle_secs;		/// �����������������ʱ�䣬ִ��on_idle

	/// recv
	volatile XTS_STATES	recv_state;	/// ����״̬
	char *  precv_buf;		/// ���ջ�����
	int		recv_len;		/// ��ǰ���õĻ���ռ�
	int		recv_buflen;	/// ��ǰ���峤��

	int		recv_count; 	/// �ܹ����յİ�
	
	SYS_SOCKET target_sock; /// ת����Ŀ���ַ

	union{
		volatile int	peerid;		/// ���ӶԷ��� id ��ʶ, ��Ӧ��ʹ��
		volatile void * pdev;
	};
	volatile void * pcmd;
};

class xsys_udp_server : public xwork_server
{
#define XUDPSESSION_INRANGE(i)	(i >= 0 && i < m_session_count)
public:
	/// ����
	xsys_udp_server();

	/// ����
	/// \param name ��������,���Ȳ�����128
	/// \param dbstring ���ݿ����Ӵ�
	/// \param nmaxexception �������̵��쳣����
	/// \param works �����߳���
	xsys_udp_server(const char * name, int nmaxexception = 5);

	virtual ~xsys_udp_server(){};

	/// ��ʼ������
	/// ttl : ͨѶ���, ��ʱ�����
	/// max_seesions : ������߻Ự�� ������ 1024
	/// recv_len : ÿ����ͼ���ճ��ȣ���С 1024
	bool open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len);
	bool open(const char * url,int ttl, int recv_len);
	bool stop(int secs = 5);
	bool close(int secs = 5);	/// �ر��ͷ�

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

	/// �����̺߳���
	void send_server(void);	/// ���ʹ����߳�
	void msg_server(void);	/// ��Ϣ��������̣߳���request���н������ݣ���do_msg���д���
	void relay_server(void);/// ת���������

protected:
	void run(void);			/// ���մ����߳�

	/// ת�������ж�
	virtual const char * get_target_url(int isession, char * pbuf, int len);

	virtual int  calc_msg_len(int isession) = 0;	/// <0|0|>0 = ��Ч�������ݳ���|��������|���������ݳ���
	virtual int  calc_msg_len(int isession, char * pbuf, int len) = 0;

	virtual int  do_msg(int i, char * msg, int msg_len) = 0;	/// ������Ϣ
	virtual int  do_cmd(const char * cmd = 0) = 0;	/// ��������ִ��

	virtual int  on_open	(int i)  {  return 0;  }
	virtual bool on_sent	(int i, int len) = 0;	/// �������
	virtual bool on_closed  (int i) = 0;
	virtual int  do_idle	(int i) = 0;

protected:
//	void timeout_check(void);
	int opened_find(bool &for_trans, int * crc, SYS_INET_ADDR * addr);

	void session_open(int i);
//	void session_close(xudp_session * psession);
	// �ر����ñ�ǵĵ�i_used���Ự������ʵ�ʵ����
	int  session_close_used(int i_used);
	int session_close_used_by_i(int i_session);

	void session_recv_reset(int i);
	int session_recv_to_request(int i, int len);
	int session_recv(int i, int len);

	void close_all_session(void);

protected:
	xudp_session *	m_psessions;	/// �Ự
	int 		*m_pused_index; 	/// ����ʹ�õ�����
	volatile int m_used_count;		/// ����ʹ�õ�����
	int			m_session_count;

	xseq_buf	m_recv_queue;	/// �����ύ����(ѭ������)
	xseq_buf	m_send_queue;	/// ���淢������
	xseq_buf	m_close_requests; 	/// �ر�֪ͨ
	xseq_buf	m_relay_queue;	/// ת��֪ͨ 2020-04-14

	volatile bool	m_has_new_cmd;	/// ��־��������Ϣ

protected:
	volatile int	m_listen_port;	/// �˿�
	char		m_serverURL[64];/// ����˵�URL��ַ

	xsys_socket * m_plisten_sock;
	xsys_mutex	m_sock_lock;

	int 		m_session_ttl;	/// ��ʱʱ��(��)
	int 		m_session_idle;	/// �Ự����ʱ��
	int			m_recv_len;		/// ÿ�ν��յ���󳤶�
	int 		m_send_len; 	/// ÿ�η������ֵ
	char *		m_precv_buf;	/// ���ջ���

	xsys_thread m_send_thread;	/// ���ͷ����߳�
	xsys_thread m_msg_thread;	/// ��Ϣ��������߳�
	xsys_thread m_relay_thread; /// ת���߳� 2020-04-14
};

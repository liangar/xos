#pragma once

#include <xsys.h>

#include <ss_service.h>
#include <xwork_server.h>
#include <xlist.h>

#include <xseq_buf.h>

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

struct xudp_session{
	SYS_INET_ADDR	addr;	/// ͨѶ��ַ
	int 	addr_crc;

	time_t	createTime;		/// �������ӵ�ʱ��
	long	last_trans_time;/// ���ͨѶʱ��
	int		peerid;			/// ���ӶԷ��� id ��ʶ, ��Ӧ��ʹ��

	/// recv
	XTS_STATES	recv_state;	/// ����״̬
	char *  precv_buf;		/// ���ջ�����
	int		recv_len;		/// ��ǰ���õĻ���ռ�
	int		recv_buflen;	/// ��ǰ���峤��
	
	int 	sent_len;		/// ������ͳ���

	int		cmd_count;
	int		running_cmdid;
};

#define PSESSION_ISOPEN(p)	((p)->addr.iSize > 0)

class xsys_udp_server : public xwork_server
{
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
	bool open(int listen_port, int ttl, int max_sessions, int recv_len);
	bool open(const char * url,int ttl, int recv_len);
	bool stop(int secs = 5);
	bool close(int secs = 5);	/// �ر��ͷ�

	/// �����̺߳���
	void run(void);			/// ���մ����߳�
	void send_server(void);	/// ���ʹ����߳�
	void msg_server(void);	/// ��Ϣ��������̣߳���request���н������ݣ���do_msg���д���

	virtual int  calc_msg_len(int i) = 0;	/// <0|0|>0 = ��Ч�������ݳ���|��������|���������ݳ���
	virtual int  do_msg(int i, char * msg, int msg_len) = 0;
	
	virtual bool on_sent	 (int i, int len) = 0;	/// �������
	virtual bool on_closed   (int i) = 0;

	xseq_buf * get_requests(void){
		return &m_requests;
	}
	xudp_session * get_session(int i){
		if (i < 0 || i >= m_session_count)
			return 0;
		return m_psessions + i;
	}

	int send(int isession, const char * s, int len);
	int send(int isession, const char * s);

	void session_close(int i);
	bool session_isopen(int i);

protected:
	int opened_find(int * crc, SYS_INET_ADDR * addr);

	void session_open(int i);
	void session_close(xudp_session * psession);
	// �ر����ñ�ǵĵ�i_used���Ự������ʵ�ʵ����
	int  session_close_used(int i_used);

	void session_recv_reset(int i);
	int session_recv_to_request(int i);
	int session_recv(int i, int len);

	void close_all_session(void);

protected:
	xudp_session *	m_psessions;	/// �����������(�Ѿ�����������)
	int 	  * m_pused_index;	/// ��������ʹ�õ�����
	int 		m_used_count;		/// ����ʹ�õ�����
	int			m_session_count;

	xseq_buf	m_requests;			/// �����ύ����(ѭ������)
	xseq_buf	m_sends;			/// ����ظ�����

protected:
	int			m_listen_port;	/// �˿�
	char		m_serverURL[64];/// ����˵�URL��ַ
	xsys_socket m_listen_sock;	
	int 		m_session_ttl;	/// ��ʱʱ��(��)
	int			m_recv_len;		/// ÿ�ν��յ���󳤶�
	char *		m_precv_buf;	/// ���ջ���

	xsys_thread m_send_thread;	/// ���ͷ����߳�
	xsys_thread m_msg_thread;	/// ��Ϣ��������߳�
	xsys_mutex	m_lock;
};

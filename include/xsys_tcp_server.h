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
	xsys_socket	sock;	/// ͨѶ���
	char	peerip[32];	/// �Է�ip��ַ
	char	servip[32];	/// ����ip��ַ
	time_t	createTime;	/// �������ӵ�ʱ��
	long	last_trans_time;	/// ���ͨѶʱ��
	int		peerid;		/// ���ӶԷ��� id ��ʶ, ��Ӧ��ʹ��

	/// recv
	XTS_STATES	recv_state;	/// ����״̬
	char *  precv_buf;		/// ���ջ�����
	int		recv_len;		/// ��ǰ���õĻ���ռ�
	int		recv_next_len;	/// ���������ݵĳ���
	int		recv_buflen;	/// ��ǰ���峤��

	/// send
	XTS_STATES	send_state;
	char *	psend_buf;
	int		send_len;
	int		sent_len;
};

class xsys_tcp_server : public xwork_server
{
public:
	/// ����
	xsys_tcp_server();

	/// ����
	/// \param name          ��������,���Ȳ�����128
	/// \param nmaxexception �������̵��쳣����
	xsys_tcp_server(const char * name, int nmaxexception = 5);

	virtual ~xsys_tcp_server(){};

	/// �򿪷���
	/// \param listen_port  ����listen�˿�
	/// \param ttl          ͨѶ���, ��ʱ�����
	/// \param max_seesions ������߻Ự��,������ 1024(Ŀǰ��select)
	/// \param recv_len     ÿ����ͼ���ճ��ȣ���С 1024
	/// \return �ɹ����
	bool open(int listen_port, int ttl, int max_sessions, int recv_len);
	bool open(const char * url,int ttl, int recv_len);
	bool stop(int secs = 5);
	bool close(int secs = 5);	/// �ر��ͷ�

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
	xtcp_session *	m_psessions;	/// �����������(�Ѿ�����������)
	int			m_session_count;
	xseq_buf	m_requests;			/// �����ύ����(ѭ������)
	xseq_buf	m_sends;			/// ����ظ�����

	xseq_buf	m_close_requests; 	/// �ر�֪ͨ

protected:
	int			m_listen_port;
	char		m_serverURL[64];/// ����˵�URL��ַ
	xsys_socket m_listen_sock;	/// �˿�
	int 		m_session_ttl;	/// ��ʱʱ��(��)
	int			m_recv_len;		/// ÿ�ν��յ���󳤶�
	char *		m_precv_buf;	/// ���ջ���
	int			m_session_init_len;	/// session ���Գ�ʼ���Ĳ���

	xsys_mutex	m_lock;
};

#pragma once

#include <xsys_tcp_server2.h>

#define XSYSHT_SESSION_NON		0
#define XSYSHT_SESSION_ACTIVE	1
#define XSYSHT_SESSION_PASSIVE	2

#define XSYSHT_LARGE_HEAD_SIZE	(2*1024)

int http_get_prop(char * v, const char * name, const char * pheader);
int http_recv_all(
	char * d, int &response_code, int &header_len, xsys_socket & sock, int maxlen
	);

int http_date(char * d, long t);
int http_encode_post_json_header(char * d, const char * url, int data_len);
int http_encode_post_json_packet(char * d, const char * url, const char * pdata);
int http_encode_response_json_packet(char * d, int response_code, const char * pdata);

struct xhttp_session_ext{
	char * purl;
	char * phost;

	int session_type;	/// non/active/passive

	int	header_len;	/// ���հ���header
	int content_len;/// �������ݳ�
	int overall_len;/// ȫ��

	int response_code;			/// �ظ��ķ�����
};

class xhttp_server : public xsys_tcp_server2
{
public:
	xhttp_server();
	xhttp_server(const char * name, int nmaxexception);

	bool open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len);
	bool close(int secs = 5);

	// �������е�����,����ʣ��ĳ���
	virtual int  calc_msg_len(int i);
	
protected:
	xhttp_session_ext * m_pexts;
};

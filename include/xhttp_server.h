#pragma once

#include <xsys_tcp_server2.h>

#define XSYSHT_SESSION_NON		0
#define XSYSHT_SESSION_ACTIVE	1
#define XSYSHT_SESSION_PASSIVE	2

#define XSYSHT_CONTENT_TYPE_NORMAL	0
#define XSYSHT_CONTENT_TYPE_JSON	1

#define XSYSHT_SPECHARS 	" \t\r\n:\",{}"
#define IS_XHT_ENDCHARS(c)	(c == '"' || c == ',' || c == '}')
#define IS_XHT_SPECHARS(c)	(strchr(XSYSHT_SPECHARS, c))

#define XSYSHT_LARGE_HEAD_SIZE	(2*1024)

struct xhttp_session{
	xsys_socket sock;
	char		  ip_port[64];
	
	const char	* path;
	int 		  content_type;
	char		* expand_parms;

	char *	prequest_buf, *prequest_data,
		 *  presponse_buf,*presponse_data;
	int 	request_data_len,
			response_max_len;

	int response_code;
	int response_header_len,
		response_content_len;	
};

int xhttp_perform(xhttp_session &session);

// encode post packet functions
int  http_post_init(const char * url);
int  http_post_init(const char * ip_port, const char * path);
void http_set_post_content_type(int content_type = XSYSHT_CONTENT_TYPE_NORMAL);
int  http_encode_post_json_header(char * d, int data_len, const char * expand_parms);
int  http_encode_post_json_header(
	char * d, const char * url, int data_len, const char * expand_parms = ""
);
int  http_encode_post_json_packet(char * d, const char * url, const char * pdata);

// parse response prop value
int http_get_prop(char * v, const char * name, const char * pheader);
int http_recv_all(
	char * d, int &response_code, int &header_len, xsys_socket & sock, int maxlen
	);

// encode response packet functions
int http_date(char * d, long t);
int  http_encode_response_json_packet(char * d, int response_code, const char * pdata);

int json_get_value(char * v, const char * name, const char * s, int maxlen);
int json_get_value(char * v, const char * name, xhttp_session &s);

// the tcp session's extention
struct xhttp_session_ext{
	char * purl;
	char * phost;

	int session_type;	/// non/active/passive

	int	header_len;	/// 接收包的header
	int content_len;/// 接收数据长
	int overall_len;/// 全长

	int response_code;			/// 回复的返回码
};

class xhttp_server : public xsys_tcp_server2
{
public:
	xhttp_server();
	xhttp_server(const char * name, int nmaxexception);

	virtual bool open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len);
	virtual bool close(int secs = 5);

	// 接收所有的数据,返回剩余的长度
	virtual int  calc_msg_len(int i);
	
protected:
	xhttp_session_ext * m_pexts;
};

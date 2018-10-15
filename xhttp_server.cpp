#include <l_str.h>
#include <xurl.h>

#include "xhttp_server.h"

xhttp_server::xhttp_server()
	:xsys_tcp_server2()
{
	strcpy(m_name, "xhttp_server");
	m_pexts = 0;
}

xhttp_server::xhttp_server(const char * name, int nmaxexception)
	:xsys_tcp_server2(name, nmaxexception)
{
	m_pexts = 0;
}

int http_get_prop(char * v, const char * name, const char * pheader)
{
	int len = strlen(name);

	const char * p = pheader;

	while (p = strchr(p, '\n')){
		++p;
		if (*p == '\r')
			break;

		if (strnicmp(p, name, len) == 0){
			p += len+1;
			p = (const char *)skipblanks((char *)p);
			getaline(v, p);
			return strlen(v);
		}
	}

	return 0;
}

int http_recv_all(
	char * d, int &response_code, int &header_len, xsys_socket & sock, int maxlen
	)
{
	// 缓冲区不能太小，最小512
	if (maxlen < 512){
		maxlen = 512;  return 0;
	}

	// recv header
	int r, len;
	{
		const char * pend = NULL;
		for (r = len = 0; pend == NULL && len < maxlen; ){
			if ((r = sock.recv(d + len, maxlen - len - 1, 10*1000)) > 0){
				const char * startpos = (len > 4)? d+len-3 : d;	// 计算起始位置
				len += r;
				d[len] = 0; // 设置字符串结尾
				pend = strstr(startpos, "\r\n\r\n");	// 查找
			}else if (r < 0)
				return r;
		}

		header_len = int(pend + 4 - d);
	}

	// get return code
	if (strnicmp(d, "HTTP/1.", 7) == 0) {
		response_code = atoi(d+9);
	}else
		response_code = -1;

	// get content length
	char content_len[16];
	r = http_get_prop(content_len, "Content-Length", d);
	if (r <= 0)
		return len;
	int data_len = atoi(content_len);

	// recv the rest data
	int will_recv_length = min(maxlen, header_len + data_len);
	int rest_len = will_recv_length - len;
	r = sock.recv_all(d+len, rest_len, 10*1000);

	return r+len;
}

int http_date(char * d, long t)
{
	struct tm* ptm = gmtime((const time_t *)&t);
	return strftime(d, 32,"%a, %d %b %Y %H:%M:%S GMT", ptm);
}

static const char * post_json_header_format =
"POST %s HTTP/1.1\r\n"
"Accept: application/json, */*\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Accept-Language: en-US,en;q=0.8,zh-Hans-CN;q=0.6,zh-Hans;q=0.4,bo-Tibt;q=0.2\r\n"
"Accept-Encoding: deflate\r\n"
"Host: %s\r\n"
"Content-Length: %d\r\n"
"Connection: Keep-Alive\r\n"
"Cache-Control: no-cache\r\n"
"User-Agent: KTAMR Http client 1.0.1\r\n\r\n"
;

int http_encode_post_json_header(char * d, const char * url, int data_len)
{
	n_url	U;
	parseurl(&U, url);

	return sprintf(d, post_json_header_format, U.http.path, U.http.host, data_len);
}

int http_encode_post_json_packet(char * d, const char * url, const char * pdata)
{
	int data_len = strlen(pdata);
	int len = http_encode_post_json_header(d, url, data_len);
	memmove(d+len, pdata, data_len+1);

	return len + data_len;
}

static const char * phttp_response_json_header_format =
"HTTP/1.1 %03d %s\r\n"
"Date: %s\r\n"
"Server: KTAMR Http Server 1.0.1\r\n"
"Cache-Control: private, max-age=0\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Content-Length: %d\r\n\r\n"
;

int http_encode_response_json_packet(char * d, int response_code, const char * pdata)
{
	const char * presult_msg = "OK";

	if (response_code > 299)
		presult_msg = "ERROR";

	char date[32];
	http_date(date, long(time(0)));

	int datalen = strlen(pdata);
	int len = sprintf(d,
		phttp_response_json_header_format,
		response_code, presult_msg, date, datalen
	);
	strcpy(d+len, pdata);

	return len + datalen;
}

bool xhttp_server::open(int listen_port, int ttl, int max_sessions, int recv_len, int send_len)
{
	bool r = xsys_tcp_server2::open(listen_port, ttl, max_sessions, recv_len, send_len);
	if (r && m_pexts == 0)
		m_pexts = (struct xhttp_session_ext *)calloc(m_session_count, sizeof(struct xhttp_session_ext));

	return r;
}

bool xhttp_server::close(int secs)
{
	bool r = xsys_tcp_server2::close(secs);
	if (m_pexts){
		::free(m_pexts);  m_pexts = 0;
	}

	return r;
}

int xhttp_server::calc_msg_len(int i)
{
	xhttp_session_ext * pext = m_pexts + i;
	if (pext->overall_len > 0)
		return pext->overall_len;

	xtcp2_session *	psession = m_psessions + i;

	char * d = psession->precv_buf;
	if (d == 0)
		psession->recv_len = 0;

	const char * p = 0;
	if (psession->recv_len < 64 ||
		(p = strstr(d, "\r\n\r\n")) == 0)
		return max(psession->recv_len, XSYSHT_LARGE_HEAD_SIZE);

	// get header lenght
	pext->header_len = int(p + 4 - d);

	// set the header's end ch for parsing
	char endch = d[pext->header_len];
	d[pext->header_len] = 0;

	// get return code & type
	if (strnicmp(d, "HTTP/1.", 7) == 0) {
		pext->response_code = atoi(d+9);
		pext->session_type = XSYSHT_SESSION_ACTIVE;
	}else
		pext->session_type = XSYSHT_SESSION_PASSIVE;

	// get content length
	char content_len[16];
	int r = http_get_prop(content_len, "Content-Length", d);
	if (r <= 0)
		return -1;

	pext->content_len = atoi(content_len);
	pext->overall_len = pext->header_len + pext->overall_len;

	// restore the end ch
	d[pext->header_len] = endch;

	return pext->overall_len;
}

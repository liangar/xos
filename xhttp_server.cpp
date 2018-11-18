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
			}else if (r <= 0)
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
	if (rest_len > 0){
		r = sock.recv_all(d+len, rest_len, 10*1000);
	}else
		r = 0;

	d[r+len] = 0;

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
"Content-Type: %s; charset=utf-8\r\n"
"Accept-Language: en-US,en;q=0.8,zh-Hans-CN;q=0.6,zh-Hans;q=0.4,bo-Tibt;q=0.2\r\n"
"Accept-Encoding: deflate\r\n"
"Host: %s\r\n"
"Content-Length: %d\r\n";

static const char * post_json_header_format_tail =
"Connection: Keep-Alive\r\n"
"Cache-Control: no-cache\r\n"
"User-Agent: KTAMR Http client 1.0.1\r\n\r\n"
;

static n_url s_http_URL;
static const char * s_content_type,
	* XHTTP_CONTENT_TYPE_JSON	= "application/json",
	* XHTTP_CONTENT_TYPE_NORMAL = "application/x-www-form-urlencoded";

int http_post_init(const char * url)
{
	s_content_type = XHTTP_CONTENT_TYPE_JSON;

	ZeroData(s_http_URL);
	return parseurl(&s_http_URL, url);
}

int  http_post_init(const char * ip_port, const char * path)
{
	s_content_type = XHTTP_CONTENT_TYPE_JSON;

	ZeroData(s_http_URL);
	s_http_URL.n_type = urlhttp;
	strcpy(s_http_URL.http.host, ip_port);
	strcpy(s_http_URL.http.path, path);
	return 0;
}

void http_set_post_content_type(int content_type)
{
	switch(content_type){
		case XSYSHT_CONTENT_TYPE_NORMAL:  s_content_type = XHTTP_CONTENT_TYPE_NORMAL;  break;
		case XSYSHT_CONTENT_TYPE_JSON  :  s_content_type = XHTTP_CONTENT_TYPE_JSON;    break;
		default:
			s_content_type = XHTTP_CONTENT_TYPE_JSON;  break;
	}
}

int http_encode_post_json_header(
		char * d, int data_len, const char * expand_parms
	)
{
	int head_len = sprintf(d,
		post_json_header_format,
		s_http_URL.http.path, s_content_type, s_http_URL.http.host, data_len
	);

	int len = strlen(expand_parms);
	if (len > 0){
		memcpy(d+head_len, expand_parms, len);
		memcpy(d+head_len+len, "\r\n", 2);
		head_len += len + 2;
	};

	len = strlen(post_json_header_format_tail);
	memcpy(d+head_len, post_json_header_format_tail, len+1);

	return (head_len+len);
}

int http_encode_post_json_header(
		char * d, const char * url, int data_len, const char * expand_parms
	)
{
	if (http_post_init(url) < 0)
		return -1;

	return http_encode_post_json_header(d, data_len, expand_parms);
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
"Content-Type: application/json; charset=utf-8\r\n"
"Content-Length: %d\r\n"
"Cache-Control: private, max-age=0\r\n"
"Server: XHttp FastCGI Server 1.0.1\r\n"
"Date: %s\r\n\r\n"
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
		response_code, presult_msg, datalen, date
	);
	memcpy(d+len, pdata, datalen+1);

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
	pext->header_len = pext->content_len = pext->overall_len = 0;

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
	// restore the end ch
	d[pext->header_len] = endch;

	if (r <= 0)
		return -1;

	pext->content_len = atoi(content_len);
	pext->overall_len = pext->header_len + pext->content_len;
	
	if (pext->overall_len > m_recv_len)
		return -2;

	return pext->overall_len;
}

static int json_skip_name_to_colon(const char * s, const char * name, int maxlen)
{
	if (maxlen < 1)
		return -1;

	// find & skip the name and blanks until arrived the ':'
	int len = strlen(name);
	const char * pstart = s;
	do{
		s = strstr(s, name);	// find the name;
		if (s == 0)
			return -2;
		if (s > pstart && !ISBLANKCH(s[-1]) && !IS_XHT_SPECHARS(s[-1]) ||
			!IS_XHT_SPECHARS(s[len])
			)
		{
			++s;
			continue;
		}

		s += len;
		while (*s && strchr(" \t\r\n", *s) && strchr("\":", *s) == NULL)  s++;
	}while(int(s - pstart) < maxlen && *s && *s != '"' && *s != ':');

	if (int(s - pstart) >= maxlen)
		return -2;

	if (*s == '"'){
		s = skipblanks(s+1);
	}
	if (*s != ':')
		return -3;

	return int(s - pstart);
}

int json_get_value(char * v, const char * name, const char * s, int maxlen)
{
	if (v == 0 || maxlen < 1)
		return -1;

	*v = 0;

	int len = json_skip_name_to_colon(s, name, maxlen);
	if (len < 0)
		return len;

	s = skipblanks(s+len+1);
	const char * e = s;
	if (*s == '"')
		e = skip2ch(++s, '"');
	else if (*s == '{')
		e = skip2ch(++s, '}');
	else
		e = skip2chs(s, " \t,\r\n");
	
	maxlen = min(maxlen-1, int(e - s));
	memcpy(v, s, maxlen);  v[maxlen] = 0;

	return maxlen;
}

int json_get_value(char * v, const char * name, xhttp_session &s)
{
	return json_get_value(v, name, s.presponse_data, s.response_content_len);
}

int xhttp_perform(xhttp_session &s)
{
	static const char szFunctionName[] = "xhttp_perform";

	int r;
	if (!s.sock.isopen()){
		r = s.sock.connect(s.ip_port);
		if (r != 0){
			WriteToEventLog("%s: 连接(%s)出错(%d)", szFunctionName, s.ip_port, r);
			return -1;
		}
	}

	// login header
	http_post_init(s.ip_port, s.path);
	http_set_post_content_type(s.content_type);
	// login packet
	int header_len = http_encode_post_json_header(
		s.prequest_buf, s.request_data_len, s.expand_parms
	);
	memmove(s.prequest_buf+header_len, s.prequest_data, s.request_data_len+1);

	int len = header_len + s.request_data_len;
	WriteToEventLog("%s: len = %d\n%s\n", szFunctionName, len, s.prequest_buf);

	r = 0;
	int i;
	for (i = 0; i < 3 && r <= 0; i++){
		if (r == 0){
			r = s.sock.send_all(s.prequest_buf, len);
			if (r == len)
				r = http_recv_all(
					s.presponse_buf, s.response_code, s.response_header_len, s.sock,
					s.response_max_len
				);
			else
				r = 0;
		}
		if (r <= 0){
			s.sock.close();
			r = s.sock.connect(s.ip_port);
		}
	}

	if (r > 0){
		s.response_content_len = r - s.response_header_len;
		s.presponse_data = s.presponse_buf + s.response_header_len;
		WriteToEventLog(
			"%s: get [%s] response\nr = %d, response_code = %d(%d,%d)\n%s\n",
			szFunctionName, s.path, r, s.response_code, s.response_header_len, s.response_content_len, s.presponse_buf
		);
	}else
		s.response_code = 503;
	
	return s.response_code;
}

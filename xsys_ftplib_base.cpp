#include <xlist.h>
#include <xsys.h>
#include <ss_service.h>
#include <xsys_ftplib.h>

int ftp_number(char *s)
{
  return
	  (s[0] - '0') * 100 +
	  (s[1] - '0') * 10  +
	  (s[2] - '0') ;
}

int ftp_is_ok(int codenumber, va_list args)
{
	for (int r = codenumber; r > 0; ){
		r = va_arg(args, int);
		if (codenumber == r)
			return r;
	}
	return -codenumber;
}

int ftp_is_ok(int codenumber, int c0, ...)
{
	if (codenumber == c0)
		return c0;
	if (c0 <= 0) {
		return -codenumber;
	}
	va_list args;
	va_start(args, c0);
	int r = ftp_is_ok(codenumber, args);
	va_end(args);

	return r;
}

int xsys_ftp_client::ftp_send_message(char * msg)
{
	const char szFunctionName[] = "ftp_send_message";

	char s[1024*2];
	sprintf(s, "%s\r\n", msg);
	int r = m_scmd.send_all(s);
	if (r <= 0){
		WriteToEventLog("%s : error(%d), %s", szFunctionName, r, msg);
		return r;
	}
	WriteToEventLog("%s : %s, ok", szFunctionName, msg);
	
	return r;
}

int xsys_ftp_client::ftp_recv_message(void)
{
	const char szFunctionName[] = "ftp_recv_message";

	m_lastmsg[0] = 0;


	int r, maxlen = MAX_MESSAGE_BUFFER_LEN;
	while ((r = m_scmd.recv_byend(m_lastmsg, maxlen, "\r\n")) > 0 && r < maxlen){
		if (isdigit(m_lastmsg[0]) &&
			isdigit(m_lastmsg[1]) &&
			isdigit(m_lastmsg[2]) &&
			m_lastmsg[3]!='-')
			break;
		// 不然需要分析
		{
			for (char * p = strchr(m_lastmsg, '\n'); p && p[1]; p = strchr(p+1, '\n')){
				if (p[4] != '-' &&
					isdigit(p[1]) &&
					isdigit(p[2]) &&
					isdigit(p[3])
					)
				{
					WriteToEventLog("%s : %s", szFunctionName, m_lastmsg);
					return ftp_number(p+1);
				}
			}
		}
	}
	if (r > 0){
		WriteToEventLog("%s: %s", szFunctionName, m_lastmsg);
	}else{
		WriteToEventLog("%s: 接收错误%d", szFunctionName, r);
	}

	if (r < 0 || r == maxlen){
		return EXIT(QUIT);
	}
	return ftp_number(m_lastmsg);
}

int xsys_ftp_client::ftp_bye(void)
{
	return ftp_command("QUIT",0,221,EOF);
}

int xsys_ftp_client::ftp_read(void)
{
	int c;

	if ( m_mode == 'I' )
		return ftp_getc(m_sdata);

	if (m_ch != EOF){
		c = m_ch;
		m_ch = EOF;
		return c;
	}
	c = ftp_getc(m_sdata);
  
	if (c == Ctrl('M')){
		c = ftp_getc(m_sdata);
		
		if (c == Ctrl('J'))
			return '\n';
		m_ch = c;
		return Ctrl('M');
	}
	return c;
}

int xsys_ftp_client::ftp_write(char c)
{
	if (m_mode == 'I' || c != '\n' )
		return ftp_putc(m_sdata,c);
	
	ftp_putc(m_sdata,Ctrl('M'));
	return ftp_putc(m_sdata, Ctrl('J'));
}

	  
int xsys_ftp_client::ftp_getc(xsys_socket & s)
{
	char c;

	int r = s.recv_all(&c, 1, 20);
	if (r < 0){
		return r;
	}
	
	return (int)c;
}


int xsys_ftp_client::ftp_putc(xsys_socket & s, char c)
{
	return s.send_all(&c, 1, 20);
}


int xsys_ftp_client::ftp_read(char *buffer, int size)
{
	int r = m_sdata.recv(buffer, size, 30);
	if (r < 0){
		return r;
	}

	if (m_mode == 'A'){
		for (register int i = 0; i < r; i++){
			if (buffer[i]== Ctrl('M') && buffer[i+1] == Ctrl('J')){
				buffer[i++] = '\n';
				memmove(buffer + i, buffer + i + 1, r-i-1);
				--r;
			}
		}
	}
	return r;
}


int xsys_ftp_client::ftp_write(char *buffer, int size)
{
	if (m_mode=='A'){
		for(int i = 0; i < size; i++){
			if (buffer[i] == '\n'){
				buffer[i++] = Ctrl('M');
				memmove(buffer + i + 1, buffer + i, size-i);
				buffer[i] = Ctrl('J');
				size++;
			}
		}
	}
	return m_sdata.send_all(buffer, size);
}

int xsys_ftp_client::ftp_close(void)
{
	m_sdata.close();
  
	int r = ftp_recv_message();
	if (r != 226){
		if (r > 0){
			return EXIT(-r);
		}
		return r;
	}
	return EXIT(r);
}

//! 取得被动式的主机ip地址与端口
/*!
 * \param ip ip地址存放
 * \return = portnumber/<0 成功,端口号/出错码<br>
 * 分析的格式例子如下:<br>
 * ...(192,168,1,1,3,44)...
 */
int xsys_ftp_client::ftp_passive(char * ip)
{
	int r;

	if (m_ispassive){
		if ((r = ftp_command("PASV", 0, 227, EOF)) <= 0) {
			set_passive(false);
			return r;
		}
	}

	char * p = strchr(m_lastmsg + 3, '(');
	if (p == 0) {
		return -1;
	}
	++p;
	int i, l;
	for (i = l = 0; l < 4; i++) {
		if (p[i] == ',') {
			l++;
			ip[i] = '.';
		}else if (p[i] >= '0' && p[i] <= '9') {
			ip[i] = p[i];
		}else
			return -1;
	}
	if (ip[0] == '.') {
		return -1;
	}
	ip[i-1] = 0;

	char * pl = strchr(p + i, ',');
	if (pl == 0) {
		return -1;
	}
	return (atoi(p+i) << 8) + atoi(pl+1);
}

int xsys_ftp_client::ftp_passive_data(const char * cmd, const char * file)
{
	char ip[32];
	int port = ftp_passive(ip);
	WriteToEventLog("PASV port = %d", port);
	
	if (port < 0) {
		return port;
	}

	//int r = m_sdata.connect(ip, 1111, 15);
	int r = m_sdata.connect(ip, port, 15);
	if (r < 0) {
		WriteToEventLog("PASV connect %s(%d) error(%d)", ip, port, r);
		m_sdata.close();
		return r;
	}
	WriteToEventLog("PASV connect %s(%d) ok", ip, port);

	return ftp_restart(cmd, file);
}


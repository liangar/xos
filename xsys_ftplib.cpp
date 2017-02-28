#include <xsys.h>

#include <xlist.h>
#include <l_str.h>
#include <ss_service.h>

#include <xsys_ftplib.h>

static const char * ftp_ls_months[] = {
	"Jan", "Feb" , "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep",  "Oct", "Nov", "Dec"
};

void xsys_ftp_client::init(void)
{
	m_files.free_all();
	m_mode = 'B';
	m_ispassive = false;
	m_errno = 0;
	m_ch = 0;
	m_seek = 0;
	m_flags = 0;
	m_timeout = 10;
	m_port = 21;
	m_title[0] = 0;
	m_dataport = 0;
	m_dataip[0] = 0;
	m_lastmsg = new char[MAX_MESSAGE_BUFFER_LEN];
	memset(m_lastmsg,0,MAX_MESSAGE_BUFFER_LEN);
	//m_lastmsg[0] = 0;
	m_localip[0] = 0;
}

int xsys_ftp_client::connect(const char * url)
{
	m_lastmsg[0] = 0;

	int r;
	if (strchr(url, ':') == 0)
		r = m_scmd.connect(url, m_port);
	else
		r = m_scmd.connect(url);

	if (r < 0){
		printf("connect (%s) error(%d).\n", url, r);
		return r;
	}

	m_scmd.get_self_ip(m_localip);
	
	if ((r = ftp_recv_message()) < 0){
		return -1;
	}
	if ((r = ftp_is_ok(r,120,220,EOF)) < 0){
		return -2;
	}

	return 0;
}

int xsys_ftp_client::open(const char * url, const char * uid, const char * pwd)
{
xsys_ftp_client_open:

	int r = connect(url);
	if (r < 0){
		return r;
	}
	r = cmd_USER(uid);
	if (m_errno == 230 || m_errno == 332){
		printf("cmd_USER(%s) error(%d).\n", uid, m_errno);
		return m_errno;
	}

	r = cmd_PASS(pwd);

	if (r < 0){
		printf("cmd_PASS(%s) error(%d).\n", pwd, r);
		return r;
	}

	if ((r = ftp_type('I')) < 0)
		m_mode = 'A';

	if (m_ispassive){
		if ((r = ftp_command("PASV", 0, 227, EOF)) <= 0) {
			m_ispassive = false;
			m_scmd.close();

			goto xsys_ftp_client_open;
		}
	}

	char b[512];
	if ((r = sys_type(b)) > 0){
		memcpy(m_systype, b, 31);  m_systype[31] = 0;
	}else
		strcpy(m_systype, "unix");

	return r;
}

void xsys_ftp_client::close(void)
{
	if (m_scmd.isopen()){
		// ftp_bye();
		m_scmd.close();
	}
	m_sdata.close();

	m_files.free_all();
}

void xsys_ftp_client::abort(void)
{
	if (m_scmd.isopen()){
		/*
		char b[1024];
		int r;

		r = ftp_putc(m_scmd, IAC);
		r = ftp_putc(m_scmd, IP);
		if (ftp_putc(m_scmd, IP) < 0){
			return;
		}
		r = ftp_putc(m_scmd, DM);
		ftp_send_message("ABOR");
		*/
		ftp_command("ABOR", 0, 225, 226, EOF);

		/*
		while ((r = ftp_recv_message()) > 0){
			if (ftp_is_ok(r, 225, 226, EOF))
				break;
		}
		*/
	}
}

int xsys_ftp_client::ftp_command(const char * cmd, const char * param, ...)
{
	char b[128];
	if (param) {
		sprintf(b, cmd, param);
	}else
		strcpy(b, cmd);

	int r;
	if ((r = ftp_send_message(b)) < 0){
		return r;
	}
	if ((r = ftp_recv_message()) < 0){
		return r;
	}

	va_list args;
	va_start(args, param);
	r = ftp_is_ok(r, args);
	va_end(args);

	return EXIT(r);
}

int xsys_ftp_client::ftp_type(char type)
{
	char s[2];
	s[0] = type;  s[1] = 0;

	int r;
	if ((r = ftp_command("TYPE %s", s, 200, EOF)) > 0)
		m_mode=(int)type;
	return r;
}

int xsys_ftp_client::ftp_retr(const char * command, const char * inp, char * outp, bool isfile)
{
	FILE * fp = 0;

	if (*outp == 0){
		isfile = false;
	}

	if (check_flag(FTP_REST) && isfile){
		m_seek = xsys_file_size(outp);
		fp = fopen(outp, "ab+");
	}else{
		m_seek = 0;
		if (isfile){
			fp = fopen(outp, "wb+");
		}
	}

	// 如果是要接收文件,但是又无文件句柄则失败
	if (isfile && fp == 0){
		return EXIT(LQUIT);
	}

	int r;
	if ((r = ftp_data(command, inp, "r")) < 0){
		if (m_seek == 0){
			return r;
		}
		m_seek = 0;
		if (fp){
			fclose(fp);
		}
		if ((r = ftp_data(command, inp, "r")) < 0){
			return EXIT(LQUIT);
		}
	}

	int len, lensum;
	char buf[1024];
	for (len = lensum = 0; (len = ftp_read(buf, sizeof(buf)))>0;){
		if (fp)
		{
			if (int(fwrite(buf, 1, len, fp)) != len)
			{
				fclose(fp);
				return EXIT(LQUIT);
			}
		}else{
			memcpy(outp + lensum, buf, len);
			lensum += len;
			outp[lensum] = 0;
		}
	}

	if (fp)
		fclose(fp);

	return ftp_close();
}

// 通知对方可以进行数据传输的socket port
int xsys_ftp_client::ftp_port(int a,int b,int c,int d,int e,int f)
{
	char ports[128];
	sprintf(ports, "%d,%d,%d,%d,%d,%d",a,b,c,d,e,f);

	return ftp_command("PORT %s", ports, 200, EOF);
}

static int MscGetServerAddress(char const *purl, SYS_INET_ADDR & SvrAddr, int iport)
{

	char url[MAX_HOST_NAME] = "";
	strcpy(url, purl);

	char * p;	int port;
	
	port = iport;
	if ((p = strchr(url, ':')) != NULL){
		*p++ = '\0';
		port = atoi(p);
		if (url[0] == '!') {
			strcpy(url, "127.0.0.1");
		}
	}

	int r;
	SysInetAnySetup(SvrAddr, AF_INET, iport);

	if (((r = SysGetHostByAddr(SvrAddr, url, sizeof(url))) < 0) &&
		((r = SysGetHostByName((const char *)url, AF_INET, SvrAddr)) < 0))
		return r;

	SysSetAddrPort(SvrAddr, port);

	return (0);
}

#ifndef SYS_IN4
#define SYS_IN4(a)		((struct sockaddr_in *) (a)->Addr)
#endif

int xsys_ftp_client::ftp_data(const char * cmd, const char * file, const char * mode)
{
	int r;
	//*
	if (m_ispassive) {
		r = ftp_passive_data(cmd, file);
		if (r >= 0)
			return r;
	}
	//*/
	
	FtpString hostname;
	/*
	if(m_dataip[0] == 0){
	if (gethostname(hostname , sizeof(hostname) ) == -1)
		return EXIT(QUIT);
	}else{
		strcpy(hostname,m_dataip);
	}
	*/
	SYS_INET_ADDR data, from;

	xsys_socket xaccept;
	if (xaccept.listen(0, 1) < 0)
		return EXIT(QUIT);

	if (SysGetSockInfo(xaccept.m_sock, data) < 0)
		return EXIT(QUIT);

	//sprintf(hostname, "%s:%d", hostname, data.Addr.sin_port);
	sprintf(hostname,"%s:%d",m_localip, SYS_IN4(&data)->sin_port);
	MscGetServerAddress(hostname, data, 0);

	// EL_WriteHexString((char *)(&data), sizeof(data));

	char * a = (char *) & SYS_IN4(&data)->sin_addr;
	char * b = (char *) & SYS_IN4(&data)->sin_port;

	r = ftp_port(CUT(a[0]),CUT(a[1]),CUT(a[2]),CUT(a[3]),CUT(b[0]),CUT(b[1]));
	if (r < 0){
		return r;
	}

	if ((r = ftp_restart(cmd, file)) < 0) {
		return r;
	}

	int fromlen = sizeof(from);
	SYS_SOCKET Accepted_Socket;
	if ((Accepted_Socket = SysAccept(xaccept.m_sock, &from, 15)) < 0){
		return EXIT(QUIT);
    }
	xaccept.close();

	m_sdata.m_sock = Accepted_Socket;
	
	set_passive(false);

	return 0;
}

int xsys_ftp_client::ftp_restart(const char * cmd, const char * file)
{
	const char szFunctionName[] = "ftp_restart";
	int r;
	if (m_seek > 0){
		if ((r = ftp_command("REST %d", (const char *)m_seek , 0, EOF)) != 350 ){
			if (r > 0){
				WriteToFileLog("%s : cannot seek to (%s,%d) by REST command",
					szFunctionName,
					file, m_seek
				);
				return -r;
			}
			WriteToFileLog("%s : seek to (%s,%d) ok",
				szFunctionName,
				file, m_seek
			);
			return r;
		}
    }
	if ((r = ftp_command(cmd, file,
			     200, 120, 150, 125, 250, EOF)) < 0)
	{
		return r;
	}

	return r;
}

int xsys_ftp_client::ftp_stor(const char * cmd, const char * inp, char * outp)
{
	if (check_flag(FTP_REST) && (m_seek = filesize(outp)) <0)
		m_seek = 0;

	if (ftp_data(cmd, outp, "w") < 0)
	{
		if (m_seek == 0)
			return -1;

		m_seek = 0;
		if (ftp_data(cmd, outp, "w") < 0)
			return -1;
	}
	FILE * fp;
	if ((fp = fopen(inp,"rb")) == NULL)
		return EXIT(LQUIT);

	if (m_seek)
		fseek(fp, m_seek, 0);

	char buffer[8 * 1024 + 8];
	int r;
	while ((r = fread(buffer, 1, sizeof(buffer)-8, fp)) > 0){
		if ((r = ftp_write(buffer, r)) < 0){
			fclose(fp);
			ftp_close();
			return r;
		}
	}

	fclose(fp);
	return ftp_close();
}

int xsys_ftp_client::filesize(const char * filename)
{
	int r = ftp_command("SIZE %s", filename, 213, EOF);
	if (r >= 0)
	{
		sscanf(m_lastmsg,"%*d %d",&r);
		return r;
	}

	return r;
}

int xsys_ftp_client::chg_mod(const char *file, int mode)
{
	char msg[128];
	sprintf(msg, "SITE CHMOD %03o %s", mode, file); 
	return ftp_command(msg, "", 200, EOF); 
}

int xsys_ftp_client::rename(const char * oldname, const char * newname )
{
	int r;
	if ((r = ftp_command("RNFR %s", oldname, 200, 350, EOF)) > 1 )
		return ftp_command("RNTO %s", newname, 200, 250, EOF);
	return r;
}

int xsys_ftp_client::sys_type(char * type)
{
	int r = ftp_command("SYST", 0, 215, 200, 0, EOF);
	if (r < 0){
		return r;
	}
	if (r != 215 && r != 200){
		strcpy(type, "UNKNOWN");
	}
	const char * p = getaword(type, m_lastmsg, " \t");
	getaword(type, p, " \t");
	return r;
}

int xsys_ftp_client::file_ls(char * filename)
{
	char * pbuf = new char[64 * 1024];
	pbuf[0] = 0;
	int r = ls(filename, pbuf);
	if (r < 0)
		return r;

	;
	m_files.free_all();

	const char * p = pbuf;
	for (m_ifile = -1; *p; ){
		char line[1024];
		p = getaline(line, p);

		const char * q = getaword(line, line, " \t");
		if (strlen(line) != 10)
			continue;

		FTP_FILE_STAT f;
		memset(&f, 0, sizeof(f));
		strcpy(f.mode, line);

		q = getaword(line, q, " \t");
		f.inodes = atoi(line);

		q = getaword(f.user, q, " \t");
		
		q = getaword(f.group, q, " \t");
		q = getaword(line, q, " \t");
		f.size = atoi(line);

		q = getaword(line, q, " \t");
		int mon = 0;
		for (; strcmp(line, ftp_ls_months[mon]) != 0 && mon < 12;  mon++)
			;
		if (mon < 12)
			++mon;

		q = getaword(line+16, q, " \t");
		q = getaword(line, q, " \t");
		sprintf(f.filetime, "%02d-%s %s", mon, line+16, line);

		if (f.mode[0] == 'l') {
			char * pline = (char *)strstr(q, "->");
			if (pline) {
				trim_all(f.name, (char *)q);
				*pline = 0;
				pline += 2;
				trim_all(f.link, pline);
			}else
				strcpy(f.name, q);
		}else
			strcpy(f.name, q);

		m_files.add(&f);
	}

	delete[] pbuf;

	return m_files.m_count;
}

int xsys_ftp_client::file_next(char * filename)
{
	if (m_files.m_count <= 0 || m_ifile >= m_files.m_count - 1)
		return 100;

	if (m_ifile < 0 && m_files.m_count > 0)
		m_ifile = 0;
	else
		m_ifile++;

	strcpy(filename, m_files[m_ifile].name);
	return 0;
}

char xsys_ftp_client::file_filetype(void)
{
	if (m_ifile >= 0 && m_ifile < m_files.m_count) {
		return m_files[m_ifile].mode[0];
	}
	return 0;
}

bool xsys_ftp_client::isfile(void)
{
	if (m_ifile >= 0 && m_ifile < m_files.m_count) {
		if (m_files[m_ifile].mode[0] == '-')
			return true;
	}
	return false;
}

int xsys_ftp_client::file_size(void)
{
	if (m_ifile >= 0 && m_ifile < m_files.m_count) {
		return m_files[m_ifile].size;
	}
	return -1;
}

void xsys_ftp_client::file_ls_close(void)
{
	m_ifile = -1;
	m_files.free_all();
}


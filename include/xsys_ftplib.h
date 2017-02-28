/* ! \file xsys_ftplib.h
 * This library is designed for simple FTP client.
 */
#ifndef __XSYS_FTPLIB_H_
#define __XSYS_FTPLIB_H_

#include "xlist.h"

enum {
	FTP_REST = 1,
	FTP_NOEXIT = 2,
	FTP_HANDLERS = 4
};
		
typedef int STATUS;
typedef char FtpString[256];

struct FTP_FILE_STAT
{
	char mode[12];
	int  inodes;
	char user[64];
	char group[64];
	int  size;
	char filetime[16];
	char name[128];
	char link[128];
} ;

#define MAX_ANSWERS 10 /* Number of known goodest answers for reqest */
#define FTP_NFDS 64
#define FTPBUFSIZ BUFSIZ
#define LQUIT (-6)
#define QUIT (-5) /* Few time ago QUIT character been 
		     equivalence to zero, changed for clear
		     conflicts with reading functions */
#define OK (0)    /* Synonym for neitral good answer */

#define Ctrl(x) ((x) - '@')
#define CUT(x) ((x)&0xff)

///
/// \class xsys_ftp_client
/// FTP客户端
/// \brief 成员函数的返回值的绝对值一般为FTP服务最后的返回码<br>
/// 返回的负数,就表明失败
///

#define MAX_MESSAGE_BUFFER_LEN 8*1024

class xsys_ftp_client
{
public :
	xsys_ftp_client()  {  init();  }
	~xsys_ftp_client() {  close(); delete []m_lastmsg;}

	/// 打开FTP服务器
	/// \param url FTP服务器地址(<host>[:port])
	/// \param uid 用户名
	/// \param pwd 密码
	/// \return >=0/<0 = 成功/出错码
	/// \brief 返回值的绝对值一般为FTP服务最后的返回码
	int open(const char * url, const char * uid, const char * pwd);

	/// 连接FTP服务器
	int connect(const char * url);
	int cmd_USER(const char * uid)  {  return ftp_command("USER %s",uid,230,331,332,EOF);  }
	int cmd_PASS(const char * pwd)  {  return ftp_command("PASS %s",pwd,230,332,EOF);  }
	void abort(void);

	void close(void);

	int ascii(void)  {  return ftp_type('A');  }
	int binary(void) {  return ftp_type('I');  }

	// COMMAND
	int cd(const char * dir)	{  return ftp_command("CWD %s", dir,200,250,EOF);  }
	int mk(const char * dir)	{  return ftp_command("MKD %s", dir,200,257,EOF);  }
	int rm(const char * dir)	{  return ftp_command("DELE %s",dir,200,250,EOF); }

	// RETR
	int get(const char * inp, char * outp)     {  return ftp_retr("RETR %s",inp, outp, true);  }
	int ls(const char * path, char * files_buf){  return ftp_retr("LIST %s",path, files_buf, false);  }
	int ls(char * files_buf)	{  return ftp_retr("LIST %s",".",files_buf, false);  }

	// STOR
	int put(const char * inp, char * outp)      {  return ftp_stor("STOR %s",inp,outp);  }

	// DATA
	int read(const char * filename)  {  return ftp_data("RETR %s",filename,"r");  }
	int write(const char * filename) {  return ftp_data("STOR %s",filename,"w");  }
	int append(const char * filename){  return ftp_data("APPE %s",filename,"r");  }

	// 
	char * pwd(void);
	char * syst(void);
	int filesize(const char * filename);
	int rename(const char * oldname, const char * newname);

	///*
	int  file_ls(char * filename);
	int  file_next(char * filename);
	char file_filetype(void);
	bool isfile(void);
	int  file_size(void);
	void file_ls_close(void);
	//*/

	int lasterror(void)  {  return m_errno;  }

	bool get_passive(void)  {  return m_ispassive;  }
	void set_passive(bool ispassive)  {  m_ispassive = ispassive;  }
	void set_dataip(const char* dataip)  {  strcpy(m_dataip,dataip);  }
	void set_dataport(int port)  {  m_dataport = port;  }
protected:
	int ftp_command(const char * cmd, const char * param, ...);
	int ftp_bye(void);
	int ftp_type(char type);
	int ftp_retr(const char * cmd, const char * inp, char * outp, bool isfile);
	int ftp_stor(const char * cmd, const char * inp, char * outp);
	int ftp_data(const char * cmd, const char * parm,const char * mode);
	int ftp_recv_message(void);
	int ftp_send_message(char * msg);
	int ftp_close(void);

	int ftp_restart(const char * cmd, const char * file);

	int ftp_read(void);
	int ftp_read(char *buffer, int size);
	int ftp_getc(xsys_socket & s);
	int ftp_putc(xsys_socket & s, char c);
	int ftp_write(char c);
	int ftp_write(char *buffer, int size);

	int chg_mod(const char *file, int mode);
	int ftp_port(int a,int b,int c,int d,int e,int f);
	int sys_type(char * type);

	int ftp_passive(char * ip);
	int ftp_passive_data(const char * cmd, const char * file);

public:
	//char m_lastmsg[1024*8];	//ivy
	char *m_lastmsg;

protected:
	void init(void);
	void set_flag(int flag)  {  m_flags |= flag;  }
	void clear_flag(int flag){  m_flags &= (~flag);  }
	bool check_flag(int flag){  return ((m_flags & flag) == flag);  }
	void set_timeout(int secs){  m_timeout = secs;  }
	void set_port(int port)  {  m_port = port;  }

	int  EXIT(int e) {  return (m_errno = e);  }
protected:
	xsys_socket	m_scmd,	/// Command socket
				m_sdata; /// Data socket
	xlist<FTP_FILE_STAT> m_files;
	int  m_ifile;

	char m_systype[32];
	char m_mode;		/// Binary, Ascii, .........
	bool m_ispassive;	/// 是否是被动FTP 
	int  m_errno;		/// Last error code
	int  m_ch;			/// Help character for ascii streams

	int m_seek;			/// 续传位置,如不支持 REST,则永为0
	int m_flags;		/// FTP_REST, FTP_NOEXIT, FTP_HANDLERS
	int m_timeout;		/// How long must be waiting next character from server

	int m_port;
	FtpString m_title;	/// Using for FtpLog, FtpConnect lets hostname
	char m_dataip[64];  /// for data link ipaddress
	int m_dataport;     /// for data link port
	char m_localip[64];	/// 已建立连接所用的ip地址
};

int ftp_number(char *s);
int ftp_is_ok(int codenumber, va_list args);
int ftp_is_ok(int codenumber, int c0, ...);

#endif /* __XSYS_FTPLIB_H_ */


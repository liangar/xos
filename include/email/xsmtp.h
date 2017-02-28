#ifndef _xsmtp_H_
#define _xsmtp_H_

#include "Smtp.h"

/// SMTP消息发送
class xsmtp
{
public:
	xsmtp();
	~xsmtp();

	/// 打开连接
	/// \param host 主机地址,可以带端口
	/// \param uid 账号
	/// \param pwd 密码
	/// \return 0/-1 = ok/error
	int  open(const char * host, const char * uid, const char * pwd);
	int  open();

	/// 关闭连接
	void close();
	int  reset();

	/// 发送
	/// \param title 标题
	/// \param msg 正文内容
	/// \param file 附件文件名
	/// \return 0/errorCode = ok/SMTP服务器返回的出错码
	int send(const char * title, const char * msg, const char * file);
	int send(const char * msg);		/// 发送数据(文件)
	int send(const char * from,
			 const char * to,
			 const char * title,
			 const char * msg,
			 const char * file);
	int send();

	void sethost(const char * ip)		{  strcpy(m_host, ip);	}
	void setport(int port)				{  m_smtp.setport(port);  }
	void setuid(const char * uid)		{  strcpy(m_smtp.m_uid, uid);  }
	void setpassword(const char * pwd)	{  strcpy(m_smtp.m_pwd, pwd);  }

	void setfrom(const char * from);		/// 设置缺省的from
	void setto(const char * to);			/// 设置缺省的to
	void settitle(const char * title);		/// 设置缺省邮件标题
	int  setfile(const char * file);
	void setcc(const char * cc);

	void getmsg(char * d, int maxlen);

	void setnull()
	{
		m_to[0]    = '\0';
		m_title[0] = '\0';
		m_file[0]  = '\0';
		m_cc[0]	   = 0;
	}

protected:
	CSmtp	m_smtp;

	char	m_host[128];
	char	m_from[256];
	char	m_to[2048];
	char	m_title[1024];
	char	m_uid[64];
	char	m_pwd[64];
	char	m_file[1024];
	char	m_cc[2048];
};

#endif // _xsmtp_H_

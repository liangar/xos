#ifndef _xsmtp_H_
#define _xsmtp_H_

#include "Smtp.h"

/// SMTP��Ϣ����
class xsmtp
{
public:
	xsmtp();
	~xsmtp();

	/// ������
	/// \param host ������ַ,���Դ��˿�
	/// \param uid �˺�
	/// \param pwd ����
	/// \return 0/-1 = ok/error
	int  open(const char * host, const char * uid, const char * pwd);
	int  open();

	/// �ر�����
	void close();
	int  reset();

	/// ����
	/// \param title ����
	/// \param msg ��������
	/// \param file �����ļ���
	/// \return 0/errorCode = ok/SMTP���������صĳ�����
	int send(const char * title, const char * msg, const char * file);
	int send(const char * msg);		/// ��������(�ļ�)
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

	void setfrom(const char * from);		/// ����ȱʡ��from
	void setto(const char * to);			/// ����ȱʡ��to
	void settitle(const char * title);		/// ����ȱʡ�ʼ�����
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

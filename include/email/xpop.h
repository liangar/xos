#ifndef _xpop_H_
#define _xpop_H_

#include "Pop3.h"
#include "xemail.h"

class xpop
{
public:
	xpop();
	~xpop();

public:
	int open();
	int open(const char * host, const char * uid, const char * pwd, int port = 110);
	int close();

	int get_date	(char * v, int maxlen = 256);
	int get_cc		(char * v, int maxlen = 256);
	int get_to		(char * v, int maxlen = 256);
	int get_from	(char * v, int maxlen = 256);

	int count(void);
	xmail * recvmail(long mail_index, bool bwill_delete);
	bool deletemail(int i);

	xlist<mailpart> * get_parts()  {  return &m_mailmsg.m_parts;  }	
	const char * save_attach(int i, const char * path);
	mailpart * get_part(const char * filename);

	int  get_timeout();
	int  set_timeout(int ms);

	int	 get_port();
	void set_port(int port);

	const char *    getbuf()  {  return m_mailmsg.m_pbuf;  }

	const char *	get_host();
	void			set_host(const char * host);
	const char *	get_password();
	void			set_password(const char * pwd);
	const char *	get_user();
	void			set_user(const char * user);
	int 			get_subject(char * v, int maxlen);

	int				geterror(char * msg, int maxlen);

private:
	CPop3Connection*	m_p3;
	char				m_host[128];
	int 				m_port;
	char				m_user[128];
	char				m_pwd[128];

	int					m_count, m_size;

	xmail				m_mailmsg;
};

#endif // _xpop_H_

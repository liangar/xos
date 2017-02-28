// POPLevel.cpp : Implementation of xpop
#include <xpop.h>

#include <ss_service.h>

/////////////////////////////////////////////////////////////////////////////
// xpop

xpop::xpop()
{
	m_host[0] = '\0';

	m_p3 = new CPop3Connection;
	m_count = m_size -1;
}

xpop::~xpop()
{
	if (m_p3){
		delete m_p3;  m_p3 = 0;
	}
	m_mailmsg.close();
}


const char * xpop::get_host()
{
	return (const char *)m_host;
}

void xpop::set_host(const char * host)
{
	strcpy(m_host, host);
}

int xpop::get_port()
{
	return m_port;
}

void xpop::set_port(int port)
{
	m_port = port;
}

const char * xpop::get_user()
{
	return (const char *)m_user;
}

void xpop::set_user(const char * user)
{
	strcpy(m_user, user);
}

const char * xpop::get_password()
{
	return (const char *)m_pwd;
}

void xpop::set_password(const char * pwd)
{
	strcpy(m_pwd, pwd);
}

int xpop::get_timeout()
{
	return int(m_p3->GetTimeout());
}

int xpop::set_timeout(int ms)
{
	if (m_p3){
		m_p3->SetTimeout(SYS_UINT32(ms));
	}

	return 0;
}

int xpop::geterror(char * msg, int maxlen)
{
#ifdef WIN32
	SYS_UINT32 dwError = GetLastError();
#else
	SYS_UINT32 dwError = errno;
#endif

	if (msg) {
		if (maxlen < 0 || maxlen > 4 * 1024)
			maxlen = 4 * 1024;
		strncpy(msg, m_p3->GetLastCommandResponse().c_str(), maxlen);
	}
	return int(dwError);
}

int xpop::open()
{
	bool blRet = m_p3->Connect(m_host, m_user, m_pwd, m_port);
	if(blRet == false)
		return -1;
	if (!m_p3->Noop())
	{
		return -2;
	}

	if (!m_p3->Statistics(m_count, m_size))
	{
		return -3;
	}
	return 0;
}

int xpop::open(const char * host, const char * uid, const char * pwd, int port)
{
	set_host(host);
	set_user(uid);
	set_password(pwd);
	set_port(port);

	int r = open();
	if (r){
		WriteToEventLog("xpop::open : cannot open pop3(%s, %s, %s)", m_host, m_user, m_pwd);
	}
	return r;
}

int xpop::close()
{
	m_mailmsg.close();
	bool blRet = m_p3->Disconnect();
	if(blRet == false)
		return -1;
	m_count = m_size = -1;
	return 0;
}

int xpop::count(void)
{
	return m_count;
}

xmail * xpop::recvmail(long mail_index, bool will_delete)
{
	try
	{
		if (m_count <= 0)
			return 0;
		else if (mail_index > m_count)
			return 0;

		m_mailmsg.close();

		// get mail message
		if (!m_p3->Retrieve(mail_index, &m_mailmsg.m_pbuf)){
			WriteToEventLog("retrieve mail error: {%s}", m_p3->GetLastCommandResponse().c_str());
			return 0;
		}

		m_mailmsg.parse_header();
		WriteToEventLog("get %d:\n{\nfrom: %s\nto: %s\ntitle: %s\nencodetype: %s\n}\n", mail_index,
			m_mailmsg.m_from,
			m_mailmsg.m_to,
			m_mailmsg.m_title,
			m_mailmsg.m_content_type);
	}catch (...) {
		WriteToEventLog("recvMail Exception");
		return 0;
	}

	return &m_mailmsg;
}

bool xpop::deletemail(int i)
{
	return m_p3->Delete(i);
}

int xpop::get_from(char * v, int maxlen)
{
	if (v){
		*v = '\0';
		if (m_mailmsg.m_from)
			strncpy(v, m_mailmsg.m_from, maxlen);
	}
	return 0;
}

int xpop::get_to(char * v, int maxlen)
{
	if (v){
		*v = '\0';
		if (m_mailmsg.m_to)
			strncpy(v, m_mailmsg.m_to, maxlen);
	}
	return 0;
}

int xpop::get_cc(char * v, int maxlen)
{
	if (v){
		*v = '\0';
		if (m_mailmsg.m_cc)
			strncpy(v, m_mailmsg.m_cc, maxlen);
	}
	return 0;
}

int xpop::get_date(char * v, int maxlen)
{
	if (v){
		*v = '\0';
		if (m_mailmsg.m_date)
			strncpy(v, m_mailmsg.m_date, maxlen);
	}
	return 0;
}

int xpop::get_subject(char * v, int maxlen)
{
	if (v){
		*v = '\0';
		if (m_mailmsg.m_title)
			strncpy(v, m_mailmsg.m_title, maxlen);
	}
	return 0;
}

const char * xpop::save_attach(int i, const char * path)
{
	return m_mailmsg.save_attach(i, path);
}

mailpart * xpop::get_part(const char * filename)
{
	for (int i = 0; i < m_mailmsg.m_parts.m_count; i++) {
		if (strcmp(m_mailmsg.m_parts[i].filename, filename) == 0)
			return &m_mailmsg.m_parts[i];
	}
	return 0;
}


#include <xsmtp.h>

#include <l_str.h>
#include <ss_service.h>
xsmtp::xsmtp()
{
	m_uid[0]   = '\0';
	m_pwd[0]   = '\0';
	m_from[0]  = '\0';
	setnull();
}

xsmtp::~xsmtp()
{
	close();
}

int xsmtp::open(const char * host, const char * uid, const char * pwd)		// 打开连接
{
	setuid(uid);
	setpassword(pwd);
	sethost(host);
	return open();
}

int xsmtp::open()
{
	return m_smtp.Connect(m_host)? 0 : -1;
}

void xsmtp::close()					// 关闭连接
{
	m_smtp.Close();
	setnull();
}

int xsmtp::reset()
{
	close();
	return open();
}

int xsmtp::send(const char * title, const char * msg, const char * file)
{
	settitle(title);
	setfile(file);
	return send(msg);
}

int xsmtp::send(const char * message)		// 发送数据(文件)
{
	static const char szFunctionName[] = "xsmtp::send";

	CSmtpMessage msg;
	msg.Subject = m_title;

	char word[256];
	getaword(word, m_from, '@');
	msg.Sender.Name = word;

	if (strcmp(word, m_from) == 0){
		sprintf(word, "%s@%s", m_from, m_host);
		msg.Sender.Address = word;
	}else
		msg.Sender.Address = m_from;

	/*
	getaword(word, m_to, '@');
	msg.Recipient.Name = word;
	msg.Recipient.Address = m_to;
	//*/

	// TO
	const char * p;
	for (p = m_to; *p; ){
		p = getaword(word, p, ',');
		
		char name[256];
		const char * q = getaword(name, word, '@');
		CSmtpAddress addr;
		addr.Name = name;
		addr.Address = word;
		
		msg.Recipient.push_back(addr);
	}

	// cc
	for (p = m_cc; *p; ){
		p = getaword(word, p, ',');

		char name[256];
		const char * q = getaword(name, word, '@');
		CSmtpAddress addr;
		addr.Name = name;
		addr.Address = word;

		msg.CC.push_back(addr);
	}

	// smtp donot support null message
	if (message == 0 || *message == '\0') {
		message = m_title;
	}

	CSmtpMessageBody body = message;
	msg.Message.push_back(body);
	
	body.Encoding = "application/octet-stream";

	if (m_file[0] != '\0'){
		msg.MimeType = mimeMixed;
	}
	
	if (m_file[0] != '\0'){
		CSmtpAttachment setfile = m_file;

		sprintf(word, "xsmtp__%d", time(0));
		setfile.ContentId = word;

		msg.Attachments.push_back(setfile);
	}

	int rt = m_smtp.SendMessage(msg);
	return rt;
}

int xsmtp::send (const char * from,
				const char * to,
				const char * title,
				const char * msg,
				const char * file)
{
	setfrom(from);
	setto(to);
	settitle(title);
	setfile(file);
	return send(msg);
}

int xsmtp::send()
{
	return send("");
}


void xsmtp::setfrom(const char * from)		// 设置缺省的from
{
	if (from){
		strcpy(m_from, from);
	}
}

void xsmtp::setto(const char * to)			// 设置缺省的to
{
	if (to && *to) {
		if (m_to[0] == 0) {
			strcpy(m_to, to);
		}else{
			size_t l = strlen(m_to);
			m_to[l] = ',';
			strcpy(m_to + l + 1, to);
		}
	}
}

void xsmtp::settitle(const char * title)		// 设置缺省邮件标题
{
	if (title){
		strcpy(m_title, title);
	}
}

int xsmtp::setfile(const char * file)
{
	if (file){
		strcpy(m_file, file);
	}
	return 0;
}

void xsmtp::getmsg(char * d, int maxlen)
{
	m_smtp.GetError(d, maxlen);
}

void xsmtp::setcc(const char * cc)
{
	if (cc && *cc) {
		if (m_cc[0] == 0) {
			strcpy(m_cc, cc);
		}else{
			size_t l = strlen(m_cc);
			m_cc[l] = ',';
			strcpy(m_cc + l + 1, cc);
		}
	}
}

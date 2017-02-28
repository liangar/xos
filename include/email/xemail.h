#ifndef _xmail_H_
#define _xmail_H_

#include <xlist.h>

struct mailpart
{
	char * encodetype;
	char * filename;
	char * pbody;
	int  len;
};

class xmail
{
public:
	xmail();
	~xmail();

	void close(void);

	void parse_header(char * s = 0);
	int  parse_parts(void);

	/// 保存附件到指定的目录并返回文件名称
	/// 如果不是文件,就返回0
	const char * save_attach(const mailpart * part, const char * path);
	const char * save_attach(int i, const char * path);

protected:
	void getitem(char * n);
	void getvalue(char * v);
	char * finditem(const char * name, char * buf = 0);
	void getall(void);

	void parse_part(mailpart & part);

public:
	char * m_from;
	char * m_to;
	char * m_cc;
	char * m_title;	
	char * m_date;
	char * m_mimever;
	char * m_boundary;
	char * m_content_type;
	char * m_msgid;
	char * m_encode_type;
	char * m_filename;

	xlist<mailpart> m_parts;

	char * m_pbuf;

protected:
	char * m_pheaderBuf;
	char * m_pbody;
};

#endif // _xmail_H_


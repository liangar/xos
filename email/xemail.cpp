#include <xsys.h>

#include <string.h>

#include <l_str.h>
#include <lfile.h>
#include <ss_service.h>

#include <xbase64.h>
#include <uu_code.h>

#include <xemail.h>

// Content-Transfer-Encoding
const char xmime_uuencode[] = "uuencode";
const char xmime_xxencode[] = "xxencode";
const char xmime_text[] = "text";

xmail::xmail()
{
	m_encode_type = m_filename = 
	m_from = m_to = m_cc =
	m_title = m_date = m_mimever =
	m_boundary = m_content_type = m_msgid = 0;

	m_pheaderBuf = 0;
	m_pbody = 0;

	m_pbuf = 0;
}

xmail::~xmail()
{
	close();
}

void xmail::close(void)
{
	if (m_pheaderBuf){
		delete[] m_pheaderBuf;  m_pheaderBuf = 0;
	}
	if (m_pbuf){
		free(m_pbuf);  m_pbuf = 0;
	}
	m_parts.free_all();
}

// 查找名称对应值的所在地址
char * xmail::finditem(const char * name, char * buf)
{
	if (buf == 0) {	// 缺省在headerbuf中查找
		buf = m_pheaderBuf;
		if (buf == 0) {
			return 0;
		}
	}

	int l = int(strlen(name));
	for (char * p = buf; *p; ){
		// 找不到
		if ((p = strstr(p, name)) == 0)
			break;

		// 检查是否是单词
		if (p == buf || ISBLANKCH(p[-1]) || (!isdigit(p[-1]) && !isupper(p[-1]) && !islower(p[-1]))){
			return p + l;
		}
		p += l;
	}
	return 0;
}


void xmail::getitem(char * i)
{
	if (i) {
		char * s = skipblanks(i);
		if (*s == '"'){
			getaword(i, s+1, "\"\r\n");
		}else
			getaword(i, i, ";\r\n");
		trim_all(i, i, " \t\r\n");
	}
}

void xmail::getvalue(char * v)
{
	if (v){
		getaword(v, v, " \t\"\r\n");
	}
}

void xmail::getall(void)
{
	getitem(m_from);
	getitem(m_to);
	getitem(m_cc);
	getitem(m_title);
	getitem(m_date);
	getitem(m_mimever);
	getitem(m_msgid);
	getitem(m_content_type);
	getitem(m_encode_type);

	getitem(m_filename);

	getvalue(m_boundary);

	// 检查title是否是base64编码，如是则解码
	int l = int(strlen(m_title));
	if (l >= 16 && *m_title == '=' && m_title[1] == '?'){
		char * pend = m_title + l;
		while (!isBase64Char(pend[-1]) && pend > m_title)
			pend --;
		if (pend > m_title) {
			char * pbegin = pend - 1;
			while (isBase64Char(pbegin[-1]))
				pbegin --;

			int len = dw_b64decode(m_title, pbegin, int(pend - pbegin));
			m_title[len] = '\0';
		}
	}
}

void xmail::parse_part(mailpart & m)
{
	if (m.encodetype){
		if (strcmp(m.encodetype, "base64") == 0) {
			m.len = dw_b64decode(m.pbody, m.pbody, m.len);
		}else if (strcmp(m.encodetype, "quoted-printable") == 0) {
			strreplace(m.pbody, "=\r\n", "");
			strreplace(m.pbody, "=0A", "\r\n");
			m.len = int(strlen(m.pbody));
			if (m.pbody[m.len - 1] == '=') {
				m.len--;
				m.pbody[m.len] = 0;
			}
		}else if (strcmp(m.encodetype, xmime_uuencode) == 0 ||
			strcmp(m.encodetype, xmime_xxencode) == 0)
		{
			m.len = uu_decode(m.pbody, m.pbody);
		}
	}
	if (m.filename) {
		_strupr(m.filename);
		char * p = strrchr(m.filename, '/');
		if (p == 0)
			p = strrchr(m.filename, '\\');
		if (p)
			m.filename = p + 1;
	}
}

// 解析邮件头,分析出所有的属性字段
void xmail::parse_header(char * s)
{
	if (s == 0){
		s = m_pbuf;
		if (s == 0){
			return;
		}
	}

	char * p = strstr(s, "\r\n\r\n");
	if (!p){
		m_pbody = s + strlen(s);
	}else{
		m_pbody = p + 2;
	}
	int l = int(m_pbody - s);
	m_pheaderBuf = new char[l+1];
	memcpy(m_pheaderBuf, s, l);
	m_pheaderBuf[l] = '\0';

	m_from	= finditem("From:");
	m_to	= finditem("To:");
	m_cc	= finditem("CC:");
	m_title = finditem("Subject:");
	m_date	= finditem("Date:");
	m_mimever = finditem("MIME-Version:");
	m_boundary= finditem("boundary=");
	if (m_boundary == 0)
		m_boundary= finditem("Boundary=");
	m_content_type = finditem("Content-Type:");
	if (m_content_type == 0)
		m_content_type = finditem("Content-type:");
	m_msgid = finditem("Message-Id:");
	m_encode_type  = finditem("Content-Transfer-Encoding:");
	m_filename     = finditem("filename=");
	if (m_filename == 0)
		m_filename = finditem("name=");

	getall();
}

// 用mailpart解析,对于base64的重新解析生成代码

// 对于有分隔行的,开始分隔之后,有头说明,以空行和内容相分隔
// 头说明中有编码类型与文件名称的,为附件
// 无分隔行的,很可能是uu/xxencode类型,进行猜测判断解码,只支持644模式的
int xmail::parse_parts(void)
{
	static const char szFunctionName[] = "xmail::parse_parts";

	if (!m_pbody || *m_pbody == '\0'){
		return 0;
	}

	char * pbegin, * pend;
	if (!m_boundary) {
		mailpart m;
		const char * p;

		if ((pbegin = strstr(m_pbody, "begin 6")) != 0 && isdigit(pbegin[7]) && isdigit(pbegin[8]) && pbegin[9] == ' ') {
			m.filename = pbegin + 10;
			const char * p = getaline(m.filename, m.filename);
			if (*p == 'M') {
				m.encodetype = (char *)xmime_uuencode;
			}else if (*p == 'h') {
				m.encodetype = (char *)xmime_xxencode;
				WriteToEventLog("%s : cannot support encode type(%s - %s)", szFunctionName, m.filename, xmime_xxencode);
			}else{
				WriteToEventLog("%s : unknown encode type(%s)", szFunctionName, m.filename);
				return 0;
			}
			m.pbody = (char *)p;
			p = strstr(m.pbody, "\nend");
			if (p == 0)
				return 0;
		}else{
			// 解码单体邮件
			if (m_filename == 0 || * m_filename == 0) {
				return 0;
			}
			if (m_filename && * m_filename && m_encode_type == 0)
				m_encode_type = (char *)xmime_text;

			m.encodetype = m_encode_type;
			m.filename   = m_filename;
			m.pbody = m_pbody;
			p = m_pbody + strlen(m_pbody);
		}
		while (ISBLANKCH(p[-1])) {
			p--;
		}
		m.len = int(p - m.pbody);
		*((char *)p) = 0;
		parse_part(m);
		m_parts.add(&m);

		return 1;
	}

	char boundary[1024];
	int l = sprintf(boundary, "\r\n--%s", m_boundary);
	int i = 0;
	for (pbegin = strstr(m_pbody, boundary); pbegin && *pbegin; i++){
		// find boundary
		if (l > (int)strlen(pbegin)){
			break;
		}

		// 如果最后一个没有分隔,可以容错识别
		if ((pend = strstr(pbegin + l, boundary)) == 0) {
			pend = pbegin + strlen(pbegin);
		}

		// get the part header & body
		char * pheader = pbegin + l;
		char * p = strstr(pheader, "\r\n\r\n");
		if (p) {
			*p = '\0';
			char * pbody = p + 4;
			// get encode type, filename, len
			mailpart m;
			m.encodetype = finditem("Content-Transfer-Encoding:", pheader);
			if (m.encodetype == 0)
				m.encodetype = finditem("Content-transfer-encoding:", pheader);

			m.filename = finditem("filename=", pheader);
			if (m.filename == 0)
				m.filename = finditem("name=", pheader);
			getitem(m.encodetype);
			getitem(m.filename);
			p = pend;
			while (ISBLANKCH(p[-1])) {
				p--;
			}
			m.len = int(p - pbody);
			m.pbody = pbody;
			*p = 0;
			pend += l;

			parse_part(m);

			m_parts.add(&m);
		}
		pbegin = pend;
	}
	return m_parts.m_count;
}

const char * xmail::save_attach(const mailpart * part, const char * path)
{
	if (part->filename == 0 || part->filename[0] == '\0')
		return 0;
	char msg[128];
	char fullpath[2048];
	if (path && * path){
		strcpy(fullpath, path);
		int l = int(strlen(fullpath));
		if (fullpath[l-1] != '/' || fullpath[l-1] != '\\')
			fullpath[l++] = '\\';
		strcpy(fullpath + l, part->filename);
	}
	if (write_file(fullpath, part->pbody, part->len, msg, sizeof(msg)) != part->len){
		WriteToEventLog("write %s : %s", part->filename, msg);
		return 0;
	}
	return part->filename;
}

const char * xmail::save_attach(int i, const char * path)
{
	if (i < 0 || i > m_parts.m_count - 1){
		return 0;
	}
	return save_attach(&m_parts[i], path);
}

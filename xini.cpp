#include "xsys.h"

#include "ss_service.h"
#include "iniFile.h"
#include "xini.h"

xini::xini()
{
	m_file[0] = 0;
	m_path[0] = 0;
}

xini::xini(const char * filename)
{
	open(filename);
}

int xini::open(const char * filename)
{
	m_file[0] = 0;
	m_path[0] = 0;
	const char * p = strrchr(filename, '\\');
	int pathlen;
	if (p == 0 && (p = strrchr(filename, '/')) == 0) {
		getfullname(m_file, filename, MAX_PATH);
		if ((p = strrchr(m_file, '\\')) == 0 &&
			(p = strrchr(m_file, '/')) == 0) {
			pathlen = 0;
		}else{
			pathlen = int(p - m_file);
		}
	}else{
		strcpy(m_file, filename);
		pathlen = int(p - filename);
	}
	memcpy(m_path, m_file, pathlen);  m_path[pathlen] = 0;

	return 0;
}

long xini::get(const char * section, const char * keyname, long defaultvalue)
{
	return long(ini_get_int(section, keyname, defaultvalue, m_file));
}

void xini::get(const char * section, const char * keyname, char * value, int maxlen, const char * defaultvalue)
{
	ini_get_string(section, keyname, defaultvalue, value, maxlen, m_file);
}

long xini::set(const char * section, const char * keyname, long value)
{
	char s[16];
	strcpy(s, xsys_ltoa(value));
	ini_write_string(section, keyname, s, m_file);
	return value;
}

const char * xini::set(const char * section, const char * keyname, const char * value)
{
	ini_write_string(section, keyname, value, m_file);
	return value;
}

void xini::formatpath(char * path)
{
	int l = strlen(path);
	if (path[l - 1] == '\\' || path[l - 1] == '/') {
		path[l - 1] = 0;
	}
}

void xini::getpath(const char * section, const char * key, char * v, int vsize)
{
	get(section, key, v, vsize);
	formatpath(v);
}


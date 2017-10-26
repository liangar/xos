#include <xsys.h>

#include <l_str.h>
#include <lfile.h>
#include <ss_service.h>

#include <xsys_log.h>

static bool s_pause_flag = false;

xsys_log::xsys_log()
	: m_hlogfile(NULL)
	, m_bdebug(false)
	, m_prev_time(0)
	, m_idle(60)
{
	memset(m_file_name, 0, sizeof(m_file_name));
	memset(m_bak_path, 0, sizeof(m_bak_path));
}

xsys_log::~xsys_log()
{
	down();
}

int xsys_log::init(bool b_in_debug, int idle, const char * bakpath)
{
	m_bdebug  = b_in_debug;
	m_idle = idle;
	
	if (bakpath && *bakpath){
		if (strlen(bakpath) < sizeof(m_bak_path))
			strcpy(m_bak_path, bakpath);
		else
			return -1;
	}else
		m_bak_path[0] = '\0';

	return 0;
}

int xsys_log::down(void)
{
	int r = close();

	return r;
}

bool xsys_log::open(const char * file_name)
{
	getfullname(m_file_name, file_name, sizeof(m_file_name));
	return reopen();
}

bool xsys_log::reopen(void)
{
	bool old_flag = s_pause_flag;

	s_pause_flag = true;

	xsys_sleep_ms(500);

	int r = close();
	
	r = bak();
	
	FILE * hlogfile = fopen(m_file_name, "w+");
	if (hlogfile == NULL){
		hlogfile = fopen(m_file_name, "w+");
	}
	m_hlogfile = hlogfile;
	
	s_pause_flag = old_flag;
	
	return (hlogfile != NULL);
}

int xsys_log::close(void)
{
    if (m_hlogfile != NULL){
		bool old_flag = s_pause_flag;
		s_pause_flag = true;
		xsys_sleep_ms(100);

        fclose(m_hlogfile);  m_hlogfile = NULL;
		
		s_pause_flag = old_flag;
    }
	return 0;
}

int xsys_log::flush(void)
{
	if (!s_pause_flag && m_hlogfile)
		return fflush(m_hlogfile);
	
	return 0;
}

int xsys_log::write(const char * s)
{
	if (!s || *s == 0)
		return 0;

	if (m_bdebug)
		fputs(s, stdout);

//	if (!m_hlogfile)
//		xsys_sleep_ms(200);

	int r = 0;
	if (!s_pause_flag && m_hlogfile){
		r = fputs(s, m_hlogfile);
		if (r < 0){
			xsys_sleep_ms(300);
			r = fputs(s, m_hlogfile);
		}
	}
	return r;
}

int xsys_log::write_aline(const char * s)
{
	write(s);
	write("\n");
	return 0;
}

int xsys_log::bak(void)
{
	// no path for bak
	if (m_bak_path[0] == '\0')
		return 100;

	char timestring[16];
	time_t t = get_modify_time(timestring, m_file_name);
	if (t <= 0)
		return -100;

	// get file name
	const char * pfile_name = strrchr(m_file_name, '/');
	
	if (pfile_name == NULL)
		pfile_name = strrchr(m_file_name, '\\');
	if (pfile_name)
		++pfile_name;
	else
		pfile_name = m_file_name;

	// get the full 
	char topath[MAX_PATH];
	
	char tt[MAX_PATH], m[8];
	
	memcpy(m, timestring, 6);  m[6] = 0;
#ifndef WIN32
	sprintf(tt, "%s/%s/%s", m_bak_path, m, pfile_name);
#else
	sprintf(tt, "%s\\%s\\%s", m_bak_path, m, pfile_name);
#endif
	getfullname(topath, tt, MAX_PATH);
	xsys_md(topath, true);

	char *p = strrchr(topath, '.');
	if (p) {
		sprintf(p, ".%s.log", timestring);
	}
	
	return rename(m_file_name, topath);
}

int scan_aline(char * d)
{
	unsigned ks;
	while ((ks = getchar()) != '\n'){
		if ((ks & 0x7F) == ks)
			*d++ = ks;
	}
	*d = '\0';
	
	return strlen(d);
}

void test_xsys_log(char * b)
{
	xsys_log log;
	const char * p;

	printf("test_xsys_log in\n");

	for(;;)
	{
		scan_aline(b);

		if (::stricmp(b, "exit") == 0)  break;
		if (b[0] == '?'){	// show usage
			printf( "usage :"
				"\n\texit            : exit the program"
				"\n\ti:<debug>,<idle>,<bak_path>           : init, set config"
				"\n\td:                                    : down"
				"\n\to:<file name>     : open log"
				"\n\tc:                : close"
				"\n\tr:                : reopen"
				"\n\tw:                : write string"
				"\n\tl:                : write a line"
				"\n");
			continue;
		}

		int r;
		switch(b[0]){
			case 'i':
				p = getaword(b, b+2, ',');
				p = getaword(b+1, p, ',');
				r = log.init((b[0] == 'Y'), atoi(b+1), p);
				break;

			case 'd':
				r = log.down();
				break;

			case 'o':
				r = log.open(b+2);
				break;
				
			case 'r':
				r = log.reopen();
				break;
				
			case 'c':
				r = log.close();
				break;
				
			case 'w':
				r = log.write(b+2);
				break;
				
			case 'l':
				r = log.write_aline(b+2);
				break;

			default:{
				printf("not support cmd\n");
				r = -1;
			}
		}
		if (r < 0)
			printf("error: %d\n", r);
	}
}

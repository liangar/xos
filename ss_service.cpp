#include <xsys.h>

#include <l_str.h>
#include <lfile.h>

#include <xsys_log.h>

#include <ss_service.h>

static xsys_log s_log;

static FILE * hlogfile = NULL;
static bool   bdebug = true;
static time_t prev_time = 0;
static long   t_idle = 60;
static char   run_path[MAX_PATH];

void set_run_path(const char * p)
{
	strcpy(run_path, p);
	::chdir(run_path);
}

char * get_run_path()
{
	return run_path;
}

void terminatethread(void * &hthread, int seconds, int returncode)
{
	xsys_thread h;
	h.m_thread = (unsigned long)hthread;
	terminatethread(h, seconds, returncode);
	hthread = 0;
}

void getfullname(char * pbuf, const char * pfilename, int maxlen)
{
	if (strchr(pfilename, '\\') != 0 || strchr(pfilename, '/') != 0)
		strncpy(pbuf, pfilename, maxlen);
	else{
		char p[MAX_PATH];
		if (run_path[0]){
#ifdef WIN32
			sprintf(p, "%s\\%s", run_path, pfilename);
#else
			sprintf(p, "%s/%s", run_path, pfilename);
#endif
		}else{
			strncpy(p, pfilename, MAX_PATH);
		}
		strncpy(pbuf, p, maxlen);
	}
}

void EL_puts(const char * s)
{
	/*
	if (bdebug)
		printf("%s", s);
	if (hlogfile)
		fputs(s, hlogfile);
	//*/
	s_log.write(s);
}

bool openservicelog(const char * pfilename, bool b_in_debug, int idle, bool saveold, const char * bakpath)
{
	char logpath[MAX_PATH];
	
	bdebug = b_in_debug;
	
	getfullname(logpath, pfilename, sizeof(logpath));

	::chdir(run_path);

	s_log.init(b_in_debug, idle, bakpath);
	return s_log.open(logpath);

	/*
	if (saveold){	// save old log
		char timestring[16];
		time_t t = get_modify_time(timestring, logpath);
		if (t > 0) {
			char topath[MAX_PATH];
			
			if (bakpath == 0 || *bakpath == 0) {	// 无指定bak目录
				strcpy(topath, logpath);
			}else{
				char t[MAX_PATH], m[8], logname[512];
				
				memset(logname, 0x0, sizeof(logname));
				const char * p = strrchr(pfilename, '/');
				if (p == 0)
					p = strrchr(pfilename, '\\');
				if (p == 0 || p[0] == '\0'){
					strcpy(logname, pfilename);
				} else {
					p++;
					strcpy(logname, p);
				}
				
				memcpy(m, timestring, 6);  m[6] = 0;
#ifndef WIN32
				sprintf(t, "%s/%s/%s", bakpath, m, logname);
#else
				sprintf(t, "%s\\%s\\%s", bakpath, m, logname);
#endif
				getfullname(topath, t, MAX_PATH);
				xsys_md(topath, true);
			}
			char *p = strrchr(topath, '.');
			if (p) {
				sprintf(p, ".%s.log", timestring);
			}
			rename(logpath, topath);
		}
	}
	
	hlogfile = fopen(logpath, "w+");
	
	t_idle = idle;
	return (hlogfile != NULL);
	//*/
}

void closeservicelog(void)
{
	/*
    if (hlogfile != NULL){
        fclose(hlogfile);  hlogfile = NULL;
    }
	//*/
	s_log.close();
}

bool reopenservicelog(void)
{
	return s_log.reopen();
}

int flushservicelog(void)
{
	return s_log.flush();
}

void To_Upper(char *s)
{
	if (s == 0)
		return;

	int l = int(strlen(s));
	for(int i = 0; i < l; i++){
		s[i] = toupper(s[i]);
	}
}

void To_Lower(char *s)
{
	if (s == 0)
		return;
	
	int l = int(strlen(s));
	for(int i = 0; i < l; i++){
		s[i] = tolower(s[i]);
	}
}

char * getpathfile(const char * filename, char * fullFileName, char *outPath, char *outFileName)
{
	char szFileName[256];
	int pathlen = 0, l = 0;
	
	char * p1 = strrchr((char *)filename, '\\');
	char * p2 = strrchr((char *)filename, '/');
	if (p1 == 0 && p2 == 0) {
		getfullname(szFileName, filename, sizeof(szFileName));
		p1 = strrchr(szFileName, '\\');
		p2 = strrchr(szFileName, '/');
	}
	else{
		strcpy(szFileName, filename);
		p1 = strrchr(szFileName, '\\');
		p2 = strrchr(szFileName, '/');
	}
	
	if (p1 > p2){
		pathlen = int(p1 - szFileName);
		l = (int)strlen(szFileName)-pathlen-1;
		memcpy(outFileName, p1+1, l);
		outFileName[l] = '\0';
	}else if (p2 > p1){
		pathlen = int(p2 - szFileName);
		l = (int)strlen(szFileName)-pathlen-1;
		memcpy(outFileName, p2+1, l);
		outFileName[l] = '\0';
	}
	else{
		pathlen = 0;
		l = (int)strlen(szFileName);
		memcpy(outFileName, szFileName, l);
		outFileName[l] = '\0';
	}
	
	memcpy(outPath, szFileName, pathlen);  
	outPath[pathlen] = 0;
	
	l = (int)strlen(szFileName);
	memcpy(fullFileName, szFileName, l);
	fullFileName[l] = 0;
	return fullFileName;
}

void writetoeventlog(int wType, int dwID, char const * pFormat, va_list v)
{
    char s[512];

	if (time(0) - prev_time > t_idle){
		EL_WriteNow();
		prev_time = time(0);
	}

    SysVSNPrintf(s, sizeof(s)-1, pFormat, v);  s[sizeof(s)-1] = '\0';

	s_log.write_aline(s);
	/*
	EL_puts(s);
	EL_puts("\n");
	if (hlogfile){
		fflush(hlogfile);
	}
	//*/
}

void WriteToEventLog(int wEventType, int dwEventID, char const * pFormat, ...)
{
    va_list pArg;

	va_start(pArg, pFormat);
	writetoeventlog(wEventType, dwEventID, pFormat, pArg);
    va_end(pArg);
}

void WriteToEventLog(char const * pFormat, ...)
{
    va_list pArg;

	va_start(pArg, pFormat);
	writetoeventlog(1, 1, pFormat, pArg);
    va_end(pArg);
}

void WriteToFileLog(const char * logfile, char const * pFormat, ...)
{
	va_list pArg;
    char s[512], logpath[MAX_PATH], nowstring[32];
	
	getfullname(logpath, logfile, sizeof(logpath));
	
	//WriteToEventLog("begin open FileLog [%s].", logpath);
	FILE * hf = fopen(logpath, "ab");
	if (hf == NULL){
		return;
	}
	getnowtime(nowstring);
	fputs(nowstring, hf);  
	fputs("\t", hf);

	va_start(pArg, pFormat);
	SysVSNPrintf(s, sizeof(s)-1, pFormat, pArg);  s[sizeof(s)-1] = '\0';
    va_end(pArg);

	fputs(s, hf);
	fputs("\n", hf);
	fflush(hf);
	fclose(hf);  
	hf = NULL;
}

char * getnowtime(char *d)
{
	long t = long(time(0));
	time2string(d, t);
	return d;
}

char * getsimple_now(char *d)
{
	long t = long(time(0));
	time2simple(d, t);
	return d;
}

char * time2string(char * d, long t)
{
	struct tm *ltime;
    ltime = localtime((const time_t*)&t);
	sprintf(d, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	return d;
}

long string2time(const char * s, bool bendflag)
{
	char tmp[32];
	const char *p;
	struct tm ltime;

	memset(&ltime, 0, sizeof(ltime));
	if (bendflag){
		ltime.tm_hour = 23;
		ltime.tm_min = 59;
		ltime.tm_sec = 59;
	}
	else{
		ltime.tm_hour = 0;
		ltime.tm_min = 0;
		ltime.tm_sec = 0;
	}

	p = getaword(tmp, s, '-');
	ltime.tm_year = atoi(tmp) - 1900;
	
	p = getaword(tmp, p, '-');
	ltime.tm_mon = atoi(tmp)-1;
	
	p = getaword(tmp, p, ' ');
	ltime.tm_mday = atoi(tmp);
	
	p = getaword(tmp, p, ':');
	ltime.tm_hour = atoi(tmp);
	
	p = getaword(tmp, p, ':');
	ltime.tm_min = atoi(tmp);
	
	p = getaword(tmp, p, ' ');
	ltime.tm_sec = atoi(tmp);

	return long(mktime(&ltime));
}

char * string2simple(char *d, char *s, bool bendflag)
{
	char tmp[32];
	const char *p;
	struct tm ltime;

	if (bendflag){
		ltime.tm_hour = 23;
		ltime.tm_min = 59;
		ltime.tm_sec = 59;
	}
	else{
		ltime.tm_hour = 0;
		ltime.tm_min = 0;
		ltime.tm_sec = 0;
	}
	
	p = getaword(tmp, s, '-');
	if (p == 0 || tmp[0] == '\0')
		return d;
	ltime.tm_year = atoi(tmp);
	
	p = getaword(tmp, p, '-');
	if (p == 0 || tmp[0] == '\0')
		return d;
	ltime.tm_mon = atoi(tmp);
	
	p = getaword(tmp, p, ' ');
	if (p == 0	|| tmp[0] == '\0'){
		return d;
	}
	ltime.tm_mday = atoi(tmp);
	
	p = getaword(tmp, p, ':');
	if (p == 0 || tmp[0] == '\0'){
		sprintf(d, "%04d%02d%02d%02d%02d%02d", ltime.tm_year, ltime.tm_mon, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
		return d;
	}
	ltime.tm_hour = atoi(tmp);
	
	p = getaword(tmp, p, ':');
	if (p == 0 || tmp[0] == '\0'){
		sprintf(d, "%04d%02d%02d%02d%02d%02d", ltime.tm_year, ltime.tm_mon, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
		return d;
	}
	ltime.tm_min = atoi(tmp);
	
	p = getaword(tmp, p, ' ');
	ltime.tm_sec = atoi(tmp);

	sprintf(d, "%04d%02d%02d%02d%02d%02d", ltime.tm_year, ltime.tm_mon, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	return d;
}

char * time2simple(char * d, long t)
{
	struct tm *ltime;
    ltime = localtime((const time_t*)&t);
	sprintf(d, "%04d%02d%02d%02d%02d%02d", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	return d;
}

char * time2simpledate(char *d, long t)
{
	struct tm *ltime;
    ltime = localtime((const time_t*)&t);
	sprintf(d, "%04d%02d%02d", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday);
	return d;
}

char * getnextquarter(char *d, char *s) {
	long l = simple2time(s) + 15 * 60;
	time2simple(d, l);
	return d;
}
char * getnextquarter(char * s) {
	char t[16], d[16];
	memcpy(t, s, 12);
	t[12] = 0;

	char * p = getnextquarter(d, t);
	if (p == 0) {
		return 0;
	}

	memcpy(s, d, 14);

	return s;
}

char * getnexthour(char *d, char *s) {
	long l = simple2time(s) + 60 * 60;
	time2simple(d, l);
	return d;
}
char * getnexthour(char * s) {
	char t[16], d[16];
	memcpy(t, s, 12);
	t[12] = 0;

	char * p = getnexthour(d, t);
	if (p == 0) {
		return 0;
	}

	memcpy(s, d, 14);

	return s;
}

char * getnextdate(char *d, char *s) {
	long l = simple2time(s) + 24 * 60 * 60;
	time2simpledate(d, l);
	return d;
}

char * getnextdate(char * s)
{
	char t[12], d[12];
	memcpy(t, s, 8);  t[8] = 0;

	char * p = getnextdate(d, t);
	if (p == 0) {
		return 0;
	}

	memcpy(s, d, 8);

	return s;
}

char * getprevdate(char *d, char *s) {
	long l = simple2time(s) - 24 * 60 * 60;
	time2simpledate(d, l);
	return d;
}

char * getprevdate(char * s) {
	char t[12], d[12];
	memcpy(t, s, 8);
	t[8] = 0;

	char * p = getprevdate(d, t);
	if (p == 0) {
		return 0;
	}

	memcpy(s, d, 8);

	return s;
}

long getnextimebyminute(char * hhmm) {
	struct tm tt;
	char t[4];
	time_t now = time(0);

	memcpy(&tt, localtime(&now), sizeof(struct tm));
	tt.tm_hour = 0;
	tt.tm_min = 0;
	tt.tm_sec = 0;

	t[2] = 0;
	if (*hhmm) {
		t[0] = hhmm[0];
		t[1] = hhmm[1];
		tt.tm_hour = atoi(t);
		if (hhmm[2]) {
			t[0] = hhmm[2];
			t[1] = hhmm[3];
			tt.tm_min = atoi(t);
//			if (hhmm[4]) {
//				tt.tm_sec = atoi(hhmm + 4);
//			}
		}
	}

	long r = (long(mktime(&tt)) - now) / 60;
	if (0 >= r) {
		r += 24 * 60;
	}
	return r;
}

long dt_nextday(long t) {
	char s[12], d[12];
	sprintf(s, "%d", t);
	getnextdate(d, s);

	return long(atoi(d));
}

long getmaxdaywithmonth(int year, int month) {
	switch (month) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return 31;
	case 4:
	case 6:
	case 9:
	case 11:
		return 30;
	case 2:
		return ((((year % 4) == 0) && (year % 100) != 0) || (year % 400 == 0)) ? 29 : 28;
	}
	return 28;
}

long getmaxdaywithmonth(char * s) {
	struct tm stime;
	long l = simple2time(s);
	memcpy(&stime, localtime((time_t *) &l), sizeof(struct tm));
	return getmaxdaywithmonth(stime.tm_year + 1900, stime.tm_mon + 1);
}

char * getnextmonth(char * d, char * s) {
	struct tm stime;
	long l = simple2time(s);
	memcpy(&stime, localtime((time_t *) &l), sizeof(struct tm));

	if (12 == ++stime.tm_mon) {
		stime.tm_mon = 0;
		stime.tm_year++;
	}

	int maxday = getmaxdaywithmonth(stime.tm_year + 1900, stime.tm_mon + 1);
	stime.tm_mday = maxday >= stime.tm_mday ? stime.tm_mday : maxday;

//	if (28 < stime.tm_mday) {
//		int y = stime.tm_year + 1900;
//		int m = stime.tm_mon + 1;
//		if (29 == stime.tm_mday) {
//			if (2 == m && !((((y % 4) == 0) && (y % 100) != 0) || (y % 400 == 0))) stime.tm_mday = 28;
//		} else if (30 == stime.tm_mday) {
//			if (2 == m && ((((y % 4) == 0) && (y % 100) != 0) || (y % 400 == 0)))
//				stime.tm_mday = 29;
//			else
//				stime.tm_mday = 28;
//		} else if (31 == stime.tm_mday) {
//			if (2 == m) {
//				if (((((y % 4) == 0) && (y % 100) != 0) || (y % 400 == 0)))
//					stime.tm_mday = 29;
//				else
//					stime.tm_mday = 28;
//			} else if (4 == m || 6 == m || 9 == m || 11 == m) stime.tm_mday = 30;
//		}
//	}
	sprintf(d, "%04d%02d%02d", 1900 + stime.tm_year, stime.tm_mon + 1, stime.tm_mday);
	return d;
}

char * getnextmonth(char * s) {
	char t[12], d[12];
	memcpy(t, s, 8);
	t[8] = 0;

	char * p = getnextmonth(d, t);
	if (p == 0) {
		return 0;
	}

	memcpy(s, d, 8);

	return s;
}

/// YYYYMMDD
char * getprevyear(char * d, char * s) {
	long l = simple2time(s) - 365 * 24 * 60 * 60;
	time2simpledate(d, l);
	return d;
}

/// YYYYMMDD 用原位置
char * getprevyear(char * s) {
	char t[12], d[12];
	memcpy(t, s, 8);
	t[8] = 0;

	char * p = getprevyear(d, t);
	if (p == 0) {
		return 0;
	}

	memcpy(s, d, 8);

	return s;
}

/// 取当前的前一年
long getprevyear(long t) {
	return t - 365 * 24 * 60 * 60;
}

/// YYYYMMDD
char * getprevmonthday(char * d, char * s) {
	*d = 0;
	long t = simple2time(s);
	if (t == 0) return d;

	t -= 30 * 24 * 60 * 60;
	char v[16];
	time2simple(v, t);
	memcpy(d, v, 8);
	d[8] = 0;

	return d;
}

/// YYYYMMDD 用原位置
char * getprevmonthday(char * s) {
	char t[12], d[12];
	memcpy(t, s, 8);
	t[8] = 0;

	char * p = getprevmonthday(d, t);
	if (p == 0) {
		return 0;
	}

	memcpy(s, d, 8);
	s[8] = 0;

	return s;
}

/// 取当前的前个月
long getprevmonthday(long t) {
	return t - 30 * 24 * 60 * 60;
}

/// YYYYMM
char * getPrevMonth(char * d, char * s)
{
	char v[16];
	memcpy(v, s, 6);  v[6] = '0';  v[7] = '1';  v[8] = 0;

	*d = 0;
	long t = simple2time(v) - 1;
	if (t <= 0)
		return d;

	time2simple(v, t);
	memcpy(d, v, 6);  d[6] = 0;

	return d;
}

/// YYYYMM 用原位置
char * getPrevMonth(char * s)
{
	return getPrevMonth(s, s);
}

/// 取当前的前个月
long getPrevMonth(long t)
{
	char v[16];
	time2simple(v, t);
	getPrevMonth(v);
	v[6] = '0';  v[7] = '1';  v[8] = 0;

	return simple2time(v);
}

char * simple2string(char * d, const char * s)
{
	int l = strlen(s);

	if (l > 8){
		// date YYYYMMDDhhmmss -> YYYY-MM-DD hh:mm:ss
		if (l == 14){
			d[18] = s[13];  d[17] = s[12];
		}else{
			d[18] = '0';  d[17] = '0';
		}
		d[16] = ':';
		if (l == 12){
			d[15] = s[11];  d[14] = s[10];
		}else{
			d[15] = '0';  d[14] = '0';
		}
		d[13] = ':';

		d[12] = s[ 9];  d[11] = s[ 8];  d[10] = ' ';
		d[19] = 0;
	}	
	d[9] = s[7];  d[8] = s[6];  d[7] = '-';
	d[6] = s[5];  d[5] = s[4];  d[4] = '-';

	memcpy(d, s, 4);
	if (l <= 8)
		d[10] = 0;
	return d;
}

long simple2time(const char * s)
{
	struct tm t;
	memset(&t, 0, sizeof(t));
	
	char v[16];
	
	int l = int(strlen(s));
	if (l == 8) {
		if (!dt_ValidDate(s)) {
			return 0;
		}
		memcpy(v, s  , 4);  v[4] = 0;  t.tm_year = atoi(v) - 1900;
		memcpy(v, s+4, 2);  v[2] = 0;  t.tm_mon  = atoi(v) - 1;
		memcpy(v, s+6, 2);  v[2] = 0;  t.tm_mday = atoi(v);
	}else if (l == 14) {
		if (!dt_ValidDateTime(s))
			return 0;
		memcpy(v, s  , 4);  v[4] = 0;  t.tm_year = atoi(v) - 1900;
		memcpy(v, s+4, 2);  v[2] = 0;  t.tm_mon  = atoi(v) - 1;
		memcpy(v, s+6, 2);  v[2] = 0;  t.tm_mday = atoi(v);
		memcpy(v, s+8, 2);  v[2] = 0;  t.tm_hour = atoi(v);
		memcpy(v, s+10,2);  v[2] = 0;  t.tm_min  = atoi(v);
		memcpy(v, s+12,2);  v[2] = 0;  t.tm_sec  = atoi(v);
	}else
		return 0;
	
	return long(mktime(&t));
}


bool dt_ValidDate(const char * s)
{
	int l = isdigits(s);
	if (l != 8) {
		return false;
	}
	
	struct tm t;
	memset(&t, 0, sizeof(t));
	
	char v[16];
	memcpy(v, s  , 4);  v[4] = 0;  t.tm_year = atoi(v) - 1900;
	memcpy(v, s+4, 2);  v[2] = 0;  t.tm_mon  = atoi(v) - 1;
	memcpy(v, s+6, 2);  v[2] = 0;  t.tm_mday = atoi(v);
	
	if (t.tm_year < 0 || t.tm_year > 1000 || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday > 31)
		return false;
	
	return (memcmp(s, time2simpledate(v, long(mktime(&t))), 8) == 0);
}

bool dt_ValidTime(const char * s)
{
	int l = isdigits(s);
	if (l != 6) {
		return false;
	}
	struct tm t;
	memset(&t, 0, sizeof(t));
	t.tm_mday = 1;
	t.tm_year = 106;
	
	char v[16];
	memcpy(v, s  , 2);  v[2] = 0;  t.tm_hour = atoi(v);
	memcpy(v, s+2, 2);  v[2] = 0;  t.tm_min  = atoi(v);
	memcpy(v, s+4, 2);  v[2] = 0;  t.tm_sec  = atoi(v);
	
	long n = long(mktime(&t));
	return (strcmp(s, time2simple(v, n) + 8) == 0);
}

bool dt_ValidDateTime(const char * s)
{
	int l = isdigits(s);
	if (l != 14) {
		return false;
	}
	struct tm t;
	memset(&t, 0, sizeof(t));
	t.tm_mday = 1;
	
	char v[16];
	memcpy(v, s  , 4);  v[4] = 0;  t.tm_year = atoi(v) - 1900;
	memcpy(v, s+4, 2);  v[2] = 0;  t.tm_mon  = atoi(v) - 1;
	memcpy(v, s+6, 2);  v[2] = 0;  t.tm_mday = atoi(v);
	memcpy(v, s+8, 2);  v[2] = 0;  t.tm_hour = atoi(v);
	memcpy(v, s+10,2);  v[2] = 0;  t.tm_min  = atoi(v);
	memcpy(v, s+12,2);  v[2] = 0;  t.tm_sec  = atoi(v);
	
	if (t.tm_year < 0 || t.tm_year > 1000 || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday > 31)
		return false;
	
	return (strcmp(s, time2simple(v, long(mktime(&t)))) == 0);
}

static const char * xwork_datetime_format[] = {
	"####-##-## ##:##:##",
	"####-##-##",
	"##:##:##",
	0
};

int dt_datetime_format(const char * dtstring)
{
	for (int i = 0; xwork_datetime_format[i]; i++){
		if (checkmask(dtstring, xwork_datetime_format[i]))
			return i;
	}
	return -1;
}

void EL_WriteHexString(const char * s, int len)
{
	char d[32];

	sprintf(d, "length = %d :{\n", len);
	EL_puts(d);
	for (int i = 0; i < len; i++, s++){
		sprintf(d, "%02x ", (unsigned char)(*s));
		EL_puts(d);
		if ((i + 1)%16 == 0 && i+1 != len)
			EL_puts("\n");
	}
	EL_puts("\n}\n");
}

void EL_WriteNow()
{
	char nowstring[32];
	getnowtime(nowstring);
	EL_puts(nowstring);  EL_puts("\n");
}

void write_buf_log(const char * title, unsigned char * buf, int len)
{
	int i, j, n;
	unsigned char * p = buf;
	char sshow[20];  sshow[16] = '\n';  sshow[17] = '\0';
	char s[64];

	EL_puts(title);
	EL_puts(":\n     0001 0203 0405 0607 0809 0A0B 0C0D 0E0F 0123456789ABCDEF\n");
	for (i = n = 0; n < len; i++){
		int l = sprintf(s, "%3d: ", i+1);
		for (j = 0; j < 16; j++){
			static const char * pformats[] = {"%02X", "%02X "};
			static const char * pblanks [] = {"  ", "   "};
			if (n < len){
				unsigned char c;
				c = *p++;  n++;
				sshow[j] = (c > 0x20 && c <= 126)? c : ' ';
				l += sprintf(s+l, pformats[(j&0x01)], c);
			}else{
				sshow[j] = ' ';
				strcpy(s+l, pblanks[(j&0x01)]);  l += 2 + (j & 0x01);
			}
		}
		EL_puts(s);
		EL_puts(sshow);
	}
}

char * get_error_message(unsigned int errorcode, char * msg, int maxlen)
{
	if (msg){
		strcpy(msg, strerror(errorcode));
	}
	return msg;
}

void WriteSystemErrorLog(unsigned int errorcode, const char * ptitle, const char * subtitle)
{
	char msg[512];
	get_error_message(errorcode, msg, sizeof(msg));
	WriteToEventLog("%s [%s]: %s", ptitle, subtitle, msg);
}

void WriteSystemErrorLog(const char * ptitle, const char * subtitle)
{
	char msg[512];
	strcpy(msg, xlasterror());
	WriteToEventLog("%s [%s]: %s", ptitle, subtitle, msg);
}

long dt_to_daybegin(long t)
{
	struct tm tnow;

	memcpy(&tnow, localtime((time_t *)&t), sizeof(struct tm));
	tnow.tm_hour = 0;
	tnow.tm_min  = 0;
	tnow.tm_sec  = 0;
	return long(mktime(&tnow));
}

/// stime : format hhmmss
long dt_to_day_time(long ltime, const char * stime)
{
	struct tm tnow;
	char t[4];

	memcpy(&tnow, localtime((time_t *)&ltime), sizeof(struct tm));
	tnow.tm_hour = 0;
	tnow.tm_min  = 0;
	tnow.tm_sec  = 0;

	t[2] = 0;
	if (*stime){
		t[0] = stime[0];  t[1] = stime[1];
		tnow.tm_hour = atoi(t);
		if (stime[2]){
			t[0] = stime[2];  t[1] = stime[3];
			tnow.tm_min = atoi(t);
			if (stime[4]){
				tnow.tm_sec = atoi(t+4);
			}
		}
	}

	return long(mktime(&tnow));
}


long dt_gettoday(void)
{
	return dt_to_daybegin(long(time(0)));
}

long dt_gettoday_time(const char * stime)
{
	return dt_to_day_time(long(time(0)), stime);
}

long dt_today(void)  {  
	return dt_gettoday();  
}

long dt_year(long t)
{
	struct tm tnow;

	memcpy(&tnow, localtime((time_t *)&t), sizeof(struct tm));
	return tnow.tm_year+1900;
}

long dt_month(long t)
{
	struct tm tnow;

	memcpy(&tnow, localtime((time_t *)&t), sizeof(struct tm));
	return tnow.tm_mon + 1;
}

long dt_day(long t)
{
	struct tm tnow;

	memcpy(&tnow, localtime((time_t *)&t), sizeof(struct tm));
	return tnow.tm_mday;
}

long dt_hour(long t) {
	struct tm tnow;

	memcpy(&tnow, localtime((time_t *) &t), sizeof(struct tm));
	return tnow.tm_hour;
}
long dt_minute(long t) {
	struct tm tnow;

	memcpy(&tnow, localtime((time_t *) &t), sizeof(struct tm));
	return tnow.tm_min;
}

bool all_is_digit(const char * s, int len) {
	if (len < 0) {
		len = 10000;
	}
	int i;
	for (i = 0; i < len && s[i] != 0; i++) {
		if (!isdigit(s[i])) {
			return false;
		}
	}
	return i > 0;
}

/// 全空
bool all_is_blank(const char * s, int len)
{
	if (len < 0) {
		len = 10000;
	}
	for (int i = 0; i < len && s[i] != 0; i++) {
		if (s[i] != ' ') {
			return false;
		}
	}
	return true;
}

///是十六进制的字符
bool all_is_0xdigit(const char * s, int len)
{
	if (len < 0) {
		len = 10000;
	}
	for (int i = 0; i < len && s[i] != 0; i++) {
		if (!(s[i]>='a'&& s[i]<='f' || s[i]>='A'&&s[i]<='F' || isdigit(s[i]))) {
			return false;
		}
	}
	return true;
}


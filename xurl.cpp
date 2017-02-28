#include <xsys.h>

#include <l_str.h>

#include <xurl.h>

/////////////////////////////////////////////////////////////////////////
//  filename: parseurl.cpp
//  goal    : for translate the url scription to c struct
//
/////////////////////////////////////////////////////////////////////////
typedef int parseurlfunc(LPn_url purl, const char * urlstring);

typedef struct tagparsestate{
    const char *	protocolname;   // command name, ex, "POST", etc.
    urltype         type;
    parseurlfunc * 	pf;   			// transform function point
} parsestate, * LPparsestate;

static int parsetcp(LPn_url purl, const char * urlstring);
static int parsesmtp(LPn_url purl, const char * urlstring);
static int parsepop3(LPn_url purl, const char * urlstring);
static int parsemail(LPn_url purl, const char * urlstring);
static int parseftp(LPn_url purl, const char * usrlstring);

static parsestate thetable[] = {
    {"tcp" , urltcp , parsetcp },  // parse tcp (tcp://(<hostname>|!):<portnumber>)
    {"smtp", urlsmtp, parsesmtp},  // parse smtp(smtp://<user>@<hostname>[:port])
    {"pop3", urlpop3, parsepop3},  // parse pop3(pop3://<hostname>[:port]/<uid>;<pwd>)
	{"mail", urlmail, parsemail},  // parse mail(mail://<smtphostname>[:smtpport];<pop3hostname>[:pop3port]/<pop3account>,<pwd>)
	{"ftp",  urlftp,  parseftp},   // parse ftp (ftp://<host>[:port][ \t]*(<uid>,<pwd>)[ \t]*<path>
    {NULL  , urlnull, NULL     }
};

int parseurl(LPn_url purl, const char * urlstring)
{
    const char * q = strchr(urlstring, ':');  int l = int(q - urlstring);

    if (q == NULL || q[1] != '/' || q[2] != '/')  return -1;

	LPparsestate p;
    for (p = thetable; p->protocolname; p++)
        if (_strnicmp(p->protocolname, urlstring, l) == 0)
            break;

	if (p->protocolname == NULL)
        return 1;
    
	purl->n_type = p->type;

    return ((* p->pf)(purl, q+3));
}

static int parsetcp(LPn_url purl, const char * ptcp)
{
	int l;  const char * p;

	if ((p = strchr(ptcp, ':')) == NULL || !isdigit(p[1]))
		return -1;
	
	purl->tcp.port = atoi(p+1);
	if (*ptcp == '!'){
		gethostname(purl->tcp.ip, 64);  purl->s_r = N_URL_RECV;  // is server, listen req
	}else{
		if ((l = int(p - ptcp)) >= 64)		// error: too long string
			return 1000;
		strncpy(purl->tcp.ip, ptcp, l);  purl->tcp.ip[l] = '\0';
		purl->s_r = N_URL_SEND;  // is client, to connect server and send
	}

	return 0;
}

static const char * getaccount(char * paccount, char * ip, int * port, const char * pstr, int dport)
{
	// skip space
	while (isspace(*pstr))  pstr++;

	// get account
	while (*pstr && *pstr != ':' && *pstr != '@' && *pstr != ';' && *pstr != ',' && *pstr != '/')
		*paccount++ = *pstr++;
	if (*pstr != '@')
		return 0;
	*paccount = '\0';

	// get ip
	pstr++;
	while (*pstr && *pstr != ':' && *pstr != '@' && *pstr != ';' && *pstr != ',' && *pstr != '/')
		*ip++ = *pstr++;
	*ip = '\0';

	// get port
	if (*pstr != ':')
		*port = dport;
	else{
		pstr++;
		*port = atoi(pstr);
		while (isdigit(*pstr))  pstr++;
	}

	return pstr;
}

static const char * getuidpwd(char * uid, char * pwd, const char * pstr)
{
	// get uid
	while (*pstr && *pstr != ',' && *pstr != '/')
		*uid++ = *pstr++;
	*uid = '\0';
	if (*pstr != ',')
		*pwd = '\0';
	else{
		pstr++;
		while (*pstr && *pstr != '/' && *pstr != ',')
			*pwd++ = *pstr++;
		*pwd = '\0';
	}
	return pstr;
}

static int parsesmtp(LPn_url purl, const char * psmtp)
{
	if (getaccount(purl->smtp.account, purl->smtp.ip, &purl->smtp.port, psmtp, 25) == NULL)
		return -1;

	purl->s_r = N_URL_SEND;

	return 0;
}

static int parsepop3(LPn_url purl, const char * ppop3)
{
	const char * p;	char * q;
	for (p = ppop3, q = purl->pop3.ip; *p && *p != ':' && *p != '/';)
		*q++ = *p++;
	*q = '\0';
	// get port
	if (*p == ':'){
		purl->pop3.port = atoi(++p);
		p = strchr(p, '/');
	}else
		purl->pop3.port = 110;
	
	if (p == NULL || *p != '/')
		return -1;

	if (getuidpwd(purl->pop3.account, purl->pop3.pwd, ++p) == NULL || purl->pop3.account[0] == '\0')
		return -2;

	purl->s_r = N_URL_RECV;

	return 0;
}

static int parsemail(LPn_url purl, const char * pmail)
{
	const char * p = pmail;	char * q;

	// skip space
	while (isspace(*p))  p++;

	// get smtp hostname or ip
	for (q = purl->mail.smtpip; *p && *p != ':' && *p != ';';)
		*q++ = *p++;
	*q = '\0';

	// get smtp port
	if (*p == ':'){
		purl->mail.smtpport = atoi(++p);
		if ((p = strchr(p, ';')) == NULL)
			return -1;
	}else
		purl->mail.smtpport = 25;

	if (*p++ != ';')
		return -2;
	
	// get pop3 hostname or ip
	for (q = purl->mail.pop3ip; *p && *p != ':' && *p != '/';)
		*q++ = *p++;
	*q = '\0';

	if (*p == ':'){
		purl->mail.pop3port = atoi(++p);
		p = strchr(p, '/');
	}else
		purl->mail.pop3port = 110;

	if (p == NULL || *p != '/')
		return -2;

	if (getuidpwd(purl->mail.account, purl->mail.pwd, ++p) == NULL)
		return -3;

	purl->s_r = N_URL_RECV | N_URL_SEND;

	return 0;
}

static int parseftp(LPn_url purl, const char * urlstring)
{
	char url[512];

	trim_all(url, (char *)urlstring);

	const char * p = getaword(purl->ftp.host, url, " \t", "(/");
	if (*p == '(') {
		p = getaword(purl->ftp.uid, p + 1, ',');// set uid
		p = getaword(purl->ftp.pwd, p, ')');	// set pwd
	}
	strcpy(purl->ftp.path, p);					// set root path

	size_t l = strlen(purl->ftp.path);			// set the tail of recv path char != '/'
	if (purl->ftp.path[l-1] == '/') {
		purl->ftp.path[l-1] = 0;
	}

	return 0;
}


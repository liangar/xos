#ifndef _XURL_H_
#define _XURL_H_

#define N_URL_SEND	1
#define N_URL_RECV	2

enum urltype{
	urlnull = 0,
	urltcp,
	urlsmtp,
	urlpop3,
	urlmail,
	urlftp,
	urlhttp
};

typedef struct tagn_tcp{
    char ip[64];    // hostname or dotted-decimal
    int  port;      // port number
} n_tcp, * LPn_tcp;

typedef struct tagn_smtp{
    char ip[64];
	int  port;
    char account[64];
} n_smtp;

typedef struct tagn_pop3{
	char ip[64];
	int  port;
	char account[64];
	char pwd[64];
} n_pop3;

typedef struct tagn_mail{
	char smtpip[64];
	int  smtpport;
	char pop3ip[64];
	int  pop3port;
	char account[64];
	char pwd[64];
}n_mail, * LPn_mail;

typedef struct tagn_ftp{
	char host[64];
	char uid[32];
	char pwd[32];
	char path[128];
}n_ftp, * LPn_ftp;

typedef struct tagn_http{
	char host[64];
	char path[128];
}n_http, * LPn_http;

typedef struct tagn_url{
    urltype	n_type;
    int  	s_r;   	// send, recieve(N_URL_SEND|RECV)
    union {
	    n_tcp 	tcp;
    	n_smtp 	smtp;
	    n_pop3	pop3;
		n_mail	mail;
		n_ftp	ftp;
		n_http	http;
    };
} n_url, * LPn_url;

int parseurl(LPn_url purl, const char * urlstring);

#endif // _XURL_H_

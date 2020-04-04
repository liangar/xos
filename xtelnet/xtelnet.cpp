#include <xsys.h>

#include <iostream>

#include <l_str.h>
#include <xtype.h>
#include <ss_service.h>
#include <xsys_log.h>

#ifdef WIN32
#include <crtdbg.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif // _DEBUG
#endif // WIN32

using namespace std;

int		nsends, nrecvs;
bool	bisBIN = false;
bool	bisTCP = true;
int 	nsessions = 1;
const char * g_phost = 0;

xsys_socket * g_psock;

static unsigned int recv_show(void * pvoid);

char sendbuf[512];

/*
char * multichar_2_utf8(const char *s, int &len)
{
	// 计算由ansi转换为unicode后，unicode编码的长度
	// cp_acp指示了转换为unicode编码的编码类型
	len = MultiByteToWideChar(CP_ACP, 0, (LPCCH)s, -1, NULL, 0);
	wchar_t * w_string = (wchar_t *)calloc(len+1, 2);

	// ansi到unicode转换
	// cp_acp指示了转换为unicode编码的编码类型
	MultiByteToWideChar(CP_ACP, 0, (LPCCH)s, -1, w_string, len);

	// 计算unicode转换为utf8后，utf8编码的长度
	// cp_utf8指示了unicode转换为的类型
	len = WideCharToMultiByte(CP_UTF8, 0, w_string, -1, NULL, 0, NULL, NULL);
	char * utf8_string = (char *)calloc(len+1, 1);

	// unicode到utf8转换
	// cp_utf8指示了unicode转换为的类型
	WideCharToMultiByte(CP_UTF8, 0, w_string, -1, utf8_string, len, NULL, NULL);

	free(w_string);

	return utf8_string;
}

char * utf8_2_multichar(const char * s, int &len)
{
	string strOutGBK = "";
	len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	wchar_t * wsz_s = (wchar_t *)calloc(len+1, 2);
	MultiByteToWideChar(CP_UTF8, 0, s, -1, wsz_s, len);

	len = WideCharToMultiByte(CP_ACP, 0, wsz_s, -1, NULL, 0, NULL, NULL);
	char *psz_s = (char *)calloc(len+1, 1);
	WideCharToMultiByte(CP_ACP, 0, wsz_s, -1, psz_s, len, NULL, NULL);

	free(wsz_s);

	return psz_s;
}

//*/

static int parse_args(int argc, char * argv[])
{
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-' && argv[i][0] != '/'){
			g_phost = argv[i];
			continue;
		}

		if (isdigits(argv[i] + 1) > 0){
			nsessions = atoi(argv[i] + 1);
			continue;
		}

		if (argv[i][2] == 0)
		{
			switch (argv[i][1]) {
			case 'b':  bisBIN = true;  break;
			case 'U':  bisTCP = false; break;
			}
			continue;
		}
	}

	return argc;
}

static bool is_run = false;

void main(int argc, char **argv)
{
    if (argc < 2){
        printf("usage: xtelnet [-b] [-U] <host:port>\n");
        return;
    }

	parse_args(argc, argv);

	if (bisBIN)
		printf("in binary mode\n");

	/*
	{
		float f = 123456.00;
		double d= 123456.00;

		char * p = (char *)&f;
		EL_WriteHexString(p, sizeof(float));
		f = 48;	EL_WriteHexString(p, sizeof(float));
		f = 0;  EL_WriteHexString(p, sizeof(float));
		f = 1;  EL_WriteHexString(p, sizeof(float));
		p = (char *)&d;
		EL_WriteHexString(p, sizeof(double));
	}
	*/

#ifdef WIN32
#ifdef _DEBUG
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );
#endif // _DEBUG
#endif // WIN32

	xsys_init(true);

	openservicelog("xtelnet.log", true);

/*
	int len;
	char * putf8 = multichar_2_utf8("123张三：此是UTF8转换测试，请核对。456ABC", len);
	EL_WriteHexString(putf8, len);  putchar('\n');
	char * ps = utf8_2_multichar(putf8, len);
	EL_WriteHexString(ps, len);  putchar('\n');
	printf("%s\n", ps);

	free(putf8);
	free(ps);

	test_xsys_log(sendbuf);
//*/

	char b[1024*4];

	g_psock = new xsys_socket[nsessions];
	int i, r, open_count = 0;
	for (i = 0; i < nsessions; i++){
		r = g_psock[i].connect(g_phost, 0, 30, bisTCP);
		printf("[%d]: connect return: %d\n", i, r);
		if (r >= 0){
			printf("peer ip: %s\n", g_psock[i].get_peer_ip(b));
			printf("self ip: %s\n", g_psock[i].get_self_ip(b));
			open_count++;
		}
	}

	if (open_count > 0){
	xsys_thread h1;
	h1.init(recv_show, 0);

	xsys_sleep_ms(10);

	try{
		nsends = nrecvs = 0;
		r = 0;
		for (i = 0; i < nsessions; i++)
			if (g_psock[i].isopen())  ++r;
		open_count = r;

		while (is_run && open_count > 0){
			// putchar('>');
            cin.getline(b, sizeof(b));

			if (strcmp(b, "exit") == 0)
				break;

			i = 0;
			const char * p = b;
			if (nsessions > 1 && (p = strchr(b, ':')) && (r = int(p - b)) < 5 && r > 0){
				if (-isdigits(b) == r + 1){
					i = atoi(b) - 1;
					if (i < 0){
						i = 0;  p = b;
					}else
						++p;
				}
			}

            r = strlen(p);
			if (bisBIN){
				r = x_hex2array((unsigned char *)sendbuf, p, 0);
				r = g_psock[i].send(sendbuf, r);
			}else{
				r = c2string(b, p);
				r = g_psock[i].send(b, r);
			}

			xsys_sleep_ms(800);
            if (r < 0){
                break;
			}
            nsends += r;
		}
	}catch(...){
		printf("some error occured.\n");
	}

	for (i = 0; i < nsessions; i++)
		g_psock[i].close();
	h1.down();
	xsys_sleep(3);
	}else{
		printf("%s\n", xlasterror());
	}

	closeservicelog();

	xsys_down();

#ifdef WIN32
#ifdef _DEBUG
	_CrtMemCheckpoint( &s2 );
	if ( _CrtMemDifference( &s3, &s1, &s2) ){
		_CrtMemDumpStatistics( &s3 );
		_CrtDumpMemoryLeaks();
	}
#endif // _DEBUG
#endif // WIN32

	printf("xtelnet end ok(%d bytes send).\n", nsends);
}

static unsigned int recv_show(void * pvoid)
{
	char aline[512];

//	char old_ip[MAX_IP_LEN];
//	char ip[MAX_IP_LEN];
//	old_ip[0] = ip[0] = 0;

	is_run = true;

	printf("recver: start.\n");

	fd_set fdsr;
	struct timeval tv;

	int l = 1, i, n, r;
	while (l > 0){
		FD_ZERO(&fdsr);
		for (i = n = 0; i < nsessions; i++)
			if (g_psock[i].isopen()){
				FD_SET(g_psock[i].m_sock, &fdsr);
				if (n < int(g_psock[i].m_sock))
					n = int(g_psock[i].m_sock);
			}

		tv.tv_sec  = 30;
		tv.tv_usec = 0;
		r = ::select(n+1, &fdsr, NULL, NULL, &tv);

		if (r == 0)  continue;

		if (r < 0)
			break;

		for (i = n = 0; i < nsessions; i++){
			if (!g_psock[i].isopen())
				continue;

			if (!FD_ISSET(g_psock[i].m_sock, &fdsr))
				continue;

			l = SysRecvData(g_psock[i].m_sock, aline, sizeof(aline)-1, 100);
			FD_CLR(g_psock[i].m_sock, &fdsr);

			if (l < 0){
				g_psock[i].close();  continue;
			}

			aline[l] = 0;
			printf("%d <-:\n", i+1);
			if (bisBIN){
				write_buf_log("RECV", (unsigned char *)aline, l);
			}else{
				puts(aline);
			}

			nrecvs += l;
		}
	}
	xsys_sleep_ms(100);

	printf("recv_show end(%d bytes).\n", nrecvs);

	is_run = false;

	return 0;
}

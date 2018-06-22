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
const char * g_phost = 0;

xsys_socket g_sock;

static unsigned int recv_show(void * pvoid);

char sendbuf[512];

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

int recv_data(void)
{
	int l = 0, r;
	do {
		char aline[512];
		r = g_sock.recv(aline, sizeof(aline)-1, 5);
		if (r < 1)
			return r;
		aline[r] = 0;
		puts(aline);
		l += r;
	}while (r == 511);
	return l;
}

static int parse_args(int argc, char * argv[])
{
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-' && argv[i][0] != '/'){
			g_phost = argv[i];
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

	int r = g_sock.connect(g_phost, 0, 30, bisTCP);

	printf("connect return: %d\n", r);

	if (r >= 0){
	xsys_thread h1;
	h1.init(recv_show, 0);

	xsys_sleep_ms(10);

//	r = recv_data();

	try{
		char b[1024*4];
		printf("peer ip: %s\n", g_sock.get_peer_ip(b));
		printf("self ip: %s\n", g_sock.get_self_ip(b));

		nsends = nrecvs = 0;
		while (is_run && g_sock.isopen()){
			// putchar('>');
            cin.getline(b, sizeof(b));

			if (strcmp(b, "exit") == 0)
				break;

            r = strlen(b);
			if (bisBIN){
				r = x_hex2array((unsigned char *)sendbuf, b, r);
				r = g_sock.send(sendbuf, r);
			}else{
				r = c2string(b, b);
				r = g_sock.send(b, r);
			}

			xsys_sleep_ms(800);
            if (r < 0){
                break;
			}
            nsends += r;
//			r = recv_data();
		}
	}catch(...){
		printf("some error occured.\n");
	}

	g_sock.close();
	h1.down();

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

	int l = 1;
	while (l > 0 || g_sock.isopen()){
//		if (bisTCP){
			l = g_sock.recv(aline, sizeof(aline), -1);
//		}else
//			l = g_sock.recv_from(aline, sizeof(aline), ip, -1);	// this is ok, but unnecessary.
		if (l <= 0)
			break;

        aline[l] = 0;
		if (bisBIN){
			write_buf_log("RECV", (unsigned char *)aline, l);
		}else{
			putchar('<');  puts(aline);
		}

		nrecvs += l;
	}
	xsys_sleep_ms(100);

	printf("recv_show end(%d bytes).\n", nrecvs);

	is_run = false;

	return 0;
}

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

int		g_nsends, g_nrecvs;
bool	bisBIN = false;
const char * g_pport = NULL;

xsys_socket 	g_server_sock;
SYS_INET_ADDR	g_client_addr;

char g_client_ip_port[MAX_IP_LEN];

char sendbuf[512];

static unsigned int recv_show(void * pvoid);

static int parse_args(int argc, char * argv[])
{
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-' && argv[i][0] != '/'){
			g_pport = argv[i];
			continue;
		}

		if (argv[i][2] == 0)
		{
			switch (argv[i][1]) {
			case 'b':  bisBIN = true;  break;
			}
			continue;
		}
	}

	return argc;
}


void main(int argc, char **argv)
{
    if (argc < 2){
        printf("usage: xudp_server [-b] <port>\n");
        return;
    }

	parse_args(argc, argv);

	ZeroData(g_client_addr);

#ifdef WIN32
#ifdef _DEBUG
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );
#endif // _DEBUG
#endif // WIN32

	xsys_init(true);

	openservicelog("xudp_server.log", true);

	int r = g_server_sock.udp_listen(atoi(g_pport));

	printf("listen return: %d\n", r);

	if (r >= 0){
	xsys_thread h1;
	h1.init(recv_show, 0);

	xsys_sleep(1);
//	r = recv_data();

	try{
		char b[1024];
		printf("self ip: %s\n", g_server_sock.get_self_ip(b));

		g_nsends = g_nrecvs = 0;
		while (g_server_sock.isopen()){
			// putchar('>');
            cin.getline(b, sizeof(b));

			if (strcmp(b, "exit") == 0)
				break;

			if (g_nrecvs == 0)
				continue;

            r = strlen(b);
			if (bisBIN){
				r = x_hex2array((unsigned char *)sendbuf, b, r);
				r = g_server_sock.sendto(sendbuf, r, &g_client_addr);
			}else{
				r = c2string(b, b);
				r = g_server_sock.sendto(b, r, &g_client_addr);
			}

			xsys_sleep_ms(800);
            if (r < 0){
                break;
			}
            g_nsends += r;
		}
	}catch(...){
		printf("some error occured.\n");
	}

	g_server_sock.close();
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

	printf("xtelnet end ok(%d bytes send).\n", g_nsends);
}

static unsigned int recv_show(void * pvoid)
{
	char aline[512];

	printf("recver: start.\n");

	int l;
	while (true){
		SYS_INET_ADDR from_addr;
		ZeroData(from_addr);
		l = g_server_sock.recv_from(aline, sizeof(aline), &from_addr, 1000);
		if (l == ERR_TIMEOUT)
			continue;

		if (l <= 0)
			break;

		if (memcmp(&from_addr, &g_client_addr, sizeof(SYS_INET_ADDR)) != 0){
			memcpy(&g_client_addr, &from_addr, sizeof(SYS_INET_ADDR));
			SysInetRevNToA(g_client_addr, g_client_ip_port, MAX_IP_LEN);
			printf("peer ip: %s\n", g_client_ip_port);
			g_nrecvs = 0;
		}

        aline[l] = 0;
		g_nrecvs += l;

		if (bisBIN){
			write_buf_log("RECV", (unsigned char *)aline, l);
		}else{
			putchar('<');  puts(aline);
		}

		// g_server_sock.sendto("ok, I see.\r\n", 12, &g_client_addr, 10);

		g_nrecvs += l;
	}
	xsys_sleep_ms(100);

	printf("recv_show end(%d bytes).\n", g_nrecvs);
	return 0;
}

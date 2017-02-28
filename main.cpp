#include "xsys.h"

#include "ss_service.h"
#include "xsys_tcp_server.h"
#include "xini.h"

xsys_tcp_server g_tcp_server("xsys_tcp_server");
xini			g_ini;
xsys_event		g_notify;
xsys_thread		g_client_thread;

static unsigned int tcp_client_run(void *);

int main(int argc, char *argv[])
{
	xsys_init();

	printf("Hello, here is xos test space, please enter the test cmd.\n");

	char b[1024];	int r;

	set_run_path("E:\\user\\develop\\xos\\develop\\xos");

	openservicelog("test_xos.log", true);
	WriteToEventLog("will in test");
	g_ini.open("test.ini");

	{
		g_tcp_server.open();
		long port = g_ini.get("tcp_server", "port", 8009);
		g_tcp_server.setport(port);
		g_tcp_server.start();
	}

	g_notify.init(1);
	g_client_thread.init(tcp_client_run, 0);

	for (;;) {
		try{
			scanf("%s", b);
			if (strcmp(b, "exit") == 0)
				break;
			if (b[0] == '?'){
				continue;
			}

			switch (b[0]){
				case 's':
					r = g_notify.set();
					break;

				default:
					break;
			}

			if (r == 0)
				printf("ok");
			else
				printf("error : %d", r);
		}catch(...){
			printf("some error occured.\n");
		}
	}
	g_notify.down();
	printf("notify down ok\n");
	g_client_thread.down();
	printf("g_client_thread.down ok\n");
	g_tcp_server.stop();
	printf("g_tcp_server.stop ok\n");
	g_tcp_server.close();
	printf("g_tcp_server.close ok\n");

	closeservicelog();

	xsys_shutdown();
	xsys_cleanup();

	printf("test end ok.\n");

	return 0;
}

static unsigned int tcp_client_run(void *)
{
	static const char szFunctionName[] = "sessionworker";

	char url[128], buf[2048];
	g_ini.get("tcp_client", "url", url);

	xsys_socket sock;
	while (g_notify.wait(24*3600) == 0) {
		printf("will connect to %s.\n", url);
		int r = sock.connect(url);
		printf("connect : %d\n", r);
		r = sock.sendblob("0000000010ABCDEabcde");
		printf("send : %d\n", r);
		// r = sock.recv(buf, 16, 3);
		r = sock.recvblob(buf);
		printf("recv : %d\n", r);
		WriteToEventLog("%s : recv[%s]", szFunctionName, buf);

		getnowtime(buf);
//		g_ini.set("tcp_client", "lasttime", buf);
		sock.close();
		g_notify.reset();
	}
	return 0;
}


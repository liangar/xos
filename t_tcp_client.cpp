#include "xsys.h"

xsys_event	ge_notifyarrive;	// 命令通知事件
int		g_idle = -1;
int		g_cmd  = 0;
int		n = 5, nsends, nrecvs, nfailsends;

xsys_socket g_sock;

static unsigned int DoRecvTest(void * pvoid);
static unsigned int DoSendTest(void * pvoid);

void main(void)
{
	xsys_init(true);

    ge_notifyarrive.init(1);

	int r = g_sock.connect("!:8900");

	xsys_thread h0,h1;
	h0.init(DoSendTest, 0);
	h1.init(DoRecvTest, 0);

	try{
		char * b = new char[4096];	int r = 0;
		nsends = nrecvs = nfailsends = 0;
		while (1){
			printf("\nenter 'exit' to stop: ");
			scanf("%s", b);
			if (lstrcmpi(b, "exit") == 0){
				break;
			}
			if (b[0] == '?'){
				continue;
			}
			g_cmd = b[0];

			switch (b[0]){
			case 's':	// 连续发送n个包
				n = atoi(b+1);  if (n < 1)  n = 1;
				// send packets
				ge_notifyarrive.set();
				break;

			case 'a':   // 自动随机发送,以连续发送包[1,r]
				n = atoi(b+1);  if (n < 1)  n = 5;
				break;

			case 't':	// 随机等待时间为[0, r] ms, r < 1,停止自动随机发送
				g_idle = atoi(b+1);
				break;

			case 'p':	// 打印当前状态
				printf("all : sends = %d, recvs = %d, nfailsends = %d.\n", nsends, nrecvs, nfailsends);
				break;
			}

			if (r == 0)
				printf("ok");
			else
				printf("error : %d", r);
		}
		delete[] b;
	}catch(...){
		printf("some error occured.\n");
	}

	ge_notifyarrive.set();
	g_cmd = -1;

	g_sock.close();

	h0.down();
	h1.down();

	ge_notifyarrive.down();

	xsys_shutdown();
	xsys_cleanup();

	printf("t_tcp_client end ok.\n");
}

static unsigned int DoSendTest(void * pvoid)
{
	int id = int(pvoid);

	char * p = new char[32 *1024];

	long idle;

	if (g_idle < 1)
		idle = SYS_INFINITE_TIMEOUT;
	else
		idle = long(rand() % g_idle);

	int r;

	while (((r = ge_notifyarrive.wait(idle)) == 0 || r == ERR_TIMEOUT) && g_cmd != -1){
		ge_notifyarrive.reset();
		for (int i = 0; i < n; i++){
			int l = int(rand() % 16 * 1024) + 100;
			memset(p, 'T', l);
			r = g_sock.sendblob(p, l, 3);

			if (r >= 0){
				printf("o");  nsends++;
			}else{
				printf("X");  nfailsends++;
			}
		}
		if (g_idle < 1)
			idle = SYS_INFINITE_TIMEOUT;
		else
			idle = long(rand() % g_idle);
	}
	delete[] p;
	printf("DoSendTest end.\n");
	return 0;
}

static unsigned int DoRecvTest(void * pvoid)
{
	char * p = new char[32 *1024];

	int l;
	while ((l = g_sock.recvblob(p, -1)) >= 0){
		if (l == 0){
			printf("x");
			continue;
		}

		printf(".");  nrecvs++;
	}
	delete[] p;
	printf("DoRecvTest end.\n");
	return 0;
}
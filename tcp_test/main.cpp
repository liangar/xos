#include <xsys.h>
#include <iostream>
#include <l_str.h>

#include "test_tcp.h"

#ifdef WIN32
#include <crtdbg.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif // _DEBUG
#endif // WIN32

char b[128*1024];

using namespace std;

int g_port;

int main(int argc, char **argv)
{
#ifdef WIN32
#ifdef _DEBUG
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );
#endif // _DEBUG
#endif // WIN32

	if (argc != 2){
		printf("usage: tcp_test <listen port>\n");
		return -1;
	}

	g_port = atoi(argv[1]);

	xsys_init(true);

	openservicelog("test_tcp.log", true, 60, true, "./");

	/*
	{
	test_server * psvr = new test_server;
	psvr->open();
	psvr->m_idle = 1;
	psvr->start();
	xsys_sleep(10);
	psvr->stop();
	psvr->close();
	delete psvr;
	}
	//*/

	test_tcp * ptcp = new test_tcp("TCP-测试",10);	///测试tcp服务
	xseq_buf	t_sends;			/// 测试队列	
	t_sends.init(32, 1000);
	int i=1;
	ptcp->m_idle = 30;
	ptcp->m_works= 2;
	cout << "please enter a command or '|' to exit: ";
	while(1)
	{
		cin.getline(b, 512);
		if(strcmp(b,"start")==0)
		{
			cout<< "command:start" <<endl;
			ptcp->open(g_port, 200, 10, 8*1024);
			ptcp->start();
		}else if (strcmp(b,"stop")==0)
		{
			cout<< "command:stop" <<endl;
			ptcp->stop();
			ptcp->close();
		}else if (strcmp(b,"put")==0)
		{
			cout<< "put:" <<i <<endl;
			char temp[10];
			sprintf(temp,"TestData%d",i);
			t_sends.put(i,temp);
			i++;
		}else if (strcmp(b,"get")==0)
		{
			xseq_buf_use use;
			int r = t_sends.get(&use);
			if(r>=0){
				cout << "get:(id=" << use.id <<") " << use.p  <<endl;
				i--;
			}else{
				cout << "not Data" <<endl;
			}
		}else if (strncmp(b,"send:", 5) == 0)
		{
			int i = atoi(b+5);

			cout << "请输入发送数据,exit退出发送命令" << endl;
			for (;;)
			{
				char s[512], d[512];
				cin.getline(s,512);
				if(strcmp(s,"exit")==0)
				{
					cout << "发送命令已退出" << endl;
					break;
				}
				int l = c2string(d, s);
				ptcp->send(i,d);
				printf("send (%d) bytes\n", l);
			}
		}else if (strncmp(b, "close:", 6) == 0){
			ptcp->session_close(atoi(b+6));
		}else if (strcmp(b,"|")==0){
			break;
		}else{
			cout << "=================command list ====================" << endl;
			cout<<"1 'start'    启动tcp服务" << endl;
			cout<<"2 'stop'     结束tcp服务" << endl;
			cout<<"3 'send:<i>' 发送数据" << endl;
			cout<<"4 'put'      测试队列，往队列放数据" << endl;
			cout<<"5 'get'      测试队列，从队列取数据" << endl;
			cout<<"6 'close:<i>'关闭会话"  << endl;
			cout<<"7 '|'  退出" << endl;
		}
	}

	t_sends.down();
	delete ptcp;

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

	return 0;
}

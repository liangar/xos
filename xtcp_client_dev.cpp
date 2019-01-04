#include <xsys.h>
#include <iostream>

#include <l_str.h>
#include <xtype.h>
#include <ss_service.h>

#include "xtcp_client_dev.h"

using namespace std;
xtcp_client_dev::xtcp_client_dev(const char * name, int recv_max_len)
	: xdev_comm(name, recv_max_len)
{
	m_exit_flag = 0;
}

xtcp_client_dev::~xtcp_client_dev()
{
}

unsigned int xtcp_client_dev::read_func(void* lparam)
{
	static const char szFunctionName[]="xtcp_client_dev::read_func";

	xtcp_client_dev * pworker = (xtcp_client_dev*)lparam;
	xsys_socket * psock = &pworker->m_sock;

	if (pworker->m_exit_flag || !psock->isopen())
		return 0;

	WriteToEventLog("%s:in",szFunctionName);

	char buf[256];
	
	int l;
	while ((l = psock->recv(buf, sizeof(buf), -1)) > 0){
        buf[l] = 0;
		WriteToEventLog("%s:read %d bytes.", szFunctionName, l);
		pworker->save_data(buf, l);
	}

	psock->close();

	WriteToEventLog("%s:out\n", szFunctionName);
	xsys_sleep_ms(100);
	return 0;
}


/// ip:port
int  xtcp_client_dev::open(const char * parms)
{
	int r = m_sock.connect(parms);

	// 创建串口的接收线程
	m_exit_flag = 0;
	r = m_read_thread.init(read_func, this);

	if (r != 0){
		WriteToEventLog("create tcp client read thread error(%d)\n", r);
		close();
		return -1;
	}

	WriteToEventLog("open tcp client ok\n");
	return 0;
}

int xtcp_client_dev::send(const char *buf, int len)
{
	static const char szFunctionName[] = "xtcp_client_dev::send";
	if (!m_sock.isopen()){
		WriteToEventLog("%s: socket has closed.", szFunctionName);
		return -1;
	}
	int l = m_sock.send(buf, len);
	
	WriteToEventLog("xtcp_client_dev::send(%d): write %d bytes.", len, l);
	return l;
}

int xtcp_client_dev::close()
{
	m_exit_flag = 1;
	if (m_sock.isopen()){
		m_sock.close();

		m_read_thread.wait(3);
	}

	m_read_thread.down();

	WriteToEventLog("xtcp_client_dev closed\n");
	return 0;
}


void test_tcp_client(char * b)
{
	xtcp_client_dev * pclient = new xtcp_client_dev("TEST TCP_CLIENT", 4096);

	pclient->init(0);

	for(;;)
	{
		cin.getline(b, 256);

		if (::stricmp(b, "exit") == 0)  break;
		if (b[0] == '?' || b[1] != ':'){	// show usage
			printf( "usage :"
				"\n\t'exit'            : exit the program"
				"\n\t'?:all'           : list all command"
				"\n\t'o:<ip:port>'     : connect to host, ep. 192.168.1.1:80"
				"\n\t's:<commend>'     : send command to port"
				"\n\t'H:<hex data>'    : send hex data"
				"\n\t'c:               : close"
				"\n");
			continue;
		}
		int r = 0;
		switch(b[0]){
			case 'o':
				r = pclient->open(b+2);
				break;

			case 's':
				pclient->clear_all_recv();

				r = strlen(b+2);
				b[r+2] = '\r';  b[r+2+1] = 0;
				c2string(b+2, b+2);
				r = pclient->sendstring((const char *)(b+2));

				b[256] = 0;
				r = pclient->recv(b+256, 256, 10000);
				if (r > 0)
					b[256+r-1] = 0;
				printf("r = %d, [%s]\n", r, b+256);

				break;

			case 'H':
				pclient->clear_all_recv();

				r = strlen(b+2);
				r = x_hex2array((unsigned char *)b+256, b+2, r*2);
				pclient->send(b+256, r);

				b[0] = 0;
				r = pclient->recv(b, 256, 10000);
				printf("r = %d:\n", r);
				{
					int i;
					for (i = 0; i < r; i++){
						printf("%02X ", (unsigned char)b[i]);
					}
					if (i)
						printf("\n");
				}
				break;

			case 'c':  r = pclient->close();  break;
		}

		xsys_sleep_ms(800);
		if (r < 0){
			printf("error = %d\n", r);
		}else
			printf("ok = %d\n", r);
	}
	pclient->down();

	delete pclient;
}

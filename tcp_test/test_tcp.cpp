#include <xsys_tcp_server.h>
#include "test_tcp.h"

test_tcp::test_tcp()
	: xsys_tcp_server()
{
}
test_tcp::test_tcp(const char * name, int nmaxexception)
: xsys_tcp_server(name, nmaxexception)
{
}

void test_tcp::check_recv_state(int i) 
{
	xtcp_session * psession = m_psessions + i;

	WriteToEventLog("check_recv_state(recv len = %d)", psession->recv_len);

	//*
	if (psession->recv_len < 4){
		return;
	}
	//*/
	const char * p = strchr(psession->precv_buf, ',');
	if (p){
		int len = atoi(psession->precv_buf);
		if (len <= psession->recv_len - int(p - psession->precv_buf) - 1){
			psession->recv_state = XTS_RECVED;
		}
	}
}

void test_tcp::check_send_state(int i) 
{
	xtcp_session * psession = m_psessions + i;

	WriteToEventLog("check_send_state(id = %d, send = %d, sent=%d)", i, psession->send_len, psession->sent_len);
}
bool test_tcp::on_connected(int i)
{
	xtcp_session * psession = m_psessions + i;

	WriteToEventLog("on_connected: id = %d, peer_ip = %s; self_ip = %s", i, psession->peerip, psession->servip);

	return true;
}

bool test_tcp::on_recved(int i)
{
	xtcp_session * psession = m_psessions + i;

	write_buf_log("RECVED", (unsigned char *)psession->precv_buf, psession->recv_len);
	return true;
}

bool test_tcp::on_closed(int i)
{
	return true;
}

test_server::test_server()
	: xwork_server("test_server", 5)
{
	m_works = 1;
}

void test_server::run(void)
{
	printf("...\n");
}

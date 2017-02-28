#ifndef XSYS_TCP_TEST_H
#define XSYS_TCP_TEST_H

#include <xsys_tcp_server.h>
class test_tcp : public xsys_tcp_server
{
public:
	/// ππ‘Ï
	test_tcp();
	~test_tcp(){};
	test_tcp(const char * name, int nmaxexception = 5);

	void check_recv_state(int i);
	void check_send_state(int i);

	bool on_connected(int i);
	bool on_recved   (int i);
	bool on_closed   (int i);
};

class test_server : public xwork_server
{
public:
	test_server();
	void run(void);
};

#endif // XSYS_TCP_TEST_H

#pragma once

#include <xasc_server.h>

class test_tcp : public xasc_server
{
public:
	test_tcp();

protected:
	int  calc_pkg_len(const unsigned char * pbuf, int len);

	int  do_msg(int i, char * msg, int msg_len);

	int  do_idle(int i);	/// 空闲处理
	bool on_sent(int i, int len);	/// 发送完成
};

#include "test_tcp.h"

test_tcp::test_tcp()
	: xasc_server("TEST_XASC")
{
}

int test_tcp::calc_pkg_len(const unsigned char * pbuf, int len)
{
	const char * p = strchr((const char *)pbuf, '\n');
	if (p >= (const char *)pbuf && p < (const char * )pbuf+len)
		return len;

	if (len < most_len_)
		return most_len_;

	return max_len_;
}

int test_tcp::do_msg(int i, char * msg, int msg_len)
{
	log_print("[%d]<-: {%s}", i, msg);
	return 0;
}

/// 空闲处理
int test_tcp::do_idle(int i)
{
	log_print("[%d]: idle", i);
	
	return 0;
}

/// 发送完成
bool test_tcp::on_sent(int i, int len)
{
	log_print("[%d]: sent %d bytes", i, len);

	return true;
}

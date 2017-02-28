// xtcp_client_dev.h: interface for the xtcp_client_dev class.
//
//////////////////////////////////////////////////////////////////////
#ifndef _XTCP_Client_H
#define _XTCP_Client_H

#include <xsys.h>

#include "xdev_comm.h"

class xtcp_client_dev : public xdev_comm
{
public:
	xtcp_client_dev(const char * name, int recv_max_len);
	virtual ~xtcp_client_dev();

	int  open(const char * parms);	/// 1:9600;N;8;1
	int  close(void);

	int  send(const char* buf, int len);
	bool isopen(void)  {  return (m_sock.isopen() != 0);  }

protected:
	xsys_socket m_sock;

	// 退出数据接收线程标志
	int  m_exit_flag;

protected:
	xsys_thread	m_read_thread;
	xsys_mutex  m_buf_lock;
	xsys_event  m_has_data;

	// 串口数据接收线程
	static unsigned int read_func(void* lparam);
};

#endif // _XTCP_Client_H
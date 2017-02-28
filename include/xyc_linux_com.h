// xserial_port.h: interface for the xserial_port class.
//
//////////////////////////////////////////////////////////////////////
#ifndef _XYC_COM_H
#define _XYC_COM_H

#include <xsys.h>

#include "xdev_comm.h"

class xserial_port : public xdev_comm
{
public:
	xserial_port();
	virtual ~xserial_port();

	int  open(const char * parms);	/// 1:9600;N;8;1
	int  close(void);

	int  send(const char* buf, int len);
	bool isopen(void)  {  return (m_fd > 0);  }

protected:
	int  open(int port, int baudrate, char bits, char stopbits, char parity);


// 已打开的串口文件描述符
	int m_fd;

	// 退出数据接收线程标志
	int  m_exit_flag;

protected:
	xsys_thread	m_read_thread;
	xsys_mutex  m_buf_lock;
	xsys_event  m_has_data;

	// 串口数据接收线程
	static unsigned int read_func(void* lparam);
};

#endif // _XYC_COM_H

// xserial_port.h: interface for the xserial_port class.
//
//////////////////////////////////////////////////////////////////////
#ifndef _XSERIALPORT_H
#define _XSERIALPORT_H

#include <xsys.h>

class xserial_port
{
public:
	xserial_port();
	virtual ~xserial_port();

	int  open(int port, int baudrate, char bits, char stopbits, char parity);
	int  open(const char * parms);	/// 1:9600;N;8;1
	int  close(void);

	bool isopen(void)  {  return (m_fd > 0);  }

	int  send(const char* buf, int len);
	int  send(const char* buf);
	int  recv(char* buf, int maxlen, int msec);
	int  peek(char & c);
	int  peek(char * buf, int maxlen);
	void skip(int bytes);
	int  recv_bytes(char* buf, int maxlen, int seconds);
	int  recv_byend (char * buf, int maxlen, char * endstr, int seconds);
	int  recv_byend (char * buf, int maxlen, char endch, int seconds);

	void clear_all_recv(void);

protected:
	// 已打开的串口文件描述符
	int m_fd;

	// 串口数据接收Buffer
	int  m_recv_len;
	char m_recv_buf[1024];

	// 退出数据接收线程标志
	int  m_exit_flag;

protected:
	xsys_thread	m_read_thread;
	xsys_mutex  m_buf_lock;
	xsys_event  m_has_data;

	// 串口数据接收线程
	static unsigned int read_func(void* lparam);

	void save_data(const char * buf, int len);
};

#endif // _XSERIALPORT_H

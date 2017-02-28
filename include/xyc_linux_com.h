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


// �Ѵ򿪵Ĵ����ļ�������
	int m_fd;

	// �˳����ݽ����̱߳�־
	int  m_exit_flag;

protected:
	xsys_thread	m_read_thread;
	xsys_mutex  m_buf_lock;
	xsys_event  m_has_data;

	// �������ݽ����߳�
	static unsigned int read_func(void* lparam);
};

#endif // _XYC_COM_H

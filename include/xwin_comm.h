#ifndef _XWIN_COMM_H_
#define _XWIN_COMM_H_

#include "xdev_comm.h"

#define COMM_BUF_LEN 4*1024

class xserial_port : public xdev_comm
{
public:
	xserial_port();
	virtual ~xserial_port();

	int  open(const char * parms);	/// 1:9600;N;8;1
	int  close(void);

	int  send(const char *buf, int len);
	bool isopen(void);

	void set_485GPIO_ctl(int n_485_gpio)  {  m_485_gpio = n_485_gpio;  }
public:
    HANDLE m_hHasData;

protected:
	int  open(void);

	void SetBaud( int baud );
	void SetPort( int n );
    void SetNotify(HANDLE hHasData) {  m_hHasData = hHasData;  }
	int  GetInCount();

	int  Recv(void *buf, int len);
	int  ReadComm(void *lpszBlock, int nMaxLength);

protected:
	static void WINAPI THWorker(xserial_port *comm);
	void Worker();

protected:
	void        *m_parm;

    HANDLE      m_hPort;

	int         m_baudRate;
	BYTE		m_Parity;
	int         m_nPort;
	BYTE		m_StopBits;
	BOOL		m_XonXoffControl;

	int m_485_gpio;

	OVERLAPPED  m_ovRead;
	OVERLAPPED  m_ovWrite;
	OVERLAPPED  m_ovReadThread;

	HANDLE      m_hThread;
	DWORD       m_dwThreadId;
	BOOL        m_bWantExitThread;
	BOOL        m_bThreadExited;
};

#endif

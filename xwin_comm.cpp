#include <xsys.h>
#include <stdio.h>
#include <iostream>

#include <l_str.h>
#include <xtype.h>

#include "xwin_comm.h"

static LPSTR  defaultSetting = "COM1\x0 baud=9600 parity=N data=8 stop=1";

#define INCREASE_BUF_LEN 1024

using namespace std;

xserial_port::xserial_port()
    : xdev_comm("xwin_comm", 1024)
    , m_hPort(INVALID_HANDLE_VALUE) // invalidate to start
    , m_baudRate(9600)
    , m_Parity(NOPARITY)
    , m_StopBits(ONESTOPBIT)
    , m_nPort(1)
    , m_hThread(NULL)
    , m_bWantExitThread(FALSE)
    , m_bThreadExited(TRUE)
{
    m_ovRead.hEvent = m_ovWrite.hEvent = NULL;
    memset(&m_ovRead, 0, sizeof(m_ovRead));
    memset(&m_ovWrite, 0, sizeof(m_ovWrite));
    if ((m_ovRead.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )) == NULL)
		throw "can't create event1 in xserial_port::xserial_port";

    if ((m_ovWrite.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )) == NULL)
		throw "can't create event2 in xserial_port::xserial_port";
}

xserial_port::~xserial_port()
{
    if (m_ovRead.hEvent != NULL)
    	CloseHandle(m_ovRead.hEvent);
    if (m_ovWrite.hEvent != NULL)
    	CloseHandle(m_ovWrite.hEvent);
}

void WINAPI xserial_port::THWorker(xserial_port *comm)
{
	try {
		comm->Worker();
	}catch(...){  // const char *s) {
		// TRACE("Exception in xserial_port::THworker %s", s);
		comm->m_bThreadExited = TRUE;
	}
}

void xserial_port::Worker()
{
	m_bThreadExited = TRUE;

	if (m_hPort == INVALID_HANDLE_VALUE ||
        !SetCommMask(m_hPort, EV_RXCHAR ) ) {
		return;
	}

    m_bThreadExited = FALSE;
	m_bWantExitThread = FALSE;
	while ( !m_bWantExitThread ) {
		DWORD dwEvtMask = 0;

		WaitCommEvent( m_hPort, &dwEvtMask, NULL );

		if(m_bWantExitThread)
			break;

		if ((dwEvtMask & EV_RXCHAR) == EV_RXCHAR) {
			Sleep(10);
			int nBytesNew = GetInCount();

			if (nBytesNew < 1)
				continue;;

			char temp[512];
			temp[0] = 0;
			int ret = Recv(temp, min(sizeof(temp), nBytesNew));
			save_data(temp, ret);
		}
	}

	m_bThreadExited = TRUE;
}


void xserial_port::SetBaud( int baud )
{
	if(m_baudRate != baud) {
		m_baudRate = baud;
		if(isopen()) {
			close();
			open();
		}
	}
}


void xserial_port::SetPort( int n )
{
	if(m_nPort != n) {
		m_nPort = n;
		if(isopen()) {
			close();
			open();
		}
	}
}

int xserial_port::GetInCount()
{
	COMSTAT statPort;
	DWORD dwErrorCode = 0;

	if(isopen()) {
		int len;
		statPort.cbInQue = 0;
		do{
			len = statPort.cbInQue;
			Sleep(3);
			ClearCommError(m_hPort, &dwErrorCode, &statPort);
		}while(len != statPort.cbInQue);

		return len;
	}
	return 0;
}

int xserial_port::open()
{
	char sCom[20];
    COMMTIMEOUTS  CommTimeOuts ;

	if(isopen())
		close();

    sprintf(sCom, "COM%d", m_nPort);
	m_hPort = CreateFile(sCom, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
	if (m_hPort == INVALID_HANDLE_VALUE) {
		// TRACE("Error open %s", sCom);
		return -1;
    }

	// get any early notifications
	SetCommMask( m_hPort, EV_RXCHAR ) ;

	// setup device buffers
	SetupComm(m_hPort, COMM_BUF_LEN, COMM_BUF_LEN);

	// purge any information in the buffer
	PurgeComm(m_hPort, PURGE_TXABORT | PURGE_RXABORT |
		PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O

	CommTimeOuts.ReadIntervalTimeout = 100;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 100;
	CommTimeOuts.ReadTotalTimeoutConstant = 800;

	// write timeout = WriteTotalTimeoutMultiplier * bytes_to_send + WriteTotalTimeoutConstant
	// CBR_9600 is approximately 1byte/ms. so if baud rate < 9600
	// we can use CBR_9600/rate to estimate the timeout.
	// if > 9600, use 1. here, we use 10 to all.
	CommTimeOuts.WriteTotalTimeoutMultiplier = 100;
	CommTimeOuts.WriteTotalTimeoutConstant = 2000;	//-1; //!! orginal is 0 ;

	SetCommTimeouts( m_hPort, &CommTimeOuts ) ;	

	DCB dcbPort;
	memset(&dcbPort, 0, sizeof(DCB));
	dcbPort.DCBlength = sizeof( DCB ) ;
	if (GetCommState(m_hPort, &dcbPort)) {
		// fill in the fields of the structure
		dcbPort.BaudRate = m_baudRate;
		dcbPort.ByteSize = 8;
		dcbPort.Parity = m_Parity;
		dcbPort.StopBits = m_StopBits;

		/*
		// no flow control
		dcbPort.fOutxCtsFlow = FALSE;
		dcbPort.fOutxDsrFlow = FALSE;
		dcbPort.fOutX = FALSE;
		dcbPort.fInX = FALSE;
		*/

		/*
		// DsrDtrFlowControl
		dcbPort.fOutxCtsFlow = FALSE;
		dcbPort.fOutxDsrFlow = TRUE;
		dcbPort.fDtrControl = DTR_CONTROL_HANDSHAKE;
		dcbPort.fOutX = FALSE;
		dcbPort.fInX = FALSE;
		*/

		/*
		if (m_XonXoffControl){
			// xon/xoff control
			dcbPort.fOutxCtsFlow = FALSE;
			dcbPort.fOutxDsrFlow = FALSE;
			dcbPort.fOutX = TRUE;
			dcbPort.fInX = TRUE;
			dcbPort.XonChar = 0x11;
			dcbPort.XoffChar = 0x13;
			dcbPort.XoffLim = 32;
			dcbPort.XonLim = 32;
		}
		*/

		dcbPort.fBinary = TRUE ;
		// dcbPort.fParity = TRUE ;

		SetCommState(m_hPort, &dcbPort);
        EscapeCommFunction( m_hPort, SETDTR ) ;
    }

	DWORD dwThreadId;
	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)THWorker,
		this, CREATE_SUSPENDED, &dwThreadId);

	if (m_hThread == NULL)
		throw "xserial_port::open can't create worker thread";

	m_recv_len = 0;

	//	SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
	ResumeThread(m_hThread);

    return 0;
}

int xserial_port::open(const char * parms)
{
	DCB   d;	char sCom[32];
	const char * lps = NULL;
	const char * lpCommName;

	memset(&d, 0, sizeof(d));

	lpCommName = parms;
	if (parms != NULL && (lps = strchr(parms, ':')) != NULL)
		lps += 1;

	if (lpCommName == NULL)
		lpCommName = defaultSetting;
	if (lps == NULL){
        lps = defaultSetting + 6;
    }
    if (strnicmp(lpCommName, "COM", 3) != 0 &&
		(lpCommName[1] != ':' || *lpCommName < '1' || *lpCommName > '9')
		)
        return -1;

	COMMCONFIG cc;  DWORD dwSize = sizeof(cc);

	getaword(sCom, (char *)lpCommName, ':');

	memset(&cc, 0, sizeof(cc));
	cc.dcb.DCBlength = sizeof(DCB);
	BOOL bisok = GetDefaultCommConfig(sCom, &cc, &dwSize);
	memcpy(&d, &cc.dcb, sizeof(d));
    if (BuildCommDCB(lps, &d) == 0)     // ep. "baud=1200 parity=N data=8 stop=1 "
        return -2;

	m_Parity = d.Parity;
	m_StopBits = d.StopBits;
	if (sCom[0] >= '1' && sCom[0] <= '9')
		m_nPort = int(sCom[0] - '0');
	else
		m_nPort = int(sCom[3] - '0');
	m_baudRate = d.BaudRate;
	m_XonXoffControl = d.fOutX;

    return open();
}

int xserial_port::close(void)
{
	if (!m_bThreadExited) {
		m_bWantExitThread = TRUE;
		SetCommMask( m_hPort,  0 ) ;
		if (WaitForSingleObject(m_hThread, 2000) != WAIT_OBJECT_0)
            TerminateThread(m_hThread, 0);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	if (m_hPort != INVALID_HANDLE_VALUE) {
		EscapeCommFunction( m_hPort, CLRDTR ) ;
		// purge any outstanding reads/writes and close device handle
		PurgeComm( m_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
		CloseHandle(m_hPort);
		m_hPort = INVALID_HANDLE_VALUE;
    }
	m_recv_len = 0;

	return 0;
}

bool xserial_port::isopen(void)
{
    return (m_hPort != INVALID_HANDLE_VALUE);
}

// return bytes recv
int xserial_port::Recv(void *lpszBlock, int nMaxLength)
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	DWORD      dwError;

	// only try to read number of bytes in queue
	ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
	dwLength = min( (DWORD) nMaxLength, ComStat.cbInQue ) ;

	if(dwLength==0)
		return 0;

	if ((fReadStat = ReadFile(m_hPort, lpszBlock, dwLength, &dwLength, &m_ovRead)) == 0){
		if (GetLastError() == ERROR_IO_PENDING){
			//            OutputDebugString("\n\rIO Pending");
			// We have to wait for read to complete.
			// This function will timeout according to the
			// CommTimeOuts.ReadTotalTimeoutConstant variable
			// Every time it times out, check for port errors
			while(!GetOverlappedResult( m_hPort, &m_ovRead, &dwLength, TRUE )){
				dwError = GetLastError();
				if(dwError == ERROR_IO_INCOMPLETE)
					// normal result if not finished
					continue;
				else{
					ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
					// TRACE("Comm Err: %u",  dwErrorFlags);
					break;
				}
			}
		}else{
			// some other error occurred
			dwLength = 0 ;
			ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
			// TRACE("Comm Err: %u",  dwErrorFlags);
		}
	}
	return ( dwLength ) ;
}

int xserial_port::send(const char *lpByte, int dwBytesToWrite)
{
	BOOL        fWriteStat ;
	DWORD       dwBytesWritten ;
	DWORD       dwErrorFlags;
	DWORD       dwBytesSent = 0;
	COMSTAT     ComStat;

	fWriteStat = WriteFile( m_hPort, lpByte, dwBytesToWrite, &dwBytesWritten, &m_ovWrite ) ;
	// return TRUE;

	FlushFileBuffers(m_hPort);
	// Note that normally the code will not execute the following
	// because the driver caches write operations. Small I/O requests
	// (up to several thousand bytes) will normally be accepted
	// immediately and WriteFile will return true even though an
	// overlapped operation was specified

	if (!fWriteStat){
		if(GetLastError() == ERROR_IO_PENDING){
			// We should wait for the completion of the write operation
			// so we know if it worked or not

			// This is only one way to do this. It might be beneficial to
			// place the write operation in a separate thread
			// so that blocking on completion will not negatively
			// affect the responsiveness of the UI

			// If the write takes too long to complete, this
			// function will timeout according to the
			// CommTimeOuts.WriteTotalTimeoutMultiplier variable.
			// This code logs the timeout but does not retry
			// the write.

			if (!GetOverlappedResult( m_hPort, &m_ovWrite, &dwBytesWritten, TRUE )){
				ClearCommError( m_hPort, &dwErrorFlags, &ComStat ) ;
				return FALSE;
			}

			dwBytesSent += dwBytesWritten;
		}else{
			// some other error occurred
			ClearCommError( m_hPort, &dwErrorFlags, &ComStat) ;
			// TRACE("Comm Err: %u",  dwErrorFlags);
			return -1;
		}
	}
	return dwBytesSent;
}

void test_serial_port(char * b)
{
	xserial_port * pport = new xserial_port;

	pport->init(NULL);

	for(;;)
	{
		cin.getline(b, 2*1024);
		
		if (::stricmp(b, "exit") == 0)  break;
		if (b[0] == '?' || b[1] != ':'){	// show usage
			printf( "usage :"
				"\n\t'exit'            : exit the program"
				"\n\t'?:all'           : list all command"
				"\n\t'o:<port>'        : open port, ep. 1:9600,N,8,1"
				"\n\t's:<commend>'     : send command to port"
				"\n\t'H:<hex data>'    : send hex data"
				"\n\t'c:               : close port"
				"\n");
			continue;
		}
		int r = 0;
		switch(b[0]){
		case 'o':
			r = pport->open(b+2);
			break;
			
		case 's':
			pport->clear_all_recv();
			
			r = strlen(b+2);
			b[r+2] = '\r';  b[r+2+1] = 0;
			r = pport->sendstring((const char *)(b+2));
			
			r = pport->recv(b+256, 256, 10000);
			printf("r = %d, [%s]\n", r, b+256);
			
			break;
			
		case 'H':
			pport->clear_all_recv();
			
			r = strlen(b+2);
			r = x_hex2array((unsigned char *)b+256, b+2, r*2);
			pport->send(b+256, r);
			
			r = pport->recv_bytes(b, 2*1024, 10000);
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
			
		case 'c':  r = pport->close();  break;
		}
		
		xsys_sleep_ms(800);
		if (r < 0){
			printf("error = %d\n", r);
		}else
			printf("ok = %d\n", r);
	}
	pport->down();

	delete pport;
}

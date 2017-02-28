/*!
	Module : POP3.H
	Purpose: Defines the interface for a MFC class encapsulation of the POP3 protocol
	Created: PJN / 04-05-1998
	
	Copyright (c) 1998 - 2001 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)

	All rights reserved.
	
	Copyright / Usage Details:
	  
	You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
	when your product is released in binary form. You are allowed to modify the source code in any way you want 
	except you cannot modify the copyright details at the top of each module. If you want to distribute source 
	code with your application, then you are only allowed to distribute versions released by the author. This is 
	to maintain a single distribution point for the source code. 
*/


/////////////////////////////// Defines ///////////////////////////////////////
#ifndef _CPop3_H_
#define _CPop3_H_

#include <xsys.h>

#ifdef WIN32
#pragma warning(disable: 4786)
#endif

#include <string>

#include <xlist.h>


using namespace std;

/////////////////////////////// Classes ///////////////////////////////////////

///The main class which encapsulates the POP3 connection
class CPop3Connection
{
public:
	///Constructors / Destructors
	CPop3Connection();
	~CPop3Connection();
	
	///Methods
	bool	Connect(char * pszHostName, const char * pszUser, const char * pszPassword, int nPort = 110);
	bool	Disconnect();
	bool	Statistics(int& nNumberOfMails, int& nTotalMailSize);
	bool	Delete(int nMsg);
	bool	GetMessageSize(int nMsg, int& dwSize);
	bool	GetMessageID(int nMsg, string& sID);
	bool	Retrieve(int nMsg, char ** message);
	bool	GetMessageHeader(int nMsg, char ** message);
	bool	Reset();
	bool	UIDL();
	bool	Noop();
	string 	GetLastCommandResponse() const { return m_sLastCommandResponse; };
	int	GetTimeout() const { return m_dwTimeout; };
	void	SetTimeout(int dwTimeout) { m_dwTimeout = dwTimeout; };
	
protected:
	virtual bool	ReadStatResponse(int& nNumberOfMails, int& nTotalMailSize);
	virtual bool	ReadCommandResponse();
	virtual bool	ReadListResponse(int nNumberOfMails);
	virtual bool	ReadUIDLResponse(int nNumberOfMails);
	virtual bool	ReadReturnResponse(char ** messagee, int dwSize = 1024);
	virtual bool	ReadResponse(char ** pszBuffer, const char * pszTerminator, int & nReceived, int nGrowBy=256);
	bool			List();
	char *			GetFirstCharInResponse(char * pszData) const;
	
	xsys_socket		m_socket;
	int 			m_nNumberOfMails;
	bool			m_bListRetrieved;
	bool			m_bStatRetrieved;
	bool			m_bUIDLRetrieved;
	xlist_s<int>	m_msgSizes;
	xlist_p<char *>	m_msgIDs;
	bool			m_bConnected;
	string			m_sLastCommandResponse;
	int		m_dwTimeout;
};

#endif // _CPop3_H_


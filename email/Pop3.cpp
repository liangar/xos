/*
Module : POP3.CPP
Purpose: Implementation for a MFC class encapsulation of the POP3 protocol
Created: PJN / 04-05-1998
History: http://www.naughter.com/
*/

//////////////// Includes ////////////////////////////////////////////

#include <Pop3.h>

#include <l_str.h>
#include <ss_service.h>

#include "xemail_string.h"

CPop3Connection::CPop3Connection()
{
	m_nNumberOfMails = 0;
	m_bListRetrieved = false;
	m_bStatRetrieved = false;
	m_bUIDLRetrieved = false;
	m_bConnected = false;
	m_dwTimeout = 20;  //default timeout of 2 seconds for normal release code
}

CPop3Connection::~CPop3Connection()
{
	Disconnect();
}

bool CPop3Connection::Connect(char * pszHostName, const char * pszUser, const char * pszPassword, int nPort)
{
	//Connect to the POP3 Host
	int r;
	if ((r = m_socket.connect(pszHostName, nPort)) < 0) {
		WriteToEventLog("Could not connect to the POP3 mailbox(%s), xlasterror:%s\n", pszHostName, xlasterror());
		return false;
	}
	//We're now connected !!
	m_bConnected = true;

	try{
		//check the response
		if (!ReadCommandResponse()) {
			throw "connected to read response\n";
		}

		//Send the POP3 username and check the response
		char sBuf[128];
		// ASSERT(strlen(pszAsciiUser) < 100); 
		sprintf(sBuf, "USER %s\r\n", pszUser);
		if (m_socket.send_all(sBuf) <= 0) {
			throw "Send the USER command\n";
		}
		if (!ReadCommandResponse()) {
			throw "Read a USER command response\n";
		} 

		//Send the POP3 password and check the response
		// ASSERT(strlen(pszAsciiPassword) < 100);
		sprintf(sBuf, "PASS %s\r\n", pszPassword);
		if (m_socket.send_all(sBuf) <= 0) {
			throw "Send the PASS command\n";
		}
		if (!ReadCommandResponse()){
			throw "read a PASS command response\n";
		}
	}catch (const char * msg) {
		WriteToEventLog("POP3 Failed : %s", msg);
		Disconnect();
		return false;
	}

	return true;
}

bool CPop3Connection::Disconnect()
{		   
	bool bSuccess = false;		

	//disconnect from the POP3 server if connected 
	if (m_bConnected)
	{
		if (m_socket.send_all("QUIT\r\n") <= 0){
			WriteToEventLog("Failed to send the QUIT command to the POP3 server\n");
			bSuccess = false;
		}else
			bSuccess = ReadCommandResponse();

		//Reset all the state variables
		m_bConnected = false;
		m_bListRetrieved = false;
		m_bStatRetrieved = false;
		m_bUIDLRetrieved = false;
	}else
		WriteToEventLog("CPop3Connection, Already disconnected\n");

	//free up our socket
	m_socket.close();

	m_msgSizes.free_all();

	return bSuccess;
}

bool CPop3Connection::Delete(int nMsg)
{
	bool bSuccess = true;

	//Must be connected to perform a delete
	// ASSERT(m_bConnected);

	//if we haven't executed the LIST command then do it now
	if (!m_bListRetrieved)
		bSuccess = List();

	//Handle the error if necessary  
	if (!bSuccess)
		return false;

	//Send the DELE command along with the message ID
	char sBuf[20];
	sprintf(sBuf, "DELE %d\r\n", nMsg);
	if (m_socket.send_all(sBuf) <= 0)
	{
		WriteToEventLog("Failed to send the DELE command to the POP3 server\n");
		return false;
	}

	return ReadCommandResponse();
}

bool CPop3Connection::Statistics(int& nNumberOfMails, int& nTotalMailSize)
{
	//Must be connected to perform a "STAT"
	// ASSERT(m_bConnected);

	//Send the STAT command
	char sBuf[10];
	strcpy(sBuf, "STAT\r\n");
	if (m_socket.send_all(sBuf) <= 0)
	{
		WriteToEventLog("Failed to send the STAT command to the POP3 server\n");
		return false;
	}

	return ReadStatResponse(nNumberOfMails, nTotalMailSize);
}

bool CPop3Connection::GetMessageSize(int nMsg, int& dwSize)
{
	bool bSuccess = true;

	//if we haven't executed the LIST command then do it now
	if (!m_bListRetrieved)
		bSuccess = List();

	//Handle the error if necessary  
	if (!bSuccess)
		return false;

	//nMsg must be in the correct range
	// ASSERT((nMsg > 0) && (nMsg <= m_msgSizes.GetSize()));

	//retrieve the size from the message size array
	dwSize = m_msgSizes[nMsg - 1];

	return bSuccess;
}

bool CPop3Connection::GetMessageID(int nMsg, string& sID)
{
	bool bSuccess = true;

	//if we haven't executed the UIDL command then do it now
	if (!m_bUIDLRetrieved)
		bSuccess = UIDL();

	//Handle the error if necessary  
	if (!bSuccess)
		return false;

	//nMsg must be in the correct range
	// ASSERT((nMsg > 0) && (nMsg <= m_msgIDs.m_count));

	//retrieve the size from the message size array
	sID = m_msgIDs[nMsg - 1];

	return bSuccess;
}

bool CPop3Connection::List()
{
	//Must be connected to perform a "LIST"
	// ASSERT(m_bConnected);

	//if we haven't executed the STAT command then do it now
	int nNumberOfMails = m_nNumberOfMails;
	int nTotalMailSize;
	if (!m_bStatRetrieved)
	{
		if (!Statistics(nNumberOfMails, nTotalMailSize))
			return false;
		else
			m_bStatRetrieved = true;
	}

	//Send the LIST command
	char sBuf[10];
	strcpy(sBuf, "LIST\r\n");
	if (m_socket.send_all(sBuf) <= 0)
	{
		WriteToEventLog("Failed to send the LIST command to the POP3 server\n");
		return false;
	}
	//And check the response
	m_bListRetrieved = ReadListResponse(nNumberOfMails);
	return m_bListRetrieved;
}

bool CPop3Connection::UIDL()
{
	//Must be connected to perform a "UIDL"
	// ASSERT(m_bConnected);

	//if we haven't executed the STAT command then do it now
	int nNumberOfMails = m_nNumberOfMails;
	int nTotalMailSize;
	if (!m_bStatRetrieved)
	{
		if (!Statistics(nNumberOfMails, nTotalMailSize))
			return false;
		else
			m_bStatRetrieved = true;
	}

	//Send the UIDL command
	if (m_socket.send_all("UIDL\r\n") <= 0)
	{
		WriteToEventLog("Failed to send the UIDL command to the POP3 server\n");
		return false;
	}
	//And check the response
	m_bUIDLRetrieved = ReadUIDLResponse(nNumberOfMails);
	return m_bUIDLRetrieved;
}

bool CPop3Connection::Reset()
{
	//Must be connected to perform a "RSET"

	//Send the RSET command
	if (m_socket.send_all("RSET\r\n") <= 0)
	{
		WriteToEventLog("Failed to send the RSET command to the POP3 server\n");
		return false;
	}

	//And check the command
	return ReadCommandResponse();
}

bool CPop3Connection::Noop()
{
	//Must be connected to perform a "NOOP"

	//Send the NOOP command
	if (m_socket.send_all("NOOP\r\n") <= 0)
	{
		WriteToEventLog("Failed to send the NOOP command to the POP3 server\n");
		return false;
	}

	//And check the response
	return ReadCommandResponse();
}

bool CPop3Connection::Retrieve(int nMsg, char ** message)
{
	//Must be connected to retrieve a message
	// ASSERT(m_bConnected);

	//work out the size of the message to retrieve
	int dwSize;
	if (GetMessageSize(nMsg, dwSize))
	{
		//Send the RETR command
		char sBuf[20];
		int nCmdLength = sprintf(sBuf, "RETR %d\r\n", nMsg); 
		if (m_socket.send_all(sBuf, nCmdLength) != nCmdLength)
		{
			WriteToEventLog("Failed to send the RETR command to the POP3 server\n");
			return false;
		}

		//And check the command
		return ReadReturnResponse(message, dwSize);
	}

	return false;
}

bool CPop3Connection::GetMessageHeader(int nMsg, char ** message)
{
	// Must be connected to retrieve a message
	// ASSERT(m_bConnected);

	// make sure the message actually exists
	int dwSize;
	if (GetMessageSize(nMsg, dwSize)) {
		// Send the TOP command
		char sBuf[16];
		int nCmdLength = sprintf(sBuf, "TOP %d 0\r\n", nMsg);
		if (m_socket.send_all(sBuf, nCmdLength) != nCmdLength)
		{
			WriteToEventLog("Failed to send the TOP command to the POP3 server\n");
			return false;
		}

		// And check the command
		return ReadReturnResponse(message, dwSize);
	}

	return false;
}

bool CPop3Connection::ReadCommandResponse()
{
	char * p = NULL;
	int  nMessageSize;
	bool bSuccess = ReadResponse(&p, "\r\n", nMessageSize);
	if (*p) {
		free(p);
	}

	return bSuccess;
}

char * CPop3Connection::GetFirstCharInResponse(char * pszData) const
{
	while ((*pszData != '\n') && *pszData)
		++pszData;

	//skip over the "\n" onto the next line
	if (*pszData)
		++pszData;

	return pszData;
}

bool CPop3Connection::ReadResponse(char ** p, const char * pszTerminator, int &nReceived, int nGrowBy)
{
	//retrieve the reponse using until we
	//get the terminator or a timeout occurs
	nReceived = m_socket.recv_byend(p, pszTerminator, nGrowBy);
	if (nReceived < 0) {
		return false;
	}
	if (nReceived == 0) {
		if (*p) {
			free(*p);
			*p = 0;
			return false;
		}
	}

	//determine if the response is an error
	bool bSuccess = (_strnicmp(*p,"+OK", 3) == 0);

	if (!bSuccess)
	{
		m_sLastCommandResponse = *p; //Hive away the last command reponse
		WriteToEventLog("POP3 : %s", *p);
	}

	return bSuccess;
}

bool CPop3Connection::ReadReturnResponse(char ** message, int dwSize)
{
	//Must be connected to perform a "RETR"
	// ASSERT(m_bConnected);

	//Retrieve the message body
	int nMessageSize;
	if (!ReadResponse(message, "\r\n.\r\n", nMessageSize, dwSize)){
		WriteToEventLog("Error retrieving the RETR response");
		return false;
	}

	//remove the first line which contains the +OK from the message
	char* pszFirst = GetFirstCharInResponse(*message);
	// VERIFY(pszFirst);

	//transfer the message contents to the message class
	nMessageSize -= int(*message - pszFirst);

	memmove(*message, pszFirst, nMessageSize);
	(*message)[nMessageSize] = '\0';

	return true; 
}

bool CPop3Connection::ReadListResponse(int nNumberOfMails)
{
	//Must be connected to perform a "LIST"
	// ASSERT(m_bConnected);

	//retrieve the reponse
	char* sBuf = 0;
	int nMessageSize;
	if (!ReadResponse(&sBuf, "\r\n.\r\n", nMessageSize, 2048))
	{
		WriteToEventLog("Error retrieving the LIST response from the POP3 server");
		return false;
	}

	//Retrieve the message sizes and put them
	//into the m_msgSizes array
	m_msgSizes.free_all();
	m_msgSizes.init(nNumberOfMails, nNumberOfMails/2);

	//then parse the LIST response
	char* pszSize = GetFirstCharInResponse(sBuf);
	// VERIFY(pszSize);
	for (; *pszSize; pszSize++)
		if (*pszSize == '\t' || *pszSize == ' '){
			int s = atoi(pszSize);
			m_msgSizes.add(int(s));
		}

	free(sBuf);

	return true; 
}

bool CPop3Connection::ReadUIDLResponse(int nNumberOfMails)
{
	//Must be connected to perform a "UIDL"
	// ASSERT(m_bConnected);

	//retrieve the reponse
	int nSize;
	char* sBuf = 0;
	if (!ReadResponse(&sBuf, "\r\n.\r\n", nSize, nNumberOfMails*14))
	{
		WriteToEventLog("Error retrieving the UIDL response from the POP3 server");
		return false;
	}

	//Retrieve the message ID's and put them
	//into the m_msgIDs array
	m_msgIDs.free_all();
	m_msgIDs.init(nNumberOfMails, nNumberOfMails/2);

	//then parse the UIDL response
	char* pszSize = GetFirstCharInResponse(sBuf);

	// VERIFY(pszSize);
	for (int iCount=0; iCount < nNumberOfMails; iCount++)
	{
		char* pszBegin = pszSize;
		while (*pszSize != '\n' && *pszSize != '\0')
			++pszSize;

		char sMsg[16];
		char sID[1024];
		*pszSize = '\0';
		sscanf(pszBegin, "%s %s", sMsg, sID);

		char * p = new char[strlen(sID) + 1];  strcpy(p, sID);
		m_msgIDs.add(p);
		pszSize++;
	}

	free(sBuf);

	return true; 
}

bool CPop3Connection::ReadStatResponse(int& nNumberOfMails, int& nTotalMailSize)
{
	//Must be connected to perform a "STAT"
	// ASSERT(m_bConnected);

	//retrieve the reponse
	char * sMessageBuf = 0;
	int  nSize;
	if (!ReadResponse(&sMessageBuf, "\r\n", nSize))
	{
		WriteToEventLog("Error retrieving the STAT response from the POP3 server");
		return false;
	}
	bool bGetNumber = true;
	for (char* pszNum=sMessageBuf; *pszNum!='\0'; pszNum++)
	{
		if (*pszNum=='\t' || *pszNum==' ')
		{						
			if (bGetNumber)
			{
				nNumberOfMails = atoi(pszNum);
				m_nNumberOfMails = nNumberOfMails;
				bGetNumber = false;
			}
			else
			{
				nTotalMailSize = atoi(pszNum);
				free(sMessageBuf);
				return true;
			}
		}
	}

	free(sMessageBuf);
	return false; 
}

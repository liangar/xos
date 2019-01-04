//////////////////////////////////////////////////////////////////////
/*
Smtp.cpp: implementation of the CSmtp and CSmtpMessage classes

Written by Robert Simpson (robert@blackcastlesoft.com)
Created 11/1/2000
Version 1.7 -- Last Modified 06/18/2001

1.7 - Modified the code that gets the GMT offset and the code that
parses the date/time as per Noa Karsten's suggestions on
codeguru.

Note that CSimpleMap does not sort the key values, and CSmtp
takes advantage of this by writing the headers out in the reverse
order of how they will be parsed before being sent to the SMTP
server.  If you modify the code to use std::map or any other map
class, the headers may be alphabetized by key, which may cause
some mail clients to show the headers in the body of the message
or cause other undesirable results when viewing the message.
*/
//////////////////////////////////////////////////////////////////////

#include <Smtp.h>

#include <lfile.h>
#include <ss_service.h>

#include <xbase64.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction for CSmtpMessageBody
//////////////////////////////////////////////////////////////////////
CSmtpMessageBody::CSmtpMessageBody(const char * pszBody, const char * pszEncoding, const char * pszCharset, EncodingEnum encode)
{
	// Set the default message encoding method
	// To transfer html messages, make Encoding = "text/html"
	if (pszEncoding) Encoding = pszEncoding;
	if (pszCharset)  Charset  = pszCharset;
	if (pszBody)     Data     = pszBody;
	TransferEncoding          = encode;
}

const CSmtpMessageBody& CSmtpMessageBody::operator=(const char * pszBody)
{
	Data = pszBody;
	return *this;
}

const CSmtpMessageBody& CSmtpMessageBody::operator=(const string& strBody)
{
	Data = strBody;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction for CSmtpAttachment
//////////////////////////////////////////////////////////////////////
CSmtpAttachment::CSmtpAttachment(const char * pszFilename, const char * pszAltName, bool bIsInline, const char * pszEncoding, const char * pszCharset, EncodingEnum encode)
{
	if (pszFilename) FileName = pszFilename;
	if (pszAltName)  AltName  = pszAltName;
	if (pszEncoding) Encoding = pszEncoding;
	if (pszCharset)  Charset  = pszCharset;
	TransferEncoding = encode;
	Inline = bIsInline;
}

const CSmtpAttachment& CSmtpAttachment::operator=(const char * pszFilename)
{
	FileName = pszFilename;
	return *this;
}

const CSmtpAttachment& CSmtpAttachment::operator=(const string& strFilename)
{
	FileName = strFilename;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction for CSmtpAddress
//////////////////////////////////////////////////////////////////////
CSmtpAddress::CSmtpAddress(const char * pszAddress, const char * pszName)
{
	if (pszAddress) Address = pszAddress;
	if (pszName) Name = pszName;
}

const CSmtpAddress& CSmtpAddress::operator=(const char * pszAddress)
{
	Address = pszAddress;
	return *this;
}

const CSmtpAddress& CSmtpAddress::operator=(const string& strAddress)
{
	Address = strAddress;
	return *this;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction for CSmtpMessage
//////////////////////////////////////////////////////////////////////
CSmtpMessage::CSmtpMessage()
{
	Timestamp = (long)time(0);

	MimeType = mimeGuess;
}

// Write all the headers to the e-mail message.
// This is done just before sending it, when we're sure the user wants it to go out.
void CSmtpMessage::CommitHeaders()
{
	char szWeek[4];
	char szMonth[4];
	char szOut[1024];
	string strHeader;
	string strValue;
	int n;
	struct tm * tblock;

	Headers.createvalue("X-Priority", "3 (Normal)");
	Headers.createvalue("X-MSMail-Priority", "Normal");
	Headers.createvalue("X-Mailer", "CSmtp Class Mailer by Robert Simpson (robert@blackcastlesoft.com)");
	Headers.createvalue("Importance", "Normal");

	// Get the time/date stamp and GMT offset for the Date header.
	tblock = localtime((const time_t*)&Timestamp);
	switch(tblock->tm_wday) {
		case 0:
			strcpy(szWeek, "Sun");
			break;
		case 1:
			strcpy(szWeek, "Mon");
			break;
		case 2:
			strcpy(szWeek, "Tue");
			break;
		case 3:
			strcpy(szWeek, "Wed");
			break;
		case 4:
			strcpy(szWeek, "Thu");
			break;
		case 5:
			strcpy(szWeek, "Fri");
			break;
		case 6:
			strcpy(szWeek, "Sat");
			break;
		default:
			strcpy(szWeek, "   ");
			break;
	}

	switch(tblock->tm_mon) {
		case 0:
			strcpy(szMonth, "Jan");
			break;
		case 1:
			strcpy(szMonth, "Feb");
			break;
		case 2:
			strcpy(szMonth, "Mar");
			break;
		case 3:
			strcpy(szMonth, "Apr");
			break;
		case 4:
			strcpy(szMonth, "May");
			break;
		case 5:
			strcpy(szMonth, "Jun");
			break;
		case 6:
			strcpy(szMonth, "Jul");
			break;
		case 7:
			strcpy(szMonth, "Aug");
			break;
		case 8:
			strcpy(szMonth, "Sep");
			break;
		case 9:
			strcpy(szMonth, "Oct");
			break;
		case 10:
			strcpy(szMonth, "Nov");
			break;
		case 11:
			strcpy(szMonth, "Dec");
			break;
		default:
			strcpy(szMonth, "   ");
			break;
	}

	// Add the date/time stamp to the message headers
	sprintf(szOut,"%s, %d %s %d %2.2d:%2.2d:%2.2d +0800",szWeek, tblock->tm_mday,
		szMonth, tblock->tm_year, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);

	Headers.createvalue("Date", szOut);
	Headers.createvalue("Subject", Subject.c_str());

	// Write out the TO header
	/*if (Recipient.Name.length())
	{
		sprintf(szOut,"\"%s\" ",Recipient.Name.c_str());
		strValue += szOut;
	}
	if (Recipient.Address.length())
	{
		sprintf(szOut,"<%s>",Recipient.Address.c_str());
		strValue += szOut;
	}*/
	// Write out all the TO'd names
	// modified by chenqg 2008-08-30
	strValue.erase();
	for (n = 0; n < int(Recipient.size()); n++)
	{
		if (strValue.length()) strValue += ",\r\n\t";
		if (Recipient[n].Name.length())
		{
			sprintf(szOut,"\"%s\" ",Recipient[n].Name.c_str());
			strValue += szOut;
		}
		sprintf(szOut,"<%s>",Recipient[n].Address.c_str());
		strValue += szOut;
	}
	Headers.createvalue("To", strValue.c_str());

	// Write out all the CC'd names
	strValue.erase();
	for (n = 0;n < int(CC.size());n++)
	{
		if (strValue.length()) strValue += ",\r\n\t";
		if (CC[n].Name.length())
		{
			sprintf(szOut,"\"%s\" ",CC[n].Name.c_str());
			strValue += szOut;
		}
		sprintf(szOut,"<%s>",CC[n].Address.c_str());
		strValue += szOut;
	}
	Headers.createvalue("CC", strValue.c_str());

	// Write out the FROM header
	strValue.erase();
	if (Sender.Name.length())
	{
		sprintf(szOut,"\"%s\" ",Sender.Name.c_str());
		strValue += szOut;
	}
	sprintf(szOut,"<%s>",Sender.Address.c_str());
	strValue += szOut;
	Headers.createvalue("From", strValue.c_str());
}

// Parse a message into a single string
int CSmtpMessage::Parse(string& strDest)
{
	string strHeader;
	string strValue;
	string strTemp;
	string strBoundary;
	string strInnerBoundary;
	char szOut[1024];
	int n;

	strDest.erase();
	// Get a count of the sections to see if this will be a multipart message
	n  = Message.size();
	n += Attachments.size();

	// If we have more than one section, then this is a multipart MIME message
	if (n > 1){
		sprintf(szOut,"CSmtpMsgPart123X456_000_%8.8X",time(0));
		strBoundary = szOut;

		strcpy(szOut,"multipart/");

		if (MimeType == mimeGuess) {
			MimeType = (Attachments.size() == 0)? mimeAlternative : mimeMixed;
		}
		switch(MimeType)
		{
		case mimeAlternative:	strcat(szOut,"alternative");	break;
		case mimeMixed:			strcat(szOut,"mixed");			break;
		case mimeRelated:		strcat(szOut,"related");		break;
		}
		strcat(szOut,";\r\n\tboundary=\"");
		strcat(szOut,strBoundary.c_str());
		strcat(szOut,"\"");

		Headers.createvalue("Content-Type", szOut);
	}

	Headers.createvalue("MIME-Version", MIME_VERSION);
	if (MessageId.length())
	{
		sprintf(szOut,"<%s>",MessageId.c_str());
		Headers.createvalue("Message-ID", szOut);
	}

	// Finalize the message headers
	CommitHeaders();

	// Write out all the message headers -- done backwards on purpose!
	for (n = Headers.m_values.m_count - 1;  n >= 0; n--)
	{
		sprintf(szOut, "%s: %s\r\n", Headers[n].name, Headers[n].value);
		strDest += szOut;
	}
	if (strBoundary.length())
	{
		sprintf(szOut, "\r\n%s\r\n", MULTIPART_MESSAGE);
		strDest += szOut;
	}

	// If we have attachments and multiple message bodies, create a new multipart section
	// This is done so we can display our multipart/alternative section separate from the
	// main multipart/mixed environment, and support both attachments and alternative bodies.
	if (Attachments.size() && Message.size() > 1 && strBoundary.length())	{
		sprintf(szOut, "CSmtpMsgPart123X456_001_%8.8X-Inner",time(0));
		strInnerBoundary = szOut;

		sprintf(szOut,"\r\n--%s\r\nContent-Type: multipart/alternative;\r\n\tboundary=\"%s\"\r\n",strBoundary.c_str(),strInnerBoundary.c_str());
		strDest += szOut;
	}

	for (n = 0; n < int(Message.size()); n++) {
		// If we're multipart, then write the boundary line
		if (strBoundary.length() || strInnerBoundary.length())
		{
			strDest += "\r\n--";
			// If we have an inner boundary, write that one.  Otherwise write the outer one
			if (strInnerBoundary.length()) strDest += strInnerBoundary;
			else strDest += strBoundary;
			strDest += "\r\n";
		}
		strValue.erase();
		strDest += "Content-Type: ";
		strDest += Message[n].Encoding;
		// Include the character set if the message is text
		if (_strnicmp(Message[n].Encoding.c_str(),"text/",5) == 0)
		{
			sprintf(szOut,";\r\n\tcharset=\"%s\"",Message[n].Charset.c_str());
			strDest += szOut;
		}
		strDest += "\r\n";

		// Encode the message
		strValue = Message[n].Data;
		EncodeMessage(Message[n].TransferEncoding,strValue,strTemp);

		// Write out the encoding method used and the encoded message
		strDest += "Content-Transfer-Encoding: ";
		strDest += strTemp;

		// If the message body part has a content ID, write it out
		if (Message[n].ContentId.length())
		{
			sprintf(szOut,"\r\nContent-ID: <%s>",Message[n].ContentId.c_str());
			strDest += szOut;
		}
		strDest += "\r\n\r\n";
		strDest += strValue;
	}

	// If we have multiple message bodies, write out the trailing inner end sequence
	if (strInnerBoundary.length()){
		sprintf(szOut,"\r\n--%s--\r\n",strInnerBoundary.c_str());
		strDest += szOut;
	}

	// Process any attachments
	for (n = 0; n < int(Attachments.size()); n++) {
		char *	pData = 0;
		char *	pszFile;
		char *	pszExt;
		char szType[MAX_PATH];
		char szFilename[MAX_PATH];

		memset(szType, 0, sizeof(szType));
        memset(szFilename, 0, sizeof(szFilename));

		// Get the filename of the attachment
		strValue = Attachments[n].FileName;

		// Open the file
		strcpy(szFilename,strValue.c_str());
		int size = read_file(&pData, szFilename, szType, MAX_PATH);
		if (size <= 0){
			WriteToEventLog("CSmtpMessage::Parse : %s", szType);
			if (pData)
				free(pData);
			return -1;
		}
		// Write out our boundary marker
		if (strBoundary.length() > 0) {
			sprintf(szOut,"\r\n--%s\r\n",strBoundary.c_str());
			strDest += szOut;
		}

		// If no alternate name is supplied, strip the path to get the base filename
		if (!Attachments[n].AltName.length()){
			// Strip the path from the filename
			pszFile = strrchr(szFilename, '/');
			if (!pszFile)
				pszFile = strrchr(szFilename,'\\');

			if (!pszFile)
				pszFile = szFilename;
			else
				pszFile ++;
		}else
			pszFile = (char *)Attachments[n].AltName.c_str();

		// Set the content type for the attachment.
		strcpy(szType,"application/octet-stream");

		// Check the registry for a content type that overrides the above default
		pszExt = strchr(pszFile,'.');
		if (pszExt)
		{
			if (strstr("jpg,jpeg,gif,bmp,tiff", pszExt+1) != 0){
				sprintf(szType, "image/%s", pszExt+1);
			}
		}

		// If the attachment has a specific encoding method, use it instead
		if (Attachments[n].Encoding.length())
			strcpy(szType,Attachments[n].Encoding.c_str());

		// Write out the content type and attachment types to the message
		sprintf(szOut,"Content-Type: %s",szType);
		strDest += szOut;
		// If the content type is text, write the charset
		if (_strnicmp(szType,"text/",5) == 0)
		{
			sprintf(szOut,";\r\n\tcharset=\"%s\"",Attachments[n].Charset.c_str());
			strDest += szOut;
		}
		sprintf(szOut,";\r\n\tname=\"%s\"\r\n",pszFile);
		strDest += szOut;

		// Encode the attachment
		EncodeMessage(Attachments[n].TransferEncoding,strValue,strTemp,(unsigned char *)(pData),SYS_UINT32(size));

		// Write out the transfer encoding method
		sprintf(szOut,"Content-Transfer-Encoding: %s\r\n",strTemp.c_str());
		strDest += szOut;

		// Write out the attachment's disposition
		strDest += "Content-Disposition: ";

		if (Attachments[n].Inline) strDest += "inline";
		else strDest += "attachment";

		strDest += ";\r\n\tfilename=\"";
		strDest += pszFile;
		strDest += '\"';

		// If the attachment has a content ID, write it out
		if (Attachments[n].ContentId.length())
		{
			sprintf(szOut,"\r\nContent-ID: <%s>",Attachments[n].ContentId.c_str());
			strDest += szOut;
		}
		strDest += "\r\n\r\n";

		// Write out the encoded attachment
		strDest += strValue;
		strTemp.erase();
		strValue.erase();

		free(pData);  pData = 0;
	}

	// If we are multipart, write out the trailing end sequence
	if (strBoundary.length()){
		sprintf(szOut,"\r\n--%s--\r\n",strBoundary.c_str());
		strDest += szOut;
	}
	return 0;
}

// Parses text into quoted-printable lines.
// See RFC 1521 for full details on how this works.
void CSmtpMessage::EncodeQuotedPrintable(string& strDest, string& strSrc)
{
	string strTemp;
	string strTemp2;
	char * pszTok1;
	char * pszTok2;
	char szSub[16];
	char ch;
	int n;

	strDest.erase();
	if (!strSrc.length()) return;

	// Change = signs and non-printable characters to =XX
	pszTok1 = (char *)strSrc.c_str();
	pszTok2 = pszTok1;
	do
	{
		if (*pszTok2 == '=' || *pszTok2 > 126 ||
			(*pszTok2 < 32 && (*pszTok2 != '\r' && *pszTok2 != '\n' && *pszTok2 != '\t')))
		{
			ch = *pszTok2;
			*pszTok2 = 0;
			strTemp += pszTok1;
			*pszTok2 = ch;
			sprintf(szSub,"=%2.2X",(unsigned char)*pszTok2);
			strTemp += szSub;
			pszTok1 = pszTok2 + 1;
		}
		pszTok2 ++;
	} while (*pszTok2);

	// Append anything left after the search
	if (strlen(pszTok1)) strTemp += pszTok1;

	pszTok1 = (char *)strTemp.c_str();
	while (pszTok1)
	{
		pszTok2 = strchr(pszTok1,'\r');
		if (pszTok2) *pszTok2 = 0;
		while (1)
		{
			if (strlen(pszTok1) > 76)
			{
				n = 75; // Breaking at the 75th character
				if (pszTok1[n-1] == '=') n -= 1; // If the last character is an =, don't break the line there
				else if (pszTok1[n-2] == '=') n -= 2; // If we're breaking in the middle of a = sequence, back up!

				// Append the first section of the line to the total string
				ch = pszTok1[n];
				pszTok1[n] = 0;
				strDest += pszTok1;
				pszTok1[n] = ch;
				strDest += "=\r\n";
				pszTok1 += n;
			}
			else // Line is less than or equal to 76 characters
			{
				n = (int)strlen(pszTok1); // If we have some trailing data, process it.
				if (n)
				{
					if (pszTok1[n-1] == ' ' || pszTok1[n-1] == '\t') // Last character is a space or tab
					{
						sprintf(szSub,"=%2.2X",(unsigned char)pszTok1[n-1]);
						// Replace the last character with an =XX sequence
						pszTok1[n-1] = 0;
						strTemp2 = pszTok1;
						strTemp2 += szSub;
						// Since the string may now be larger than 76 characters, we have to reprocess the line
						pszTok1 = (char *)strTemp2.c_str();
					}
					else // Last character is not a space or tab
					{
						strDest += pszTok1;
						if (pszTok2) strDest += "\r\n";
						break; // Exit the loop which processes this line, and move to the next line
					}
				}
				else
				{
					if (pszTok2) strDest += "\r\n";
					break; // Move to the next line
				}
			}
		}
		if (pszTok2)
		{
			*pszTok2 = '\r';
			pszTok2 ++;
			if (*pszTok2 == '\n') pszTok2 ++;
		}
		pszTok1 = pszTok2;
	}
}

// Breaks a message's lines into a maximum of 76 characters
// Does some semi-intelligent wordwrapping to ensure the text is broken properly.
// If a line contains no break characters, it is forcibly truncated at the 76th char
void CSmtpMessage::BreakMessage(string& strDest, string& strSrc, int nLength)
{
	string strTemp = strSrc;
	string strLine;
	char * pszTok1;
	char * pszTok2;
	char * pszBreak;
	const char * pszBreaks = " -;.,?!";
	char ch;
	bool bNoBreaks;
	int nLen;

	strDest.erase();
	if (!strSrc.length()) return;

	nLen = (int)strTemp.length();
	nLen += (nLen / 60) * 2;

	strDest.reserve(nLen);

	// Process each line one at a time
	pszTok1 = (char *)strTemp.c_str();
	while (pszTok1)
	{
		pszTok2 = strchr(pszTok1,'\r');
		if (pszTok2) *pszTok2 = 0;

		bNoBreaks = (!strpbrk(pszTok1,pszBreaks));
		nLen = (int)strlen(pszTok1);
		while (nLen > nLength)
		{
			// Start at the 76th character, and move backwards until we hit a break character
			pszBreak = &pszTok1[nLength - 1];

			// If there are no break characters in the string, skip the backward search for them!
			if (!bNoBreaks)
			{
				while (!strchr(pszBreaks,*pszBreak) && pszBreak > pszTok1)
					pszBreak--;
			}
			pszBreak ++;
			ch = *pszBreak;
			*pszBreak = 0;
			strDest += pszTok1;

			strDest += "\r\n";
			*pszBreak = ch;

			nLen -= (int)(pszBreak - pszTok1);
			// Advance the search to the next segment of text after the break
			pszTok1 = pszBreak;
		}
		strDest += pszTok1;
		if (pszTok2)
		{
			strDest += "\r\n";
			*pszTok2 = '\r';
			pszTok2 ++;
			if (*pszTok2 == '\n') pszTok2 ++;
		}
		pszTok1 = pszTok2;
	}
}

// Makes the message into a 7bit stream
void CSmtpMessage::Make7Bit(string& strDest, string& strSrc)
{
	char * pszTok;

	strDest = strSrc;

	pszTok = (char *)strDest.c_str();
	do
	{
		// Replace any characters above 126 with a ? character
		if (*pszTok > 126 || *pszTok < 0)
			*pszTok = '?';
		pszTok ++;
	} while (*pszTok);
}

// Encodes a message or binary stream into a properly-formatted message
// Takes care of breaking the message into 76-byte lines of text, encoding to
// Base64, quoted-printable and etc.
void CSmtpMessage::EncodeMessage(EncodingEnum code, string& strMsg, string& strMethod, unsigned char * pByte, SYS_UINT32 dwSize)
{
	string strTemp;
	char * pszTok1;
	char * pszTok2;
	char * pszBuffer = NULL;

	if (!pByte)
	{
		pszBuffer = (char *)malloc(strMsg.length() + 1);
		strcpy(pszBuffer,strMsg.c_str());
		pByte = (unsigned char *)pszBuffer;
		dwSize = (SYS_UINT32)strMsg.length();
	}

	// Guess the encoding scheme if we have to
	if (code == encodeGuess) code = GuessEncoding(pByte, dwSize);

	switch(code)
	{
	case encodeQuotedPrintable:
		strMethod = "quoted-printable";
		strMsg = (char *)pByte;

		EncodeQuotedPrintable(strTemp, strMsg);
		break;
	case encodeBase64:
		strMethod = "base64";
		{
			pszTok1 = (char *)malloc(dwSize + dwSize / 2 + 16);
			dw_b64encode(pszTok1, pByte, dwSize);
		}
		strMsg = pszTok1;
		free(pszTok1);

		BreakMessage(strTemp, strMsg);
		break;
	case encode7Bit:
		strMethod = "7bit";

		pszTok1 = (char *)malloc((dwSize+1) * sizeof(char));
		strcpy(pszTok1,(char *)pByte);
		strMsg = pszTok1;
		free(pszTok1);

		Make7Bit(strTemp, strMsg);
		strMsg = strTemp;
		BreakMessage(strTemp, strMsg);
		break;
	case encode8Bit:
		strMethod = "8bit";

		pszTok1 = (char *)malloc((dwSize+1) * sizeof(char));
		strcpy(pszTok1,(char *)pByte);
		strMsg = pszTok1;
		free(pszTok1);

		BreakMessage(strTemp, strMsg);
		break;
	}

	if (pszBuffer) free(pszBuffer);

	strMsg.erase();

	// Parse the message text, replacing CRLF. sequences with CRLF.. sequences
	pszTok1 = (char *)strTemp.c_str();
	do
	{
		pszTok2 = strstr(pszTok1,"\r\n.");
		if (pszTok2)
		{
			*pszTok2 = 0;
			strMsg += pszTok1;
			*pszTok2 = '\r';
			strMsg += "\r\n..";
			pszTok1 = pszTok2 + 3;
		}
	} while (pszTok2);
	int len = (int)strlen(pszTok1);
	strMsg += pszTok1;
}

// Makes a best-guess of the proper encoding to use for this stream of bytes
// It does this by counting the # of lines, the # of 8bit bytes and the number
// of 7bit bytes.  It also records the line and the count of lines over
// 76 characters.
// If the stream is 90% or higher 7bit, it uses a text encoding method.  If the stream
// is all at or under 76 characters, it uses 7bit or 8bit, depending on the content.
// If the lines are longer than 76 characters, use quoted printable.
// If the stream is under 90% 7bit characters, use base64 encoding.
EncodingEnum CSmtpMessage::GuessEncoding(unsigned char * pByte, SYS_UINT32 dwLen)
{
	int n7Bit = 0;
	int n8Bit = 0;
	int nLineStart = 0;
	int nLinesOver76 = 0;
	int nLines = 0;
	SYS_UINT32 n;

	// Count the content type, byte by byte
	for (n = 0;n < dwLen; n++)
	{
		if (pByte[n] > 126 || (pByte[n] < 32 && pByte[n] != '\t' && pByte[n] != '\r' && pByte[n] != '\n'))
			n8Bit ++;
		else n7Bit ++;

		// New line?  If so, record the line size
		if (pByte[n] == '\r')
		{
			nLines ++;
			nLineStart = (n - nLineStart) - 1;
			if (nLineStart > 76) nLinesOver76 ++;
			nLineStart = n + 1;
		}
	}
	// Determine if it is mostly 7bit data
	if ((n7Bit * 100) / dwLen > 89)
	{
		// At least 90% text, so use a text-base encoding scheme
		if (!nLinesOver76)
		{
			if (!n8Bit) return encode7Bit;
			else return encode8Bit;
		}
		else return encodeQuotedPrintable;
	}
	return encodeBase64;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction for CSmtp
//////////////////////////////////////////////////////////////////////
CSmtp::CSmtp()
{
	m_bExtensions = true;	// Use ESMTP if possible
	m_Timeout = 60;		// Default to 30 second timeout

	// Try and get the SMTP service entry by name
	m_port = 25;

	m_uid[0] = m_pwd[0] = '\0';
}

CSmtp::~CSmtp()
{
	// Make sure any open connections are shut down
	Close();
}

void CSmtp::setport(int port)
{
	m_port = port;
}

// Connects to a SMTP server.  Returns true if successfully connected, or false otherwise.
bool CSmtp::Connect(char * pszServer)
{
	int nRet;

	// Shut down any active connection
	Close();

	int r = m_socket.connect(pszServer, m_port);
	if (r != 0) {
		m_strResult = "smtp client cannot connect to : ";
		m_strResult += pszServer;
		goto error;
	}

	// Get the initial response string
	if ((nRet = SendCmd(NULL)) != 220){
		goto error;
	}

	// Send a HELLO message to the SMTP server
	if (SendHello())
		goto error;

	return true;

error:
	Close();
	return false;
}

// Closes any active SMTP sessions and shuts down the socket.
void CSmtp::Close()
{
	if (m_bConnected)
		SendQuitCmd();
	// Shutdown and close the socket
	m_socket.close();
	m_strResult.erase();
}

// Send a command to the SMTP server and wait for a response
int CSmtp::SendCmd(const char * pszCmd)
{
    static const char szFunctionName[] = "CSmtp::SendCmd";

	int r = 0;
	char szResult[CMD_RESPONSE_SIZE];
	char * pszPos;
	char * pszTok;
	bool bReportProgress = false;

	// If we have a command to send, then send it.
	if (pszCmd){
		int l = strlen(pszCmd);

		// Make sure the input buffer is clear before sending
		memset(szResult,0x0, CMD_RESPONSE_SIZE);
		while ((r = m_socket.recv(szResult, CMD_RESPONSE_SIZE, 0)) > 0){
			WriteToEventLog("%s: for clear buffer, recv(%s),r=%d",szFunctionName,szResult,r);
		}
		r = m_socket.send_all(pszCmd, l);
		if (r < 0) {
			memcpy(szResult, pszCmd, 128);  szResult[128] = 0;
			m_strResult = "Send error: ";
			m_strResult += szResult;
			return -1;
		}
	}

	// Prepare to receive a response
	memset(szResult,0x0, CMD_RESPONSE_SIZE);
	pszPos = szResult;
	// Wait for the specified timeout for a full response string
	time_t begintime = time(0);
	long idle = m_Timeout;
	while (idle > 0){
		r = m_socket.recv(pszPos, CMD_RESPONSE_SIZE - int(pszPos - szResult), idle);
		idle -= long(time(0) - begintime);
		// Treats a graceful shutdown as an error
		if (r < 0)
			break;

		// Add the data to the total response string & check for a LF
		pszPos += r;
		if ((pszTok = strrchr(szResult,'\n')) != 0) {
			pszTok[-1] = '\0';
			break;
		}
	}

	// if not reset command, assign the response string
	if (pszCmd == 0 || stricmp(pszCmd, "RSET\r\n") != 0) {
		m_strResult = szResult;
		// Evaluate the numeric response code
		if (r >= 0) {
			szResult[3] = 0;
			r = atoi(szResult);
		}
		else r = -1;
	}else
		r = 0;

	return r;
}

// Placeholder function -- overridable
// This function is called when the SMTP server gives us an unexpected error
// The <nError> value is the SMTP server's numeric error response, and the <pszErr>
// is the descriptive error text
//
// <pszErr> may be NULL if the server failed to respond before the timeout!
// <nError> will be -1 if a catastrophic failure occurred.
//

// Placeholder function -- overridable
// Currently the only warning condition that this class is designed for is
// an authentication failure.  In that case, <nWarning> will be 535,
// which is the RFC error for authentication failure.  If authentication
// fails, you can override this function to prompt the user for a new
// username and password.  Change the <m_uid> and <m_pwd> member
// variables and return true to retry authentication.
//
// <pszWarning> may be NULL if the server did not respond in time!
//

// Placeholder function -- overridable
// This is a progress callback to indicate that data is being sent over the wire
// and that the operation may take some time.
// Return true to continue sending, or false to abort the transfer
// E-Mail's a message
// Returns 0 if successful, -1 if an internal error occurred, or a positive
// error value if the SMTP server gave an error or failure response.
int CSmtp::SendMessage(CSmtpMessage &msg)
{
	static const char szFunctionName[] = "CSmtp::SendMessage";

	int nRet;
	int n;
	//  int nRecipients = 0;
	int nRecipientCount = 0;

	// Check if we have a sender
	if (!msg.Sender.Address.length()) return -1;

	// Check if we have recipients
	if (!msg.Recipient.size() && !msg.CC.size()) return -1;

	// Check if we have a message body or attachments
	// *** Commented out to remove the requirement that a message have a body or attachments
	//  if (!msg.Message.size() && !msg.Attachments.size()) return -1;

	// Send the sender's address
	nRet = SendFrom((char *)msg.Sender.Address.c_str());
	if (nRet) return nRet;

	// If we have a recipient, send it
	nRecipientCount = 0; // Count of recipients
	/*if (msg.Recipient.Address.length())
	{
		nRet = SendTo((char *)msg.Recipient.Address.c_str());
		if (!nRet) nRecipientCount ++;
	}*/
	// If we have any TO's, send those.
	for (n = 0; n < int(msg.Recipient.size()); n++)
	{
		nRet = SendTo((char *)msg.Recipient[n].Address.c_str());
		if (!nRet) nRecipientCount ++;
	}

	// If we have any CC's, send those.
	for (n = 0;n < int(msg.CC.size());n++)
	{
		nRet = SendTo((char *)msg.CC[n].Address.c_str());
		if (!nRet) nRecipientCount ++;
	}

	// If we have any bcc's, send those.
	for (n = 0;n < int(msg.BCC.size());n++)
	{
		nRet = SendTo((char *)msg.BCC[n].Address.c_str());
		if (!nRet)
			nRecipientCount ++;
	}
	// If we failed on all recipients, we must abort.
	if (!nRecipientCount){
		WriteToEventLog("%s: no mail recver", szFunctionName);
	}else{
		nRet = SendData(msg);
	}

	return nRet;
}

// Simplified way to send a message.
// <pvAttachments> can be either an char * containing NULL terminated strings, in which
// case <dwAttachmentCount> should be zero, or <pvAttachments> can be an char * *
// containing an array of char *'s, in which case <dwAttachmentCount> should equal the
// number of strings in the array.
int CSmtp::SendMessage(CSmtpAddress &addrFrom, CSmtpAddress &addrTo, const char * pszSubject, char * pszMessage, void * pvAttachments, SYS_UINT32 dwAttachmentCount)
{
	CSmtpMessage message;
	CSmtpMessageBody body;
	CSmtpAttachment attach;

	body = pszMessage;

	message.Sender = addrFrom;
	message.Recipient.push_back(addrTo);
	message.Message.push_back(body);
	message.Subject = pszSubject;

	// If the attachment count is zero, but the pvAttachments variable is not NULL,
	// assume that the ppvAttachments variable is a string value containing NULL terminated
	// strings.  A double NULL ends the list.
	// Example: char * pszAttachments = "foo.exe\0bar.zip\0autoexec.bat\0\0";
	if (!dwAttachmentCount && pvAttachments)
	{
		char * pszAttachments = (char *)pvAttachments;
		while (strlen(pszAttachments))
		{
			attach.FileName = pszAttachments;
			message.Attachments.push_back(attach);
			pszAttachments = &pszAttachments[strlen(pszAttachments)];
		}
	}

	// dwAttachmentCount is not zero, so assume pvAttachments is an array of char *'s
	// Example: char * *ppszAttachments = {"foo.exe","bar.exe","autoexec.bat"};
	if (pvAttachments && dwAttachmentCount)
	{
		char * *ppszAttachments = (char * *)pvAttachments;
		while (dwAttachmentCount-- && ppszAttachments)
		{
			attach.FileName = ppszAttachments[dwAttachmentCount];
			message.Attachments.push_back(attach);
		}
	}
	return SendMessage(message);
}

// Yet an even simpler method for sending a message
// <pszAddrFrom> and <pszAddrTo> should be e-mail addresses with no decorations
// Example:  "foo@bar.com"
// <pvAttachments> and <dwAttachmentCount> are described above in the alternative
// version of this function
int CSmtp::SendMessage(char * pszAddrFrom, char * pszAddrTo, char * pszSubject, char * pszMessage, void * pvAttachments, SYS_UINT32 dwAttachmentCount)
{
	CSmtpAddress addrFrom(pszAddrFrom);
	CSmtpAddress addrTo(pszAddrTo);

	return SendMessage(addrFrom,addrTo,pszSubject,pszMessage,pvAttachments,dwAttachmentCount);
}

// Tell the SMTP server we're quitting
// Returns 0 if successful, or a positive
// error value if the SMTP server gave an error or failure response.
int CSmtp::SendQuitCmd()
{
	int nRet;

	if (!m_bConnected) return 0;

	nRet = SendCmd("QUIT\r\n");

	m_bConnected = false;

	if (nRet != 221){
		WriteToEventLog("Send QUIT error(%d)", nRet);
		return nRet;
	}

	return 0;
}

// Initiate a conversation with the SMTP server
// Returns 0 if successful, or a positive
// error value if the SMTP server gave an error or failure response.
int CSmtp::SendHello()
{
	int nRet = 0;
	char szMsg[MAX_PATH];
	SYS_UINT32 dwSize = 64;

	// First try a EHLO if we're using ESMTP
	if (m_bExtensions) nRet = SendCmd("EHLO Solar\r\n");

	// If we got a 250 response, we're using ESMTP, otherwise revert to regular SMTP
	if (nRet != 250)
	{
		m_bUsingExtensions = false;
		szMsg[0] = 'H';
		szMsg[1] = 'E';
		nRet = SendCmd(szMsg);
	}
	else m_bUsingExtensions = true;

	// Raise any unexpected responses
	if (nRet != 250){
		return nRet;
	}

	// We're connected!
	m_bConnected = true;

	// Send authentication if we have any.
	// We don't fail just because authentication failed, however.
	if (m_bUsingExtensions) SendAuthentication();

	return 0;
}

// Requests authentication for the session if the server supports it,
// and attempts to submit the user's credentials.
// Returns 0 if successful, or a positive
// error value if the SMTP server gave an error or failure response.
int CSmtp::SendAuthentication()
{
	int nRet = 0;
	char szMsg[MAX_PATH];
	char szAuthType[MAX_PATH];

	// If we don't have a username, skip authentication
	if (!m_uid[0]) return 0;

	// Make the authentication request
	nRet = SendCmd("AUTH LOGIN\r\n");
	// If it was rejected, we have to abort.

	while (nRet == 334 && nRet != 235)
	{
		// Decode the authentication string being requested
		strcpy(szAuthType,&(m_strResult.c_str())[4]);

		int len = dw_b64decode(szMsg, szAuthType, (int)strlen(szAuthType));
		szMsg[len] = '\0';

		if (!stricmp(szMsg,"Username:"))
			dw_b64encode_nocrlf(szMsg, m_uid, (int)strlen(m_uid));
		else if (!stricmp(szMsg,"Password:"))
			dw_b64encode_nocrlf(szMsg, m_pwd, (int)strlen(m_pwd));
		else
			break;

		strcat(szMsg, "\r\n");
		nRet = SendCmd(szMsg);

		// If we got a failed authentication request, raise a warning.
		// this gives the owner a chance to change the username and password.
		if (nRet == 535){
			// Return false to fail, or true to retry
			break;
		}
	}

	// Raise an error if we failed to authenticate
	return (nRet == 235) ? 0:nRet;
}

// Send a MAIL FROM command to the server
// Returns 0 if successful, or a positive
// error value if the SMTP server gave an error or failure response.
int CSmtp::SendFrom(char * pszFrom)
{
	int nRet = 0;
	char szMsg[MAX_PATH];

	sprintf(szMsg,"MAIL FROM: <%s>\r\n", pszFrom);

	while (1)
	{
		nRet = SendCmd(szMsg);
		// Send authentication if required, and retry the command
		if (nRet == 530 || nRet == 334){
			nRet = SendAuthentication();
			if (nRet == 235) // authentication successful
				nRet = 250;
		}else
			break;
	}
	// Raise an error if we failed
	return (nRet == 250) ? 0:nRet;
}

// Send a RCPT TO command to the server
// Returns 0 if successful, or a positive
// error value if the SMTP server gave an error or failure response.
int CSmtp::SendTo(char * pszTo)
{
	int nRet;
	char szMsg[MAX_PATH];

	sprintf(szMsg,"RCPT TO: <%s>\r\n",pszTo);

	nRet = SendCmd(szMsg);
	return (nRet == 250 || nRet == 251) ? 0:nRet;
}

void CSmtp::GetError(char * msg, int maxlen)
{
	if (maxlen < 0 || maxlen > 4 * 1024)
		maxlen = 4 * 1024;
	strncpy(msg, m_strResult.c_str(), maxlen);
}

// Send the body of an e-mail message to the server
// Returns 0 if successful, or a positive
// error value if the SMTP server gave an error or failure response.
int CSmtp::SendData(CSmtpMessage &msg)
{
	static const char szFunctionName[] = "CSmtp::SendData";

	int nRet;
	string strMsg;
	// Parse the body of the email message
	if ((nRet = msg.Parse(strMsg)) < 0){
		SendCmd("RSET\r\n");

		WriteToEventLog("%s: Parse error(%d)", szFunctionName, nRet);
		return -1;
	}
	strMsg += "\r\n.\r\n";

	// Send the DATA command.  We need a 354 to proceed
	nRet = SendCmd("DATA\r\n");
	if (nRet != 354)
	{
		WriteToEventLog("%s: Send DATA command error (%d)", szFunctionName, nRet);
		return nRet;
	}

	// Send the body and expect a 250 OK reply.
	nRet = SendCmd((char *)strMsg.c_str());

	if (nRet != 250){
		WriteToEventLog("%s: Send mail text error(%d)", szFunctionName, nRet);
		return nRet;
	}

	return 0;
}

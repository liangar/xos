#ifndef _CSmtp_H_
#define _CSmtp_H_

#include <xsys.h>
#include <name_value.h>

#ifdef WIN32
#pragma warning(disable: 4786)
#endif

#include <vector>
#include <string>

#include "xemail_string.h"

using namespace std;

/// Some forward declares
class CSmtp;
class CSmtpAddress;
class CSmtpMessage;
class CSmtpAttachment;
class CSmtpMessageBody;
class CSmtpMimePart;

#ifndef LPBYTE
typedef unsigned char * LPBYTE;
#endif

#ifdef LPCTSTR
typedef const char * LPCTSTR;
#endif

/// These are the only 4 encoding methods currently supported
enum EncodingEnum
{
  encodeGuess,
  encode7Bit,
  encode8Bit,
  encodeQuotedPrintable,
  encodeBase64
};

/// This code supports three types of mime-types, and can optionally guess a mime type
/// based on message content.
enum MimeTypeEnum
{
  mimeGuess,
  mimeMixed,
  mimeAlternative,
  mimeRelated
};

/// Attachments and message bodies inherit from this class
/// It allows each part of a multipart MIME message to have its own attributes
class CSmtpMimePart
{
public:
  string Encoding;  /// Content encoding.  Leave blank to let the system discover it
  string Charset;   /// Character set for text attachments
  string ContentId; /// Unique content ID, leave blank to let the system handle it
  EncodingEnum TransferEncoding; /// How to encode for transferring to the server
};

/// This class represents a user's text name and corresponding email address
class CSmtpAddress
{
public: /// Constructors
  CSmtpAddress(const char * pszAddress = NULL, const char * pszName = NULL);

public: /// Operators
  const CSmtpAddress& operator=(const char * pszAddress);
  const CSmtpAddress& operator=(const string& strAddress);

public: /// Member Variables
  string Name;
  string Address;
};

/// This class represents a file attachment
class CSmtpAttachment : public CSmtpMimePart
{
public: /// Constructors
  CSmtpAttachment(const char * pszFilename = NULL, const char * pszAltName = NULL, bool bIsInline = false, const char * pszEncoding = NULL, const char * pszCharset = MESSAGE_CHARSET, EncodingEnum encode = encodeGuess);

public: /// Operators
  const CSmtpAttachment& operator=(const char * pszFilename);
  const CSmtpAttachment& operator=(const string& strFilename);

public: /// Member Variables
  string FileName;  /// Fully-qualified path and filename of this attachment
  string AltName;   /// Optional, an alternate name for the file to use when sending
  bool   Inline;    /// Is this an inline attachment?
};

/// Multiple message body part support
class CSmtpMessageBody : public CSmtpMimePart
{
public: /// Constructors
  CSmtpMessageBody(const char * pszBody = NULL, const char * pszEncoding = MESSAGE_ENCODING, const char * pszCharset = MESSAGE_CHARSET, EncodingEnum encode = encodeGuess);

public: /// Operators
  const CSmtpMessageBody& operator=(const char * pszBody);
  const CSmtpMessageBody& operator=(const string& strBody);

public: /// Member Variables
  string       Data;             /// Message body;
};

/// This class represents a single message that can be sent via CSmtp
class CSmtpMessage
{
public:
	/// Constructors
	CSmtpMessage();

	/// Member Variables
	CSmtpAddress                Sender;           /// Who the message is from
	vector<CSmtpAddress>        Recipient;        /// The intended recipient
	string                      Subject;          /// The message subject
	vector<CSmtpMessageBody>	Message;          /// An array of message bodies
	vector<CSmtpAddress>		CC;               /// Carbon Copy recipients
	vector<CSmtpAddress>		BCC;              /// Blind Carbon Copy recipients
	vector<CSmtpAttachment>		Attachments;      /// An array of attachments
	name_value					Headers;          /// Optional headers to include in the message
	long						Timestamp;        /// Timestamp of the message
	MimeTypeEnum                MimeType;         /// Type of MIME message this is
	string                      MessageId;        /// Optional message ID

public: /// Public functions
	int Parse(string& strDest);

private: /// Private functions to finalize the message headers & parse the message
	EncodingEnum GuessEncoding(unsigned char * pByte, SYS_UINT32 dwLen);
	void EncodeMessage(EncodingEnum code, string& strMsg, string& strMethod, unsigned char * pByte = NULL, SYS_UINT32 dwSize = 0);
	void Make7Bit(string& strDest, string& strSrc);
	void CommitHeaders();
	void BreakMessage(string& strDest, string& strSrc, int nLength = 76);
	void EncodeQuotedPrintable(string& strDest, string& strSrc);
	void AddToHeaders(string & name, string & value);
	void AddToHeaders(const char * name, string & value);
	void AddToHeaders(const char * name, const char * value);
};

/// The main class for connecting to a SMTP server and sending mail.
class CSmtp
{
public:
	/// Constructors
	CSmtp();
	virtual ~CSmtp();

	/// Member Variables.  Feel free to modify these to change the system's behavior
	bool	m_bExtensions;	/// Use ESMTP extensions (true)
	int		m_Timeout;		/// Timeout for issuing each command (30 seconds)
	int		m_port;            /// Port to communicate via SMTP (25)
	char	m_uid[128];        /// Username for authentication
	char	m_pwd[128];        /// Password for authentication

	/// These represent the primary public functionality of this class
	bool Connect(char * pszServer);
	int  SendMessage(CSmtpMessage& msg);
	int  SendMessage(CSmtpAddress& addrFrom, CSmtpAddress& addrTo, const char * pszSubject, char * pszMessage, void * pvAttachments = NULL, SYS_UINT32 dwAttachmentCount = 0);
	int  SendMessage(char * pszAddrFrom, char * pszAddrTo, char * pszSubject, char * pszMessage, void * pvAttachments = NULL, SYS_UINT32 dwAttachmentCount = 0);
	void Close();

	void GetError(char * msg, int maxlen);
	void setport(int port);
private:
	/// Private Member Variables
	xsys_socket m_socket;         /// Socket being used to communicate to the SMTP server
	string   m_strResult;       /// string result from a SendCmd()
	bool     m_bConnected;      /// Connected to SMTP server
	bool     m_bUsingExtensions;/// Whether this SMTP server uses ESMTP extensions

	/// These functions are used privately to conduct a SMTP session
	int  SendCmd(const char * pszCmd);
	int  SendAuthentication();
	int  SendHello();
	int  SendQuitCmd();
	int  SendFrom(char * pszFrom);
	int  SendTo(char * pszTo);
	int  SendData(CSmtpMessage &msg);
};

#endif // _CSmtp_H_


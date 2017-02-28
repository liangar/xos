#ifndef _xemail_string_H_
#define _xemail_string_H_

// When the SMTP server responds to a command, this is the
// maximum size of a response I expect back.
#ifndef CMD_RESPONSE_SIZE
#define CMD_RESPONSE_SIZE 1024
#endif

// The CSmtp::SendCmd() function will send blocks no larger than this value
// Any outgoing data larger than this value will trigger an SmtpProgress()
// event for all blocks sent.
#ifndef CMD_BLOCK_SIZE
#define CMD_BLOCK_SIZE  1024
#endif

// Default mime version is 1.0 of course
#ifndef MIME_VERSION
#define MIME_VERSION "1.0"
#endif

// This is the message that would appear in an e-mail client that doesn't support
// multipart messages
#ifndef MULTIPART_MESSAGE
#define MULTIPART_MESSAGE "This is a multi-part message in MIME format."
#endif

// Default message body encoding
#ifndef MESSAGE_ENCODING
#define MESSAGE_ENCODING "text/plain"
#endif

// Default character set
#ifndef MESSAGE_CHARSET
#define MESSAGE_CHARSET "iso-8859-1"
#endif

#endif // _xemail_string_H_

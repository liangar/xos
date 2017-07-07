/*******************************************************************
* HTTP helper
* Distributed under GPL license
* Copyright (c) 2005-06 Stanley Huang <stanleyhuangyc@yahoo.com.cn>
* All rights reserved.
*******************************************************************/

#pragma once

#ifndef _HTTPCLIENT_H
#define _HTTPCLIENT_H
#define FLAG_REQUEST_ONLY 0x1
#define FLAG_KEEP_ALIVE 0x2
#define FLAG_KEEP_HEADER 0x4
#define FLAG_CHUNKED 0x8

extern const char http_content_type_json_utf8[];
extern const char http_post_contentType[];

typedef enum {
	HS_IDLE=0,
	HS_REQUESTING,
	HS_RECEIVING,
	HS_STOPPING,
} HTTP_STATES;

typedef enum {
	HM_GET = 0,
	HM_HEAD,
	HM_POST,
	HM_POST_STREAM,
	HM_POST_MULTIPART,
} HTTP_METHOD;

#define postPayload_STRING 0
#define postPayload_BINARY 1
#define postPayload_FD 2
#define postPayload_CALLBACK 3

#define POST_BUFFER_SIZE 1024
typedef int (*PFNpostPayloadCALLBACK)(void* buffer, int bufsize);

typedef struct {
	void* data;
	int type;
	size_t length;
} POST_CHUNK;

typedef struct {
	int sockfd;
	HTTP_METHOD method;
	HTTP_STATES state;
	const char *url;
	const char *proxy;
	unsigned short flags;
	unsigned short port;
	char* referer;
	char* header;
	char* buffer;			// response packet
	int bufferSize;			// response buffer size
	char* postPayload;		// request post data(all parameters [name=value&]*)
	char* hostname;
	int postPayloadBytes;	// post bytes
	int dataSize;			// response data size
	int bytesStart;
	int bytesEnd;
	int payloadSize;		// content-length
	//info parsed from response header
	char* contentType;		// content-type
	int httpVer;	// 0 for 1.0, 1 for 1.1
	int httpCode;
	//Multipart-Post 
	POST_CHUNK* chunk;		// for file data
	int chunkCount;
	const char* filename;
} HTTP_REQUEST;

// req : request 参数
// url : 要访问的目标
// proxy:代理
void httpInitReq(HTTP_REQUEST* req, const char* url, char* proxy);

// return : >=0 / < 0 = ok / error
int httpRequest(HTTP_REQUEST* param);
// return : >=0 / < 0 = ok / error
int httpGetResponse(HTTP_REQUEST* param);

// 释放
void httpClean(HTTP_REQUEST* param);

int httpSend(HTTP_REQUEST* param, char* data, size_t length);
int httpPostFile(HTTP_REQUEST* req, char* url, char* fieldname, const char* filename);
int PostFileStream(char* url, const char* filename);

void httpInitPostReq(HTTP_REQUEST * preq, char* url);
void httpSetPostBuffer(HTTP_REQUEST* preq, char * buffer);
void httpAddPostParameter(HTTP_REQUEST * preq, const char * name, const char * value);
int  httpPostData(HTTP_REQUEST * preq, char * post_data);
int  httpPostData(HTTP_REQUEST * preq, char* url, char * post_data);

#endif

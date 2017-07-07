/*******************************************************************
* HTTP helper
* Distributed under GPL license
* Copyright (c) 2005-06 Stanley Huang <stanleyhuangyc@yahoo.com.cn>
* All rights reserved.
*******************************************************************/
#include <xsys.h>

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

#include <httpclient.h>

/****************************************************************************
* HTTP protocol client
****************************************************************************/
#define HEADER_END_FLAG  "\r\n\r\n"
#ifdef _DEBUG
#define DEBUG printf
#else
#define DEBUG
#endif
#define CONN_RETRIES 3
#define HTTP_GET_HEADER "%s %s HTTP/1.%d\r\nAccept: */*\r\nConnection: %s\r\nUser-Agent: Mozilla/5.0\r\nHost: %s\r\n%s\r\n"
#define HTTP_POST_HEADER "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: %s\r\nUser-Agent: Mozilla/5.0\r\nContent-Length: %d\r\n%s\r\n"
#define HTTP_POST_MULTIPART_HEADER "POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0\r\nAccept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\nAccept-Language: en-us,en;q=0.5\r\nAccept-Encoding: gzip,deflate\r\nAccept-Charset: ISO-8859-1;q=0.7,*;q=0.7\r\nKeep-Alive: 300\r\nConnection: keep-alive\r\nContent-Type: multipart/form-data; boundary=%s\r\nContent-Length: %d\r\n%s\r\n"
#define MULTIPART_BOUNDARY "---------------------------24464570528145"
#define HTTP_POST_STREAM_HEADER "POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0\r\nAccept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\nAccept-Language: en-us,en;q=0.5\r\nAccept-Encoding: gzip,deflate\r\nAccept-Charset: ISO-8859-1;q=0.7,*;q=0.7\r\nKeep-Alive: 300\r\nConnection: close\r\nContent-Type: application/octet-stream; filename=%s\r\nContent-Length: %d\r\n%s\r\n"

const char http_post_contentType[] = "application/x-www-form-urlencoded";
const char http_content_type_json_utf8[] = "application/json; charset=UTF-8";
/* */
void httpInitReq(HTTP_REQUEST* req, const char* url, char* proxy)
{
	memset(req, 0, sizeof(HTTP_REQUEST));
	req->url = url;
	if (proxy && *proxy) req->proxy = proxy;
}

static const char* parseURL(const char* url, HTTP_REQUEST* param)
{
	const char *ptr;
	size_t len;
	const char *path;

	if (strncmp(url,"http://",7) != 0 ) { 
		return NULL;
	}
	
	url += 7;
	ptr = strchr(url, '/');
	if (ptr == NULL) {
		path="/";
		len = strlen(url);
	} else {
		len = (int)(ptr - url);
		path = ptr;
	}
	ptr = strchr(url, ':');
	if (ptr && *(ptr + 1) != '/') {
		unsigned short int port = atoi(ptr + 1);
		if (port) {
			param->port = port;
		}
		len = (int)(ptr - url);
	}
	param->hostname = (char*)malloc(len + 1);
	memcpy(param->hostname, url, len);
	param->hostname[len] = 0;
	if (!param->port) param->port = 80;
	return path;
}
	
#define MAX_HEADER_SIZE (8192 - 1)

int httpSend(HTTP_REQUEST* param, char* data, size_t length)
{
	int offset = 0;
	int bytes;
	do {
		bytes = send(param->sockfd, data + offset, int(length) - offset, 0);
		if (bytes <= 0) break;
		offset += bytes;
		DEBUG("Sent %d/%d bytes\n", offset, length);
	} while(offset < int(length));

	return int(offset);
}

void httpClean(HTTP_REQUEST* param)
{
	if (param->sockfd) {
		closesocket(param->sockfd);
		param->sockfd = 0;
	}
	if (param->buffer) {
		free(param->buffer);
		param->buffer = 0;  param->bufferSize = 0;
	}
	if (param->hostname){
		free(param->hostname);
		param->hostname = 0;
	}
	if (param->header){
		free(param->header);
		param->header = 0;
	}
}

int httpRequest(HTTP_REQUEST* param)
{
	const char *path;
	struct hostent *target_host;
	int ret = 0;
	int bytes;

	param->state = HS_REQUESTING;
	param->payloadSize=0;

	if (param->proxy) {
		parseURL(param->proxy, param);
		path = param->url;
	} else {
		path = parseURL(param->url, param);
		if (!path) return -1;
	}

	if (param->header) free(param->header);
	param->header = (char*)malloc(MAX_HEADER_SIZE + 1);
	if ((param->bytesStart|param->bytesEnd) == 0) {
		char headerAddition[128];
		headerAddition[0] = 0;
		if (param->referer) {
			xsnprintf(headerAddition, sizeof(headerAddition), "Referer: %s\r\n", param->referer);
		}
		switch (param->method) {
		case HM_GET:
			xsnprintf(param->header, MAX_HEADER_SIZE + 1, HTTP_GET_HEADER, "GET",
				path, param->httpVer,
				(param->flags & FLAG_KEEP_ALIVE) ? "Keep-Alive" : "close",
				param->hostname, headerAddition
			);
			break;
		case HM_HEAD:
			xsnprintf(param->header, MAX_HEADER_SIZE + 1, HTTP_GET_HEADER, "HEAD",
				path, param->httpVer,
				(param->flags & FLAG_KEEP_ALIVE) ? "Keep-Alive" : "close",
				param->hostname, headerAddition
			);
			break;
		case HM_POST:
			xsnprintf(param->header, MAX_HEADER_SIZE + 1, HTTP_POST_HEADER,
				path, param->hostname,
				(param->contentType)? param->contentType : http_post_contentType,
				param->postPayloadBytes, headerAddition
			);
			break;
		case HM_POST_STREAM: {
			xsnprintf(param->header, MAX_HEADER_SIZE + 1, HTTP_POST_STREAM_HEADER,
				path, param->hostname,
				param->filename, 0, headerAddition
			);
			break;
			} break;
		case HM_POST_MULTIPART: {
			size_t bytes = 0;
			int i;
			for (i = 0; i < param->chunkCount; i++) {
				if (param->chunk[i].type == postPayload_STRING) {
					param->chunk[i].length = strlen((char*)param->chunk[i].data);
				}
				bytes += param->chunk[i].length;
				bytes += sizeof(MULTIPART_BOUNDARY) - 1 + 6;
			}
			bytes += sizeof(MULTIPART_BOUNDARY) - 1 + 6;
			xsnprintf(param->header, MAX_HEADER_SIZE + 1, HTTP_POST_MULTIPART_HEADER, path, param->hostname, MULTIPART_BOUNDARY, bytes, headerAddition);
			} break;
		}
	} else {
		char tokenRange[48],*p=tokenRange;
		p+=xsnprintf(p, sizeof(tokenRange), "Range: bytes=%u-", param->bytesStart);
		if (param->bytesEnd>0) {
			xsnprintf(p, 16, "%u\r\n",param->bytesEnd);
		} else {
			strcpy(p,"\r\n");
		}
		xsnprintf(param->header, MAX_HEADER_SIZE + 1, HTTP_GET_HEADER, path, "close", param->hostname, tokenRange);
	}
	
	do {
		int hdrsize = (int)strlen(param->header);

		if (!param->sockfd) {
			struct sockaddr_in server_addr;

			if ((target_host = gethostbyname((const char*)param->hostname)) == NULL) {
				ret = -1;
				continue;
			}

			if ((param->sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
				DEBUG("Failed to open socket\n");
				ret = -1;
				continue;
			}

			memset(&server_addr.sin_zero,0,8);
			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = ((struct in_addr *)(target_host->h_addr))->s_addr;
			server_addr.sin_port = htons(param->port);
			DEBUG("Connecting to server...\n");

			if (connect(param->sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) < 0) {
				DEBUG("Failed to connect\n");
				ret = -1;
				continue;
			}
		}
		DEBUG("Sending request...\n");
		if (httpSend(param, param->header, hdrsize) != hdrsize) {
			ret = -1;
			break;
		}

		if (param->method == HM_POST) {
			ret = httpSend(param, param->postPayload, param->postPayloadBytes);
			if (ret != param->postPayloadBytes){
				ret = -ret;
				break;
			}
			ret = 0;
		} else if (param->method == HM_POST_MULTIPART) {
			POST_CHUNK* chunk;
			int i;
			char* sendbuf = (char*)malloc(POST_BUFFER_SIZE);
			for (i = 0; i < param->chunkCount && param->state != HS_STOPPING; i++) {
				chunk = param->chunk + i;
				bytes = xsnprintf(sendbuf, POST_BUFFER_SIZE, "--%s\r\n", MULTIPART_BOUNDARY);
				if (httpSend(param, sendbuf, bytes) != bytes) break;
				switch (chunk->type) {
				case postPayload_STRING:
				case postPayload_BINARY:
					if (httpSend(param, (char*)chunk->data, chunk->length) != int(chunk->length)) break;
					break;
				case postPayload_FD:
					for(;;) {
						bytes = _read((int)chunk->data, sendbuf, POST_BUFFER_SIZE);
						if (bytes <= 0) break;
						if (httpSend(param, sendbuf, bytes) != bytes) break;
					}
					break;
				case postPayload_CALLBACK:
					for(;;) {
						bytes = (*(PFNpostPayloadCALLBACK)chunk->data)(sendbuf, POST_BUFFER_SIZE);
						if (bytes < 0) {
							param->state = HS_STOPPING;
							break;
						}
						if (bytes == 0) break;
						if (httpSend(param, sendbuf, bytes) != bytes) break;
					}
					break;
				}
				if (httpSend(param, "\r\n", 2) != 2) break;
			}
			free(sendbuf);
			httpSend(param, "--", 2);
			httpSend(param, MULTIPART_BOUNDARY, sizeof(MULTIPART_BOUNDARY) - 1);
			httpSend(param, "--\r\n", 4);
		}
	} while(0);

	if (!ret && param->method != HM_POST_STREAM && param->method != HM_POST_MULTIPART)
		return httpGetResponse(param);

	return ret;
}

int httpGetResponse(HTTP_REQUEST* param)
{
	int receivedBytes;
	int rspHeaderBytes;
	int ret;
	char *p = 0;

	do {
		//receive header
		DEBUG("Receiving response...\n");
		receivedBytes=0;
		for(;;) {
			ret = recv(param->sockfd, param->header + receivedBytes, MAX_HEADER_SIZE-receivedBytes, 0);
			if (ret <= 0) break;
			receivedBytes += ret;
			param->header[receivedBytes] = 0;
			DEBUG("Received %d/%d bytes\n", ret, receivedBytes);
			if ((p = strstr(param->header,"\r\n\r\n"))) {
				*(p+2) = '\0';
				break;
			}
			if (receivedBytes == MAX_HEADER_SIZE) {
				p = param->header + receivedBytes - 4;
			}
		}
		if (!p) {
			DEBUG("Invalid server response\n");
			ret = -1;
			continue;
		} else {
			ret = 0;
			p+=4;
		}
		rspHeaderBytes = (int)(p - param->header);

	//process header
	{
		char *p;
		DEBUG("Response header:\n%s\n", param->header);
		if (p = strstr(param->header,"HTTP/1.")) {
			param->httpCode = atoi(p+9);
		}
		if (param->httpCode >= 404) {
			DEBUG("Invalid response or file not found on server\n");
			return -1;
		}
		while ((p = strstr(p, "\r\n"))) {
			char *q;
			q=strchr((p += 2),':');
			if (!q) continue;
			*q = 0;
			if (!stricmp(p,"Content-length")) {
				param->payloadSize=atoi(q+2);
			} else if (!stricmp(p,"Content-type")) {
				param->contentType = q+2;
			} else if (!stricmp(p, "Transfer-Encoding")) {
				if (!strncmp(p + 19, "chunked", 7)) {
					param->flags |= FLAG_CHUNKED;
				}
			}
			*q = ':';
		}
	}
	DEBUG("Payload bytes: %d\n",param->payloadSize);
	if (param->payloadSize == 0 && param->dataSize > 0)
		param->payloadSize = param->dataSize - 1;
	if (param->flags & FLAG_REQUEST_ONLY) {
		closesocket(param->sockfd);
		param->sockfd=0;
		param->state=HS_IDLE;
		return 0;
	}
	// receive payload
	if (param->method != HM_HEAD) {
		int bytes;
		int recvBytes = 0;
		int bytesToRecv;
		int payloadWithHeader = receivedBytes - rspHeaderBytes;
		int eof = 0;
		param->state=HS_RECEIVING;
		do {
			if (param->bufferSize > 0) {
				if (param->payloadSize > 0) {
					if (param->bufferSize < param->payloadSize + 1) {
						free(param->buffer);
						param->bufferSize = param->payloadSize + 1;
						param->buffer = (char*)calloc(1, param->bufferSize);
					}
				} else {
					param->bufferSize *= 2;
					param->buffer = (char*)realloc(param->buffer, param->bufferSize);
				}
			} else {
				DEBUG("Allocating %d bytes for payload buffer\n",param->payloadSize);
				if (param->payloadSize) {
					param->bufferSize = param->payloadSize + 1;
					param->buffer = (char*)calloc(1, param->bufferSize);
				} else {
					param->bufferSize = 1024 + payloadWithHeader;
					param->buffer = (char*)calloc(1, param->bufferSize);
				}
			}
			if (recvBytes == 0) {
				recvBytes = payloadWithHeader;
				if (recvBytes > 0) {
					DEBUG("Header includes %d bytes of payload\n", recvBytes);
					memcpy(param->buffer, param->header + rspHeaderBytes, recvBytes);
				}
			}
			//receive payload data
			bytesToRecv = param->payloadSize ? param->payloadSize : param->bufferSize - 1;
			for (; recvBytes < bytesToRecv; recvBytes += bytes) {
				bytes = recv(param->sockfd, param->buffer + recvBytes, bytesToRecv - recvBytes, 0);
				if (bytes<=0) {
					eof = 1;
					break;
				}
			}
			if (recvBytes == param->payloadSize) eof = 1;
			DEBUG("Payload received: %d bytes\n", recvBytes);
		} while (!eof);
		DEBUG("End of stream\n");
		*(param->buffer + recvBytes)=0;
		param->dataSize = recvBytes;
	}
	
	} while(0);
	if (param->sockfd && !(param->flags & FLAG_KEEP_ALIVE)) {
		closesocket(param->sockfd);
		param->sockfd=0;
	}
	param->state=HS_IDLE;
	return ret;
}

void httpSetPostBuffer(HTTP_REQUEST* preq, char * buffer)
{
	if (preq == 0)
		return;
	preq->postPayload = buffer;
}

void httpAddPostParameter(HTTP_REQUEST * preq, const char * name, const char * value)
{
	int l = preq->postPayloadBytes;
	const char * pformat = (l > 2)?"&%s=%s" : "%s=%s";
	l += sprintf(preq->postPayload+l, pformat, name, value);
	
	preq->postPayloadBytes = l;
}

void httpInitPostReq(HTTP_REQUEST * preq, char* url)
{
	httpInitReq(preq, url, 0);
	
	preq->method = HM_POST;
}

int httpPostData(HTTP_REQUEST * preq, char * post_data)
{
	preq->postPayload = post_data;
	preq->postPayloadBytes = strlen(post_data);

	int ret = httpRequest(preq);

	return ret;
}

int httpPostData(HTTP_REQUEST * preq, char* url, char * post_data)
{
	httpInitPostReq(preq, url);
	
	return httpPostData(preq, post_data);
}

static int fd;
static char fileheader[512];
static int filelen;

static int ReadData(void* buffer, int bufsize)
{
	if (fileheader[0]) {
		int ret = xsnprintf((char *)buffer, bufsize, "%s", fileheader);
		fileheader[0] = 0;
		return ret;
	} else {
		return _read(fd, buffer, bufsize);
	}
}

#define FILE_CHUNK_HEADER "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n\r\n"

int httpPostFile(HTTP_REQUEST* req, char* url, char* fieldname, const char* filename)
{
	int ret;
	POST_CHUNK chunk;
	const char *p;

	httpInitReq(req, url, 0);

	fd = _open(filename, O_BINARY | O_RDONLY);
	if (fd <= 0) return -1;

	p = strrchr(filename, '\\');
	if (!p) p = strrchr(filename, '/');
	if (!p)
		req->filename = filename;
	else
		req->filename = p + 1;

	filelen = _lseek(fd, 0, SEEK_END);
	chunk.data = (void*)ReadData;
	chunk.length = filelen + xsnprintf(fileheader, sizeof(fileheader), FILE_CHUNK_HEADER, fieldname, req->filename);
	chunk.type = postPayload_CALLBACK;
	_lseek(fd, 0, SEEK_SET );
	req->method = HM_POST_MULTIPART;
	req->chunk = &chunk;
	req->chunkCount = 1;
	ret = httpRequest(req);
	_close(fd);
	if (!ret) {
		ret = httpGetResponse(req);
	}
	return 0;
}

int PostFileStream(char* url, const char* filename)
{
	int ret;
	HTTP_REQUEST req;
	
	char buf[1024];
	int bytes;

	memset(&req, 0, sizeof(req));

	fd = _open(filename, O_BINARY | O_RDONLY);
	if (fd <= 0) return 0;

	req.filename = strrchr(filename, '\\');
	if (!req.filename)
		req.filename = filename;
	else
		req.filename++;

	req.url = url;
	req.method = HM_POST_STREAM;
	ret = httpRequest(&req);

	while ((bytes = _read(fd, buf, sizeof(buf))) > 0 
		&& httpSend(&req, buf, bytes) == bytes);

	_close(fd);
	return ret;
}

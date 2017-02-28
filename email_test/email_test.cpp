// email_test.cpp : 定义控制台应用程序的入口点。
//

#include <xsys.h>
#include <l_str.h>
#include <lfile.h>
#include <ss_service.h>

#include <crtdbg.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#include "xpop.h"
#include "xsmtp.h"
#include "xbase64.h"

using namespace std;

void test(void);
char b[64 * 1024];
int _tmain(int argc, _TCHAR* argv[])
{
	int r;
	if ((r = xsys_init(true)) != 0){
		return r;
	}

	set_run_path("K:\\user\\liangar\\develop\\xos\\email_test\\temp");	
	// 打开LOG文件
	openservicelog("email_test.log", true);

	printf("Welcome email test!\n");
	
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );

	_CrtSetBreakAlloc(48);

	try{
		test();
	}catch (...) {
		return -1;
	}

	_CrtMemCheckpoint( &s2 );
	if ( _CrtMemDifference( &s3, &s1, &s2) ){
	   _CrtMemDumpStatistics( &s3 );
	   _CrtDumpMemoryLeaks();
	}

	// 释放资源并退出
	closeservicelog();
	xsys_down();

	return 0;
}

void test(void)
{
	printf("please enter a command or '|' to exit and '?' for help: \n");

	//xdial dial;
	xpop  pop;
	xsmtp smtp;
	xmail * pmail;

	int ret;
	char * p1, * p2, * p3;

	while (1){
		ret = 0;
		scanf("%s", b);
		if (b[0] == '|')  break;
		if (b[0] == '?'){	// show usage
			printf("usage :"
				"\n\t'|'                       : exit the program"
				"\n\t's:<smtphost>,<uid>,<pwd>': set mail props"
				"\n\t                            for ep, 's:tom@msn.com,marry@msn.com,info for u'"
				"\n\t'a:<full filename>'       : attach a sp. file"
				"\n\t'w:<text>'                : write text"
				"\n\t'S:<from>,<to>,<title>'   : send the mail and close the session"
				"\n\t'r:<pophost>,<uid>,<pwd>' : login to pop3 server"
				"\n\t'g:<i>,<path>'            : get a mail, save attatch to path then delete it"
				"\n\t'c:'                      : close session"
//				"\n\t'd:<entry>,<uid>,<pwd>'   : dial"
//				"\n\t'D:'                      : disconnect dial"
				"\n\t'm:<simple time>'         : get prev month"
				"\n");
			continue;
		}
		p1 = b+2;
		if ((p2 = getaword(p1, p1, ',')) != 0)
			p3 = getaword(p2, p2, ',');
		else
			p3 = 0;
		if (*p1)  trim_all(p1, p1);
		if (p2)   trim_all(p2, p2);
		if (p3)   trim_all(p3, p3);

		printf("%s\n", getnowtime(b+8*1024));
		switch(b[0]) {
		// test smtp
		case 's':
			if ((ret = smtp.open(p1, p2, p3)) < 0){
				strcpy(b, "error in open smtp connect.\n");
			}
			break;

		case 'a':
			if ((ret = smtp.setfile(p1)) < 0){
				strcpy(b, "error in open smtp connect.\n");
			}
			break;

		case 'w':
			break;

		case 'S':
			smtp.setfrom(p1);
			smtp.setto(p2);
			smtp.settitle(p3);
			if ((ret = smtp.send()) != 0){
				smtp.getmsg(b, 4096);
				printf("send error:{\n%s}\n", b);
				break;
			}
			smtp.close();
			break;

		// test pop3
		case 'r':
			if ((ret = pop.open(p1, p2, p3)) < 0){
				ret = pop.geterror(b, 2048);
				printf("get error:{\n%s}\n", b);
			}
			printf("all emails : %d\n", pop.count());
			break;

		case 'g':
			if ((pmail = pop.recvmail(atoi(p1), false)) == 0){
				pop.geterror(b, 2048);
				ret = -1;
				break;
			}else
				ret = 0;

			pmail->parse_parts();
			p3 = (char *)(pop.save_attach(1, p2));
			if (p3)
				printf("write file : %s ok.\n", p3);
			break;

		case 'c':
			pop.close();
			break;
/*
		// test dial
		case 'd':
			if ((ret = dial.open(p1, p2, p3)))
				dial.geterror(ret, b, sizeof(b));
			break;
		case 'D':
			dial.close();
			break;
*/
		case 'e':
			ret = dw_b64encode_nocrlf(b, p1, (int)strlen(p1));
			printf("%s\n", b);
			break;

		case 'p':
			ret = dw_b64decode(b, p1, (int)strlen(p1));
			printf("%s\n", b);
			break;

		case 'P':
			{
				char msg[128], fullname[128];
				strcpy(fullname, p1);
				int l = read_file(b, fullname, msg, sizeof(msg));

				bool isbase64 = false;
				char * p = strchr(b, ',');
				if (p == 0) {
					l = dw_b64decode(b, b, l);
					if (l < 10 || strchr(b, ',') == 0) {
						ret = -2;
						break;
					}
					isbase64 = true;
				}

				if (isbase64) {
					if ((l = read_file(b, fullname, msg, sizeof(msg))) < 1){
						ret = -1;
						break;
					}
					l = dw_b64decode(b, b, l);
					char msg[128];
					write_file(fullname, b, l, msg, sizeof(msg));
				}
				printf("%s\n", b);
			}
			break;

		case 'E':
			{
				// read the file
				char msg[128], fullname[128], *p;
				strcpy(fullname, p1);
				int l = read_file(&p, fullname, msg, sizeof(msg));

				xmail x;
				x.m_pbuf = p;
				x.parse_header();
				WriteToEventLog("{\nfrom: %s\nto: %s\ntitle: %s\nencodetype: %s\n}\n",
					x.m_from,
					x.m_to,
					x.m_title,
					x.m_content_type);

				x.parse_parts();

				printf("%dparts:\n", x.m_parts.m_count);
				for (int i = 0; i < x.m_parts.m_count; i++){
					printf("filename = %s%s\n", ((x.m_parts[i].filename)? x.m_parts[i].filename : "null"), x.m_parts[i].pbody);
				}
				x.close();
			}
			break;

		case 'm':
			getPrevMonth(b, b+2);
			if (b[0] == 0){
				ret = -1;
				strcpy(b, "time format error");
			}else{
				printf("prev month = %s\n", b);
			}
			break;

		default:
			break;
		}
		printf("%s\n", getnowtime(b+8*1024));
		if (ret < 0){
			printf("error %d : %s\n", ret, b);
		}else
			printf("ok\n");
	}
}
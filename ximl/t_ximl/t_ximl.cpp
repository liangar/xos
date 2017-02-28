#define WINVER			0x0401

#include <xsys.h>
#include <iostream.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#endif

#include <lfile.h>

#include <sxml_parse.h>
#include <httpclient.h>

void t_parse(void);

static char * pximl = "\
<?xml version=1.0 ?>\n\
<a_tag>\n\
	<b_prop>this is a prop value for a_tag服务类型.</b_prop>\n\
	<c_prop a = ' aa dd' b=#aa9922></c_prop>\n\
</a_tag>\n\
<a_tag>\n\
	<c_prop a = 'aa cc' b=#aa88dd></c_prop>\n\
</a_tag>";

char b[128*1024];

int main(void)
{
	xsys_init(false);

	char * p1 = b+2;

	int r;

	try
	{
		while (1){
			cout << "please enter a command or '|' to exit: ";
			cin.getline(b, 512);
			if (b[0] == '|')  break;
			if (b[0] == '\0' || b[1] != ':'){  cerr << "syntax error." << endl;  continue;  }

			char * p2 = strchr(p1, ',');
			char * p3;
			if (p2){
				*p2++ = '\0';
				p3 = strchr(p2, ',');
				if (p3)
					*p3++ = '\0';
				else
					p3 = 0;
			}else
				p2 = 0;

			switch (b[0]){
			case 'e':	t_parse();  break;
			case 'f':
				{
					if ((r = X_parsefile((ximl_T *)(b+128), (ximl_TT **)(b+256), p1)) < 0){
						cerr << "error ocurred!" << endl;
						X_geterror(b);
						cerr << b << endl;
						break;
					}
					X_D(b+1024, (ximl_T *)(b+128));
					cout << b+1024 << endl;
					X_free();
				}
				break;

			case 'p':	// test X_get_prop(const char * ptag, const char * pname)
				{
					ximl_T t;
					if ((r = X_parsefile(&t, (ximl_TT **)(b+256), p1)) < 0){
						cerr << "error ocurred!" << endl;
						X_geterror(b);
						cerr << b << endl;
						break;
					}
					ximl_P * prop = X_get_prop(p2, p3);
					if (prop)
						cout << prop->pvalue << endl;
					else
						cerr << "not found" << endl;
					X_free();
				}
				break;

			case 'P':	// test X_get_prop(const char * pname)
				{
					ximl_T t;
					if ((r = X_parsefile(&t, (ximl_TT **)(b+256), p1)) < 0){
						cerr << "error ocurred!" << endl;
						X_geterror(b);
						cerr << b << endl;
						break;
					}
					ximl_P * prop = X_get_prop(p2);
					if (prop)
						cout << prop->pvalue << endl;
					else
						cerr << "not found" << endl;
					X_free();
				}
				break;
				
			case 'h':
				{
					HTTP_REQUEST req;
					httpInitReq(&req, p1, 0);
					r = httpRequest(&req);
					if (r < 0){
						printf("error: %d", r);
					}else{
						r = write_file("t.html", req.buffer, b, 512);
						printf("%s\n", req.buffer);
					}
					httpClean(&req);
				}
				break;

			case 's':
				{
					HTTP_REQUEST req;
					httpInitReq(&req, "http://192.168.1.115:449", 0);
					req.method = HM_POST;
					req.contentType = "INFOSEC_SIGN/1.0";
					r = read_file(b+128, p1, b+8*1024, 512);
					req.postPayload = b+128;
					req.postPayloadBytes = r;
					r = httpRequest(&req);
					if (r < 0){
						printf("error: %d", r);
					}else{
						r = sprintf(b, "<?xml version=\"1.0\" encoding = \"GB2312\"?>%s", req.buffer);
						r = write_file("ts.xml", b, b+16*1024, 512);
						printf("%s\n", req.buffer);
					}
					httpClean(&req);
				}
				break;

			default	:	break;
			}
		}

#ifdef _DEBUG
		_CrtDumpMemoryLeaks();
#endif
	}catch (...){
		cerr << "main catch exception" << endl;
	}

	xsys_down();
	return 0;
}

void t_parse()
{
	ximl_T t;  ximl_TT * ptt;	int n;

	strcpy(b, pximl);
	n = X_parse(&t, &ptt, b);
	if (n < 0){
		X_geterror(b);
		cerr << b << endl;
		return;
	}
	X_D(b+4096, &t);

	// X_show(b+4096);
	cout << b+4096 << endl;

	X_free();
}

#include <xsys.h>
#include <xsys_ftplib.h>

#include <iostream>

#ifdef WIN32
#include <crtdbg.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif // _DEBUG
#endif // WIN32

#include <l_str.h>

using namespace std;

char b[4*2048];
char * p;
long ih;
char * parms[5];	//存放分解出来的参数
int nparms;			//第i个参数

int main()
{
	void testftp();

	xsys_init(true);
#ifdef WIN32
#ifdef _DEBUG
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );
//	_CrtSetBreakAlloc(86);

#endif // _DEBUG
#endif // WIN32

	try{
		testftp();
	}catch(...){
		cout << "main : some error occured in testing." << endl;
	}
#ifdef WIN32
#ifdef _DEBUG
	_CrtMemCheckpoint( &s2 );
	if ( _CrtMemDifference( &s3, &s1, &s2) ){
		_CrtMemDumpStatistics( &s3 );
		_CrtDumpMemoryLeaks();
	}
#endif // _DEBUG
#endif // WIN32

	xsys_down();

	return 0;
}

void parse_parms()
{
	for (nparms = 0; nparms < 5; nparms++)
        	parms[nparms] = "";
	nparms = 0;
	p = strchr(b, ':');
	while (p){
		*p++ = '\0';
		parms[nparms++] = p++;
        	p = strchr(p, ',');
	}
}

void testftp()
{
	xsys_ftp_client ftp;
	int l;	char cmd;
	char filename[MAX_PATH];
	
	cout << "enter the cmd or ? to show usage : ";

	while (1){
		cin.getline(b, 2048);
		if (strcmp(b, "exit") == 0)  break;
		if (b[1] != ':' || b[0] == '?'){
			cout << "the cmds:\n"
				"exit                         // stop running\n"
				"c:<host>,<user>,<pass>       // Connect FTP Server\n" 
				"d:                           // disconnect FTP Server\n"
				"g:<remotefile>,<localfile>   // get file from FTP Server\n"
				"p:<localfile>,<remotefile>   // put file to FTP Server\n"
				"m:<remotefile>               // delete file from FTP Server\n"
				"s:<remotedir>                // set current directory\n"
				"S:                           // switch passive option\n"
				"l:                           // list files\n" 
				"f:                           // get files\n"
				"q:                           // quit for find files\n"
				"r:<resourcename>,<destname>  // rename file\n"
				"e:                           // show the error message"<< endl;
			continue;
		}
		cmd = b[0];	
		parse_parms();
		switch (cmd){
		case 'c':	l = ftp.open(parms[0], parms[1], parms[2]);	break;
		case 'd':	ftp.close();	l = 0;	break;
		case 'g':	l = ftp.get(parms[0], parms[1]);	break;
		case 'p':	l = ftp.put(parms[0], parms[1]);	break;
                case 'm':	l = ftp.rm(parms[0]);	break;
		case 's':	l = ftp.cd(parms[0]);	break;
		case 'l':
			b[0] = '.';  b[1] = 0;
			if ((l = ftp.file_ls(b)) >= 0){
			    cout << ftp.file_filetype() << " " << b << " " << ftp.file_size() << endl;
			    while((l = ftp.file_next(filename)) == 0){
			    	cout << ftp.file_filetype() << " " << filename << " " << ftp.file_size() << endl;
			    }
			}
			ftp.file_ls_close();
			break;
		case 'f':
			if ((l = ftp.file_next(filename)) == 0){
			    	cout << ftp.file_filetype() << " " << filename << " " << ftp.file_size() << endl;
			}
			break;
		case 'q':	ftp.file_ls_close();	break;
		case 'r':
			l = ftp.rename(parms[0],parms[1]);
			break;
		case 'e':
			//printf("The error code is: %d\n\n", geterror(ih));
			l = 1 ;
			break;
		case 'P':
			{
				b[19] = 0;	// get send time
				char * pstate = (char *)getaword(filename, (const char *)(parms[0]+30), ')');

				cout << filename << ": ";
				if (strncmp(pstate, " ok.", 4) == 0){
					cout << "发送成功" << endl;
				}else{
					cout << "发送失败" << endl;
				}
				l = 0;
			}
			break;
		case 'S':
			{
				ftp.set_passive(!(ftp.get_passive()));
				l = 0;
				break;
			}
		default:
			printf("Unknown command error!\n");
			l = 0;
			break;
		}
		if (l >= 0)
			printf("ok : %d\n", l);
		else{
		    printf("error: %d, %d\n", l, ftp.lasterror());
		}
	}
	cout.flush();
}


#include <iostream.h>
#include <xsys.h>
#include <gzip.h>

#ifdef WIN32
#include <crtdbg.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif // _DEBUG
#endif // WIN32

char b[4*2048];
char * p;
long ih;
char * parms[5];	//存放分解出来的参数
int nparms;			//第i个参数

int main()
{
	void test_gzip();

	xsys_init(true);

#ifdef WIN32
#ifdef _DEBUG
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );
#endif // _DEBUG
#endif // WIN32

	try{
		test_gzip();
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

void usage(void)
{
	cout << "the cmds:\n"
		"exit                      // stop running\n"
		"s:<zippath>               // set zip path\n"
		"t:<ziptype>               // type = (gz,tar)\n"
		"a:<filename>,<fullname>   // add file to zip packeg\n" 
		"z:<zipfile>               // zip to file in zip path\n"
		"u:<zipfile>,<temp_path>   // unzip file to temp path\n"
	;
}

void test_gzip()
{
	xsys_gzip gzip;
	int l;	char cmd;
	
	cout << "enter the cmd or ? to show usage : ";

	while (1){
		cin.getline(b, 2048);
		if (strcmp(b, "exit") == 0)  break;
		if (b[1] != ':' || b[0] == '?'){
			usage();
			continue;
		}
		cmd = b[0];	
		parse_parms();
		switch (cmd){
		case 's':	l = (gzip.set_zip_path(parms[0])? 0 : -1);	break;
		case 't':	gzip.set_type(parms[0]);  l = 0;	break;
		case 'a':	l = (gzip.add(parms[0], parms[1])?0 : -1);	break;
		case 'z':
			{
			l = gzip.zip(b+1024, parms[0]);
			if (l >= 0)
				cout << "the result file name is : " << b+1024 << endl;
				sprintf(b, "%s/%s", gzip.get_zip_path(), b+1024);
				cout << "file size is : " << xsys_file_size(b) << endl;
			}
			break;
		case 'u':
			{
			l = gzip.unzip(parms[0], parms[1]);
			if (l > 0){
				cout << "there're " << l << " files in zipfile(" << parms[0] << "):\n"
					 << gzip.lastmsg() << endl;

				cout << "result file list :\n";
				for (int i = 0; i < gzip.m_files.m_count; i++){
					xsys_zipfile * pfile = gzip.m_files.get(i);
					cout << pfile->filename << ":\t" << pfile->fullname << endl;
				}
			}
			}
			break;

		default:
			cout << "not supported command error." <<endl;
			usage();
			l = 0;
			break;
		}
		if (l >= 0)
			cerr << "ok : " << l << endl;
		else{
		    cerr << "error:" << l << "," << gzip.lastmsg() << endl;
		}
	}
}

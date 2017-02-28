#include <stdio.h>
#include <iostream.h>

#include "xsys.h"
#include "ss_service.h"
#include "hdfileparse.h"
#include "ackfileparse.h"

char b[2048];
int main(int argc, char **argv)
{
	xsys_init(true);

    set_run_path("E:\\user\\develop\\xos\\develop\\xos");
	openservicelog("parsehdfile_test.log", true);

	ehcc_detail *pdetail;
	hdfileparse ohdparse;
	ehcc_ack_detail *pack;
	ackfileparse oackparse; 
	char outFileName[1024];
	memset(outFileName, 0x0, sizeof(outFileName));

	while (true){
		cin.getline(b, 512);
		if (stricmp(b, "exit") == 0) {
			break;
		}
		int r;
/*
		strcpy(b, "HDCHCMSHMCMS00000001.20070207131600.CSV");
		if ((r = ohdparse.open(b)) < 0){
			cerr << "error : " << b << ",errmsg : " << ohdparse.getlasterrmsg() << endl;
			//ACKÎÄ¼þÊä³ö
			r = ohdparse.writeackfile();
			if (r < 0){
				cerr << "error : write ACK_" << b << ",errmsg : " << ohdparse.getlasterrmsg() << endl;
			}
			continue;
		}
		
		while((pdetail=ohdparse.fetch()) != 0){
			fprintf(stderr, "%c,%s,%s,%c,%s,%d,%d,%f\n", 
				pdetail->hurtype, pdetail->imsi, pdetail->peertelcode, pdetail->ismco, 
				pdetail->startcalltime, pdetail->duration, pdetail->volume, pdetail->value);
		}
		r = ohdparse.writeackfile();
		if (r < 0){
			cerr << "error : write ACK_" << b << ",errmsg : " << ohdparse.getlasterrmsg() << endl;
		}
		ohdparse.clear();
//*/
///*
		strcpy(b, "ACK_HDCHCMSHMCMS00000001.20070207131600.CSV");
		if ((r = oackparse.open(b)) < 0){
			cerr << "error : " << b << ",errmsg : " << oackparse.getlasterrmsg() << endl;
			continue;
		}
		
		while((pack=oackparse.fetch()) != 0){
			fprintf(stderr, "%d,%d,%s,%s,%s\n", 
				pack->row, pack->column, pack->errorpart, pack->errorlevel, 
				pack->errorinfor);
		}
		oackparse.clear();
//*/
	}

	closeservicelog();

	xsys_down();

	return 0;
}
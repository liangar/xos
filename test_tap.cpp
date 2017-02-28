#include <stdio.h>
#include <iostream.h>

#include "xsys.h"
#include "ss_service.h"

#include "tap_parse_mng.h"
#include "eh_interfacefiles.h"

char b[2048];
int main(int argc, char **argv)
{
        set_run_path("/home/mcbmaint/develop/xos");
	openservicelog("tap_test.log", true);
	if (tapp_api_init("eh_mail.ini", "tap_parsers")){
		cerr << " init error " << endl;
		closeservicelog();
		return -1;
	}
	eh_hur hurs[4];
	eH_InterfaceFiles f;
	char outFileName[1024];

	strcpy(f.sender, "CHCMS");
	strcpy(f.recipient, "HMCMS");
	strcpy(f.createtime, "070207131600");
	strcpy(f.checkcode, "CHCMS");

	memset(outFileName, 0x0, sizeof(outFileName));

	sprintf(outFileName, "HD%s%s00000001.%s.CSV", f.sender, f.recipient, f.createtime);

	while (true){
		cin.getline(b, 512);
		if (stricmp(b, "exit") == 0) {
			break;
		}
		int h;
		char * pimsi = strchr(b, ',');
		if (pimsi) {
			*pimsi++ = 0;
		}
		h = hdfile_parse(b, outFileName, f);
/*
		if ((h = tapp_api_open(b)) < 0){
			cerr << "error : " << b << endl;
			continue;
		}
		int r;
		if ((r = tapp_api_check(h)) != 0) {
			cerr << "error : "  << r << endl;
			continue;
		}
		for (int i = 0; ((r = tapp_api_fetch(hurs, h)) > 0) && i<300; i++) {
			for (int j = 0; j < 4; j++) {
				if ((pimsi == 0 || strcmp(pimsi, hurs[j].imsi) == 0) && hurs[j].nc > 0) {
					fprintf(stderr, "%s%3d %15s %s %5d %3d %5d %f\n", hurs[j].hurtype, r, hurs[j].imsi, hurs[j].firstcalltime, hurs[j].dc, hurs[j].nc, hurs[j].nv, hurs[j].value);
				}
			}
		}
		tapp_api_close(h);
*/
	}
	tapp_api_down();

	closeservicelog();

	return 0;
}

#include <stdio.h>

#include "xsys.h"
#include "l_str.h"
#include "ss_service.h"

#include "tap_parse_mng.h"
#include "xini.h"

static tapp_m	tapp_apis[8];
static int	tapp_count = 0;

static bool load_tapp_is_ok(ptapp_m p)
{
	return !(p->h == NULL ||
			 p->fp_tapp_open == NULL ||
			 p->fp_tapp_close == NULL ||
			 p->fp_tapp_check == NULL ||
			 p->fp_tapp_fetch == NULL);
}

int tapp_api_init(const char * filename, const char * inisection)
{
	const char szFunctionName[] = "tapp_api_init";
	int i;
	xini oIni;

	memset(tapp_apis, 0, sizeof(tapp_apis));
	tapp_count = 0;

	char b[2048], *p;

	oIni.open(filename);
	oIni.get(inisection, 0, b, 2048, "\0\0");

    for (i = 0, p = b; *p; p += strlen(p)+1, i++){
		oIni.get(inisection, p, tapp_apis[i].file, 512, p);
		trimstr(tapp_apis[i].name, p);	// fill the name

		if ((tapp_apis[i].h = SysOpenModule(tapp_apis[i].file)) == NULL){
			sprintf(tapp_apis[i].msg, "cannot load the file[%s]", tapp_apis[i].file);
			WriteToEventLog("%s : %s - %s.", szFunctionName, p, tapp_apis[i].msg);
			continue;
		}

		// get functions
		tapp_apis[i].fp_tapp_open	= (fnc_tapp_open  *)SysGetSymbol(tapp_apis[i].h, "tapp_open");
		tapp_apis[i].fp_tapp_close	= (fnc_tapp_close *)SysGetSymbol(tapp_apis[i].h, "tapp_close");
		tapp_apis[i].fp_tapp_check	= (fnc_tapp_check *)SysGetSymbol(tapp_apis[i].h, "tapp_check");
		tapp_apis[i].fp_tapp_fetch	= (fnc_tapp_fetch *)SysGetSymbol(tapp_apis[i].h, "tapp_fetch");

		if (load_tapp_is_ok(tapp_apis + i)){
			WriteToEventLog("%s : init the net driver <%s> ok.", szFunctionName, tapp_apis[i].name);
			tapp_apis[i].state = TAPP_RUN;
		}else{
			WriteToEventLog("%s : load the driver <%s> error, perhaps some function is not found.", szFunctionName, tapp_apis[i].name);
			tapp_apis[i].state = TAPP_CANNOTRUN;
		}
    }
	tapp_count = i;

	return 0;
}

void tapp_api_down()
{
	for (int i = 0; i < tapp_count; i++){
		if (tapp_apis[i].state == TAPP_RUN){
			tapp_api_close(i);
			if (tapp_apis[i].h){
				SysCloseModule(tapp_apis[i].h);  tapp_apis[i].h = 0;
			}
		}
	}
	tapp_count = 0;
}

int tapp_api_open(char * filename)
{
	for (int i = 0; i < tapp_count; i++){
		if (tapp_apis[i].state == TAPP_RUN){
			int r = (* tapp_apis[i].fp_tapp_open)(filename, &tapp_apis[i].htap3);
			if (r == 0){
				return i;
			}
			tapp_api_close(i);
		}
	}
	return -1;
}

void tapp_api_close(int h)
{
	if (h >= 0 && h < tapp_count && tapp_apis[h].htap3 && tapp_apis[h].state == TAPP_RUN){
		(* tapp_apis[h].fp_tapp_close)(&tapp_apis[h].htap3);
	}
}

int	tapp_api_check(int h)
{
	if (h >= 0 && h < tapp_count && tapp_apis[h].htap3 && tapp_apis[h].state == TAPP_RUN){
		return (* tapp_apis[h].fp_tapp_check)(&tapp_apis[h].htap3);
	}
	return -1;
}

int	tapp_api_fetch(eh_hur * phur, int h)
{
	if (h >= 0 && h < tapp_count && tapp_apis[h].htap3 && tapp_apis[h].state == TAPP_RUN){
		return (* tapp_apis[h].fp_tapp_fetch)(phur, &tapp_apis[h].htap3);
	}
	return -1;
}

const char * tapp_api_geterror(int h)
{
	if (h < 0 || h >= tapp_count) {
		return 0;
	}
	if (tapp_apis[h].htap3->msg[0]){
		strcpy(tapp_apis[h].msg, tapp_apis[h].htap3->msg);
	}
	return tapp_apis[h].msg;
}

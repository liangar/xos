#include "des.h"
#include "lfile.h"
#include "MD5.h"
#include "name_value.h"

typedef int (fnc_xos_test)	(char * argv[], int argc);

name_value	g_nv;
deshandle	g_hdes = 0;

int xsys_t_des_open(char * argv[], int argc)
{
	if (argc != 1){
		return -1;
	}
	g_hdes = des_open(argv[0]);
	return ((g_hdes == 0)? -2 : 0);
}

int xsys_t_des_close(char *, int)
{
	return des_close(g_hdes);
}

int xsys_t_des_encode_decode(char * argv[], int argc)
{
	if (argc != 1 || g_hdes == 0){
		return -1;
	}
	char * s = argv[0];
	int l = strlen(s);
	char * d = (char *)malloc(l+16);
	int l0 = des_encode(g_hdes, d, s, l);
	int l1 = des_decode(g_hdes, d, d, l0);

	if (strcmp(d, s) == 0){
		return -2;
	}
	return 0;
}


#include <string.h>

#include <des.h>
#include <xstrcrypt.h>

#include "nr_crypt.h"

const char NR_DES_KEY[] = "mcb/nrt";

char * nr_crypt(char * d, char * s)
{
	deshandle h = des_open(NR_DES_KEY);

	char t[256];
	int l = des_encode(h, t, s, strlen(s));
	xstrcrypt(d, t, l);

	des_close(h);

	return d;
}

char * nr_decrypt(char * d, char * s)
{
	int l = xstrdecrypt(d, s);
	deshandle h = des_open(NR_DES_KEY);
	des_decode(h, d, d, l);
	des_close(h);

	return d;
}


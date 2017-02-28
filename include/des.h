#ifndef _XDES_H_
#define _XDES_H_

typedef unsigned char * deshandle;

deshandle des_open(const char *key, int l = -1); /* initialize all des arrays */
int des_close(deshandle & h);

void des_encode(deshandle h, char * d, char * s); /* encrypt 64-bit inblock */
void des_decode(deshandle h, char * d, char * s); /* decrypt 64-bit inblock */

int des_encode(deshandle h, char * d, char * s, int len);
int des_decode(deshandle h, char * d, char * s, int len);

#endif /* _XDES_H_ */

#ifndef _XSTRCRYPT_H_
#define _XSTRCRYPT_H_

void xstrcrypt  (char * d, char const * s);
void xstrcrypt  (char * d, char const * s, int l);
int  xstrdecrypt(char * d, char const * s);
void xhexdecrypt(char * d, const char * s, int l);

#endif // _XSTRCRYPT_H_

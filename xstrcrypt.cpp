#include <string.h>
#include <xstrcrypt.h>

static inline char atohex(char a)
{
	if (a <= '\x09')  return a + '0';
	return 'a' + a - '\xa';
}

void xstrcrypt(char * d, const char * s)
{
	unsigned char b;
	int l = strlen(s);
	d += l * 2;  * d = '\0';
    for (d--, l--;  l >= 0;  l--){
		b = ((unsigned int)s[l]) ^ ((unsigned int)0x65);
        *d-- = atohex(b & 0xf);
		*d-- = atohex((b >> 4) & 0xf);
    }
}

int xstrdecrypt(char * d, const char * s)
{
	unsigned char b0, b1;

	int i;
	for (i = 0;  *s;  d++, i++){
		b0 = *s++;  if (b0 < 'a')  b0 -= '0';  else  b0 -= 'a' - 10;
		b1 = *s++;  if (b1 < 'a')  b1 -= '0';  else  b1 -= 'a' - 10;
		*d = (unsigned char)(((b0 << 4) | b1) ^ ((unsigned int)0x65));
	}
	*d = '\0';
	return i;
}

void xstrcrypt(char * d, char const * s, int l)
{
	unsigned char b;
	d += l * 2;  * d = '\0';
    for (d --, l--;  l >= 0;  l--){
		b = ((unsigned int)s[l]) ^ ((unsigned int)0x65);
        *d-- = atohex(b & 0xf);
		*d-- = atohex((b >> 4) & 0xf);
    }
}

void xhexdecrypt(char * d, const char * h, int l)
{
    unsigned char b0, b1;
    for (int i = 0; i < l; i++){
        b0 = (*h++) & 0x0f;  b1 = (*h++) & 0x0f;
        *d = (unsigned char)(((b0 << 4) | b1) ^ ((unsigned int)0x65));
    }
    *d = '\0';
}
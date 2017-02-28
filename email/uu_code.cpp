#include <string.h>

#include <l_str.h>

#include <uu_code.h>

const char xmime_uuencode_chars[] = "`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
const char xmime_xxencode_chars[] = "+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


/*Uuencode编码*/
/*
s: 未编码的二进制代码
d：编码过的Uue代码
*/
static void uue(unsigned char * s, unsigned char * d)
{
	int i,k = 2;
	unsigned char t = 0;
	for (i = 0;  i < 3;  i++){
		*(d+i) = *(s+i) >> k;
		*(d+i) |= t;
		if (*(d+i) == 0)
			*(d+i) += 96;
		else
			*(d+i) += 32;
		t = *(s+i) << (8-k);
		t >>= 2;
		k += 2;
	}

	*(d+3) = *(s+2) & 63;
	if (*(d+3) == 0)
		*(d+3) += 96;
	else
		*(d+3) += 32;
}

/*Uuencode解码*/
/*
s：未解码的Uue代码
d：解码过的二进制代码
*/
static void uuu(unsigned char * d, unsigned char * s)
{
	unsigned char b[4];

	memcpy(b, s, 4);

	int i,k=2;
	unsigned char t=NULL;
	if(*b==96) *b=NULL;
	else *b-=32;
	for(i=0;i<3;i++)
	{
		*(d+i)=*(b+i)<<k;
		k+=2;
		if(*(b+i+1)==96)
			*(b+i+1)=NULL;
		else *(b+i+1)-=32;

		t = *(b+i+1) >> (8 - k);
		*(d+i)|=t;
	}
}

int uu_encode(char * d, char * s)
{
	return 0;
}

int uu_decode(char * d, char * s)
{
	int l;
	for (l = 0; *s; ) {
		// get a line and trim it
		char * pline = s;
		s = (char *)getaline(pline, (const char *)s);
		trimstr(pline);

		// caculate
		int len = strlen(pline);	// line length
		int n0 = (*pline++ - 32);		// 
		int n1 = (n0 + 2) / 3 * 4;	// text length

		if (len != n1 + 1 || n0 < 1)
			break;

		for (int i = 0; i < n0; )
		{
			uuu((unsigned char *)d, (unsigned char *)pline);
			i += 3;
			if (i > n0) {
				d += 3 - (i - n0);
				l += 3 - (i - n0);
			}else{
				d += 3, l += 3;
			}
			pline += 4;
		}
	}
	*d = 0;
	return l;
}

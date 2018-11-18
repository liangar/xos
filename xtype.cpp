#include <xsys.h>
#include <ss_service.h>

#include <xtype.h>

const char hex_chars[17] = "0123456789ABCDEF";
inline void hex2chars(char * d, unsigned char h)
{
	d[0] = hex_chars[((h & 0xF0) >> 4)];
	d[1] = hex_chars[((h & 0x0F))];
}

/// str -> bcd
/// "0123456789" -> 0x01 0x23 0x45 0x67 0x89
int x_str2bcd(char * d, const char * s, int len)
{
	int l, i, j;

	memset(d, 0, (len+1)/2);

	l = strlen(s);
	if (l > len)
		l = len;
	for (i = j = 0; s[i] != '\0' && i+1 < l; i += 2, j++) {
		d[j] = (s[i+1] & 0xf) | ((s[i] & 0xf) << 4);
	}
	if (s[i] != '\0') {
		d[j++] = (s[i] & 0xf);
	}
	return j;
}

/// "1234567" ->  0x67 0x45 0x23 0x01 0x00 0x00
int x_str2bcdv(char * d, const char * s, int len)
{
	int l, i, j;

	memset(d, 0, (len+1)/2);

	l = strlen(s);
	if (l > len)
		l = len;
	for (i = l-1, j = 0; s[i] != '\0' && i > 0; i -= 2, j++) {
		d[j] = (s[i] & 0xf) | ((s[i-1] & 0xf) << 4);
	}
	if (i == 0) {
		d[j++] = (s[i] & 0xf);
	}
	return (len+1)/2;
}

/// 0x10 0x32 0x54 0x76 0x98 -> "1032547698"
/// 0x89 0x67 0x45 0x23 0x01 -> "8967452301"
int x_bcd2strv(char * d, const char * s, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		*d++ = ((s[i] & 0xf0) >> 4) | 0x30;
		*d++ = (s[i] & 0x0f) | 0x30;
	}
	*d = 0;
	return 2 * len;
}


/// 0x10 0x32 0x54 0x76 0x98 -> "9876543210"
/// 0x89 0x67 0x45 0x23 0x01 -> "0123456789"
int x_bcd2str(char * d, const char * s, int len)
{
	int i;
	for (i = len-1; i >= 0; i--) {
		*d++ = ((s[i] & 0xf0) >> 4) | 0x30;
		*d++ = (s[i] & 0x0f) | 0x30;
	}
	*d = 0;
	return 2*len;
}

/// 0x10 0x32 0x54 0x76 0x98 -> 9876543210
int x_bcd2long(const char *s, int bytes)
{
	int l = 0, v = 1;
	int i;
	for (i = 0; i < bytes; i++){
		l = l + (((s[i] & 0xf0) >> 4) * 10 + (s[i] & 0x0f)) * v;
		v *= 100;
	}
	return l;
}

/// 9876543210 -> 0x10 0x32 0x54 0x76 0x98
void x_long2bcd(char * d, long v, int bytes)
{
	int i, j;
	memset(d, 0, bytes);
	for (i = 0; v > 0 && i < bytes; i++){
		j = v % 100;
		d[i] = ((j / 10) << 4) | (j % 10);
		v /= 100;
	}
}

/// 9876543210 -> 0x98 0x76 0x54 0x32 0x10
void x_long2bcdv(char * d, long v, int bytes) {
	int i, j;
	memset(d, 0, bytes);
	for (i = bytes - 1; v > 0 && i >= 0; i--) {
		j = v % 100;
		d[i] = ((j / 10) << 4) | (j % 10);
		v /= 100;
	}
}


/// 0x10 0x32 0x54 0x76 0x98 -> 9876543210
int x_bcd2longv(const char *s, int bytes)
{
	int l = 0;
	int i;
	for (i = bytes-1; i >= 0; i--){
		l = l*100 + (((s[i] & 0xf0) >> 4) * 10 + (s[i] & 0x0f));
	}
	return l;
}

void x_long2nt(unsigned char * d, long v)
{
	d[0] = (unsigned char)(v & 0xff);  v >>= 8;
	d[1] = (unsigned char)(v & 0xff);  v >>= 8;
	d[2] = (unsigned char)(v & 0xff);  v >>= 8;
	d[3] = (unsigned char)(v & 0xff);
}

void x_long2ntv(unsigned char * d, long v)
{
	d[3] = (unsigned char)(v & 0xff);  v >>= 8;
	d[2] = (unsigned char)(v & 0xff);  v >>= 8;
	d[1] = (unsigned char)(v & 0xff);  v >>= 8;
	d[0] = (unsigned char)(v & 0xff);
}

void x_long2nt3(unsigned char * d, long v)
{
	d[0] = (unsigned char)(v & 0xff);  v >>= 8;
	d[1] = (unsigned char)(v & 0xff);  v >>= 8;
	d[2] = (unsigned char)(v & 0xff);
}

void x_long2nt3v(unsigned char * d, long v)
{
	d[2] = (unsigned char)(v & 0xff);  v >>= 8;
	d[1] = (unsigned char)(v & 0xff);  v >>= 8;
	d[0] = (unsigned char)(v & 0xff);
}

void x_short2nt(unsigned char * d, unsigned short v)
{
	d[0] = (unsigned char)(v & 0xff);  v >>= 8;
	d[1] = (unsigned char)(v & 0xff);
}

void x_short2ntv(unsigned char * d, unsigned short v)
{
	d[1] = (unsigned char)(v & 0xff);  v >>= 8;
	d[0] = (unsigned char)(v & 0xff);
}

unsigned short x_nt2short(unsigned char * d)
{
	unsigned short v = (unsigned short)d[1];
	v <<= 8;
	v |= (unsigned short)d[0];

	return v;
}

unsigned short x_nt2shortv(unsigned char * d)
{
	unsigned short v = (unsigned short)d[0];
	v <<= 8;
	v |= (unsigned short)d[1];

	return v;
}

long x_nt2long(unsigned char * d)
{
	long v = d[3];  v <<= 8;
	v |= d[2];  v <<= 8;
	v |= d[1];  v <<= 8;
	v |= d[0];

	return v;
}

long x_nt2longv(unsigned char * d)
{
	long v = d[0];  v <<= 8;
	v |= d[1];  v <<= 8;
	v |= d[2];  v <<= 8;
	v |= d[3];

	return v;
}

long x_nt2long3(unsigned char * d)
{
	long v = d[2];  v <<= 8;
	v |= d[1];  v <<= 8;
	v |= d[0];

	return v;
}

long x_nt2long3v(unsigned char * d)
{
	long v = d[0];  v <<= 8;
	v |= d[1];  v <<= 8;
	v |= d[2];

	return v;
}

void x_long2nt(unsigned char * d, long v, int bytes)
{
	switch(bytes){
		case 3: x_long2nt3(d, v); break;
		case 2: x_short2nt(d, (unsigned short)v);  break;
		case 1: d[0] = (unsigned char)v;  break;
		default: x_long2nt(d, v);  break;
	}
}

void x_long2ntv(unsigned char * d, long v, int bytes)
{
	switch(bytes){
		case 3: x_long2nt3v(d, v); break;
		case 2: x_short2ntv(d, (unsigned short)v);  break;
		case 1: d[0] = (unsigned char)v;  break;
		default: x_long2ntv(d, v);  break;
	}
}

long x_nt2long(unsigned char * d, int bytes)
{
	switch(bytes){
		case 3: return x_nt2long3(d);
		case 2: return x_nt2short(d);
		case 1: return long(d[0]);
	}
	return x_nt2long(d);
}

long x_nt2longv(unsigned char * d, int bytes)
{
	switch(bytes){
		case 3: return x_nt2long3v(d);
		case 2: return x_nt2shortv(d);
		case 1: return long(d[0]);
	}
	return x_nt2longv(d);
}

static int x_hex2long(char c)
{
	if (c >= 'A' && c <= 'Z'){
		return (int)(c - 'A' + 10);
	}else if (c >= 'a' && c <= 'z'){
		return (int)(c - 'a' + 10);
	}else if (c >= '0' && c <= '9'){
		return (int)(c - '0');
	}

	return -1;
}

/// "1A2B3C4D" -> 0x1A 0x2B 0x3C 0x4D
/// "1A2B3C4D5" -> 0x1A 0x2B 0x3C 0x4D 0x05
/// @len lt decode bytes, return real decode bytes
int x_hex2array(unsigned char * d, const char * s, int len)
{
	int l, i, j;

//	memset(d, 0, (len+1)/2);

	l = strlen(s);
	for (i = j = 0; s[i] != '\0' && i+1 < l;) {
		int s0 = x_hex2long(s[i++]);
		int s1 = x_hex2long(s[i++]);
		if (s0 < 0 || s1 < 0)
			return -1;

		d[j++] = s1 | (s0 << 4);
		if (s[i] && !isxdigit(s[i]))
			++i;
//		printf("%02X ", d[j]);
	}
	if (s[i] != '\0') {
		int s0 = x_hex2long(s[i]);
		if (s0 < 0)
			return -1;

		d[j++] = (s0 & 0xf);
	}
//	printf("\n");

	l = (len+1)/2;
	if (l > j)
		memset(d+j, 0, l-j);
	else
		l = j;
	return l;
}

/// "1A2B3C4D" -> 0xA1 0xB2 0xC3 0xD4
/// "1A2B3C4D5" -> 0xA1 0xB2 0xC3 0xD4 0x05
int x_hex2array2(unsigned char * d, const char * s, int len)
{
	int l, i, j;

	memset(d, 0, (len+1)/2);

	l = strlen(s);
	for (i = j = 0; s[i] != '\0' && i+1 < l;) {
		int s0 = x_hex2long(s[i++]);
		int s1 = x_hex2long(s[i++]);
		if (s0 < 0 || s1 < 0)
			return -1;

		d[j++] = s0 | (s1 << 4);
		if (s[i] && !isxdigit(s[i]))
			++i;
//		printf("%02X ", d[j]);
	}
	if (s[i] != '\0') {
		int s0 = x_hex2long(s[i]);
		if (s0 < 0)
			return -1;

		d[j++] = (s0 & 0xf);
	}
//	printf("\n");

	return j;
}

// 0x1A 0x2B 0x3C 0x4D 0x05 -> "1A2B3C4D05"
int x_array2hex(char * d, const unsigned char * s, int len)
{
	int l = (len + 1) / 2;
	int i, j;
	for (i = j = 0; i < l; i++){
		hex2chars(d+j, s[i]);
		j += 2;
	}
	d[len] = 0;

	return len;
}

int x_hex2bcd(unsigned char *d, const unsigned char * s, int len)
{
	int i;
	for (i = 0; i < len; i++){
		int n = *s++;
		*d++ = ((n / 10) << 4) | (n % 10);
	}
	return len;
}

int x_bcd2hex(unsigned char *d, const unsigned char * s, int len)
{
	int i;
	for (i = 0; i < len; i++){
		*d++ = (*s & 0x0F) + (*s++ >> 4) * 10;
	}
	return len;
}

void invert_bytes(char * d, int l)
{
	--l;
	for (int i = 0; i < l; i++, l--){
		char t = d[i];
		d[i] = d[l];
		d[l] = t;
	}
}

int x_hex2long(const char * s)
{
	long v0, v;

	if (s == 0)
		return 0;

	while (*s == '0')
		++s;

	for (v = 0; *s; ++s){
		v0 = x_hex2long(*s);
		if (v0 < 0)
			return v0;
		v = (v << 4) | v0;
	}
	return v;
}

void x_long2hex(char * d, long v)
{
	sprintf(d, "%0X", (unsigned int)v);
}

void x_long2hex(char * d, long v, int len)
{
	char sformat[8];
	sprintf(sformat, "%%0%dX", len);
	sprintf(d, sformat, v);
}

char * x_lpad0(char * d, int l)
{
	int len = (char)(strlen(d));
	l -= len;
	if (l > 0){
		memmove(d+l, d, len+1);
		memset(d, '0', l);
	}
	return d;
}

int hamming_weight_4(unsigned int n)
{
    n = (n&0x55555555) + ((n>>1)&0x55555555);
    n = (n&0x33333333) + ((n>>2)&0x33333333);
    n = (n&0x0f0f0f0f) + ((n>>4)&0x0f0f0f0f);
    n = (n&0x00ff00ff) + ((n>>8)&0x00ff00ff);
    n = (n&0x0000ffff) + ((n>>16)&0x0000ffff);

    return int(n);
}

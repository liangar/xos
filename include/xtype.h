#pragma once

#ifndef int32
typedef long           int32;	/* 32-bit signed integer */
#endif

#ifndef uint
typedef unsigned int   uint;	/* 16 or 32-bit unsigned integer */
#endif

#ifndef uint32
typedef unsigned long  uint32;	/* 32-bit unsigned integer */
#endif

#ifndef uint16
typedef unsigned short uint16;	/* 16-bit unsigned integer */
#endif

#ifndef byte_t
typedef unsigned char  byte_t;	/*  8-bit unsigned integer */
#endif

#ifndef uint8
typedef unsigned char  uint8;	/* 8-bit unsigned integer */
#endif

extern const char hex_chars[17];
inline void hex2chars(char * d, unsigned char h);

int x_str2bcd(char * d, const char * s, int len);
int x_str2bcdv(char * d, const char * s, int len);
int x_bcd2str(char * d, const char * s, int len);
int x_bcd2strv(char * d, const char * s, int len);
int x_bcd2long(const char *s, int bytes);
int x_bcd2longv(const char *s, int bytes);
void x_long2bcd(char * d, long v, int bytes);
void x_long2bcdv(char * d, long v, int bytes);

int x_hex2bcd(unsigned char *d, const unsigned char * s, int len);
int x_bcd2hex(unsigned char *d, const unsigned char * s, int len);

int x_hex2array(unsigned char * d, const char * s, int len);
int x_hex2array2(unsigned char * d, const char * s, int len);
int x_array2hex(char * d, const unsigned char * s, int len);

int x_hex2long(const char * s);
void x_long2hex(char * d, long v);
void x_long2hex(char * d, long v, int len);

void x_long2nt(unsigned char * d, long v);
void x_long2nt3(unsigned char *d, long v);
void x_long2ntv(unsigned char * d, long v);
void x_long2nt3v(unsigned char *d, long v);
long x_nt2long(unsigned char * d);
long x_nt2long3(unsigned char * d);
long x_nt2longv(unsigned char * d);
long x_nt2long3v(unsigned char * d);

void x_short2nt(unsigned char * d, unsigned short v);
void x_short2ntv(unsigned char * d, unsigned short v);
unsigned short x_nt2short(unsigned char * d);
unsigned short x_nt2shortv(unsigned char * d);

void x_long2nt(unsigned char * d, long v, int bytes);
void x_long2ntv(unsigned char * d, long v, int bytes);

long x_nt2long(unsigned char * d, int bytes);
long x_nt2longv(unsigned char * d, int bytes);

char * x_lpad0(char * d, int l);

void invert_bytes(char * d, int l);

int hamming_weight_4(unsigned int n);

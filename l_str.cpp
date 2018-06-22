#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <l_str.h>

const char * L_BLANKS = " \t\r\n\x8";

// support \r\n line
const char * getaline(char * d, const char * s)
{
	if (d == NULL || s == NULL){
		if (d)  *d = '\0';  return NULL;
	}
	const char * p = strchr(s, '\n');
	if (p == NULL)
		p = strchr(s, '\r');

	if (p == NULL)
		p = s + strlen(s);

	// # p -> ('\n' | '\r' | '\0')
	int l = p - s;
	if (d != s)
		memmove(d, s, l + 1);
	// # d[l] = ('\n' | '\r' | '\0')

	if (d[l]){
		// # p -> ('\n' | '\r')
		if (d[l-1] == '\r') {
			d[l-1] = 0;
		}else
			d[l] = 0;
		p++;
		// # p -> new line
	}

	return p;
}

const char * getaline(char * d, const char * s, char c)
{
	if (d == NULL || s == NULL){
        if (d)  *d = '\0';  return NULL;
    }
	while (*s != c && *s)  *d++ = *s++;
	if (*s == c)  s++;
	*d = '\0';

	return s;
}

const char * getaword(char *d, const char *s, char c)
{
    if (!d || !s){
        if (d)  *d = '\0';  return NULL;
    }
    const char * p = strchr(s, c);
    if (p == NULL)
        p = s + strlen(s);
    else
        p ++;
    int l = p - s;
    if (l > 0 && s[l-1] == c)
        l --;
    if (d != s)
        memmove(d, s, l);
    d[l] = '\0';
    return p;
}

// +++
const char * getaword(char * d, const char * s, const char * seps)
{
	if (!d || !s){
		if (d)  *d = '\0';  return 0;
	}
	while (*s && strchr(seps, *s))  s++;	// skip seperator chars
	if (*s){
		do {
			*d++ = *s++;
		} while (*s && strchr(seps, *s) == NULL);

		while (*s && strchr(seps, *s)) s++; // skip seperator chars
	}
	*d = '\0';
	return s;
}

// +++
const char *  getaword(char * d, const char * s, const char * seps, const char * ends)
{
	if (!d || !s){
		if (d)  *d = '\0';  return 0;
	}
	while (*s && strchr(seps, *s))  s++;	// skip seperator chars
	if (*s){
		do {
			*d++ = *s++;
		} while (*s && strchr(seps, *s) == NULL && strchr(ends, *s) == NULL);

		while (*s && strchr(seps, *s) && strchr(ends, *s) == NULL) s++; // skip seperator chars
	}
	*d = '\0';
	return s;
}

// +++
char * skipchs (char * s, const char * chars)
{
	if (!s)  return 0;
	while (*s && strchr(chars, *s))  s++;
	return s;
}

// +++
char * skipblanks(char * s)
{
	while (ISBLANKCH(*s))  s++;
	return s;
}

// +++
int trimstr(char * d)
{
	if (d == NULL)  return -1;

	char *r = d;

	int i =0;
    for (i = 0; *d; d++){
		if (!ISBLANKCH(*d))
			r[i++] = *d;
		else if (i && !ISBLANKCH(r[i-1])){
			r[i++] = ' ';
		}
	}
	if (i && r[i-1] == ' ')  i--;
    r[i] = '\x0';
    return i+1;
}

// 
int trimstr(char * d, char * s)
{
	if (d == NULL)  return -1;
	if (s == NULL){
		*d = '\0';  return 0;
	}

	bool instring = false;	char ch = 0;
	int i = 0;
    for (i = 0; *s; s++){
		if (instring || !ISBLANKCH(*s)){
			d[i++] = *s;
			if (*s == '\'' || *s == '"'){
				if (instring){
					if (ch == *s)
						instring = false;
				}else{
					instring = true;  ch = *s;
				}
			}
		}else{
			if (i){
				if (!ISBLANKCH(d[i-1]))
					d[i++] = ' ';
				if (*s == '\n')
					d[i-1] = '\n';
			}
		}
	}
	if (i && d[i-1] == ' ')  i--;
    d[i] = '\x0';
    return i+1;
}

int	trim_head(char * d, char * s)
{
	s = skipblanks(s);
	return strmove(d, s);
}

int trim_head0(char * d)
{
	char * s;
	for (s = d; *s == '0' || *s == ' '; s++)
		;
	int i = 0;
	if (d != s){
		if (*s == '\0'){
			d[0] = '0';  d[++i] = '\0';	// 只保留一个0
		}else{
			do{
				*d++ = *s++; ++i;
			}while(*s);
			*d = '\0';
		}
	}
	return i;
}


int	trim_tail(char * d, char * s)
{
	char * p = s + strlen(s) - 1;
	while (p >= s && ISBLANKCH(*p))  p--;
	int l = int(p + 1 - s);
	if (l > 0 && d != s)
		memmove(d, s, l);
	d[l] ='\0';
	return l;
}

int	trim_all(char * d, char * s)
{
	if (s == 0 || *s == '\0') {
		*d = '\0';  return 0;
	}
	int l;
	s = skipblanks(s);
	if (*s == '\0'){
		l = 0;
	}else{
		char * e = s + strlen(s) - 1;
		while (e >= s && ISBLANKCH(*e))  e--;
		l = int(e + 1 - s);
		if (l > 0 && d != s)
			memmove(d, s, l);
	}
	d[l] = '\0';

	return l;
}

int	trim_all(char * d, char * s, char *blanks)
{
	if (s == 0 || *s == '\0') {
		*d = '\0';  return 0;
	}
	s = skipchs(s, blanks);
	char * e = s + strlen(s) - 1;
	while (e >= s && strchr(blanks, *e))  e--;
	int l = int(e + 1 - s);
	if (l > 0 && d != s)
		memmove(d, s, l);
	d[l] = '\0';

	return l;
}

int	trim_all(char * d, char * s, char blank)
{
	if (s == 0 || *s == '\0') {
		*d = '\0';  return 0;
	}
	while (*s == blank)  s++;
	char * e = s + strlen(s) - 1;
	while (e >= s && *e == blank)  e--;
	int l = int(e + 1 - s);
	if (l > 0 && d != s)
		memmove(d, s, l);
	d[l] = '\0';

	return l;
}

int	trim_all(char * d)
{
	return trim_all(d, d);
}

// ++
int endcmp(const char * d, const char * e)
{
    if (d == NULL && e == NULL)
        return 0;
    if ((d == NULL && e != NULL) || (d != NULL && e == NULL))
        return -1;
    int l0 = strlen(d);
    int l1 = strlen(e);
    if (l0 < l1)
        return -2;
	return strcmp(d+l0-l1,e);
}

// ++
int padr(char * d, char * s, int len)
{
    int i;
    if (d == NULL || s == NULL)  return -1;
    if (d == s)
        i = strlen(s);
    else
        for (i = 0; i < len && s[i]; i++)
            d[i] = s[i];
    for (; i < len; i++)
		d[i] = ' ';
    d[i] = '\0';
    return len;
}

char * ht_lpad0(char * d, int l)
{
	int len = strlen(d);
	l -= len;
	if (l > 0){
		memmove(d+l, d, len+1);
		memset(d, '0', l);
	}
	return d;
}

char * ht_lpad(char * d, int l)
{
	int len = strlen(d);
	l -= len;
	if (l > 0){
		memmove(d+l, d, len+1);
		memset(d, ' ', l);
	}
	return d;
}

// ++
int bcdtoint(char * d, int len)
{
    int n = 0;
    for (int i = 0; i < len; i += 2)
        n = n * 100 + ((d[0] & 0xf0) >> 4) * 10 + (d[0] & 0x0f);
    if (len % 2)
        n /= 10;
    return n;
}

// ++
int strmove(char * d, char * s)
{
    if (d == NULL || s == NULL)
        return -1;
    int l = strlen(s);
	if (d != s)
		memmove(d, s, l+1);
    return l;
}

// ++
int strreplace(char * d, char * s0, char * s1)
{
    if (d == NULL || s0 == NULL || s1 == NULL)  return -1;
    if (*s0 == '\0')  return 0;
    int l0 = strlen(s0), l1 = strlen(s1);
    char * p, * q;
    p = q = d;
    while (*q && (p = strstr(q, s0)) != NULL){
        strmove(p+l1, p+l0);
        memcpy(p, s1, l1);
        q = p+l1;
    }
    return 0;
}

int strreplace(char * s0, int l0, char * s1, int l1)
{
	if (l1 <= 0)  l1 = strlen(s1);
	strmove(s0+l1, s0+l0);
	memcpy(s0, s1, l1);
	return 0;
}

int strinsert(char * d, char * s, int l)
{
	if (!d || l == 0 || !s || (l < 0 && *s == '\0'))  return 0;
	if (l < 0)
		l = strlen(s);
	strmove(d+l, d);
	memcpy(d, s, l);
	return l;
}

int	countlines(const char * s, char c)
{
	if (s == 0 || *s == '\0')  return 0;

	int i = 0;
	for (i = 0; *s; s++)
		if (*s == c) i++;

	if (i && s[-1] != c)
		++i;
	
	return i;
}

int	countwords(const char * s, const char * seps)
{
	if (s == 0 || *s == '\0')  return 0;
	if (seps == 0)  return -1;
	if (*seps == '\0')  return 1;

	int i = 0;
	for (i = 0; *s;){
		while (*s && strchr(seps, *s))  s++;	// skip seperator chars
		if (*s){
			i++;
			while (*s && strchr(seps, *s) == NULL) s++;
		}
	}
	return i;
}

int	splitstr(char * d, char splitchar) // 返回字符串数目
{
	char *p;	int i;
	for (p = d, i = 0; *p; d = p, i++){
		if ((p = strchr(d, splitchar)) == 0){
			p = d + strlen(d);
			++i;
			break;
		}
		*p++ = '\0';
	}
	if (p[-1] != '\0')
		p[1] = '\0';
	return i;
}

int	splitstr(char * d, char * splitstr)
{
	char * p, * q;	int i, l0;

	l0 = strlen(splitstr);
	if (l0 < 1)
		return 1;

	for (q = p = d, i = 0; *p; d = p, i++){
		if ((p = strstr(d, splitstr)) == 0){
			q += strmove(q, d) + 1;
			p = d + strlen(d);
			++i;
			break;
		}
		*p = '\0';  p += l0;
		q += strmove(q, d) + 1;
	}
	*q = '\0';

	return i;
}

// 将字符串s(最大长度maxlen)中以p开始的一个子串取到d中,返回下一子串位置,如果最后,则返回0
char * getaword(char * d, char * s, const char *p, int maxlen)
{
	if (p >= s) {
		return 0;
	}
	strcpy(d, p);
	return (char *)(p + strlen(p) + 1);
}

bool checkmask(const char * data, const char * mask)
{
	int i = 0;
	for (i = 0; mask[i] && mask[i] != '*'; i++) {
		int ch = mask[i], d = data[i];
		if (ch == '@') {
			if (!isupper(d))  return false;
		}else if (ch == '$') {
			if (!islower(d))  return false;
		}else if (ch == '#'){
			if (!isdigit(d))  return false;
		}else if (ch == '&') {
			if (!isxdigit(d)) return false;
		}else if (ch == '^'){
			if (!isdigit(d) && !isupper(d) && !islower(d)) {
				return false;
			}
		}else if (ch != d)
			return false;
	}
	if (mask[i] == '*') {
		return true;
	}
	return (data[i] == 0);
}

// return
//	 > 0 : s length
//   < 0 : s[-r - 1] is not digit
int isdigits(const char * s)
{
	int i;
	for (i = 0; s[i] != 0; i++) {
		if (!isdigit(s[i])) {
			return -(i + 1);
		}
	}
	return i;
}

int strwordwrap(char *d, const char * s, int rowlen, int rows, int startpos)
{
	if (rowlen < 2 || rows < 1)
		return -1;

	int i = 0, j = startpos;
	for (i = 0; i < rows && *s; i++){
		while (j < rowlen && *s){
			if (*s < 0){
				if (j+2 > rowlen)
					break;
				*d++ = *s++;
				*d++ = *s++;
				j += 2;
			}else{
				*d++ = *s++;
				++j;
			}
		}
		*d++ = '\r';
		*d++ = '\n';
		j = 0;
	}
	*d = 0;

	return i;
}

int mbs2wcs(wchar_t * d, const char * s, int l)
{
	return mbstowcs(d, s, l);
}

int wcs2mbs(char * d, const wchar_t * s, int l)
{
	return wcstombs(d, s, l);
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

int c2string(char * d, const char * s)
{
	char * d0 = d;
	while (*s){
		if (*s == '\\'){
			++s;
			switch (*s){
				case '\0': *d++ = '\\'; break;
				case '\\': *d++ = '\\'; break;
				case 'r': *d++ = '\r';  break;
				case 'n': *d++ = '\n';  break;
				case 't': *d++ = '\t';  break;
				case 'x': ++s;  *d = x_hex2long(*s++);  *d <<= 4;  *d++ |= x_hex2long(*s);  break;
				default : *d++ = '\\';  *d++ = *s;  break;
			}
			if (*s)
				++s;
		}else
			*d++ = *s++;
	}
	*d = 0;
	return int(d - d0);
}

int string2c(char * d, const char * s)
{
	char *d0 = d;

	while (*s){
		switch (*s){
			case '\r':  *d++ = '\\';  *d++ = 'r';  break;
			case '\n':  *d++ = '\\';  *d++ = 'n';  break;
			case '\t':  *d++ = '\\';  *d++ = 't';  break;
			case '\\':  *d++ = '\\';  *d++ = '\\'; break;
			default:  *d++ = *s++;  break;
		}
	}
	*d = 0;

	return int(d - d0);
}

const char * strin(const char * str_array, const char * sfind)
{
	const char * p;
	int l;
	for (p = str_array; *p; p += strlen(p) + 1){
		if (strcmp(sfind, p) == 0)
			return p;
	}
	return 0;
}

const char * striin(const char * str_array, const char * sfind)
{
	const char * p;
	int l;
	for (p = str_array; *p; p += strlen(p) + 1){
		if (stricmp(sfind, p) == 0)
			return p;
	}
	return 0;
}

const char *  skip0s(const char * s)
{
	while (*s == '0' || *s == ' ' || *s == '\t')
		++s;
	return s;
}

int compare_number(const char * s0, const char * s1)
{
	return strcmp(skip0s(s0), skip0s(s1));
}

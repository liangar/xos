#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <des.h>

#include "des.tbl"

#ifndef BYTE
#define BYTE unsigned char
#endif

static void fctf(BYTE * out_array, BYTE * in_array, BYTE * key);
static void select(BYTE * out_array, BYTE * in_array, int sel_array[][4][16]);
static void shift(BYTE * array, int iteration);
static void hxor(BYTE * a, BYTE * b, int l);
static void permut(BYTE * out_array, BYTE * in_array, int * sel_array);
static int  bittest(BYTE * s, int bitno);
static void bitset(BYTE * array, int bitno);

deshandle des_open(const char * key, int l)
{
	BYTE * h = (BYTE *)calloc(8*16, 1);

	BYTE bkey[7], k[12];	int i;

	memset(k, '\0', 8);
	memset(bkey, '\0', 7);

	if (l <= 0)
		strncpy((char *)k, key, 8);
	else{
		if (l > 8)  l = 8;
		memcpy(k, key, l);
	}

	permut(bkey, k, p1);	/* PERMUTATION NU 1 */

	for (i = 0 ; i < 16 ; i++) {
		shift(bkey, i);				/* SHIFT ACCORDING TO TABLE 'TSHIFT' */
		permut(h + i*7, bkey, p2);	/* GENERATE KEY ORDER iter */
	}
	return h;
}

int des_close(deshandle & h)
{
	if (h){
		free(h);  h = 0;
	}
	return 0;
}

int des_encode(deshandle h, char * d, char * s, int len)
{
	int l;
	if (len <= 0)  return len;
	if ((len & 0x7) != 0)
		l = 8 - (len & 0x7);
	else
		l = 0;

	char * p = (char *)calloc(len+l, 1);
	memcpy(p, s, len);  l += len;

	for (len = 0, s = p; len < l; len += 8, d += 8, s += 8){
		des_encode(h, d, s);
	}
	free(p);

	return l;
}

int des_decode(deshandle h, char * d, char * s, int len)
{
	if (len <= 0 || (len & 0x7) != 0)  return -1;

	int i;
	for (i = 0; i < len; i += 8, d += 8, s += 8){
		des_decode(h, d, s);
	}
	return i;
}

void des_encode(deshandle h, char * d, char * s)
{
	BYTE  base[8], nbase[8], *ln, *rn, *ln_1, *rn_1;	int i;

	memset(base, '\0', 8);
	memset(nbase, '\0', 8);
	permut(base, (BYTE *)s, ip);	/* INITIAL PERMUTATION 'IP'          */

	ln = nbase;  rn = nbase + 4;
	ln_1 = base;  rn_1 = base + 4;

	for (i = 0; i < 16 ; i++){
		memcpy(ln, rn_1, 4);   			/* Ln = Rn-1 */
		fctf  (rn, rn_1, h + i);   /* F(Rn-1,Kn) */
		hxor  (rn, ln_1, 4);			/* Rn = Ln-1 + F(Rn-1,Kn) */
		memcpy(base, nbase, 8);
	}

	memcpy(base+4,nbase,4);		/* SWAP LEFT & RIGHT */
	memcpy(base,nbase+4,4);

	memset(d, 0x00, 8);
	permut((BYTE *)d, base, ip_1);	/* OUTPUT PERMUTATION 'IP-1' */
}

void des_decode(deshandle h, char * d, char * s)
{
	BYTE  base[8], nbase[8], *ln, *rn, *ln_1, *rn_1;

	memset(base, '\0', 8);
	memset(nbase, '\0', 8);
	permut(base, (BYTE *)s, ip);	/* INVERSE PERMUTATION 'IP'        */

	memcpy(nbase,base,8);	/* SWAP LEFT & RIGHT */
	memcpy(base+4,nbase,4);
	memcpy(base,nbase+4,4);

	ln = base;	rn = base+4;
	ln_1 = nbase;  rn_1 = nbase+4;

	for (int i = 15; i >= 0 ; i--) {
		memcpy(rn_1,ln,4);   		/* Rn-1 = Ln */
		fctf(ln_1, ln, h + i);   /* F(Ln,Kn) */
		hxor(ln_1,rn,4);				/* Ln-1 = Rn + F(Ln,Kn) */
		memcpy(base, nbase, 8);
	}

	memset(d, 0x00, 8);
	permut((BYTE *)d, base, ip_1);	/* PERMUTATION 'IP-1' */
}


/* FUNCTION 'F' AS DEFINED IN STANDARD */
static void fctf(BYTE * out_array, BYTE * in_array, BYTE * key)
{
	BYTE  base[8], nbase[8];

	memset(base, 0x00, 6);
	memset(nbase, '\x0', 6);
	permut(base, in_array, expand);	/* EXPANSION OF 32 INTO 48 BITS */
	hxor(base, key, 6);		/* XOR WITH Kn */
	select(nbase, base, sel);	/* SELECT ONLY 32 BITS OUT OF 48 */
	memset(out_array, 0x00,4);
	permut(out_array, nbase,perm);	/* APPLY FINAL PERMUTATION 'PERM'*/
}

/* SELECTION FUNCTION */
static void select(BYTE * out_array, BYTE * in_array, int sel_array[][4][16])
{
	int b, bn, i, j, bs, exponent, val4;
	
	memset(out_array,0x00,4);
	for ( b = 0; b<8 ; b++) {
		bn = (b*6)+1;			/* Bit offset = byte offset x 6  */
		i  = 0;
		if (bittest(in_array, bn+5)) i = 1;	/* GET Ith COLUMN */
		if (bittest(in_array, bn)) i += 2;
		j  = 0; exponent = 8;			/* GET Jth COLUMN */
		for ( bs = 1; bs<5 ; bs++) {
			if (bittest(in_array, bn+bs)) j += exponent;
			exponent >>= 1;
		}
		val4 = sel_array[b][i][j];
		
		if ( (b&1) == 0 ) val4 <<= 4;		/* PUT IN HIGH NIBBLE IF EVEN */
		out_array[b>>1] |= val4;		/* LOAD NIBBLE AT OFFSET */
	}
}

/* GENERATION OF THE 16 KEY VARIATIONS */
/* SHIFT SCHEME OF 1 OR 2 BITS LEFT */
static void shift(BYTE * array, int iteration)
{
	int i, j;
	union {
		BYTE  s[4];
		long int li;
	} us;

	BYTE  c[7],d[7];

	memset(&us, '\0', sizeof(us));

	for (i=6,j=0; i>=0;j++,i--) {		/* REVERSE ARRAY */
		c[j] = array[i];
	}
	memcpy(us.s,c+3,4);
	us.s[0] = ( us.s[0]&0xf0 ) | ( us.s[3]>>4 );
	us.li <<= tshift[iteration];		/* SHIFT 'C' PART */

	memcpy(d+3,us.s,4);			/* STORE RESULT   */
	memcpy(us.s,c,4);
	us.s[3] &= 0x0f;
	us.li <<= tshift[iteration];		/* SHIFT 'D' PART */
	us.s[0] |=  us.s[3]>>4 ;
	
	memcpy(d,us.s,3);
	d[3] = (d[3]&0xf0) | (us.s[3]&0x0f);

	for (i=6,j=0; i>=0;j++,i--) {		/* REVERSE ARRAY BACK */
		array[j] = d[i];
	}
}

/* XOR 'B' INTO 'A' ON LENGTH 'LG' */
static void hxor(BYTE * a, BYTE * b, int l)
{
	for (; l--;) *a++ ^= *b++;
}

/* GENERAL PERMUTATION ROUTINE */
static void permut(BYTE * out_array, BYTE * in_array, int * sel_array)
{
	int i;
	for (i = 1; *sel_array; i++) {
		if (bittest(in_array, *sel_array++)){
			bitset(out_array, i);
		}
	}
}

/* TEST BIT IN 'ARRAY' AT BIT OFFSET 'BITNO' */
static int bittest(BYTE * s, int bitno)
{
	--bitno;
	BYTE b = s[(bitno >> 3)];
	b >>= 7 - (bitno & 0x07);
	return (b & 0x01);
}

/* SET BIT IN 'ARRAY' AT BIT OFFSET 'BITNO' */
static void bitset(BYTE * array, int bitno)
{
	--bitno;
	*(array+(bitno>>3)) |= (0x80 >> (bitno&0x7));
}

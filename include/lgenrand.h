#ifndef _L_GENRAND_H_
#define _L_GENRAND_H_

void genrand(long & l);
void genrand(double & d);
void genrand(char * p);				// odbc format datetime
void genrand(char * p, int maxlen);

void genrand(long * l);
void genrand(double * d);

#endif

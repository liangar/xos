#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "lgenrand.h"

static double forlong[8] = {10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
static double fordouble[8]={100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
static char   forstring[] ="this is a string for test.";

// generate data type
void genrand(long & l)
{
    // srand((unsigned)time(NULL));
    double d = (double(rand()))/RAND_MAX;
    l = (long(d * forlong[rand()&0x7]));
}

void genrand(double & dd)
{
    // srand((unsigned)time(NULL));
    double d = (double(rand()))/RAND_MAX;
    dd = d * fordouble[rand()&0x7];
}

void genrand(long * l)
{
	genrand(*l);
}

void genrand(double * d)
{
	genrand(*d);
}

void genrand(char * d) // odbc format datetime
{
    time_t t = time(NULL);
    struct tm * p = localtime(&t);
    sprintf(d, "%04d-%02d-%02d %02d:%02d:%02d", p->tm_year + 1900, p->tm_mon+1, p->tm_mday,
                                                p->tm_hour, p->tm_min, p->tm_sec);
}

void genrand(char * p, int maxlen)
{
    strncpy(p, forstring + rand()%20, maxlen);  p[maxlen-1] = '\0';
}

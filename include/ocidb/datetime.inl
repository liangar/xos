// OraLib 0.0.4 / 2002-07-28
//	datetime.inl
//
//	http://606u.dir.bg/
//	606u@dir.bg

#ifndef	_DATETIME_INL
#define	_DATETIME_INL

#include <stdio.h>
#include <time.h>

namespace oralib {

enum MonthsEnum
{
	jan = 1,
	feb,
	mar,
	apr,
	may,
	jun,
	jul,
	aug,
	sep,
	oct,
	nov,
	dec
};


// helper date/time class with no input validation at all
class datetime
{
public:
	sb2		y;
	MonthsEnum	m;
	ub1		d;
	ub1		h;
	ub1		i;
	ub1		s;

public:
	inline datetime (void) {};

	inline datetime (
		IN ub1 dd,
		IN MonthsEnum mm,
		IN ub2 yyyy,
		IN OPTIONAL ub1 hh = 0,
		IN OPTIONAL ub1 mi = 0,
		IN OPTIONAL ub1 ss = 0) : y (yyyy), m (mm), d (dd), h (hh), i (mi), s (ss)
		{ };

	inline datetime (
		IN const OCIDate& o)
		{ operator = (o); };

public:
	inline ub1 day (void) const { return (d); };
	inline void day (
		IN ub1 dd)
		{ d = dd; };

	inline MonthsEnum mon (void) const
		{ return (m); };
	inline void mon (
		IN MonthsEnum mm)
		{ m = mm; };

	inline sb2 year (void) const
		{ return (y); };
	inline void year (
		IN sb2 yy)
		{ y = yy; };

	inline ub1 hour (void) const
		{ return (h); };
	inline void hour (
		IN ub1 hh)
		{ h = hh; };

	inline ub1 minute (void) const
		{ return (i); };
	inline void minute (
		IN ub1 mi)
		{ i = mi; };

	inline ub1 sec (void) const
		{ return (s); };
	inline void sec (
		IN ub1 ss)
		{ s = ss; };

	inline datetime& operator = (
		IN const OCIDate& o)
	{
		y = o.OCIDateYYYY;
		m = (MonthsEnum) (o.OCIDateMM);
		d = o.OCIDateDD;
		h = o.OCIDateTime.OCITimeHH;
		i = o.OCIDateTime.OCITimeMI;
		s = o.OCIDateTime.OCITimeSS;
		return (*this);
	};

	inline void setvalue(IN const time_t t)
	{
		struct tm *ltime = localtime(&t);
		y = ub2(1900 + ltime->tm_year);
		m = MonthsEnum(ltime->tm_mon + 1);
		d = ub1(ltime->tm_mday);
		h = ub1(ltime->tm_hour);
		i = ub1(ltime->tm_min);
		s = ub1(ltime->tm_sec);
	};

	inline bool setvalue(IN const char * t)
	{
		if (strlen(t) != 14) {
			return false;
		}
		char v[8];
		memcpy(v, t+ 0, 4);  v[4] = 0;;
		y = ub1(atoi(v) - 1900);
		y+=1900;
		memcpy(v, t+ 4, 2);  v[2] = 0;  m = MonthsEnum(atoi(v));
		memcpy(v, t+ 6, 2);  v[2] = 0;  d = ub1(atoi(v));
		memcpy(v, t+ 8, 2);  v[2] = 0;  h = ub1(atoi(v));
		memcpy(v, t+10, 2);  v[2] = 0;  i = ub1(atoi(v));
		memcpy(v, t+12, 2);  v[2] = 0;  s = ub1(atoi(v));
		return true;
	};

	inline time_t totime()
	{
		struct tm t;
		t.tm_year = y - 1900;
		t.tm_mon  = m - 1;
		t.tm_mday = d;
		t.tm_hour = h;
		t.tm_min  = i;
		t.tm_sec  = s;
		return mktime(&t);
	}

	inline const char * toSimpleTime(char * dest)
	{
		static char buf[16];
		if (dest == 0) {
			dest = buf;
		}
		sprintf(dest,"%04d%02d%02d%02d%02d%02d", int(y), int(m), int(d), int(h), int(i), int(s));
		return dest;
	}

	inline void set (
		OUT OCIDate& o) const
	{
		o.OCIDateYYYY = y;
		o.OCIDateMM = static_cast <ub1> (m);
		o.OCIDateDD = d;
		o.OCIDateTime.OCITimeHH = h;
		o.OCIDateTime.OCITimeMI = i;
		o.OCIDateTime.OCITimeSS = s;
	};

	inline const char* dt_YYYYMMDD(char * YYYYMMDD)
	{
		char d[16];
		toSimpleTime(d);
		memcpy(YYYYMMDD, d, 8);  YYYYMMDD[8] = 0;

		return YYYYMMDD;
	}

	inline const char* dt_YYYYMM(char * YYYYMM)
	{
		char d[16];
		toSimpleTime(d);
		memcpy(YYYYMM, d, 6);  YYYYMM[6] = 0;

		return YYYYMM;
	}
}; // datetime class

}; // oralib namespace


#endif // _DATETIME_INL

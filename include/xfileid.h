#ifndef _XFILEID_H_
#define _XFILEID_H_

/*! \class xfileid
 *  ÎÄ¼þ±àºÅÀà
 */
class  xfileid{
public:
	xfileid();

	long getid();
	char * getstringdate(long id, char *pdate);
	char * getsimpledate(long id, char *pdate);

private:
	void buildid(long id);

private:
	long m_id;
	int m_year, m_mon, m_day;
	long m_second;
};

#endif // _XFILEID_H_


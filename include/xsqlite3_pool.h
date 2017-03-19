#ifndef _XSQLITE3_POOL_H_
#define _XSQLITE3_POOL_H_

#include <xsys.h>
#include <xsqlite3.h>

#include <xlist.h>

struct xdb_control
{
	xsqlite3 * pdb;
	bool isused;	// �Ƿ���ʹ�ã�true/false��ʹ��/����
	long createtime;
};

/*! \class xsqlite3_pool
*  ���ӳ���
*/
class xsqlite3_pool{
public:
	//! ����
	xsqlite3_pool();

	//! ����
	~xsqlite3_pool();

	//! ��ʼ��������
	// \return -1,ȡ���ڴ�ռ�ʧ��;0,ȡ�����ݿ�����ʧ��;1,�ɹ�
	int init(const char * dsn, int maxnum = 6,long maxfreetime = 600, long m_idle = 300, long maxlifetime = 1800);

	void down(void);

	//! ��ȡ����
	xsqlite3 * open(void);

	//! �黹����
	/*
	* \param pdb �黹�����Ӷ���
	* \param isok �黹�����Ӷ����Ƿ���ã�true/false������/������
	*/
	void close(xsqlite3 * pdb, bool isok = true);

	//! �������ӳ�
	// ��init���ý����̹߳���
	static unsigned int schedule(void * sdata);

	const char * get_lastmsg(void)  {  return (const char *)m_lastmsg;  }

protected:
	void set_null(void);

public:
	char m_dsn[256];	/// ���ݿ�������(�ļ���)

	int  m_maxnum;		/// ���������
	long m_maxfreetime;	/// ������ʱ��(��)
	long m_maxlifetime; /// �������ʱ��(��)
	long m_idle;		/// ��ѭ���ʱ��(��)
	xsys_event	m_hnotify;

protected:
	xlist<xdb_control>	m_list;

	xsys_mutex m_db_lock;

	xsys_mutex m_mutex;
	xsys_thread m_monitor;
	xsys_event	m_estop;

	char m_lastmsg[1024];
};

#endif // _XSQLITE3_POOL_H_
#ifndef _CON_POOL_h
#define _CON_POOL_h

#include <xsys_oci.h>
#include <xlist.h>

#include <connection.h>

using namespace oralib;

static unsigned int schedule(void *);

struct con_control
{
	connection * con;
	bool isused;	// �Ƿ���ʹ�ã�true/false��ʹ��/����
	long createtime;
};

/*! \class con_pool
*  ���ӳ���
*/
class con_pool{
public:
	//! ����
	con_pool();

	//! ����
	~con_pool();

	//! ��ʼ��������
	// \return -1,ȡ���ڴ�ռ�ʧ��;0,ȡ�����ݿ�����ʧ��;1,�ɹ�
	int init(
		const char * service_name, const char * uid, const char * pwd,
		int maxnum = 6, long maxfreetime = 600, long maxlifetime = 1800, long m_idle = 300
	);
	int init(const char * dsn, int maxnum = 6,long maxfreetime = 600, long maxlifetime=1800, long m_idle = 300);

	void down(void);

	//! ��ȡ����
	connection * open();

	//! �黹����
	/*
	* \param con �黹�����Ӷ���
	* \param isok �黹�����Ӷ����Ƿ���ã�true/false������/������
	*/
	void close(connection * con, bool isok = true);

	//! �������ӳ�
	// ��init���ý����̹߳���
	static unsigned int schedule(void * sdata);

	const char * get_lastmsg(void)  {  return (const char *)m_lastmsg;  }

protected:
	void set_null(void);

public:
	char m_service_name[30];	/// ��������
	char m_uid[20];				/// �û���
	char m_pwd[20];				/// ����

	int  m_maxnum;			/// ���������
	long m_maxfreetime;		/// ������ʱ��(��)
	long m_maxlifetime; /// �������ʱ��(��)
	long m_idle;			/// ��ѭ���ʱ��(��)
	xsys_event	m_hnotify;

	int m_concount;		/// ��ǰ������
	int m_numno;		/// ���ӱ��

protected:
	xlist<con_control>	m_list;

	xsys_mutex m_mutex;
	xsys_thread m_monitor;
	xsys_event	m_estop;

	char m_lastmsg[1024];
};

#endif
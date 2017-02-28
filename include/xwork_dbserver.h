#ifndef _XWORK_DBSERVER_H_
#define _XWORK_DBSERVER_H_

#include <xsys_oci.h>
#include <xwork_server.h>

using namespace oralib;

/// \class xwork_dbserver
/// ������Oracle���ݿ�����ķ���
class xwork_dbserver : public xwork_server
{
public:
	/// ����
	xwork_dbserver();

	/// ����
	/// \param name ��������,���Ȳ�����128
	/// \param dbstring ���ݿ����Ӵ�
	/// \param nmaxexception �������̵��쳣����
	/// \param works �����߳���
	xwork_dbserver(const char * name, const char * dbstring, int nmaxexception = 5, int works = 1);

	bool close(int secs = 5);	/// �ر��ͷ�

	/// ��������
	void run(void);
	/// ������Ҫʵ�ֵĹ�������
	virtual void run_db(void) = 0;

protected:
	connection m_db;			/// ���ݿ�����
	const char * m_pdbstring;	/// ���ݿ����Ӵ�
};

#endif // _XWORK_DBSERVER_H_

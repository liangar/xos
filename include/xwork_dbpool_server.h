#pragma once

#include <con_pool.h>
#include <xwork_server.h>

using namespace oralib;

/// \class xwork_dbserver
/// ������Oracle���ݿ�����ķ���
class xwork_dbpool_server : public xwork_server
{
public:
	/// ����
	xwork_dbpool_server();

	/// ����
	/// \param name ��������,���Ȳ�����128
	/// \param dbstring ���ݿ����Ӵ�
	/// \param nmaxexception �������̵��쳣����
	/// \param works �����߳���
	xwork_dbpool_server(const char * name, con_pool * pdbpool, int nmaxexception = 5, int works = 1);

	bool close(int secs = 5);	/// �ر��ͷ�

	/// ��������
	void run(void);
	/// ������Ҫʵ�ֵĹ�������
	virtual void run_db(void) = 0;

protected:
	connection * m_pdb;		/// ���ݿ�����
	con_pool   * m_pdbpool;	/// ���ݿ����ӳ�
};

#ifndef _XWORK_DBSERVER_H_
#define _XWORK_DBSERVER_H_

#include <xsys_oci.h>
#include <xwork_server.h>

using namespace oralib;

/// \class xwork_dbserver
/// 带有有Oracle数据库操作的服务
class xwork_dbserver : public xwork_server
{
public:
	/// 构造
	xwork_dbserver();

	/// 构造
	/// \param name 服务名称,长度不超过128
	/// \param dbstring 数据库连接串
	/// \param nmaxexception 最多可容忍的异常次数
	/// \param works 服务线程数
	xwork_dbserver(const char * name, const char * dbstring, int nmaxexception = 5, int works = 1);

	bool close(int secs = 5);	/// 关闭释放

	/// 工作函数
	void run(void);
	/// 派生类要实现的工作函数
	virtual void run_db(void) = 0;

protected:
	connection m_db;			/// 数据库连接
	const char * m_pdbstring;	/// 数据库连接串
};

#endif // _XWORK_DBSERVER_H_

#pragma once

#include <con_pool.h>
#include <xwork_server.h>

using namespace oralib;

/// \class xwork_dbserver
/// 带有有Oracle数据库操作的服务
class xwork_dbpool_server : public xwork_server
{
public:
	/// 构造
	xwork_dbpool_server();

	/// 构造
	/// \param name 服务名称,长度不超过128
	/// \param dbstring 数据库连接串
	/// \param nmaxexception 最多可容忍的异常次数
	/// \param works 服务线程数
	xwork_dbpool_server(const char * name, con_pool * pdbpool, int nmaxexception = 5, int works = 1);

	bool close(int secs = 5);	/// 关闭释放

	/// 工作函数
	void run(void);
	/// 派生类要实现的工作函数
	virtual void run_db(void) = 0;

protected:
	connection * m_pdb;		/// 数据库连接
	con_pool   * m_pdbpool;	/// 数据库连接池
};

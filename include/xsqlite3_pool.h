#ifndef _XSQLITE3_POOL_H_
#define _XSQLITE3_POOL_H_

#include <xsys.h>
#include <xsqlite3.h>

#include <xlist.h>

struct xdb_control
{
	xsqlite3 * pdb;
	bool isused;	// 是否在使用，true/false，使用/空闲
	long createtime;
};

/*! \class xsqlite3_pool
*  连接池类
*/
class xsqlite3_pool{
public:
	//! 创建
	xsqlite3_pool();

	//! 析构
	~xsqlite3_pool();

	//! 初始化并启动
	// \return -1,取得内存空间失败;0,取得数据库连接失败;1,成功
	int init(const char * dsn, int maxnum = 6,long maxfreetime = 600, long m_idle = 300, long maxlifetime = 1800);

	void down(void);

	//! 获取连接
	xsqlite3 * open(void);

	//! 归还连接
	/*
	* \param pdb 归还的连接对象
	* \param isok 归还的连接对象是否可用，true/false，可用/不可用
	*/
	void close(xsqlite3 * pdb, bool isok = true);

	//! 管理连接池
	// 供init调用进行线程管理
	static unsigned int schedule(void * sdata);

	const char * get_lastmsg(void)  {  return (const char *)m_lastmsg;  }

protected:
	void set_null(void);

public:
	char m_dsn[256];	/// 数据库连接名(文件名)

	int  m_maxnum;		/// 最大连接数
	long m_maxfreetime;	/// 最大空闲时间(秒)
	long m_maxlifetime; /// 最大生存时间(秒)
	long m_idle;		/// 轮循检测时间(秒)
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
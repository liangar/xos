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
	bool isused;	// 是否在使用，true/false，使用/空闲
	long createtime;
};

/*! \class con_pool
*  连接池类
*/
class con_pool{
public:
	//! 创建
	con_pool();

	//! 析构
	~con_pool();

	//! 初始化并启动
	// \return -1,取得内存空间失败;0,取得数据库连接失败;1,成功
	int init(
		const char * service_name, const char * uid, const char * pwd,
		int maxnum = 6, long maxfreetime = 600, long maxlifetime = 1800, long m_idle = 300
	);
	int init(const char * dsn, int maxnum = 6,long maxfreetime = 600, long maxlifetime=1800, long m_idle = 300);

	void down(void);

	//! 获取连接
	connection * open();

	//! 归还连接
	/*
	* \param con 归还的连接对象
	* \param isok 归还的连接对象是否可用，true/false，可用/不可用
	*/
	void close(connection * con, bool isok = true);

	//! 管理连接池
	// 供init调用进行线程管理
	static unsigned int schedule(void * sdata);

	const char * get_lastmsg(void)  {  return (const char *)m_lastmsg;  }

protected:
	void set_null(void);

public:
	char m_service_name[30];	/// 服务器名
	char m_uid[20];				/// 用户名
	char m_pwd[20];				/// 密码

	int  m_maxnum;			/// 最大连接数
	long m_maxfreetime;		/// 最大空闲时间(秒)
	long m_maxlifetime; /// 最大生存时间(秒)
	long m_idle;			/// 轮循检测时间(秒)
	xsys_event	m_hnotify;

	int m_concount;		/// 当前连接数
	int m_numno;		/// 连接编号

protected:
	xlist<con_control>	m_list;

	xsys_mutex m_mutex;
	xsys_thread m_monitor;
	xsys_event	m_estop;

	char m_lastmsg[1024];
};

#endif
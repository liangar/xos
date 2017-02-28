#ifndef _XWORK_SERVER_H_
#define _XWORK_SERVER_H_

/// \file xwork_server.h
///服务类

#include <xsys.h>
#include <ss_service.h>

/// \class xwork_server
/// 服务类
/// \brief 用线程方式实现服务<br>
/// 该类的设置属性函数不多,服务的属性基本用public公开访问方式进行set/get
class xwork_server
{
public:
	char	m_name[128];		/// 服务名称

public:
	/// 构造
	xwork_server();

	/// 构造
	/// \param name 服务名称,长度不超过128
	/// \param nmaxexception 最多可容忍的异常次数
	/// \param works 服务线程数
	xwork_server(const char * name, int nmaxexception = 5, int works = 1);
	/// 析构
	virtual ~xwork_server();

	/// 设置属性
	/// \param name 服务名称,长度不超过128
	/// \param nmaxexception 最多可容忍的异常次数
	/// \param works 服务线程数
	void set_attributes(const char * name, int nmaxexception = 5, int works = 1);

	// 提供的接口函数,以便外部控制
	/// 启动
	/// \return 成功与否
	virtual bool start(void);
	/// 停止
	/// \brief 如果线程是被强行停止也算一次异常
	/// \param secs 等待时间,如果超时则强行停止
	/// \return 成功与否
	virtual bool stop(int secs = 5);
										
	virtual void run(void) = 0;			/// 工作函数,会被master调用
	bool restart(int secs = 5);			/// 重新启动

	virtual bool open(void);			/// 初始化打开
	virtual bool close(int secs = 5);	/// 关闭释放

	/// 命令通知
	/// \param cmd 通知命令,如"stop"
	/// \param parms 通知执行参数,与自定义命令相关
	void notify(const char * cmd, const char * parms = "");
	virtual void notify_stop(void);		/// 通知停止

	void getinfo(char * msg, int maxlen);	/// 取得工作情况信息

protected:
	void init_props(void);
	static unsigned int master(void * data);

public:
	/// 与上级的通讯区
	int		m_maxexception;		/// 最多可容忍的异常次数
	int		m_exceptiontimes;	/// 实际发生的异常次数
    long	m_heartbeat;		/// 心跳的click,可以判断服务是否在运行
	long    m_run_times;        /// 服务运行的次数

	int		m_works;			/// 服务线程数
	int		m_idle;				/// 正常运行轮循间隔时间

	char	m_createtime[32];	/// 创建时间
	char	m_laststart[32];	/// 最近启动时间
	long	m_laststarttime;	/// 最近启动时间
	int		m_starttimes;		/// 重启次数

    int		m_requests;		/// 接收请求数
    int		m_successes;	/// 正确处理数
    int		m_failes;		/// 失败数
    int		m_waitinges;	/// 等待滞后处理数目

    // 事件命令通知
	xsys_event	m_hnotify;		/// 通知的事件
	char		m_cmd[16];		/// 通知命令: stop,自定义命令
	char		m_parms[256];	/// 通知命令的参数

	char		m_lastmsg[256];	/// 最近消息
	bool	m_isrun;	/// 线程

protected:
	void clear_notify(void);
	int  lock_prop(int seconds);
	void unlock_prop(void);

	xsys_mutex	*m_pprop_lock;
	xsys_thread	*m_phworker;	/// 工作线程
};

#endif // _XWORK_SERVER_H_


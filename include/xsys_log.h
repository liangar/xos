#ifndef _XOS_LOG_H_
#define _XOS_LOG_H_

class xsys_log{
public:
	xsys_log();
	~xsys_log();

	int init(bool b_in_debug, int idle, const char * bakpath);
	int down(void);

	bool open(const char * file_name);
	bool reopen(void);
	int  close(void);
	
	int write(int c);
	int write(const char * s);
	int write_aline(const char * s);

	int flush(void);
protected:
	int bak(void);

protected:
	FILE *	m_hlogfile;				/// 文件句柄
	char	m_file_name[MAX_PATH];	/// 文件名称
	char	m_bak_path[MAX_PATH];	/// 备份目录，如果目录为空则不备份

    bool	m_bdebug;	/// 是否调试运行
    time_t	m_prev_time;/// 上次打印的时间戳
    long	m_idle; 	/// 打印时间戳的间隔
};

void test_xsys_log(char * b);

#endif // _XOS_LOG_H_

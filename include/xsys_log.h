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
	FILE *	m_hlogfile;				/// �ļ����
	char	m_file_name[MAX_PATH];	/// �ļ�����
	char	m_bak_path[MAX_PATH];	/// ����Ŀ¼�����Ŀ¼Ϊ���򲻱���

    bool	m_bdebug;	/// �Ƿ��������
    time_t	m_prev_time;/// �ϴδ�ӡ��ʱ���
    long	m_idle; 	/// ��ӡʱ����ļ��
};

void test_xsys_log(char * b);

#endif // _XOS_LOG_H_

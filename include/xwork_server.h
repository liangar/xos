#ifndef _XWORK_SERVER_H_
#define _XWORK_SERVER_H_

/// \file xwork_server.h
///������

#include <xsys.h>
#include <ss_service.h>

/// \class xwork_server
/// ������
/// \brief ���̷߳�ʽʵ�ַ���<br>
/// ������������Ժ�������,��������Ի�����public�������ʷ�ʽ����set/get
class xwork_server
{
public:
	char	m_name[128];		/// ��������

public:
	/// ����
	xwork_server();

	/// ����
	/// \param name ��������,���Ȳ�����128
	/// \param nmaxexception �������̵��쳣����
	/// \param works �����߳���
	xwork_server(const char * name, int nmaxexception = 5, int works = 1);
	/// ����
	virtual ~xwork_server();

	/// ��������
	/// \param name ��������,���Ȳ�����128
	/// \param nmaxexception �������̵��쳣����
	/// \param works �����߳���
	void set_attributes(const char * name, int nmaxexception = 5, int works = 1);

	// �ṩ�Ľӿں���,�Ա��ⲿ����
	/// ����
	/// \return �ɹ����
	virtual bool start(void);
	/// ֹͣ
	/// \brief ����߳��Ǳ�ǿ��ֹͣҲ��һ���쳣
	/// \param secs �ȴ�ʱ��,�����ʱ��ǿ��ֹͣ
	/// \return �ɹ����
	virtual bool stop(int secs = 5);
										
	virtual void run(void) = 0;			/// ��������,�ᱻmaster����
	bool restart(int secs = 5);			/// ��������

	virtual bool open(void);			/// ��ʼ����
	virtual bool close(int secs = 5);	/// �ر��ͷ�

	/// ����֪ͨ
	/// \param cmd ֪ͨ����,��"stop"
	/// \param parms ִ֪ͨ�в���,���Զ����������
	void notify(const char * cmd, const char * parms = "");
	virtual void notify_stop(void);		/// ֹ֪ͨͣ

	void getinfo(char * msg, int maxlen);	/// ȡ�ù��������Ϣ

protected:
	void init_props(void);
	static unsigned int master(void * data);

public:
	/// ���ϼ���ͨѶ��
	int		m_maxexception;		/// �������̵��쳣����
	int		m_exceptiontimes;	/// ʵ�ʷ������쳣����
    long	m_heartbeat;		/// ������click,�����жϷ����Ƿ�������
	long    m_run_times;        /// �������еĴ���

	int		m_works;			/// �����߳���
	int		m_idle;				/// ����������ѭ���ʱ��

	char	m_createtime[32];	/// ����ʱ��
	char	m_laststart[32];	/// �������ʱ��
	long	m_laststarttime;	/// �������ʱ��
	int		m_starttimes;		/// ��������

    int		m_requests;		/// ����������
    int		m_successes;	/// ��ȷ������
    int		m_failes;		/// ʧ����
    int		m_waitinges;	/// �ȴ��ͺ�����Ŀ

    // �¼�����֪ͨ
	xsys_event	m_hnotify;		/// ֪ͨ���¼�
	char		m_cmd[16];		/// ֪ͨ����: stop,�Զ�������
	char		m_parms[256];	/// ֪ͨ����Ĳ���

	char		m_lastmsg[256];	/// �����Ϣ
	bool	m_isrun;	/// �߳�

protected:
	void clear_notify(void);
	int  lock_prop(int seconds);
	void unlock_prop(void);

	xsys_mutex	*m_pprop_lock;
	xsys_thread	*m_phworker;	/// �����߳�
};

#endif // _XWORK_SERVER_H_


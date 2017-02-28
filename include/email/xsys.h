/*! \file xsys.h
 *  �����ϵͳ��صĵײ���<br>
 *  ��Ҫ�ӿ�Դ��ĿXMail�г��<br>
 *  �������¼�,�źŵ�,�ٽ���,�߳̿���,��̬�����,socket������<br>
 *  �Գ��õ�socket����,����һЩ����<br>
 *  �����漰�ĳ�ʱʱ������������
 */

#ifndef _XOS_H_
#define _XOS_H_

#include "xmailsys/SysInclude.h"
#include "xmailsys/SysDep.h"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif // MAX_PATH

#ifndef MAX_HOST_NAME
#define MAX_HOST_NAME	128
#endif // MAX_HOST_NAME

int  xsys_init(bool bServerDebug = false);
void xsys_down(void);

#define xsys_cleanup    SysCleanupLibrary
#define xsys_shutdown   SysShutdownLibrary

/*! \class xsys_event
 *  �¼���
 */
class xsys_event
{
public:
	xsys_event();
	~xsys_event()  {  down();  }

	//! ��ʼ������
	/*!
	 * \param iManualReset �Ƿ��ֹ�����,0/else=��/��
	 * \return 0/else=�ɹ�/ʧ��
	 */
	int  init(int iManualReset);
	void down(void);	/*!< �رպ���. */

	//! �ȴ�
	/*!
	 * \param seconds ��ʱ����,���<=0,��Ϊ����
	 * \return 0/ERR_TIMEOUT=���¼�/��ʱ
	 */
	int wait(int seconds);
	int set(void);		/*!< �������¼�. */
	int reset(void);	/*!< ����. */
	int trywait(void);	/*!< �����Ƿ���Եȴ�. */

protected:
	SYS_EVENT	m_event;
};

/*! \class xsys_semaphore
 *  �ź�����
 */
class xsys_semaphore
{
public:
	xsys_semaphore();
	~xsys_semaphore()  {  down();  }

	//! ��ʼ������
	/*!
	 * \param iInitCount ��ʼ�ź���
	 * \param iMaxCount  ����ź���
	 * \return 0/else=�ɹ�/ʧ��
	 */
	int  init(int iInitCount, int iMaxCount);
	void down(void);			/*!< �رպ���. */

	//! �ȴ��ź�
	/*!
	 * \param seconds ��ʱ����,���<=0,��Ϊ����
	 * \return 0/ERR_TIMEOUT=���¼�/��ʱ
	 */
	int  P(int seconds);
	int  V(int iCount = 1);		/*!< �ź����� */

	int  trywait(int seconds);	/*!< �����Ƿ���Եȴ� */

protected:
	SYS_SEMAPHORE m_semaphore;	/*!< �ź��� */
};

/*! \class xsys_mutex
 *  �ٽ�����
 */
class xsys_mutex
{
public:
	xsys_mutex();
	~xsys_mutex()  {  down();  }

	int  init(void);		/*!< ��ʼ�� */
	void down(void);		/*!< �ر� */

	//! ����
	/*!
	 * \param seconds ��ʱ����,���<=0,��Ϊ����
	 * \return 0/ERR_TIMEOUT=���¼�/��ʱ
	 */
	int  lock(int seconds);
	int  unlock(void);		/*!< ���� */
	int  trylock(void);		/*!< �����Ƿ���Լ��� */

protected:
	SYS_MUTEX	m_mutex;	/*!< �ٽ��� */
};

/*! \class xsys_thread
 *  �߳���
 */
class xsys_thread
{
public:
	//! ����
	xsys_thread();
	//! ����
	~xsys_thread()  {  down();  }

	//! ����
	/*! \param pThreadProc �߳�ִ�к���
	 *  \param pThreadData ִ�в���
	 *  \return 0/else=�ɹ�/ʧ��
	 */
	int init(unsigned int (*pThreadProc) (void *), void *pThreadData);
	//! �ر�
	/*! \param seconds �ȴ�����,ȱʡ3
	 *  \param iForce �Ƿ�ǿ�йر�,ȱʡ��
	 *  \return 0/1/-1=�ɹ�/ǿ�йر�/ʧ��
	 */
	int down(int seconds = 3, int iForce = 1);

	//! �ȴ��߳̽���
	/*!
	 * \param seconds ��ʱ����,���<=0,��Ϊ����
	 * \return 0/ERR_TIMEOUT=����/��ʱ
	 */
	int  wait(int seconds = 3);

protected:
	SYS_THREAD	m_thread;	/*!< �߳� */
};

/*! \class xsys_module
 *  ��̬����
 */
class xsys_module
{
public:
	xsys_module();
	~xsys_module()  {  down();  }

	int  load(const char * pszFilePath);
	void down(void);	/*!<  */

	void * get_symbol(const char * pszSymbol);

protected:
	SYS_HANDLE	m_h;	/*!< ��̬���� */
};

/*! \class xsys_ls
 *  �ļ�Ŀ¼�б���
 */
class xsys_ls
{
public:
	xsys_ls();
	~xsys_ls();

	/// ���б�ȡ�õ�һ���ļ���
	/// \param pszPath �ļ�Ŀ¼
	/// \param pszFileName ȡ�õ��ļ���
	/// \return true/false = �ɹ�/ʧ��
	bool open(const char *pszPath, char *pszFileName);
	int  ispath(void);	/// ��ǰ�ļ��Ƿ���Ŀ¼
	unsigned long get_size(void);	/// ��ǰ�ļ���С

	/// ȡ����һ���ļ�����
	/// \param filename ȡ�õ��ļ���
	/// \return true/false = ��ȡ���ļ�/û���ļ�
	bool next(char * filename);

	void close(void);	/*!< �ر��б� */

protected:
	SYS_HANDLE m_h;		/*!< �б��� */
};

/*! \class xsys_socket
 *  TCP socket ������<br>
 *  ���еķ��ͽ��պ��������г�ʱʱ�����õ�<br>
 *  ��ʱʱ�������
 */
class xsys_socket
{
public:
	xsys_socket();
	~xsys_socket();

	/// ����
	int listen(int nportnumber, int iconnections = 1);

	/// �������
	int accept(xsys_socket & client_sock);

	/// �ͻ�������
	/// \param lphostname ��ַ
	/// \param nportnumber �˿�
	/// \param timeout ��ʱʱ��(��)
	/// \return 0/else 0/������
	int connect(const char * lphostname, int nportnumber = 0, int timeout = 30);

	void close(void);	/*!< �ر� */
	int isopen(void);	/*!< �Ƿ�� */

	//! ����
	/*!
	 * \param buf ���ջ�����
	 * \param l ϣ�����յ��ֽ���
	 * \param timeout ��ʱ����,���<0,��Ϊ����
	 * \return >=0/<0 ���յ��ֽ���/������<br>
	 *        ����������ֽ��Ϳ���ͨ��xgeterror_string����ȡ��
	 */
	int recv(char * buf, int l, int timeout = SYS_INFINITE_TIMEOUT);

	//! ����
	/*!
	 * \param buf ���ͻ�����
	 * \param l ϣ�����͵��ֽ���
	 * \param timeout ��ʱ����,���<=0,��Ϊ����
	 * \return =0/<0 �ɹ�/������<br>
	 *        ����������ֽ��Ϳ���ͨ��xgeterror_string����ȡ��
	 */
	int send(const char * buf, int l, int timeout = 10);

	/// ���ղ���������
	/// \brief ����,ֱ������,����,���ߵõ�������Ϊֹ<br>
	/// ���������,û�е��������<br>
	/// ���20�������ַ�����,Ҳ����
	/// \param buf ���ջ�����
	/// \param maxlen ���ջ�����ֽ���
	/// \param endstr ������
	/// \return >=0/<0  ���յ��ֽ���/ʧ��
	int recv_byend(char * buf, int maxlen, char * endstr);

	/// ����ֱ������������
	/// \brief ����,ֱ������,���ڴ����,���ߵõ�������Ϊֹ<br>
	/// �ڴ治���ٷ���������,û�е��������<br>
	/// ���20�������ַ�����,Ҳ����<br>
	/// ���,�ڴ�Ӧ����free�ͷ�,����new
	/// \param p ���ջ�����ָ��
	/// \param endstr ������
	/// \param steplen �ڴ���������
	/// \return >=0/<0  ���յ��ֽ���/ʧ��
	int recv_byend(char ** p, const char * endstr, int steplen = 2048);

	/// ����ȫ��
	/// \param buf ���ջ�����
	/// \param len ����յ��ֽ���
	/// \param timeout ��ʱ����,���<=0,��Ϊ����
	/// \return len/<0 ��ȷ/ʧ��
	int recv_all(char * buf, int len, int timeout = SYS_INFINITE_TIMEOUT);
	int send_all(const char * buf, int len, int timeout = 30); /*!< ����ȫ�� */
	int send_all(const char * buf);	/// �����ַ���

	/// ���ڲ�Э��鷢�ͽ���
	/// ��Ľṹ:<%010d ����><���ݿ�>
	int recvblob(char * buf, int timeout = SYS_INFINITE_TIMEOUT);
	int recvblob(char **buf, int timeout = SYS_INFINITE_TIMEOUT);
	int sendblob(const char * buf, int len = 0, int timeout = 30);

	int m_isserver;		/*!< �Ƿ��Ƿ����� */
	SYS_SOCKET	m_sock;	/*!< socket��� */
};

#define xsys_set_breaker   SysSetBreakHandler
#define xsys_exec          SysExec

#define xsys_event_logv    SysEventLogV
#define xsys_event_log     SysEventLog
#define xsys_log_message   SysLogMessage

#define xsys_sleep         SysSleep
#define xsys_sleep_ms      SysMsSleep

inline long xsys_get_today_ms_time(void)  {  return long(SysMsTime());  }

#define xsys_file_info     SysGetFileInfo
#define xsys_file_modtime  SysSetFileModTime

#define xsys_exist_file    SysExistFile
#define xsys_exist_path    SysExistDir
#define xsys_md            SysMakeDir
#define xsys_rm            SysRemove
#define xsys_rm_path       SysRemoveDir
#define xsys_move_file     SysMoveFile

/// ȡ�ļ���С
/// \param fullname �ļ�ȫ·������
/// \return >=0/else �ļ���С/������
int xsys_file_size(const char * fullname);

/// �����ļ�
/// \param new_name ���ļ�����(ȫ��)
/// \param old_name Դ�ļ�����(ȫ��)
/// \return 0/errorcode = ��ȷ/�������
int xsys_cp(const char * new_name, const char * old_name);
int xsys_cp(const char * newpath, const char * oldpath, const char * filename);
int xsys_mv(const char * newpath, const char * oldpath, const char * filename);
int xsys_rm(const char * path, const char * filename);

int xsnprintf(char * d, int l, const char * formatstring, ...);

//! ȡ�����ϵͳ�����ַ���
/*!
 * \return ���س����ַ���
 */
char const * xlasterror(void);
const char * xgeterror_string(int ierror);

/// ����Ŀ¼
/// \brief ��Ŀ¼�������򴴽�,�����ƻ����е�Ŀ¼�ļ�
/// \param path Ҫ������Ŀ¼
/// \param isfile path�����Ƿ���һ���ļ�
/// \return true/false = �ɹ�/ʧ��
bool xsys_md(const char * path, bool isfile);

/// ������ת���ַ���
const char * xsys_ltoa(long l);

#ifndef WIN32
/// �����е�Сд��ĸת��Ϊ��д��ĸ
	char *strupr(char *str); 
#endif

int xsys_do_cmd(const char * cmd, const char * workpath, char * outbuf, int len);

#endif // _XOS_H_


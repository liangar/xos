/*! \file xsys_ftp.h
 *  Unix�µ�FTP������<br>
 *  ������HP/AIX������,����FtpLibrary<br>
 *  ����FTPҲ���Է�װ��������,��չ��windows/linux/sco��ϵͳ
 *  �Գ��õ�socket����,����һЩ����
 */

#ifndef _XSYS_FTP_H_
#define _XSYS_FTP_H_

typedef void * ftp_handle;
typedef void * ftp_hls;

class xsys_ftp
{
public:
	// 0. ��������������
	xsys_ftp()
		: m_hftp(0)
		, m_hls(0)
	{
	}
	~xsys_ftp(){
		close();
	}

	// 1. ����/�ر�ftp���Ӻ���
	long open(char * url, char * uid, char * pwd);	/*!< ������. */
	void close(void);	/*!< �ر�����. */

	// 2. �ļ�����

	long put(char * LocalFileName, char * RemoteFileName);	/*!< �ϴ��ļ� */
	long get(char * RemoteFileName,char * LocalFileName);	/*!< �����ļ� */
	long del(char * RemoteFileName);		/*!< ɾ���ļ� */

	long rename(char * ResourceName, char * DestName);	/*!< �ƶ��ļ� */

	// 4. Ŀ¼/����

	long cd(char * RemotePath);

	long ls(char * FileName);	/*!< ȡ�õ�һ���ļ���ַ */
	long isfile(void);			/*!< �жϵ�ǰ�ļ��Ƿ����ļ� 1/0 = yes/no */
	long file_size(void);
	char file_type(void);

	long next(char * FileName);	/*!< ȡ���б��е�ǰ���ļ�����,������ǰ��ָ���Ƶ���һ���ļ� */
	void ls_close(void);	/*!< �˳������ļ� */

	int lasterror(void);

protected:
	ftp_handle	m_hftp;
	ftp_hls		m_hls;

	char m_line[256];
	void * m_p;
};

#endif // _XSYS_FTP_H_


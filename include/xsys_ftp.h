/*! \file xsys_ftp.h
 *  Unix下的FTP操作类<br>
 *  可以在HP/AIX上运行,基于FtpLibrary<br>
 *  其他FTP也可以封装这样的类,扩展到windows/linux/sco等系统
 *  对常用的socket操作,加了一些方法
 */

#ifndef _XSYS_FTP_H_
#define _XSYS_FTP_H_

typedef void * ftp_handle;
typedef void * ftp_hls;

class xsys_ftp
{
public:
	// 0. 创建和析构函数
	xsys_ftp()
		: m_hftp(0)
		, m_hls(0)
	{
	}
	~xsys_ftp(){
		close();
	}

	// 1. 建立/关闭ftp连接函数
	long open(char * url, char * uid, char * pwd);	/*!< 打开连接. */
	void close(void);	/*!< 关闭连接. */

	// 2. 文件操作

	long put(char * LocalFileName, char * RemoteFileName);	/*!< 上传文件 */
	long get(char * RemoteFileName,char * LocalFileName);	/*!< 下载文件 */
	long del(char * RemoteFileName);		/*!< 删除文件 */

	long rename(char * ResourceName, char * DestName);	/*!< 移动文件 */

	// 4. 目录/查找

	long cd(char * RemotePath);

	long ls(char * FileName);	/*!< 取得第一个文件地址 */
	long isfile(void);			/*!< 判断当前文件是否是文件 1/0 = yes/no */
	long file_size(void);
	char file_type(void);

	long next(char * FileName);	/*!< 取得列表中当前的文件名称,并将当前的指针移到下一个文件 */
	void ls_close(void);	/*!< 退出查找文件 */

	int lasterror(void);

protected:
	ftp_handle	m_hftp;
	ftp_hls		m_hls;

	char m_line[256];
	void * m_p;
};

#endif // _XSYS_FTP_H_


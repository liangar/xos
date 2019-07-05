#ifndef _XINI_H_
#define _XINI_H_

/*! \class xini
 *  INI环境处理类
 */
class xini {
public:
	xini();
	xini(const char * filename);

	//! 打开文件
	/*!
	 * \param filename 文件名称<br>
	 * 如果没有说明路径,会用ss_service.h中的getfullpath来取得路径
	 * \return 0/else=成功/失败
	 */
	int open(const char * filename);

	long get(const char * section, const char * keyname, long defaultvalue = 0);
	// return bytes
	int  get(const char * section, const char * keyname, char * value, int maxlen = 128, const char * defaultvalue = "");
	long set(const char * section, const char * keyname, long value);
	const char * set(const char * section, const char * keyname, const char * value);

	const char * getpath()  {  return (const char *)m_path;  }	/*!< 取得INI文件所在的目录 */
	const char * getfile()  {  return (const char *)m_file;  }	/*!< 取得INI文件名(完全) */

	void getpath(const char * section, const char * key, char * v, int vsize);	/*!< 目录,会去掉字符串最后的目录符号 */

protected:
	void formatpath(char * path);

protected:
	char m_file[MAX_PATH];
	char m_path[MAX_PATH];
};

#endif // _XINI_H_


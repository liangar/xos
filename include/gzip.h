#ifndef _XSYS_GZIP_H_
#define _XSYS_GZIP_H_

#include <xsys.h>
#include <xlist.h>

#define MAX_CMD_LEN 1024
#define ZIP_FILE_NUM 16

struct xsys_zipfile{
	char filename[64];			/// 文件名
	char fullname[MAX_PATH];	/// 文件全名,包括目录
	int	 state;					/// 状态: 0/1 正确/错误
};

/// \class xsys_gzip
/// 打包解压类
class xsys_gzip{
public:
	xsys_gzip();
	~xsys_gzip();

	/// 设置zip的工作目录
	/// \brief 函数检查目录的存在性，如果不存在就创建,返回结果
	/// \param path 工作目录
	/// \return true/false = 成功/失败
	bool set_zip_path(const char * path);

	/// 设置包类型
	/// \brief 在解压时,ziptype作为参考,解压本身应能自动判断类型
	/// \param ziptype 包类型
	void set_type(const char * ziptype);

	/// 增加文件
	/// \param filename 文件名
	/// \param fullname 文件全名
	/// \return true/false = 成功/失败
	/// \brief 成功失败的消息可以通过lastmsg方法取得<br>
	/// 本函数会检查文件是否可以正确访问,并返回结果
	bool add(const char * filename, const char * fullname);

	/// 打包文件
	/// \param zipfile zip文件名称，只需要文件名，不要带扩展名，生成zipfile.tar.gz文件
	/// \return >=0/else = 打包的文件个数/失败
	int  zip(char * resultname, const char * zipfile);

	/// 解压文件
	/// \param zipfile zip文件名称, 必须是.tar.gz文件
	/// \param temp_path 解压的工作目录, 如果解压到m_zip_path中，该变量为NULL
	/// \param ignore_ext 是否忽略最后的扩展名后缀
	/// \return >=0/else = 解压的文件个数/失败
	int  unzip(const char * zipfile, const char * temp_path, bool ignore_ext = false);


	/// 取得工作消息
	const char * lastmsg(void)  {  return m_lastmsg;  }
	const char * get_zip_path(void)  {  return m_zip_path;  }

protected:
	char	m_zip_type ;
	char	m_type[32];				/// 包类型
	char	m_lastmsg[1024*4];		/// 工作消息
	char	m_zip_path[MAX_PATH];	/// 工作目录
	short	m_zip_path_len;
	char	m_cmd[MAX_CMD_LEN];

public:
	xlist<xsys_zipfile> m_files;	/// 当前包中包含的文件及状态
};

#endif // _XSYS_GZIP_H_

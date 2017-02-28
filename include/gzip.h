#ifndef _XSYS_GZIP_H_
#define _XSYS_GZIP_H_

#include <xsys.h>
#include <xlist.h>

#define MAX_CMD_LEN 1024
#define ZIP_FILE_NUM 16

struct xsys_zipfile{
	char filename[64];			/// �ļ���
	char fullname[MAX_PATH];	/// �ļ�ȫ��,����Ŀ¼
	int	 state;					/// ״̬: 0/1 ��ȷ/����
};

/// \class xsys_gzip
/// �����ѹ��
class xsys_gzip{
public:
	xsys_gzip();
	~xsys_gzip();

	/// ����zip�Ĺ���Ŀ¼
	/// \brief �������Ŀ¼�Ĵ����ԣ���������ھʹ���,���ؽ��
	/// \param path ����Ŀ¼
	/// \return true/false = �ɹ�/ʧ��
	bool set_zip_path(const char * path);

	/// ���ð�����
	/// \brief �ڽ�ѹʱ,ziptype��Ϊ�ο�,��ѹ����Ӧ���Զ��ж�����
	/// \param ziptype ������
	void set_type(const char * ziptype);

	/// �����ļ�
	/// \param filename �ļ���
	/// \param fullname �ļ�ȫ��
	/// \return true/false = �ɹ�/ʧ��
	/// \brief �ɹ�ʧ�ܵ���Ϣ����ͨ��lastmsg����ȡ��<br>
	/// �����������ļ��Ƿ������ȷ����,�����ؽ��
	bool add(const char * filename, const char * fullname);

	/// ����ļ�
	/// \param zipfile zip�ļ����ƣ�ֻ��Ҫ�ļ�������Ҫ����չ��������zipfile.tar.gz�ļ�
	/// \return >=0/else = ������ļ�����/ʧ��
	int  zip(char * resultname, const char * zipfile);

	/// ��ѹ�ļ�
	/// \param zipfile zip�ļ�����, ������.tar.gz�ļ�
	/// \param temp_path ��ѹ�Ĺ���Ŀ¼, �����ѹ��m_zip_path�У��ñ���ΪNULL
	/// \param ignore_ext �Ƿ����������չ����׺
	/// \return >=0/else = ��ѹ���ļ�����/ʧ��
	int  unzip(const char * zipfile, const char * temp_path, bool ignore_ext = false);


	/// ȡ�ù�����Ϣ
	const char * lastmsg(void)  {  return m_lastmsg;  }
	const char * get_zip_path(void)  {  return m_zip_path;  }

protected:
	char	m_zip_type ;
	char	m_type[32];				/// ������
	char	m_lastmsg[1024*4];		/// ������Ϣ
	char	m_zip_path[MAX_PATH];	/// ����Ŀ¼
	short	m_zip_path_len;
	char	m_cmd[MAX_CMD_LEN];

public:
	xlist<xsys_zipfile> m_files;	/// ��ǰ���а������ļ���״̬
};

#endif // _XSYS_GZIP_H_

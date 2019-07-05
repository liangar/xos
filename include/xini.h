#ifndef _XINI_H_
#define _XINI_H_

/*! \class xini
 *  INI����������
 */
class xini {
public:
	xini();
	xini(const char * filename);

	//! ���ļ�
	/*!
	 * \param filename �ļ�����<br>
	 * ���û��˵��·��,����ss_service.h�е�getfullpath��ȡ��·��
	 * \return 0/else=�ɹ�/ʧ��
	 */
	int open(const char * filename);

	long get(const char * section, const char * keyname, long defaultvalue = 0);
	// return bytes
	int  get(const char * section, const char * keyname, char * value, int maxlen = 128, const char * defaultvalue = "");
	long set(const char * section, const char * keyname, long value);
	const char * set(const char * section, const char * keyname, const char * value);

	const char * getpath()  {  return (const char *)m_path;  }	/*!< ȡ��INI�ļ����ڵ�Ŀ¼ */
	const char * getfile()  {  return (const char *)m_file;  }	/*!< ȡ��INI�ļ���(��ȫ) */

	void getpath(const char * section, const char * key, char * v, int vsize);	/*!< Ŀ¼,��ȥ���ַ�������Ŀ¼���� */

protected:
	void formatpath(char * path);

protected:
	char m_file[MAX_PATH];
	char m_path[MAX_PATH];
};

#endif // _XINI_H_


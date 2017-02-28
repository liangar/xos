#ifndef _XNAME_VALUE_H_
#define _XNAME_VALUE_H_

#include <xdatatype.h>
#include "l_str.h"
#include "xlist.h"

/*! \class name_value
 *  ����ֵ����<br>
 *  ��ʽΪ name <comment> = value
 */
class name_value
{
public:
	struct s_name_value{
		char * name;	/*!< ���� */
		char * value;	/*!< ֵ */
		char * comment;	/*!< ���� */
		int  len;		/*!< ���� */
	};

public:
	name_value();			/*!< ���� */
	virtual ~name_value();	/*!< ���� */

	int open(const char * s);/*!< �����ַ���Ϊ����ֵ�� */
	int close();			/*!< �ͷŹر� */

public:
	int getbuffer(char * d, int len);	/*!< ȡ������ֵ�Ե�������,ÿ������ֵ��ռ1��,����ֵ������("|')���� */
	int getbuffer(char **d);			/*!< ȡ������ֵ�Ե�������,�Զ����仺���� */

	int count()  {  return m_values.m_count;  }	/*!< ����ֵ�Եļ�¼���� */
	s_name_value & operator[] (int i)  {  return m_values[i];  }	/*!< ȡ�õ�i������ֵ�� */

	int createvalue(const char *name, const void *pv, int type = XPARM_STR, const char * pcomment = 0);	/*!< ��������ֵ */
	int putvalue(const void * pv, const char * name, int type = XPARM_STR);	/*!< ��������ֵ */
	int getvalue(void * pv, const char * name, int type = XPARM_STR);	/*!< ȡ��ĳ���Ƶ�ֵ,ȱʡֵΪ�ַ������� */
	int getvalue(void **pv, const char * name, int type = XPARM_STR);	/*!< ȡ��ĳ���Ƶ�ֵ,ȱʡֵΪ�ַ�������,�Զ�����ռ� */
	char * getvalue(const char * name);			/*!< ȡ��ĳ���Ƶ�ֵ,���򷵻�0 */

	int getcomment(void *pv, const char *name);	/*!< ȡ���� */

	
	int merge(const char * s);					/*!< ��ĳ�ַ������ݲ��� */
	int merge(name_value * p);					/*!< ����ĳ������ֵ��ʵ�� */

	void gohead(void);		/*!< ����¼����ʼ */
	int getnext(void *pv, char *name, char * comment, int type = XPARM_STR);	/*!< ȡ�õ�ǰ,������1����¼ */
	int getnext(const char **pv, const char **name, const char **comment = 0);	/*!< ȡ�õ�ǰ,������1����¼ */

	s_name_value * findvalue(const char * name);	/*!< �Ҽ�¼ */

public:

	xlist<s_name_value> m_values;
	int m_i;
};

#endif // _XNAME_VALUE_H_


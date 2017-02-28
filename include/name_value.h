#ifndef _XNAME_VALUE_H_
#define _XNAME_VALUE_H_

#include <xdatatype.h>
#include "l_str.h"
#include "xlist.h"

/*! \class name_value
 *  名字值对类<br>
 *  格式为 name <comment> = value
 */
class name_value
{
public:
	struct s_name_value{
		char * name;	/*!< 名称 */
		char * value;	/*!< 值 */
		char * comment;	/*!< 名称 */
		int  len;		/*!< 名称 */
	};

public:
	name_value();			/*!< 构造 */
	virtual ~name_value();	/*!< 析构 */

	int open(const char * s);/*!< 解析字符串为名字值对 */
	int close();			/*!< 释放关闭 */

public:
	int getbuffer(char * d, int len);	/*!< 取得名字值对到缓冲区,每个名字值对占1行,除非值用引号("|')括起 */
	int getbuffer(char **d);			/*!< 取得名字值对到缓冲区,自动分配缓冲区 */

	int count()  {  return m_values.m_count;  }	/*!< 名字值对的记录个数 */
	s_name_value & operator[] (int i)  {  return m_values[i];  }	/*!< 取得第i个名字值对 */

	int createvalue(const char *name, const void *pv, int type = XPARM_STR, const char * pcomment = 0);	/*!< 加入名字值 */
	int putvalue(const void * pv, const char * name, int type = XPARM_STR);	/*!< 加入名字值 */
	int getvalue(void * pv, const char * name, int type = XPARM_STR);	/*!< 取得某名称的值,缺省值为字符串类型 */
	int getvalue(void **pv, const char * name, int type = XPARM_STR);	/*!< 取得某名称的值,缺省值为字符串类型,自动分配空间 */
	char * getvalue(const char * name);			/*!< 取得某名称的值,无则返回0 */

	int getcomment(void *pv, const char *name);	/*!< 取解释 */

	
	int merge(const char * s);					/*!< 将某字符串内容并入 */
	int merge(name_value * p);					/*!< 并入某个名字值对实例 */

	void gohead(void);		/*!< 到记录集开始 */
	int getnext(void *pv, char *name, char * comment, int type = XPARM_STR);	/*!< 取得当前,并下移1条记录 */
	int getnext(const char **pv, const char **name, const char **comment = 0);	/*!< 取得当前,并下移1条记录 */

	s_name_value * findvalue(const char * name);	/*!< 找记录 */

public:

	xlist<s_name_value> m_values;
	int m_i;
};

#endif // _XNAME_VALUE_H_


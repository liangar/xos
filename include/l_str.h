#ifndef _L_L_STR_H
#define _L_L_STR_H

#ifdef WIN32
#include <wchar.h>
#endif

#define ISBLANKCH(c)  (c==' '||c=='\t'||c=='\xd'||c=='\xa'||c=='\x8'||c=='\x1b')

/*! \file xsys.h
 * \pre
 * goal   : �ṩһЩc string�����Ĺ���
 *			����,���е�dΪĿ���ַ,sΪԴ��ַ,d==sʱ,ִ��Ҳ��ȷ�Է������
 *
 * fnctns : getaline ��һ��,�����к��λ��
 *			getaword ��һ������,�����趨�����ַ�,�ָ�������,����������
 *					 �������������Χ�Ƶ��ʵķָ���,����һ���������������ַ���ֹͣ
 *					 �����´�ɨ����ַ���ַ
 *			skipchs  �����ַ���,�����´�ɨ����ַ���ַ
 *			endcmp   �Ƚ�e�Ƿ���d�Ľ�β,����ֵ��strcmp
 *			trimstr  ��d�еĿո�ѹ��,ȥ֮ͷβ�ո�,���ؽ���ַ����ĳ���
 *			padr     ��s����len�����ұ߼���ո�,ʹ֮����Ϊlen,����d��
 *			bcdtoint ��bcd��ת����intֵ
 *			strmove  ����d,s��ַ�ص���strcpy
 *			strreplace d��s0�ĵط�����s1
 *          strinsert  s����d�Ŀ�ʼ����,lΪ�涨�ĳ���,��<0��Ϊs������
 *			countlines ͳ���ַ����а���������,���Զ����зָ���
 *			countwords ͳ���ַ����еĵ�����,���Զ��嵥�ʵķָ�������
 *
 *			splitstr  ���ַ����зָ����滻��'\0'�γ��ַ�������
 */

extern const char * L_BLANKS;

const char *  getaline(char * d, const char * s);				/*!< ȡ1�� */
const char *  getaline(char * d, const char * s, char c);		/*!< ȡ1��,ָ���ָ��� */
const char *  getaword(char * d, const char * s, char c = '|');	/*!< ȡ1����,ָ���ָ���,������һ�����ʵ���ʼ��ַ */
const char *  getaword(char * d, const char * s, const char * seps);	/*!< ȡ1����,ָ���ָ�����,������һ�����ʵ���ʼ��ַ */
const char *  getaword(char * d, const char * s, const char * seps, const char * ends);	/*!< ȡ1����,ָ���ָ������ͽ���������,�����¸����ʻ�������ĵ�ַ */
const char *  getaword(char * d, const char * s, const char *p, int maxlen); /*!< ȡ1����,ָ���ָ�����,������һ�����ʵ���ʼ��ַ */

const char *  skipchs (const char * s, const char * chars);	/*!< ����ָ���ַ��� */
const char *  skip2chs(const char * s, const char * chars); /*!< ����ָ���ַ��� */
const char *  skip2ch (const char * s, char ch);
const char *  skipblanks(const char * s);				/*!< �����ո� */

int     endcmp    (const char * d, const char * e);		/*!< �Ƚ�d��ĩβ�Ƿ���e,���ͬstrcmp */

int     trimstr   (char * d);				/*!< ȥ���ո� */
int     trimstr   (char * d, char * s);		/*!< ȥ���ո�,ָ����Ż����� */
int		trim_head (char * d, char * s);		/*!< ȥ���׿ո�,ָ����Ż����� */
int		trim_head0(char * d);				/*!< ȥ��ǰ���0�ַ� */
int		trim_tail (char * d, char * s);		/*!< ȥ��β�ո�,ָ����Ż����� */
int		trim_all  (char * d, char * s);		/*!< ȥ�����пո�,�ϲ�����ո�,ָ����Ż����� */
int		trim_all  (char * d, char * s, char * blanks);		/*!< ȥ���ո�,ָ���ո��ַ��� */
int		trim_all  (char * d, char * s, char blank);			/*!< ȥ���ո�,ָ���ո��ַ� */
int		trim_all  (char * d);				/*!< ȥ���ո�,�ϲ�����ո� */

int     padr      (char * d, char * s, int len);	/*!< �Ҷ������ո� */
char *  ht_lpad0  (char * d, int l);
char *  ht_lpad   (char * d, int l);
int     bcdtoint  (char * d, int len);		/*!< תbcd�ַ���Ϊ���� */
int		strmove   (char * d, char * s);		/*!< �ƶ��ַ��� */
int		strreplace(char * d, char * s0, char * s1);	/*!< ���ַ���d�е�s0������s1��� */
int		strreplace(char * s0, int l0, char * s1, int l1);	/*!< ���ַ���d�е�s0������s1���,ָ��s1���� */
int		strinsert (char * d, char * s, int l);	/*!< s����d�Ŀ�ʼ����,lΪ�涨�ĳ���,��<0��Ϊs������ */

/// ת���Զ�����
/// @param d : Ŀ�ĵ�ַ
/// @param s : Դ��ַ
/// @param rowlen : �г���
/// @param rows : ��������
/// @param startpos : ��ʼλ��
/// @return ʵ������
int 	strwordwrap(char *d, const char * s, int rowlen, int rows, int startpos);

int		countlines(const char * s, char c);		/*!< ͳ���ַ����а��������� */
int		countwords(const char * s, const char * seps);	/*!< ͳ���ַ����а��������� */

int		splitstr(char * d, char splitchar); /*!< ���ַ����зָ����滻��'\0'�γ��ַ�������,�����ַ�����Ŀ */ 
int		splitstr(char * d, char * splitstr);/*!< ���ַ����зָ����滻��'\0'�γ��ַ�������,�����ַ�����Ŀ */

/*
 * compare a string to a mask
 * mask characters:
 *	@ - uppercase letter
 *	$ - lowercase letter
 *	& - hex digit
 *	# - digit
 *	* - swallow remaining characters
 *	^ - letter or digit
 *	else - any other character
 */
bool checkmask(const char * data, const char * mask);

int isdigits(const char * s);

int mbs2wcs(wchar_t * d, const char * s, int l);
int wcs2mbs(char * d, const wchar_t * s, int l);

int c2string(char * d, const char * s);
int string2c(char * d, const char * s);

/// �� str_array ���� sfind
/// str_array ���� 2 �� \0 �������ַ�������
const char * strin(const char * str_array, const char * sfind);
const char * striin(const char * str_array, const char * sfind);

const char *  skip0s(const char * s);
int compare_number(const char * s0, const char * s1);

#endif // _L_L_STR_H

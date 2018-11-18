#ifndef _L_L_STR_H
#define _L_L_STR_H

#ifdef WIN32
#include <wchar.h>
#endif

#define ISBLANKCH(c)  (c==' '||c=='\t'||c=='\xd'||c=='\xa'||c=='\x8'||c=='\x1b')

/*! \file xsys.h
 * \pre
 * goal   : 提供一些c string操作的功能
 *			其中,所有的d为目标地址,s为源地址,d==s时,执行也正确以方便程序
 *
 * fnctns : getaline 读一行,返回行后的位置
 *			getaword 读一个单词,可以设定结束字符,分隔符集合,结束符集合
 *					 程序会跳过所有围绕单词的分隔符,在下一个结束符或其他字符上停止
 *					 返回下次扫描的字符地址
 *			skipchs  眺过字符集,返回下次扫描的字符地址
 *			endcmp   比较e是否在d的结尾,返回值如strcmp
 *			trimstr  将d中的空格压缩,去之头尾空格,返回结果字符串的长度
 *			padr     将s不足len长的右边加入空格,使之长度为len,放入d中
 *			bcdtoint 将bcd串转换成int值
 *			strmove  允许d,s地址重叠的strcpy
 *			strreplace d中s0的地方换成s1
 *          strinsert  s插入d的开始部分,l为规定的长度,如<0则为s整个串
 *			countlines 统计字符串中包含的行数,可以定义行分隔符
 *			countwords 统计字符串中的单词数,可以定义单词的分隔符集合
 *
 *			splitstr  将字符串中分隔符替换成'\0'形成字符串集合
 */

extern const char * L_BLANKS;

const char *  getaline(char * d, const char * s);				/*!< 取1行 */
const char *  getaline(char * d, const char * s, char c);		/*!< 取1行,指定分隔符 */
const char *  getaword(char * d, const char * s, char c = '|');	/*!< 取1单词,指定分隔符,返回下一个单词的起始地址 */
const char *  getaword(char * d, const char * s, const char * seps);	/*!< 取1单词,指定分隔符集,返回下一个单词的起始地址 */
const char *  getaword(char * d, const char * s, const char * seps, const char * ends);	/*!< 取1单词,指定分隔符集和结束符集合,返回下个单词或结束符的地址 */
const char *  getaword(char * d, const char * s, const char *p, int maxlen); /*!< 取1单词,指定分隔符集,返回下一个单词的起始地址 */

const char *  skipchs (const char * s, const char * chars);	/*!< 跳过指定字符集 */
const char *  skip2chs(const char * s, const char * chars); /*!< 跳到指定字符集 */
const char *  skip2ch (const char * s, char ch);
const char *  skipblanks(const char * s);				/*!< 跳过空格 */

int     endcmp    (const char * d, const char * e);		/*!< 比较d的末尾是否是e,结果同strcmp */

int     trimstr   (char * d);				/*!< 去掉空格 */
int     trimstr   (char * d, char * s);		/*!< 去掉空格,指定存放缓冲区 */
int		trim_head (char * d, char * s);		/*!< 去掉首空格,指定存放缓冲区 */
int		trim_head0(char * d);				/*!< 去掉前面的0字符 */
int		trim_tail (char * d, char * s);		/*!< 去掉尾空格,指定存放缓冲区 */
int		trim_all  (char * d, char * s);		/*!< 去掉所有空格,合并多余空格,指定存放缓冲区 */
int		trim_all  (char * d, char * s, char * blanks);		/*!< 去掉空格,指定空格字符集 */
int		trim_all  (char * d, char * s, char blank);			/*!< 去掉空格,指定空格字符 */
int		trim_all  (char * d);				/*!< 去掉空格,合并多余空格 */

int     padr      (char * d, char * s, int len);	/*!< 右对齐填充空格 */
char *  ht_lpad0  (char * d, int l);
char *  ht_lpad   (char * d, int l);
int     bcdtoint  (char * d, int len);		/*!< 转bcd字符串为整数 */
int		strmove   (char * d, char * s);		/*!< 移动字符串 */
int		strreplace(char * d, char * s0, char * s1);	/*!< 将字符串d中的s0出现用s1替代 */
int		strreplace(char * s0, int l0, char * s1, int l1);	/*!< 将字符串d中的s0出现用s1替代,指定s1长度 */
int		strinsert (char * d, char * s, int l);	/*!< s插入d的开始部分,l为规定的长度,如<0则为s整个串 */

/// 转换自动换行
/// @param d : 目的地址
/// @param s : 源地址
/// @param rowlen : 行长度
/// @param rows : 行数限制
/// @param startpos : 开始位置
/// @return 实际行数
int 	strwordwrap(char *d, const char * s, int rowlen, int rows, int startpos);

int		countlines(const char * s, char c);		/*!< 统计字符串中包含的行数 */
int		countwords(const char * s, const char * seps);	/*!< 统计字符串中包含的行数 */

int		splitstr(char * d, char splitchar); /*!< 将字符串中分隔符替换成'\0'形成字符串集合,返回字符串数目 */ 
int		splitstr(char * d, char * splitstr);/*!< 将字符串中分隔符替换成'\0'形成字符串集合,返回字符串数目 */

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

/// 在 str_array 中找 sfind
/// str_array 是以 2 个 \0 结束的字符串数组
const char * strin(const char * str_array, const char * sfind);
const char * striin(const char * str_array, const char * sfind);

const char *  skip0s(const char * s);
int compare_number(const char * s0, const char * s1);

#endif // _L_L_STR_H

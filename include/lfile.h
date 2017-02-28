#ifndef _LFILE_H_
#define _LFILE_H_

#include <time.h>

//! read_file
/*!
 * goal   : 将文件整个读到内存中,其中msg为出错时的消息缓冲,lmsg为消息缓冲区的长度
 * \param d 缓冲区
 * \param filename 文件全路径名称
 * \param msg 出错消息缓冲区,如果是0则不返回消息
 * \param lmsg 出错消息缓冲区长度,如果是0则不返回消息
 * \return : >= 0  : 字符串长度<br>
 *          < 0   : 出错,msg中有出错消息<br>
 *          -100  : 参数出错,即给的参数指针有空值,msg中无出错消息,因为兹时msg可能为空
 */
int read_file (char ** d, const char * filename, char * msg, int lmsg);
int read_file (char  * d, const char * filename, char * msg, int lmsg);	/*!< 读文件 */
int read_file (char ** d, const char * filename, long startpos, char * msg, int lmsg);
int read_file (char  * d, const char * filename, long startpos, char * msg, int lmsg);	/*!< 读文件 */

//! write_file
/*!
 * \param filename 文件全路径名称,如首字符为"+"为追加写
 * \param s 被写字符串缓冲区
 * \param msg 出错消息缓冲区,如果是0则不返回消息
 * \param lmsg 出错消息缓冲区长度,如果是0则不返回消息
 * \return : >= 0  : 字符串长度<br>
 *          < 0   : 出错,msg中有出错消息<br>
 *          -100  : 参数出错,即给的参数指针有空值,msg中无出错消息,因为兹时msg可能为空
 */
int write_file(const char * filename, const char * s, char * msg, int lmsg);
int write_file(const char * filename, const char * s, int len, char * msg, int lmsg);/*!< 写二进制数据到文件 */

//! 取得文件的最后修改时间
/*!
 * \param t[out] 取得时间的字符串,格式为YYYYMMDDhhmmss
 * \param filename[in] 文件名称
 * \return 0/else 出错,修改时间<br>
 * 正确返回时,如果t不是0,存放时间字符串,失败是为空串
 */
time_t get_modify_time(char * t, const char * filename);

//! 取得文件的长度
/*!
 * \param filename 文件名称
 * \return >=0/<0 成功,文件长度/出错码<br>
 */
long get_file_size(const char * filename);

#endif // _LFILE_H_

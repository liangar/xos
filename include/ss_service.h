#ifndef _SS_SERVICE_H_
#define _SS_SERVICE_H_

void set_run_path(const char * p);	/*!< 设置当前工作目录 */
char * get_run_path();				/*!< 取得当前工作目录 */

char * getnowtime(char *t);			/*!< 取得当前时间的字符串,格式为"YYYY-MM-DD hh:mi:ss" */
char * getsimple_now(char *t);		/*!< 取得当前时间的字符串,格式为"YYYYMMDDhhmiss" */
char * time2string(char * d, long t);	/*!< 将时间转为格式为"YYYY-MM-DD hh:mi:ss"的字符串 */
char * string2simple(char *d, char *s, bool bendflag = false);	/*!< 将时间串转为简单格式 */
char * simple2string(char * d, const char * s);
char * time2simple(char * d, long t);	/*!< 将时间转为简单格式串 */
char * time2simpledate(char *d, long t);/*!< 将时间转为简单格式"YYYYMMDD" */
char * getnextdate(char *d, char *s);	/*!< 取s的下一天,格式为:"YYYYMMDD" */
char * getnextdate(char * s);			/// 在原位置上取下一天:YYYYMMDD

char * getnexthour(char *d, char *s);	/*!< 取s的下一小时,格式为:"YYYYMMDDhhmmss" */
char * getnexthour(char * s);			/// 在原位置上取下一小时:YYYYMMDDhhmmss

char * getnextquarter(char *d, char *s);	/*!< 取s的下一刻,格式为:"YYYYMMDDhhmmss" */
char * getnextquarter(char * s);			/// 在原位置上取下一刻:YYYYMMDDhhmmss

char * getprevdate(char *d, char *s);	/*!< 取s的上一天,格式为:"YYYYMMDD" */
char * getprevdate(char * s);			/// 在原位置上取上一天:YYYYMMDD

char * getprevyear(char * d, char * s);/// YYYYMMDD
char * getprevyear(char * s);			/// YYYYMMDD 用原位置
long   getprevyear(long t);			/// 取当前的前一年

char * getprevmonthday(char * d, char * s);/// YYYYMMDD
char * getprevmonthday(char * s);			/// YYYYMMDD 用原位置
long   getprevmonthday(long t);			/// 取当前的前个月

char * getPrevMonth(char * d, char * s);/// YYYYMM
char * getPrevMonth(char * s);			/// YYYYMM 用原位置
long   getPrevMonth(long t);			/// 取当前的前个月

char * getnextmonth(char * d, char * s);/*!< 取s的下一月,格式为:"YYYYMMDD" */
char * getnextmonth(char * s);			/// 在原位置上取下一月:YYYYMMDD
//long   getnextmonth(long t);			/// 取当前的下个月

long getmaxdaywithmonth(int year, int month);	//取得一个月最后一天，返回DD，month取值1-12
long getmaxdaywithmonth(char * s);	//取得一个月最后一天，返回DD

long getnextimebyminute(char * hhmm);	///计算下一个时刻距离现在的分钟数

long simple2time(const char * s);		/*!< 将简单格式串转为时间 */

long dt_nextday(long t);

bool dt_ValidDate(const char * s);			/*!< 检查串是否是简单日期格式 */
bool dt_ValidTime(const char * s);			/*!< 检查串是否是简单时间格式"hhmiss" */
bool dt_ValidDateTime(const char * s);		/*!< 检查串是否是简单时间格式"YYYYMMDDhhmiss" */
int  dt_datetime_format(const char * dtstring); /// YYYY-MM-DD hh:mi:ss | YYYY-MM-DD | hh:mi:ss

long dt_to_day_time(long t, const char * stime);/// stime : format hhmmss
long dt_to_daybegin(long t);
long dt_gettoday(void);
long dt_gettoday_time(const char * time);
long dt_today(void);
long dt_year(long t);
long dt_month(long t);
long dt_day(long t);
long dt_hour(long t);
long dt_minute(long t);

void To_Lower(char *s);		/*!< 将字符转为大写 */
void To_Upper(char *s);		/*!< 将字符转为小写 */

char * getpathfile(const char * filename, char * fullFileName, char *outPath, char *outFileName); /*!< 取得文件路径和文件名称 */

//! 打开LOG文件
/*!
 * \param pfilename		文件名称
 * \param b_in_debug	是否是调试状态<br>
 *						如果是调试状态,会在屏幕上同时显示文件内容<br>
 *						不同的是屏幕上显示不换行
 * \param idle			打印时间的时间间隔<br>
 *						对长于间隔时间的,重新打印时间
 * \param saveold		是否保存旧日值文件<br>
 *						需要保存的,系统在创建新日值文件之前会将老日值改名为加入时间后缀的文件名
 * \return true/false=成功/失败<br>
 *		如果失败应该是目录无法创建等原因
 */
bool openservicelog(const char * pfilename, bool b_in_debug, int idle = 60, bool saveold = true, const char * bakpath = 0);
void closeservicelog(void);	/*!< 关闭日值文件 */
bool reopenservicelog(void);
int  flushservicelog(void);

void writetoeventlog(int wType, int dwID, char const * pFormat, va_list v);	/*!< 写日值文件,自动打印换行 */
void WriteToEventLog(int wEventType, int dwEventID, char const * pFormat, ...);/*!< 写日值文件,自动打印换行 */
void WriteToEventLog(char const * pFormat, ...);	/*!< 写日值文件,用法同printf,自动打印换行 */
void WriteToFileLog(const char * logfile, char const * pFormat, ...); /*!< 写指定日值文件 */

void terminatethread(void * &hthread, int seconds = 3000, int returncode = 0); /*!< 停止线程 */
void getfullname(char * pbuf, const char * pfilename, int maxlen);	/*!< 取得文件的全目录字符串 */

void EL_puts(const char * s);					/*!< 写字符串到日值文件,无换行 */
void EL_WriteHexString(const char * s, int len);/*!< 写16进制字符串到日值文件 */
void EL_WriteNow();	/*!< 写当前时间到日值文件 */
void write_buf_log(const char * title, unsigned char * buf, int len);

char * get_error_message(unsigned int dwError, char * msg, int maxlen);	/*!< 取得系统出错说明 */

//! 写系统出错说明
/*!
 * \param dwError 出错号
 * \param ptitle 标题
 * \param subtitle 子标题
 */
void WriteSystemErrorLog(unsigned int dwError, const char * ptitle, const char * subtitle);
void WriteSystemErrorLog(const char * ptitle, const char * subtitle);	/*!< 写最近操作的系统出错说明 */


/// 全数字
bool all_is_digit(const char * s, int len);
/// 全空
bool all_is_blank(const char * s, int len);
///是十六进制的字符
bool all_is_0xdigit(const char * s, int len);

#endif // _SS_SERVICE_H_


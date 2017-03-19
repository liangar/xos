#ifndef _SS_SERVICE_H_
#define _SS_SERVICE_H_

void set_run_path(const char * p);	/*!< ���õ�ǰ����Ŀ¼ */
char * get_run_path();				/*!< ȡ�õ�ǰ����Ŀ¼ */

char * getnowtime(char *t);			/*!< ȡ�õ�ǰʱ����ַ���,��ʽΪ"YYYY-MM-DD hh:mi:ss" */
char * getsimple_now(char *t);		/*!< ȡ�õ�ǰʱ����ַ���,��ʽΪ"YYYYMMDDhhmiss" */
char * time2string(char * d, long t);	/*!< ��ʱ��תΪ��ʽΪ"YYYY-MM-DD hh:mi:ss"���ַ��� */
char * string2simple(char *d, char *s, bool bendflag = false);	/*!< ��ʱ�䴮תΪ�򵥸�ʽ */
char * simple2string(char * d, const char * s);
char * time2simple(char * d, long t);	/*!< ��ʱ��תΪ�򵥸�ʽ�� */
char * time2simpledate(char *d, long t);/*!< ��ʱ��תΪ�򵥸�ʽ"YYYYMMDD" */
char * getnextdate(char *d, char *s);	/*!< ȡs����һ��,��ʽΪ:"YYYYMMDD" */
char * getnextdate(char * s);			/// ��ԭλ����ȡ��һ��:YYYYMMDD

char * getnexthour(char *d, char *s);	/*!< ȡs����һСʱ,��ʽΪ:"YYYYMMDDhhmmss" */
char * getnexthour(char * s);			/// ��ԭλ����ȡ��һСʱ:YYYYMMDDhhmmss

char * getnextquarter(char *d, char *s);	/*!< ȡs����һ��,��ʽΪ:"YYYYMMDDhhmmss" */
char * getnextquarter(char * s);			/// ��ԭλ����ȡ��һ��:YYYYMMDDhhmmss

char * getprevdate(char *d, char *s);	/*!< ȡs����һ��,��ʽΪ:"YYYYMMDD" */
char * getprevdate(char * s);			/// ��ԭλ����ȡ��һ��:YYYYMMDD

char * getprevyear(char * d, char * s);/// YYYYMMDD
char * getprevyear(char * s);			/// YYYYMMDD ��ԭλ��
long   getprevyear(long t);			/// ȡ��ǰ��ǰһ��

char * getprevmonthday(char * d, char * s);/// YYYYMMDD
char * getprevmonthday(char * s);			/// YYYYMMDD ��ԭλ��
long   getprevmonthday(long t);			/// ȡ��ǰ��ǰ����

char * getPrevMonth(char * d, char * s);/// YYYYMM
char * getPrevMonth(char * s);			/// YYYYMM ��ԭλ��
long   getPrevMonth(long t);			/// ȡ��ǰ��ǰ����

char * getnextmonth(char * d, char * s);/*!< ȡs����һ��,��ʽΪ:"YYYYMMDD" */
char * getnextmonth(char * s);			/// ��ԭλ����ȡ��һ��:YYYYMMDD
//long   getnextmonth(long t);			/// ȡ��ǰ���¸���

long getmaxdaywithmonth(int year, int month);	//ȡ��һ�������һ�죬����DD��monthȡֵ1-12
long getmaxdaywithmonth(char * s);	//ȡ��һ�������һ�죬����DD

long getnextimebyminute(char * hhmm);	///������һ��ʱ�̾������ڵķ�����

long simple2time(const char * s);		/*!< ���򵥸�ʽ��תΪʱ�� */

long dt_nextday(long t);

bool dt_ValidDate(const char * s);			/*!< ��鴮�Ƿ��Ǽ����ڸ�ʽ */
bool dt_ValidTime(const char * s);			/*!< ��鴮�Ƿ��Ǽ�ʱ���ʽ"hhmiss" */
bool dt_ValidDateTime(const char * s);		/*!< ��鴮�Ƿ��Ǽ�ʱ���ʽ"YYYYMMDDhhmiss" */
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

void To_Lower(char *s);		/*!< ���ַ�תΪ��д */
void To_Upper(char *s);		/*!< ���ַ�תΪСд */

char * getpathfile(const char * filename, char * fullFileName, char *outPath, char *outFileName); /*!< ȡ���ļ�·�����ļ����� */

//! ��LOG�ļ�
/*!
 * \param pfilename		�ļ�����
 * \param b_in_debug	�Ƿ��ǵ���״̬<br>
 *						����ǵ���״̬,������Ļ��ͬʱ��ʾ�ļ�����<br>
 *						��ͬ������Ļ����ʾ������
 * \param idle			��ӡʱ���ʱ����<br>
 *						�Գ��ڼ��ʱ���,���´�ӡʱ��
 * \param saveold		�Ƿ񱣴����ֵ�ļ�<br>
 *						��Ҫ�����,ϵͳ�ڴ�������ֵ�ļ�֮ǰ�Ὣ����ֵ����Ϊ����ʱ���׺���ļ���
 * \return true/false=�ɹ�/ʧ��<br>
 *		���ʧ��Ӧ����Ŀ¼�޷�������ԭ��
 */
bool openservicelog(const char * pfilename, bool b_in_debug, int idle = 60, bool saveold = true, const char * bakpath = 0);
void closeservicelog(void);	/*!< �ر���ֵ�ļ� */
bool reopenservicelog(void);
int  flushservicelog(void);

void writetoeventlog(int wType, int dwID, char const * pFormat, va_list v);	/*!< д��ֵ�ļ�,�Զ���ӡ���� */
void WriteToEventLog(int wEventType, int dwEventID, char const * pFormat, ...);/*!< д��ֵ�ļ�,�Զ���ӡ���� */
void WriteToEventLog(char const * pFormat, ...);	/*!< д��ֵ�ļ�,�÷�ͬprintf,�Զ���ӡ���� */
void WriteToFileLog(const char * logfile, char const * pFormat, ...); /*!< дָ����ֵ�ļ� */

void terminatethread(void * &hthread, int seconds = 3000, int returncode = 0); /*!< ֹͣ�߳� */
void getfullname(char * pbuf, const char * pfilename, int maxlen);	/*!< ȡ���ļ���ȫĿ¼�ַ��� */

void EL_puts(const char * s);					/*!< д�ַ�������ֵ�ļ�,�޻��� */
void EL_WriteHexString(const char * s, int len);/*!< д16�����ַ�������ֵ�ļ� */
void EL_WriteNow();	/*!< д��ǰʱ�䵽��ֵ�ļ� */
void write_buf_log(const char * title, unsigned char * buf, int len);

char * get_error_message(unsigned int dwError, char * msg, int maxlen);	/*!< ȡ��ϵͳ����˵�� */

//! дϵͳ����˵��
/*!
 * \param dwError �����
 * \param ptitle ����
 * \param subtitle �ӱ���
 */
void WriteSystemErrorLog(unsigned int dwError, const char * ptitle, const char * subtitle);
void WriteSystemErrorLog(const char * ptitle, const char * subtitle);	/*!< д���������ϵͳ����˵�� */


/// ȫ����
bool all_is_digit(const char * s, int len);
/// ȫ��
bool all_is_blank(const char * s, int len);
///��ʮ�����Ƶ��ַ�
bool all_is_0xdigit(const char * s, int len);

#endif // _SS_SERVICE_H_


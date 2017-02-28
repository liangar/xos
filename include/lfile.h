#ifndef _LFILE_H_
#define _LFILE_H_

#include <time.h>

//! read_file
/*!
 * goal   : ���ļ����������ڴ���,����msgΪ����ʱ����Ϣ����,lmsgΪ��Ϣ�������ĳ���
 * \param d ������
 * \param filename �ļ�ȫ·������
 * \param msg ������Ϣ������,�����0�򲻷�����Ϣ
 * \param lmsg ������Ϣ����������,�����0�򲻷�����Ϣ
 * \return : >= 0  : �ַ�������<br>
 *          < 0   : ����,msg���г�����Ϣ<br>
 *          -100  : ��������,�����Ĳ���ָ���п�ֵ,msg���޳�����Ϣ,��Ϊ��ʱmsg����Ϊ��
 */
int read_file (char ** d, const char * filename, char * msg, int lmsg);
int read_file (char  * d, const char * filename, char * msg, int lmsg);	/*!< ���ļ� */
int read_file (char ** d, const char * filename, long startpos, char * msg, int lmsg);
int read_file (char  * d, const char * filename, long startpos, char * msg, int lmsg);	/*!< ���ļ� */

//! write_file
/*!
 * \param filename �ļ�ȫ·������,�����ַ�Ϊ"+"Ϊ׷��д
 * \param s ��д�ַ���������
 * \param msg ������Ϣ������,�����0�򲻷�����Ϣ
 * \param lmsg ������Ϣ����������,�����0�򲻷�����Ϣ
 * \return : >= 0  : �ַ�������<br>
 *          < 0   : ����,msg���г�����Ϣ<br>
 *          -100  : ��������,�����Ĳ���ָ���п�ֵ,msg���޳�����Ϣ,��Ϊ��ʱmsg����Ϊ��
 */
int write_file(const char * filename, const char * s, char * msg, int lmsg);
int write_file(const char * filename, const char * s, int len, char * msg, int lmsg);/*!< д���������ݵ��ļ� */

//! ȡ���ļ�������޸�ʱ��
/*!
 * \param t[out] ȡ��ʱ����ַ���,��ʽΪYYYYMMDDhhmmss
 * \param filename[in] �ļ�����
 * \return 0/else ����,�޸�ʱ��<br>
 * ��ȷ����ʱ,���t����0,���ʱ���ַ���,ʧ����Ϊ�մ�
 */
time_t get_modify_time(char * t, const char * filename);

//! ȡ���ļ��ĳ���
/*!
 * \param filename �ļ�����
 * \return >=0/<0 �ɹ�,�ļ�����/������<br>
 */
long get_file_size(const char * filename);

#endif // _LFILE_H_

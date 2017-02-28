#ifndef _nr_crypt_H_
#define _nr_crypt_H_

//! 加密字符串
/*!
 * \param s 被加密字符串
 * \param d 加密结果字符串
 * \return d
 */
char * nr_crypt(char * d, char * s);

//! 解密字符串
/*!
 * \param s 被解密字符串
 * \param d 解密结果字符串
 * \return d
 */
char * nr_decrypt(char * d, char * s);

#endif // _nr_crypt_H_


/**  
* @file CDBPasswd.h  
* @Author somebody and me  
* @date 2004/08/15  
* @version 1.0  
*/  
#ifndef _MCB_CDBPASSWD_H
#define _MCB_CDBPASSWD_H

#include <string>

using std::string;

/**  
* @brief 密码文件操作类.用于增加用户、删除用户、取用户密码和更改用户密码。  
*/
class CDBPasswd {
public:
    CDBPasswd();
    ~CDBPasswd();
    
    /**
    * @brief  初始化类。完成取环境变量，初始化passwd全路径文件名。
    * @return 初始化成功返回true，失败返回false。
    */
    bool  Init();
    
    /**
    * @brief 往passwd文件增加用户配置
    * @param sUser 用户名字符串
    * @param sPass 密码字符串
    * @return 增加用户成功返回true，失败返回false。
    */
    bool  AddUser(const string& sUser, const string& sPass);
    
    /**
    * @brief  根据传入的用户名参数，从passwd文件删除该用户配置
    * @param  sUser 要删除用户用户名字符串
    * @return 删除成功返回true，失败返回false。
    */
    bool  DelUser(const string& sUser);
    
    /**
    * @brief 根据传入的用户名信息，从passwd文件中取出该用户解密后的密码。
    * @param sUser 用户名字符串
    * @return 获取成功返回密码字符串，失败返回空串。
    */
    string GetPass(const string& sUser);
    
    /**
    * @brief  根据传入的用户名和密码，改变该用户的密码配置
    * @param sUser 用户名字符串
    * @param sPass 密码字符串
    * @return 更改成功返回true，失败返回false。
    */
    bool  ChgPass(const string& sUser, const string& sPass);

private:
    const string Encrypt(const string& sPass);
    const string Decrypt(const string& sCipher);

private:
    string m_sFileName;
    static const string m_sKey;
};

#endif


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
* @brief �����ļ�������.���������û���ɾ���û���ȡ�û�����͸����û����롣  
*/
class CDBPasswd {
public:
    CDBPasswd();
    ~CDBPasswd();
    
    /**
    * @brief  ��ʼ���ࡣ���ȡ������������ʼ��passwdȫ·���ļ�����
    * @return ��ʼ���ɹ�����true��ʧ�ܷ���false��
    */
    bool  Init();
    
    /**
    * @brief ��passwd�ļ������û�����
    * @param sUser �û����ַ���
    * @param sPass �����ַ���
    * @return �����û��ɹ�����true��ʧ�ܷ���false��
    */
    bool  AddUser(const string& sUser, const string& sPass);
    
    /**
    * @brief  ���ݴ�����û�����������passwd�ļ�ɾ�����û�����
    * @param  sUser Ҫɾ���û��û����ַ���
    * @return ɾ���ɹ�����true��ʧ�ܷ���false��
    */
    bool  DelUser(const string& sUser);
    
    /**
    * @brief ���ݴ�����û�����Ϣ����passwd�ļ���ȡ�����û����ܺ�����롣
    * @param sUser �û����ַ���
    * @return ��ȡ�ɹ����������ַ�����ʧ�ܷ��ؿմ���
    */
    string GetPass(const string& sUser);
    
    /**
    * @brief  ���ݴ�����û��������룬�ı���û�����������
    * @param sUser �û����ַ���
    * @param sPass �����ַ���
    * @return ���ĳɹ�����true��ʧ�ܷ���false��
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


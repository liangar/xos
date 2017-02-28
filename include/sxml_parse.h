#pragma once

#pragma pack(push, 1)

#define XIML_MAX_ERR    10
#define XIML_MAX_DEEP   10

#define XO_VALUE        1
#define XO_NO_CONTENT   2
#define XO_TAGS         3

#define XE_NO_TAGSTART      -1
#define XE_NAME             -2
#define XE_PROP             -3
#define XE_END_TAGSTART     -4
#define XE_END_TAGNAME      -5
#define XE_END_TAGNAME_END  -6
#define XE_END_TAG          -7
#define XE_NO_EQU           -8
#define XE_NO_PVALUE        -9
#define XE_NO_TAG_VALUE     -10
#define XE_HEADER			-11

const unsigned short
	XT_TYPE_MASK	= 0xf000,
	XT_TAG_NAME		= 0x2000,
	XT_PROP_NAME	= 0x4000,
	XT_PROP_VALUE	= 0x5000,
	XT_TAG_END		= 0x1000,
	XT_REF			= 0x8000,
	XT_LEN_MASK		= 0x0fff;

struct ximl_errmsg{ // ������Ϣ
    int line, col;  // ����
    int code;       // �������
    char * pmsg;    // ��Ϣ
};

struct ximl_T;  // ��
struct ximl_P;  // ����
struct ximl_TT; // ������

struct ximl_T{
    ximl_TT * ptype;        // ����
    ximl_T  * pparent,      // �ϼ�
            * pchildren,    // �¼�
            * next,         // ��
            * prev;         // ��
    ximl_P  * prop;         // ���Ա�
    char      nchildren;    // �¼�����
    char      nprop;        // ���Ը���
};

struct ximl_P{
    char    * pname,        // ����
            * pvalue;       // ֵ
    ximl_P  * next,         // ��
            * prev;         // ��
};

struct ximl_TT{
    char    * pname;        // ֵ
    char      n;            // ����
};

void X_show(char * d);		// ȡ����ʾ���ε�ximl�ṹ���ַ���

/// �����ṹ
/// param :
///         t        �ڵ�ĵ�ַ
///         tt       �ڵ����͵�ַ �Ŀ�ʼ��ַ
///         filename �ļ�����
/// return: >=0/-1 = �ڵ�������Ŀ/error
int X_parsefile(ximl_T * t, ximl_TT ** tt, char * filename);
int X_parse(ximl_T * t, ximl_TT ** tt, char * buf);

/// ����﷨
/// param : p  Ҫ�����ַ���
/// return: 0/<0 = ok/errorCode
int X(char * p);

/// ����ȫ��ȷ���ַ��������ڲ��ṹ
/// param :
///         t        �ڵ�ĵ�ַ
///         tt       �ڵ����͵�ַ �Ŀ�ʼ��ַ
///         filename �ļ�����
/// return: >=0/-1 = �ڵ�������Ŀ/error
int X_gen(ximl_T * p_t, ximl_TT ** p_tt);

void X_free(void);	// �ͷſռ�

void X_D(char * d, ximl_T * pt);	// ȡ��tag�Ľṹ��ʾ��

ximl_T * X_get_tag(const char * path);
ximl_P * X_get_prop(ximl_T * t, const char * pname);	// ȡ��tag������
ximl_P * X_get_prop(const char * path, const char * pname);
ximl_P * X_get_prop(const char * pname);

ximl_P * X_get_prop_head(int &pcount);	// ȡ�����ԵĿ�ʼ

int X_count(char * pt_name);	// ȡ��ĳһ�������Ƶ�tag��Ŀ

void X_geterror(char * errmsg);

#pragma pack(pop)

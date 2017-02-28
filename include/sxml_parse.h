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

struct ximl_errmsg{ // 出错消息
    int line, col;  // 行列
    int code;       // 出错代码
    char * pmsg;    // 消息
};

struct ximl_T;  // 项
struct ximl_P;  // 属性
struct ximl_TT; // 项类型

struct ximl_T{
    ximl_TT * ptype;        // 类型
    ximl_T  * pparent,      // 上级
            * pchildren,    // 下级
            * next,         // 兄
            * prev;         // 弟
    ximl_P  * prop;         // 属性表
    char      nchildren;    // 下级个数
    char      nprop;        // 属性个数
};

struct ximl_P{
    char    * pname,        // 名称
            * pvalue;       // 值
    ximl_P  * next,         // 兄
            * prev;         // 第
};

struct ximl_TT{
    char    * pname;        // 值
    char      n;            // 个数
};

void X_show(char * d);		// 取得显示树形的ximl结构的字符串

/// 解析结构
/// param :
///         t        节点的地址
///         tt       节点类型地址 的开始地址
///         filename 文件名称
/// return: >=0/-1 = 节点类型数目/error
int X_parsefile(ximl_T * t, ximl_TT ** tt, char * filename);
int X_parse(ximl_T * t, ximl_TT ** tt, char * buf);

/// 检查语法
/// param : p  要检查的字符串
/// return: 0/<0 = ok/errorCode
int X(char * p);

/// 对完全正确的字符串生成内部结构
/// param :
///         t        节点的地址
///         tt       节点类型地址 的开始地址
///         filename 文件名称
/// return: >=0/-1 = 节点类型数目/error
int X_gen(ximl_T * p_t, ximl_TT ** p_tt);

void X_free(void);	// 释放空间

void X_D(char * d, ximl_T * pt);	// 取得tag的结构显示串

ximl_T * X_get_tag(const char * path);
ximl_P * X_get_prop(ximl_T * t, const char * pname);	// 取得tag的属性
ximl_P * X_get_prop(const char * path, const char * pname);
ximl_P * X_get_prop(const char * pname);

ximl_P * X_get_prop_head(int &pcount);	// 取得属性的开始

int X_count(char * pt_name);	// 取得某一类型名称的tag数目

void X_geterror(char * errmsg);

#pragma pack(pop)

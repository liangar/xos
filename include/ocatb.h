#ifndef _xsys_ocatb_H_
#define _xsys_ocatb_H_

/// \file oratb.h
/// 数据库表操作类模板<br>
///

#include <xsys_oci.h>

#include <xlist.h>

using namespace oralib;

#ifndef XOCA_MAX_ITEMS
#define XOCA_MAX_ITEMS	512
#endif

/// \class xdbtable
/// \brief 一个数据库表操作的模板类
/// 对于具体的表,可以用自定义的结构,基于此模板派生
template<class T>
class xdbtable{
public :
	xdbtable();	/// 构造函数
	/// 构造函数
	/// \param pdb 数据库连接
	xdbtable(connection * pdb);
	/// 构造函数
	/// \param pdb 数据库连接串
	xdbtable(const char * pdsn);

	~xdbtable();	/// 析构

	void operator = (const T & s);	/// 赋值到类数据
	void setdata(const T & s);		/// 赋值到类数据
	void getdate(T & d);			/// 取值

	void setdb(connection * pdb);	/// 设置连接串
	bool open(const char * pdsn = 0);	/// 打开数据库连接
	void close();	/// 关闭并释放资源

	/// 增加新记录到数据库中
	/// \param s 记录数据
	/// \param bautocommit 是否自动提交
	virtual int add(const T & s, bool bautocommit = false) = 0;
	virtual int get(T & s) = 0;		/// 取得1条记录
	virtual int del(const T & s) = 0; /// 删除
	virtual int upd(const T & s) = 0; /// 修改
	virtual int fetch(T & s) = 0; /// 取得记录

	int  select();	/// 执行取记录集操作
	bool next();	/// 取记录集中下一条记录
	int  endexec();	/// 停止执行

protected :
	/// 取得当前记录集结果
	/// \param items 存放记录集的数组
	/// \param maxitems	最大长度
	/// \return 取得的记录数
	int get_list(xlist<T> & items, int maxitems = 200);

protected :
	T			m_data;

	bool		m_bself_connect;
	connection* m_pdb;
	statement *	m_psql;
	resultset * m_presult;
	char *		m_pdsn;
};


template<class T>
xdbtable<T>::xdbtable()
	: m_pdb(0)
	, m_psql(0)
	, m_presult(0)
	, m_pdsn(0)
	, m_bself_connect(true)
{
	m_pdsn = 0;
}

template<class T>
xdbtable<T>::xdbtable(connection * pdb)
	: m_pdb(0)
	, m_psql(0)
	, m_presult(0)
	, m_pdsn(0)
	, m_bself_connect(true)
{
	//xdbtable();
	m_pdsn = 0;
	m_bself_connect = false;
	m_pdb = pdb;
}

template<class T>
xdbtable<T>::xdbtable(const char * pdsn)
	: m_pdb(0)
	, m_psql(0)
	, m_presult(0)
	, m_pdsn(0)
	, m_bself_connect(true)
{
	//xdbtable();
	m_pdsn = 0;
	m_pdsn = new char[strlen(pdsn) + 1];
	strcpy(m_pdsn, pdsn);
}


template<class T>
xdbtable<T>::~xdbtable()
{
	close();
}


template<class T>
void xdbtable<T>::setdb(connection * pdb)
{
	close();
	m_bself_connect = false;
	m_pdb = pdb;
}

template<class T>
bool xdbtable<T>::open(const char * pdsn)
{
	if (m_pdsn == 0 && pdsn == 0){
		return false;
	}
	m_bself_connect = true;
	m_pdb = new connection;
	if ((m_pdb->open(m_pdsn)) < 0){
		delete m_pdb;  m_pdb = 0;
		return false;
	}
	return true;
}

template<class T>
void xdbtable<T>::close()
{
	if (m_presult){
		m_presult->release();  m_presult = 0;
	}
	if (m_psql){
		m_psql->release(); m_psql = 0;
	}

	if (m_bself_connect && m_pdsn){
		if (m_pdb) {
			m_pdb->close();  delete m_pdb;  m_pdb = 0;
		}
		if (m_pdsn) {
			delete[] m_pdsn;  m_pdsn = 0;
		}
	}
	m_bself_connect = false;
}

template<class T>
int xdbtable<T>::select()
{
	m_psql = m_pdb->select();
}

template<class T>
bool xdbtable<T>::next()
{
	if (m_presult == 0)
		return false;
	return m_presult->next();
}

template<class T>
int xdbtable<T>::endexec()
{
	if (m_psql == 0)
		return -1;
	m_psql->release();
	m_psql = 0;

	return 0;
}

template<class T>
void xdbtable<T>::operator = (const T & s)
{
	setdata(s);
}

template<class T>
void xdbtable<T>::setdata(const T & s)
{
	memcpy(&m_data, &s, sizeof(T));
}

template<class T>
void xdbtable<T>::getdate(T & d)
{
	memcpy(&d, &m_data, sizeof(T));
}

template<class T>
int xdbtable<T>::get_list(xlist<T> & items, int maxitems)
{
	items.free_all();

	if (maxitems <= 0){
		maxitems = 2 * 1024 * 1024;
	}

	if (maxitems < 2 * 1024){
		items.init(maxitems/2, maxitems/2 + 2);
	}else{
		items.init(2 * 1024, 2 * 1024);
	}

	T s;
	while (!m_presult->eod()){
		memset(&s, 0, sizeof(T));
		if (fetch(s) == 0)
			items.add(&s);
		next();
		if (items.m_count >= maxitems) {
			break;
		}
	}
	close();

	return items.m_count;
}

#endif // _xsys_ocatb_H_


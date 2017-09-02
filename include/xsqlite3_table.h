#pragma once

#include <xsqlite3.h>
#include <xlist.h>

#ifndef XOCA_MAX_ITEMS
#define XOCA_MAX_ITEMS	256
#endif

/// \file xdb_tb.h
/// 数据库表操作类模板<br>
///

template<class T>
class xsqlite3_table : public xsql3{
public :
	xsqlite3_table(void);

	void setdata(const T & s);

	virtual int add(const T & s, bool bautocommit = false) = 0;	// 增加新记录到数据库中
	virtual int get(T & s) = 0;			// 取得1条记录
	virtual int del(const T & s) = 0;	// 删除
	virtual int upd(const T & s) = 0;	// 修改

	virtual int bindcols(T & s) = 0;  // 绑定列

	int commit(void);
	int rollback(void);

	const char * dberror(void);

	const char * lastError(void)  { return m_msg;  }
protected :
	int get_list(xlist<T> & items, int maxitems = XOCA_MAX_ITEMS);

protected :
	T			m_rec;
	char		m_msg[256];
};


template<class T>
xsqlite3_table<T>::xsqlite3_table(void)
{
	xsql3::xsql3();
	memset(&m_rec, 0, sizeof(T));
	memset(m_msg, 0, sizeof(m_msg));
}

template<class T>
void xsqlite3_table<T>::setdata(const T & s)
{
	memcpy(&m_rec, &s, sizeof(T));
}

template<class T>
int xsqlite3_table<T>::get_list(xlist<T> & items, int maxitems)
{
	if (maxitems <= 0){
		maxitems = 10 * 1024;
	}

	if (maxitems < 5 * 1024){
		items.init(maxitems, maxitems/2 + 2);
	}else{
		items.init(10 * 1024, 2 * 1024);
	}

	int r = bindcols(m_rec);
	if (r == XSQL_OK)
		r = XSQL_HAS_DATA;

	while ((r == XSQL_HAS_DATA) && items.m_count <= maxitems){
		memset(&m_rec, 0, sizeof(T));
		r = fetch();
		if (r == XSQL_HAS_DATA)
			items.add(&m_rec);
	}
	if (r != XSQL_NO_DATA && r != XSQL_WARNING && r != XSQL_HAS_DATA) {
		return XSQL_ERROR;
	}

	return items.m_count;
}

template<class T>
int xsqlite3_table<T>::commit(void)
{
	endexec();
	return m_pdb->trans_end(XSQL_COMMIT);
}

template<class T>
int xsqlite3_table<T>::rollback(void)
{
	endexec();
	return m_pdb->trans_end(XSQL_ROLLBACK);
}

template<class T>
const char * xsqlite3_table<T>::dberror(void)
{
	geterror(m_msg, sizeof(m_msg));
	if (m_msg[0] == 0 && m_pdb) {
		m_pdb->geterror(m_msg, sizeof(m_msg));
	}
	return m_msg;
}

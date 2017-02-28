#ifndef _xsys_ocatb_H_
#define _xsys_ocatb_H_

/// \file oratb.h
/// ���ݿ�������ģ��<br>
///

#include <xsys_oci.h>

#include <xlist.h>

using namespace oralib;

#ifndef XOCA_MAX_ITEMS
#define XOCA_MAX_ITEMS	512
#endif

/// \class xdbtable
/// \brief һ�����ݿ�������ģ����
/// ���ھ���ı�,�������Զ���Ľṹ,���ڴ�ģ������
template<class T>
class xdbtable{
public :
	xdbtable();	/// ���캯��
	/// ���캯��
	/// \param pdb ���ݿ�����
	xdbtable(connection * pdb);
	/// ���캯��
	/// \param pdb ���ݿ����Ӵ�
	xdbtable(const char * pdsn);

	~xdbtable();	/// ����

	void operator = (const T & s);	/// ��ֵ��������
	void setdata(const T & s);		/// ��ֵ��������
	void getdate(T & d);			/// ȡֵ

	void setdb(connection * pdb);	/// �������Ӵ�
	bool open(const char * pdsn = 0);	/// �����ݿ�����
	void close();	/// �رղ��ͷ���Դ

	/// �����¼�¼�����ݿ���
	/// \param s ��¼����
	/// \param bautocommit �Ƿ��Զ��ύ
	virtual int add(const T & s, bool bautocommit = false) = 0;
	virtual int get(T & s) = 0;		/// ȡ��1����¼
	virtual int del(const T & s) = 0; /// ɾ��
	virtual int upd(const T & s) = 0; /// �޸�
	virtual int fetch(T & s) = 0; /// ȡ�ü�¼

	int  select();	/// ִ��ȡ��¼������
	bool next();	/// ȡ��¼������һ����¼
	int  endexec();	/// ִֹͣ��

protected :
	/// ȡ�õ�ǰ��¼�����
	/// \param items ��ż�¼��������
	/// \param maxitems	��󳤶�
	/// \return ȡ�õļ�¼��
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


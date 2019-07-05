#ifndef _X_LIST_H_
#define _X_LIST_H_

/////////////////////////////////////////////////////////////////////////////////
// author: Liangjing 2001-10-31 +,.,n
//
// goal  : 一个void * 的list,可以存储比较少的数据,以实现简单的容器
//
// note  : 几个参数说明
//         count, 总个数
//         step , 增长速度,如果空间不够,会重新分配空间,增长的速度为step
//
// 增加:
// Xlist_s	// 为简单类型,如int,float,double等设
// Xlist_p	// 为保存new的指针设置,在释放时,可以用free_all(false)规定不释放指针所指对象,
//			   满足只存放引用而未分配空间的对象使用.
//             而free_all()释放指针所指对象.
//             所以要不释放对象,务必先调用free_all(false)再delete
// 修正:
// 原来的有的版本,add方法中的增长速度只有8,而不论nstep的设置
//
/////////////////////////////////////////////////////////////////////////////////

template<class T>
class xlist{
public:
	xlist();
	~xlist();

	int init(int ncount, int nstep);	// 初始化分配,ncount为初始空间,nstep为增长速度
	int down();							// free空间

	bool add(const T * h);			// 加入
	bool del(int i);			// 删除第i个位置
	bool insert(T * h, int i);	// 插入

	bool merge(xlist<T> * plist);
	bool merge(T * pTs, int n);

	int  free_all();			// 删除所有

	T * get(int i);				// 取得第i个位置

	T & operator[] (int i)  {  return m_phandles[i];  }

	T * m_phandles;				// 数组
	int m_count, m_all, m_nstep;	// 当前空间长度,总长度,增长速度

protected:
	bool increate_a_step(void);
};
template <class T>
xlist<T>::xlist()
	: m_phandles(0)
{
	m_count = m_all = 0;  m_nstep = 8;
}

template <class T>
xlist<T>::~xlist()
{
	down();
}

template <class T>
int xlist<T>::init(int ncount, int nstep)
{
	down();

	if (ncount < 1 || ncount > 32 * 1024)
		ncount = 8;
	if (nstep < 1 || nstep > 32 * 1024)
		nstep = 8;

	m_phandles = (T *)calloc(ncount, sizeof(T));
	if (m_phandles == 0)
		return -1;
	m_count = 0;
	m_all = ncount;
	m_nstep = nstep;

	return 0;
}

template <class T>
int xlist<T>::down()
{
	free_all();
	m_nstep = 8;

	return 0;
}

template <class T>
bool xlist<T>::increate_a_step(void)
{
	int l = m_count + m_nstep;

	// 保证数据正确性的检查、调整
	if (l == 0){
		if (m_nstep < 8)
			m_nstep = l = 8;
		m_count = 0;
	}

	// 分配空间
	T * p;
	if (m_phandles == 0)
		p = (T *)calloc(l, sizeof(T));
	else
		p = (T *)realloc(m_phandles, sizeof(T) * l);

	if (p == 0)  return false;

	// 修改
	m_all = l;
	m_phandles = p;
	
	return true;
}

template <class T>
bool xlist<T>::add(const T * h)
{
	if (h == 0)  return true;

	if (m_all <= m_count){
		if (!increate_a_step())
			return false;
	}
	
	// 存放数据
	memcpy(m_phandles + m_count, h, sizeof(T));
	m_count ++;

	return true;
}

template <class T>
bool xlist<T>::insert(T * h, int i)
{
	if (h == 0)  return true;

	if (i > m_count)
		i = m_count;

	if (m_all <= m_count){
		if (!increate_a_step())
			return false;
	}
	if (m_count == 0)
		i = 0;
	else{
		if (i < 0)
			i = 0;
		else if (i > m_count - 1)
			i = m_count;

		if (i < m_count)
			memmove(m_phandles + i + 1, m_phandles + i, sizeof(T) * (m_count - i));
	}

	memcpy(m_phandles + i, h, sizeof(T));
	m_count ++;

	return true;
}

template <class T>
bool xlist<T>::del(int i)
{
	if (m_phandles == 0 || i < 0 || i >= m_count)  return false;

	m_count --;
	if (i < m_count)
		memmove(m_phandles + i, m_phandles + i + 1, (m_count - i) * sizeof(T));

	return true;
}

template <class T>
T * xlist<T>::get(int i)
{
	if (i >= 0 && i < m_count)
		return m_phandles + i;
	return 0;
}

template <class T>
int xlist<T>::free_all()
{
	if (m_phandles){
		free(m_phandles);
		m_phandles = 0;
	}
	m_all = m_count = 0;

	return 0;
}

template <class T>
bool xlist<T>::merge(xlist<T> * plist)
{
	if (plist->m_count <= 0)  return true;

	int n = m_count + plist->m_count;
	T * p;
	if (n > m_all){
		p = (T *)realloc(m_phandles, sizeof(T) * (n + m_nstep));
		if (p == 0)
			return false;
		m_all = n + m_nstep;
	}else
		p = m_phandles;

	memcpy(p + m_count, plist->m_phandles, plist->m_count * sizeof(T));

	m_phandles = p;
	m_count = n;

	return true;
}

template <class T>
bool xlist<T>::merge(T * pTs, int nitems)
{
	if (nitems <= 0)  return true;
	
	int n = m_count + nitems;
	T * p;
	if (n > m_all){
		p = (T *)realloc(m_phandles, sizeof(T) * (n + m_nstep));
		if (p == 0)
			return false;
		m_all = n + m_nstep;
	}else
		p = m_phandles;
	
	memcpy(p + m_count, pTs, nitems * sizeof(T));
	
	m_phandles = p;
	m_count = n;
	
	return true;
}

/////////////////////////////////////////////////////////
// list for simple type (char bool long float ...)
/////////////////////////////////////////////////////////

template<class T>
class xlist_s{
public:
	xlist_s();
	~xlist_s();

	int init(int ncount, int nstep);	// 初始化分配,ncount为初始空间,nstep为增长速度
	int down();							// free空间

	bool add(T v);					// 加入
	bool del(int i);				// 删除第i个位置
	bool insert(T v, int i);		// 插入

	bool merge(xlist_s<T> * plist);

	int  free_all();				// 删除所有

	T get(int i);					// 取得第i个位置

	T operator[] (int i)  {  return get(i);  }

	T * m_phandles;					// 数组
	int m_count, m_all, m_nstep;	// 当前空间长度,总长度,增长速度
};
template <class T>
xlist_s<T>::xlist_s()
: m_phandles(0)
{
	m_count = m_all = 0;  m_nstep = 8;
}

template <class T>
xlist_s<T>::~xlist_s()
{
	down();
}

template <class T>
int xlist_s<T>::init(int ncount, int nstep)
{
	if (ncount < 1 || ncount > 32 * 1024)
		ncount = 8;
	if (nstep < 1 || nstep > 32 * 1024)
		nstep = 8;

	if (m_phandles)
		free(m_phandles);
	m_phandles = (T *)calloc(ncount, sizeof(T));
	if (m_phandles == 0)
		return -1;
	m_count = 0;
	m_all = ncount;
	m_nstep = nstep;

	return 0;
}

template <class T>
int xlist_s<T>::down()
{
	free_all();
	m_nstep = 8;

	return 0;
}

template <class T>
bool xlist_s<T>::add(T v)
{
	if (m_all <= m_count){
		int l = m_count + m_nstep;
		T * p = (T *)realloc(m_phandles, sizeof(T) * l);
		if (p == 0)  return false;

		m_all = l;
		m_phandles = p;
	}
	m_phandles[m_count++] = v;

	return true;
}

template <class T>
bool xlist_s<T>::insert(T v, int i)
{
	if (i >= m_count)
		i = m_count;

	if (m_all <= m_count){
		int l = m_count + m_nstep;
		T * p = (T *)realloc(m_phandles, sizeof(T) * l);
		if (p == 0)  return false;

		m_all = l;
		m_phandles = p;
	}
	if (i < m_count)
		memmove(m_phandles + i + 1, m_phandles + i, sizeof(T) * (m_count - i));

	m_phandles[i] = v;
	m_count ++;

	return true;
}

template <class T>
bool xlist_s<T>::del(int i)
{
	if (m_phandles == 0 || i < 0 || i >= m_count)  return false;

	m_count --;
	if (i < m_count)
		memmove(m_phandles + i, m_phandles + i + 1, (m_count - i) * sizeof(T));

	return true;
}

template <class T>
T xlist_s<T>::get(int i)
{
	if (i >= 0 && i < m_count)
		return m_phandles[i];
	return 0;
}

template <class T>
int xlist_s<T>::free_all()
{
	if (m_phandles){
		free(m_phandles);
		m_phandles = 0;
	}
	m_all = m_count = 0;

	return 0;
}

template <class T>
bool xlist_s<T>::merge(xlist_s<T> * plist)
{
	if (plist->m_count == 0)  return true;

	int n = m_count + plist->m_count;
	T * p;
	if (n > m_all){
		p = (T *)realloc(m_phandles, sizeof(T) * (n + m_nstep));
		if (p == 0)
			return false;
		m_all = n + m_nstep;
	}else
		p = m_phandles;

	memcpy(p + m_count, plist->m_phandles, plist->m_count * sizeof(T));

	m_phandles = p;
	m_count = n;

	return true;
}

////////////////////////////////////////////////
// list for pointer type (struct * , char * ...)
////////////////////////////////////////////////

template<class T>
class xlist_p{
public:
	xlist_p();
	~xlist_p();

	int init(int ncount, int nstep);	// 初始化分配,ncount为初始空间,nstep为增长速度
	int down();							// free空间

	bool add(T v);					// 加入
	bool del(int i);				// 删除第i个位置
	bool insert(T v, int i);		// 插入

	bool merge(xlist_p<T> * plist);

	int  free_all(bool delobj = true);	// 删除所有

	T get(int i);					// 取得第i个位置

	T operator[] (int i)  {  return get(i);  }

	T * m_phandles;					// 数组
	int m_count, m_all, m_nstep;	// 当前空间长度,总长度,增长速度
};
template <class T>
xlist_p<T>::xlist_p()
: m_phandles(0)
{
	m_count = m_all = 0;  m_nstep = 8;
}

template <class T>
xlist_p<T>::~xlist_p()
{
	down();
}

template <class T>
int xlist_p<T>::init(int ncount, int nstep)
{
	if (m_count > 0)
		free_all();
	else if (m_phandles)
		delete[] m_phandles;

	if (ncount < 1 || ncount > 32 * 1024)
		ncount = 8;
	if (nstep < 1 || nstep > 32 * 1024)
		nstep = 8;

	m_phandles = new T[m_count];
	if (m_phandles == 0)
		return -1;
	m_count = 0;
	m_all = ncount;
	m_nstep = nstep;

	return 0;
}

template <class T>
int xlist_p<T>::down()
{
	free_all();
	m_nstep = 8;
	return 0;
}

template <class T>
bool xlist_p<T>::add(T v)
{
	if (m_all <= m_count){
		int l = m_count + m_nstep;
		T * p = new T[l];
		if (p == 0)  return false;

		if (m_phandles){
			memmove(p, m_phandles, sizeof(T) * m_all);
			delete[] m_phandles;
		}
		m_all = l;
		m_phandles = p;
	}
	m_phandles[m_count++] = v;

	return true;
}

template <class T>
bool xlist_p<T>::insert(T v, int i)
{
	if (i >= m_count)
		i = m_count;

	if (m_all <= m_count){
		int l = m_count + m_nstep;
		T * p = new T[l];
		if (p == 0)  return false;

		if (m_phandles){
			if (i > 0)
				memmove(p, m_phandles, sizeof(T) * i);
			if (i < m_count)
				memmove(p + i + 1, m_phandles + i, sizeof(T) * (m_count - i));
			delete[] m_phandles;
		}
		m_all = l;
		m_phandles = p;
	}else{
		if (i < m_count)
			memmove(m_phandles + i + 1, m_phandles + i, sizeof(T) * (m_count - i));
	}
	m_phandles[i] = v;
	m_count ++;

	return true;
}

template <class T>
bool xlist_p<T>::del(int i)
{
	if (m_phandles == 0 || i < 0 || i >= m_count)  return false;

	if (m_phandles[i]) {
		delete[] m_phandles[i];
	}
	m_count --;
	if (i < m_count)
		memmove(m_phandles + i, m_phandles + i + 1, (m_count - i) * sizeof(T));

	return true;
}

template <class T>
T xlist_p<T>::get(int i)
{
	if (i >= 0 && i < m_count)
		return m_phandles[i];
	return 0;
}

template <class T>
int xlist_p<T>::free_all(bool delobj)
{
	if (m_phandles){
		if (delobj) {
			for (int i = 0; i < m_count; i++){
				if (m_phandles[i]) {
					delete m_phandles[i];
				}
			}
		}
		delete[] m_phandles;
		m_phandles = 0;
	}
	m_all = m_count = 0;

	return 0;
}

template <class T>
bool xlist_p<T>::merge(xlist_p<T> * plist)
{
	if (plist->m_count == 0)  return true;

	int n = m_count + plist->m_count;

	T * p = new T[n + m_nstep];
	if (p == 0)
		return false;

	if (m_count > 0)
		memcpy(p, m_phandles, m_count * sizeof(T));
	memcpy(p + m_count, plist->m_phandles, plist->m_count * sizeof(T));

	if (m_phandles)
		delete[] m_phandles;
	m_phandles = p;
	m_count = n;
	m_all = n + m_nstep;

	return true;
}

#endif // _X_LIST_H_


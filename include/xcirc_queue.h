#pragma once

#ifndef MAX_NODE
#define MAX_NODE 1024
#endif // MAX_NODE

#include <xsys.h> 

/*! \class circqueue
 *  循环队列类
 */
template<class T>
class  xcirc_queue{
public:
	xcirc_queue();
	~xcirc_queue();

	int 	open(int len = 512, int timeout_ms = 30000);
	void	close(void);

	int		in(T * x);
	int		out(T **cp);
	int		out(T * x);

	bool	isempty(void);
	bool	isfull(void);

protected:
	int		m_len;			/// 队列缓冲长度
	int		m_timeout_ms;	/// 取数据等待超时毫秒数

private:
	T *		m_pbuffer;	/// 队列
	int		m_head;		/// 队头
	int		m_tail;		/// 队尾
	
	xsys_semaphore	* m_psem;
	xsys_mutex * m_hmutex;
};

template<class T>
xcirc_queue<T>::xcirc_queue()
	: m_len(512)
	, m_timeout_ms(30000)
{
	m_head = 0;
	m_tail = 0;
	m_psem = 0;
	m_hmutex = 0;
}

template<class T>
xcirc_queue<T>::~xcirc_queue()
{
	close();
}

template<class T>
int xcirc_queue<T>::open(int len, int timeout_ms)
{
	if (len >= 16)
		m_len = len;
	if (timeout_ms >= 2000)
		m_timeout_ms = timeout_ms;

	m_pbuffer = (T *)malloc(sizeof(T) * m_len);
	if (m_pbuffer == 0)
		return -1;

	memset(m_pbuffer, 0x0, m_len * sizeof(T));

	m_psem = new xsys_semaphore;
	m_hmutex = new xsys_mutex;
	m_hmutex->init();
	
	return m_psem->init(0, m_len);
}

template<class T>
void xcirc_queue<T>::close(void)
{
	if (m_psem){
		m_psem->down();
		delete m_psem;
		m_psem = 0;
	}
	if (m_hmutex) {
		m_hmutex->down();
		delete m_hmutex;
		m_hmutex =0;
	}
	
	if (m_pbuffer){
		free(m_pbuffer);  m_pbuffer = 0;
	}
}

template<class T>
int xcirc_queue<T>::in(T * x)
{
	int r;
	if ((r = m_hmutex->lock(m_timeout_ms)) != 0)
		return r;

	if (isfull()){
		m_hmutex->unlock();
		return -1;
	}

	//printf("in : head[%d],tail[%d],id[%d]\n", m_head, m_tail, x.id);
	memcpy(m_pbuffer + m_tail, x, sizeof(T));

	m_tail = (m_tail + 1) % m_len;
	//printf("in : tail[%d] end.\n", m_tail);

	m_psem->V();
	
	m_hmutex->unlock();
	return 0;
}

template<class T>
int xcirc_queue<T>::out(T **cp)
{
	T * p = 0;

	if (m_psem->P(m_timeout_ms) != 0)
		return -1;

	m_hmutex->lock(-1);

	if (isempty())
	{
		m_hmutex->unlock();
		return -1;
	}

	//printf("out : head[%d],tail[%d]\n", m_head, m_tail);
	p = new T;
	memcpy(p, m_pbuffer+m_head, sizeof(T));
	*cp = p;

	m_head = (m_head + 1) % m_len;

	//printf("out : head[%d],id[%d]\n", m_head, p->id);
	
	m_hmutex->unlock();
	return 0;
}

template<class T>
int xcirc_queue<T>::out(T * x)
{
	T * p = 0;

	if (m_psem->P(m_timeout_ms) != 0)
		return -1;

	m_hmutex->lock(-1);

	if (isempty()){
		m_hmutex->unlock();
		return -1;
	}

	//printf("out : head[%d],tail[%d]\n", m_head, m_tail);
	memcpy(x, m_pbuffer+m_head, sizeof(T));

	m_head = (m_head + 1) % m_len;

	//printf("out : head[%d],id[%d]\n", m_head, p->id);
	m_hmutex->unlock();

	return 0;
}

template<class T>
bool xcirc_queue<T>::isempty(void)
{
	//printf("isempty : head[%d],tail[%d]\n", m_head, m_tail);
	return (m_head == m_tail);
}

template<class T>
bool xcirc_queue<T>::isfull(void)
{
	//printf("isfull : head[%d],tail[%d]\n", m_head, m_tail);
	return ((m_tail + 1) % m_len == m_head);
}

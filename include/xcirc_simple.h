#pragma once

/*! \class circqueue
 *  循环队列类
 */
template<class T>
class  xcirc_simple{
public:
	xcirc_simple();
	~xcirc_simple();

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
	volatile int	m_head;		/// 队头
	volatile int	m_tail;		/// 队尾
};

template<class T>
xcirc_simple<T>::xcirc_simple()
	: m_len(512)
	, m_timeout_ms(30000)
	, m_pbuffer(0)
{
	m_head = 0;
	m_tail = 0;
}

template<class T>
xcirc_simple<T>::~xcirc_simple()
{
	close();
}

template<class T>
int xcirc_simple<T>::open(int len, int timeout_ms)
{
	if (len >= 16)
		m_len = len;

	if (timeout_ms >= 2000)
		m_timeout_ms = timeout_ms;

	m_pbuffer = (T *)calloc(m_len, sizeof(T));

	if (m_pbuffer == 0)
		return -1;

	return 0;
}

template<class T>
void xcirc_simple<T>::close(void)
{
	if (m_pbuffer){
		free(m_pbuffer);  m_pbuffer = NULL;
	}
	m_head = m_tail = 0;
}

template<class T>
int xcirc_simple<T>::in(T * x)
{
	if (isfull()){
		return -1;
	}

	//printf("in : head[%d],tail[%d],id[%d]\n", m_head, m_tail, x.id);
	memcpy(m_pbuffer + m_tail, x, sizeof(T));

	m_tail = (m_tail + 1) % m_len;
	//printf("in : tail[%d] end.\n", m_tail);

	return 0;
}

template<class T>
int xcirc_simple<T>::out(T **cp)
{
	T * p = 0;

	if (isempty())
	{
		return -1;
	}

	//printf("out : head[%d],tail[%d]\n", m_head, m_tail);
	p = new T;
	memcpy(p, m_pbuffer+m_head, sizeof(T));
	*cp = p;

	m_head = (m_head + 1) % m_len;

	//printf("out : head[%d],id[%d]\n", m_head, p->id);
	return 0;
}

template<class T>
int xcirc_simple<T>::out(T * x)
{
	if (isempty()){
		return -1;
	}

	//printf("out : head[%d],tail[%d]\n", m_head, m_tail);
	memcpy(x, m_pbuffer+m_head, sizeof(T));

	m_head = (m_head + 1) % m_len;

	//printf("out : head[%d],id[%d]\n", m_head, p->id);
	return 0;
}

template<class T>
bool xcirc_simple<T>::isempty(void)
{
	//printf("isempty : head[%d],tail[%d]\n", m_head, m_tail);
	return (m_head == m_tail);
}

template<class T>
bool xcirc_simple<T>::isfull(void)
{
	//printf("isfull : head[%d],tail[%d]\n", m_head, m_tail);
	return ((m_tail + 1) % m_len == m_head);
}

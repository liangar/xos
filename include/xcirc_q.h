#pragma once

#include <stdlib.h>

/*! \class circqueue
 *  循环队列类
 */
template<class T>
class  xcirc_q{
public:
	xcirc_q();
	~xcirc_q();

	int 	open(int len = 512, int timeout_ms = 30000);
	void	close(void);

	int		put(T * x);
	int		get(T **cp);
	int		get(T * x);

	bool	isempty(void);
	bool	isfull(void);

protected:
	int		len_;			/// 队列缓冲长度

private:
	T *		pbuffer_;	/// 队列
	volatile int	head_;		/// 队头
	volatile int	tail_;		/// 队尾
};

template<class T>
xcirc_q<T>::xcirc_q()
	: len_(512)
	, pbuffer_(nullptr)
{
	head_ = 0;
	tail_ = 0;
}

template<class T>
xcirc_q<T>::~xcirc_q()
{
	close();
}

template<class T>
int xcirc_q<T>::open(int len, int timeout_ms)
{
	if (len >= 8)
		len_ = len;

	pbuffer_ = (T *)calloc(len_, sizeof(T));
	if (pbuffer_ == 0)
		return -1;

	return 0;
}

template<class T>
void xcirc_q<T>::close(void)
{
	if (pbuffer_){
		free(pbuffer_);  pbuffer_ = 0;
	}
	head_ = tail_ = 0;
}

template<class T>
int xcirc_q<T>::put(T * x)
{
	if (isfull())
		return -1;

	//printf("in : head[%d],tail[%d],id[%d]\n", head_, tail_, x.id);
	memcpy(pbuffer_ + tail_, x, sizeof(T));

	tail_ = (tail_ + 1) % len_;
	//printf("in : tail[%d] end.\n", tail_);

	return 0;
}

template<class T>
int xcirc_q<T>::get(T **cp)
{
	T * p = 0;

	if (isempty())
		return -1;

	//printf("get : head[%d],tail[%d]\n", head_, tail_);
	p = new T;
	memcpy(p, pbuffer_+head_, sizeof(T));
	*cp = p;

	head_ = (head_ + 1) % len_;

	//printf("get : head[%d],id[%d]\n", head_, p->id);

	return 0;
}

template<class T>
int xcirc_q<T>::get(T * x)
{
	if (isempty())
		return -1;

	//printf("get : head[%d],tail[%d]\n", head_, tail_);
	memcpy(x, pbuffer_+head_, sizeof(T));

	head_ = (head_ + 1) % len_;

	//printf("get : head[%d],id[%d]\n", head_, p->id);

	return 0;
}

template<class T>
bool xcirc_q<T>::isempty(void)
{
	//printf("isempty : head[%d],tail[%d]\n", head_, tail_);
	return (head_ == tail_);
}

template<class T>
bool xcirc_q<T>::isfull(void)
{
	//printf("isfull : head[%d],tail[%d]\n", head_, tail_);
	return ((tail_ + 1) % len_ == head_);
}

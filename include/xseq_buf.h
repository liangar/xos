#pragma once

#include <xsys.h>
#include <xcirc_simple.h>

struct xseq_buf_use{
	long	id;		/// 标识,使用者自定义
	char *	p;		/// 使用空间的起始地址
	int		len;	/// 长度
};

/// 顺序使用的缓冲区类
class xseq_buf
{
public:
	xseq_buf();
	~xseq_buf();

	/// bufsize : 缓冲区大小，K 为单位
	/// users : 最多消息包
	int init(int bufsize, int uses);
	int down(void);

	bool isempty(void);
	bool isfull(void);

	int put(long id, const char * s);	/// 放数据
	int put(long id, const void * s, int len);
	int get(long * id, void ** d);	/// 取出数据
	int get(long * id, void * d);
	int get(xseq_buf_use * use);

	int free_use(xseq_buf_use &use);

	int get_free(long * id, char * pdata);

	void set_timeout_ms(long ms)
	{
		if (ms < 10)  m_timeout_ms = 10;
		else
			m_timeout_ms = ms;
	}
	void bindto(xsys_semaphore * psem)  {  m_psem->bindto(psem);  }

protected:
	xcirc_simple<xseq_buf_use>	m_uses;	/// 对缓冲区的使用

	const char * m_last_alloc;

	char * m_pbuf;			int m_buf_size;			/// 缓冲区,优先使用尾,不够则用头
	char * m_p_head_free;	int m_head_free_size;	/// 可用头
	char * m_p_tail_free;	int m_tail_free_size;	/// 可用尾

protected:
	xsys_mutex * m_hmutex;	/// 使用缓冲的临界区
	xsys_semaphore	* m_psem;

	int		m_timeout_ms;	/// 取数据等待超时毫秒数,缺省 2000
};

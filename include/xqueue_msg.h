#pragma once

#include <xipc.h>
#include <xcirc_q.h>

struct xmsg_buf_use{
	long	id;		/// 标识,使用者自定义
	char *	p;		/// 使用空间的起始地址
	int		len;	/// 长度
};

/// 顺序使用的缓冲区类
class xqueue_msg
{
public:
	xqueue_msg();
	~xqueue_msg();

	/// bufsize : 缓冲区大小，K 为单位
	/// users : 最多消息包
	int init(int bufsize, int msg_count);
	int done(void);

	bool isempty(void);
	bool isfull(void);
	
	bool clear(void);

	int put(long id, const char * s, int timeout_ms = -1);	/// 放数据
	int put(long id, const void * s, int len, int timeout_ms = -1);

	int get(long * id, void ** d, int timeout_ms = -1);	/// 取出数据,并分配空间
	int get(long * id, void * d, int timeout_ms = -1);

protected:
	void init_vars(void);
	int get(xmsg_buf_use * msg);
	/// 取数据的存储结构
	int free_use(xmsg_buf_use &msg);


protected:
	xcirc_q<xmsg_buf_use>	msgs_;	/// 对缓冲区的使用

	const char * m_last_alloc;

	char * pbuf_;		int buf_size_;			/// 缓冲区,优先使用尾,不够则用头
	char * phead_free_;	int head_free_size_;	/// 可用头
	char * ptail_free_;	int tail_free_size_;	/// 可用尾

	xmutex		* hmutex_;	/// 使用缓冲的临界区
	xsemaphore	* psem_;
};

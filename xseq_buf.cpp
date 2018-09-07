#include <xseq_buf.h>

xseq_buf::xseq_buf()
	: m_hmutex(0)
	, m_pbuf(0)
	, m_p_head_free(0)
	, m_p_tail_free(0)
{
	m_buf_size = m_head_free_size = m_tail_free_size = 0;
	m_last_alloc = 0;
	m_timeout_ms = 2000;
	m_psem = 0;
}

xseq_buf::~xseq_buf()
{
	down();
}

void xseq_buf::init_vars(void)
{
	m_p_head_free = m_pbuf;
	m_head_free_size = m_buf_size;
	m_p_tail_free = m_pbuf+m_buf_size;
	m_tail_free_size = 0;

	m_last_alloc = m_p_head_free;
}

int xseq_buf::init(int bufsize, int uses)
{
	if (bufsize < 4)
		bufsize = 4;
	bufsize *= 1024;

	m_pbuf = (char *)malloc(bufsize);
	if (m_pbuf == 0)
		return -1;

	m_buf_size = bufsize;

	init_vars();

	int r = m_uses.open(uses);
	m_timeout_ms = 2000;

	m_psem = new xsys_semaphore;
	m_hmutex = new xsys_mutex;
	m_hmutex->init();

	return m_psem->init(0, uses);
}

bool xseq_buf::clear(void)
{
	if (m_hmutex->lock(1000) != 0)
		return false;

	if (m_uses.isempty()){
		init_vars();
	}

	m_hmutex->unlock();

	return true;
}

int xseq_buf::down(void)
{
	m_uses.close();
	if (m_pbuf){
		free(m_pbuf);  m_pbuf = 0;
		m_p_head_free = m_p_tail_free = 0;
		m_buf_size = m_head_free_size = m_tail_free_size = 0;
	}
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

	return 0;
}

bool xseq_buf::isempty(void)
{
	bool b;

	m_hmutex->lock(1000);
	b = m_uses.isempty();
	m_hmutex->unlock();

	return b;
}

bool xseq_buf::isfull(void)
{
	bool b;

	m_hmutex->lock(1000);
	b = m_uses.isfull();
	m_hmutex->unlock();

	return b;
}

/// 放数据
int xseq_buf::put(long id, const char * s)
{
	if (s == 0 || *s == 0)
		return 0;

	return put(id, s, strlen(s)+1);
}

int xseq_buf::put(long id, const void * s, int len)
{
	// 2016-06-06 len < 1 -> < 0 liangar
	// 允许 0 长度的数据入列
	if ((s == 0 && len > 0) || len < 0)
		return 0;

	int r;
	/// 优先从尾部,找空闲空间
	xseq_buf_use	use;
	use.id = id;
	use.len = len;

	if ((r = m_hmutex->lock(m_timeout_ms)) != 0)
		return r;

	if (m_uses.isfull()){
		m_hmutex->unlock();
		return -1;
	}

	if (m_last_alloc == m_p_tail_free && m_tail_free_size > len){
		use.p = m_p_tail_free;
		m_p_tail_free += use.len;
		m_tail_free_size -= use.len;
	}else if (m_head_free_size > use.len){
		use.p = m_p_head_free;
		m_p_head_free += use.len;
		m_head_free_size -= use.len;
	}else
		use.p = 0;
	if (use.p){
		if (use.len > 0)
			memcpy(use.p, s, use.len);
		m_uses.in(&use);

		m_last_alloc = use.p + use.len;
	}else
		use.len = 0;

	m_psem->V();

	m_hmutex->unlock();

	return use.len;
}

int xseq_buf::free_use(xseq_buf_use &use)
{
	if (use.p == 0 || use.len == 0)
		return 0;

	m_hmutex->lock(1000);

	if (use.p < m_p_head_free){	/// 释放 < head 的空间
		m_p_tail_free = m_p_head_free;
		m_tail_free_size = m_head_free_size;

		m_p_head_free = use.p;
		m_head_free_size = use.len;
	}else{	/// 不然，一定是在 head 之后的空间
		m_head_free_size += use.len;
	}

	/// 如果追上了就合并
	if (m_p_head_free + m_head_free_size == m_p_tail_free){
		m_head_free_size += m_tail_free_size;

		/// 重置 tail
		m_p_tail_free = m_pbuf+m_buf_size, m_tail_free_size = 0;
	}

	m_hmutex->unlock();

	return use.len;
}

/// 取出数据
int xseq_buf::get(long * id, void ** d)
{
	xseq_buf_use use;

	int len = get(&use);
	if (len <= 0)
		return len;

	if (d){
		*d = malloc(use.len);
		memcpy(*d, use.p, use.len);
	}

	if (id)
		*id = use.id;

	return free_use(use);
}

int xseq_buf::get(long * id, void * d)
{
	xseq_buf_use use;

	int len = get(&use);
	if (len <= 0)
		return len;

	if (d)
		memcpy(d, use.p, use.len);

	if (id)
		*id = use.id;

	return free_use(use);
}

int xseq_buf::get(xseq_buf_use * use)
{
	memset(use, 0, sizeof(xseq_buf_use));

	if (m_psem->P(m_timeout_ms) != 0)
		return -1;

	m_hmutex->lock(1000);

	if (m_uses.isempty()){
		m_hmutex->unlock();
		return -1;
	}

	if (m_uses.out(use)){
		m_hmutex->unlock();
		return -1;
	}
	m_hmutex->unlock();

	return use->len;
}

int xseq_buf::get_free(long * id, char * pdata)
{
	xseq_buf_use use;

	ZeroData(use);

	int r = m_psem->P(m_timeout_ms);
	if (r != 0)
		return r;

	m_hmutex->lock(1000);

	if (m_uses.isempty()){
		m_hmutex->unlock();
		return -1;
	}

	if (m_uses.out(&use)){
		m_hmutex->unlock();
		return -1;
	}
	*id = use.id;
	if (use.len > 0){
		memcpy(pdata, use.p, use.len);	pdata[use.len] = 0;
	}

	m_hmutex->unlock();

	free_use(use);

	return use.len;
}

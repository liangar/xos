#include <xqueue_msg.h>

xqueue_msg::xqueue_msg()
	: hmutex_(nullptr)
	, pbuf_(nullptr)
	, phead_free_(nullptr)
	, ptail_free_(nullptr)
{
	buf_size_ = head_free_size_ = tail_free_size_ = 0;
	m_last_alloc = 0;
	psem_ = nullptr;
}

xqueue_msg::~xqueue_msg()
{
	done();
}

void xqueue_msg::init_vars(void)
{
	phead_free_ = pbuf_;
	head_free_size_ = buf_size_;
	ptail_free_ = pbuf_+buf_size_;
	tail_free_size_ = 0;

	m_last_alloc = phead_free_;
}

int xqueue_msg::init(int bufsize, int msg_count)
{
	if (bufsize < 4)
		bufsize = 4;
	bufsize *= 1024;

	pbuf_ = (char *)calloc(bufsize+sizeof(int), 1);
	if (pbuf_ == 0)
		return -1;

	buf_size_ = bufsize;

	init_vars();

	int r = msgs_.open(msg_count);
	if (r < 0){
		::free(pbuf_);  pbuf_ = nullptr;
		return r;
	}

	psem_ = new xsemaphore;
	hmutex_ = new xmutex;

	return psem_->init(0, msg_count);
}

bool xqueue_msg::clear(void)
{
	if (hmutex_->lock(2000) == std::cv_status::timeout)
		return false;

	if (msgs_.isempty()){
		init_vars();
	}

	hmutex_->unlock();

	return true;
}

int xqueue_msg::done(void)
{
	msgs_.close();
	if (pbuf_){
		::free(pbuf_);  pbuf_ = nullptr;
		phead_free_ = ptail_free_ = nullptr;
		buf_size_ = head_free_size_ = tail_free_size_ = 0;
	}
	if (psem_){
		delete psem_;
		psem_ = nullptr;
	}
	if (hmutex_) {
		delete hmutex_;
		hmutex_ =nullptr;
	}

	return 0;
}

bool xqueue_msg::isempty(void)
{
	return msgs_.isempty();
}

bool xqueue_msg::isfull(void)
{
	return msgs_.isfull();
}

/// 放数据
int xqueue_msg::put(long id, const char * s, int timeout_ms)
{
	if (s == 0 || *s == 0)
		return 0;

	return put(id, s, strlen(s)+1, timeout_ms);
}

int xqueue_msg::put(long id, const void * s, int len, int timeout_ms)
{
	// 2016-06-06 len < 1 -> < 0 liangar
	// 允许 0 长度的数据入列
	if ((s == 0 && len > 0) || len < 0 || hmutex_ == nullptr)
		return 0;

	/// 优先从尾部,找空闲空间
	xmsg_buf_use	msg;
	msg.id = id;
	msg.len = len;

	if (hmutex_->lock(timeout_ms) == std::cv_status::timeout)
		return -1;

	if (msgs_.isfull()){
		hmutex_->unlock();
		return -2;
	}

	if (m_last_alloc == ptail_free_ && tail_free_size_ > len){
		msg.p = ptail_free_;
		ptail_free_ += msg.len;
		tail_free_size_ -= msg.len;
	}else if (head_free_size_ > msg.len){
		msg.p = phead_free_;
		phead_free_ += msg.len;
		head_free_size_ -= msg.len;
	}else
		msg.p = 0;
	if (msg.p){
		if (msg.len > 0)
			memcpy(msg.p, s, msg.len);
		msgs_.put(&msg);

		m_last_alloc = msg.p + msg.len;
	}else
		msg.len = 0;

	hmutex_->unlock();

	psem_->V();

	return msg.len;
}

int xqueue_msg::free_use(xmsg_buf_use &msg)
{
	if (msg.p == 0 || msg.len == 0)
		return 0;

	if (msg.p < phead_free_){	/// 释放 < head 的空间
		ptail_free_ = phead_free_;
		tail_free_size_ = head_free_size_;

		phead_free_ = msg.p;
		head_free_size_ = msg.len;
	}else{	/// 不然，一定是在 head 之后的空间
		head_free_size_ += msg.len;
	}

	/// 如果追上了就合并
	if (phead_free_ + head_free_size_ == ptail_free_){
		head_free_size_ += tail_free_size_;

		/// 重置 tail
		ptail_free_ = pbuf_+buf_size_, tail_free_size_ = 0;
	}

	return msg.len;
}

/// 取出数据
int xqueue_msg::get(long * id, void ** d, int timeout_ms)
{
	xmsg_buf_use msg;

	if (psem_->P(timeout_ms) == std::cv_status::timeout)
		return -1;

	hmutex_->lock(2000);

	int len = get(&msg);
	if (len <= 0){
		hmutex_->unlock();
		return len;
	}

	if (d){
		*d = malloc(msg.len);
		memcpy(*d, msg.p, msg.len);
	}

	if (id)
		*id = msg.id;

	free_use(msg);

	hmutex_->unlock();

	return len;
}

int xqueue_msg::get(long * id, void * d, int timeout_ms)
{
	xmsg_buf_use msg;

	if (psem_->P(timeout_ms) == std::cv_status::timeout)
		return -1;

	hmutex_->lock(2000);

	int len = get(&msg);
	if (len <= 0){
		hmutex_->unlock();
		return len;
	}

	if (d)
		memcpy(d, msg.p, msg.len);

	if (id)
		*id = msg.id;

	free_use(msg);

	hmutex_->unlock();
	
	return len;
}

int xqueue_msg::get(xmsg_buf_use * msg)
{
	memset(msg, 0, sizeof(xmsg_buf_use));

	if (msgs_.isempty()){
		return -1;
	}

	if (msgs_.get(msg)){
		return -2;
	}

	return msg->len;
}

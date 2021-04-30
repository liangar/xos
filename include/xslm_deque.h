#pragma once
// #include <stdio.h>
// #include <stdlib.h>

/*! \class xslm_deque
 *  固定分配长度内存的双向队列
 *  
 *  该类按输入参数，分配固定空间，建立2个双向队列:
 *  free / used
 *  初始时，所有项都属于 free（空闲）
 *  使用时，先调用 get 从 free 中取 1 个项
 *          修改值
 *          然后用 add 加入到 used 队列中
 *  释放时，用 del 方法，可以传入索引，也可以用值匹配
 *
 */
template<typename T>
class  xslm_deque{

	struct xslm_index{
		int i_pre, i_post;	// 前1个，后1个
	};

public:
	xslm_deque();
	~xslm_deque();

	/// 打开队列
	/// @param max_item_count: [in], 队列的项数
	/// @return 队列项总数
	int open(int max_item_count);
	/// 释放
	void close(void);

	/// 从空闲队列中取出一个项
	/// @param x: [out] 存放索引的指针，以便于使用
	/// @return 项的索引位置, -1 没取到
	int	get(T ** x = nullptr);

	/// 将取出的项放到使用队列的尾部
	/// @param i: [in], get 所取得的索引位置
	/// @param x: [in], get 所取得的引用或空，若是其他值，则copy到i位置
	/// @return 队列当前的使用数, <0 未存放
	int	add(int i, const T * x = nullptr);

	/***
     * 放入一个外部项
	 * 等同 get 之后 add
	 * @param x: [in], get 所取得的引用或空，若是其他值，则copy到i位置
	 * @return 队列当前的使用数, <0 未存放
	 */ 
	int	add(T * x){
		T * p = nullptr;
		int i = get(&p);
		if (i < 0)  return i;
		return add(i, x);
	}

	/// 删除内部项
	/// @param i: [in], 需要删除的索引位置
	/// @return 0/1 = 索引位置非使用项 / 从使用队列中删除
	int	del(int i);

	/// 删除项
	/// 从使用队列中，找到删除项，删除
	/// @param x: [in], 需要删除的项
	/// @return 0/1 = 未找到删除项 / 从使用队列中删除
	int	del(const T * x);

	inline bool is_empty(void) {  return count_ == 0 || used_ == -1;  }
	inline bool is_full(void)  {  return count_ == amount_ || free_ == -1;  }

	inline int first(void)  {  return (count_ > 0? used_ : -1);  }
	inline int last(void)   {  return (count_ > 0? indexs_[used_].i_pre : -1);  }
	inline int post(int i)  {  return indexs_[i].i_post;  }
	inline int prev(int i)  {  return indexs_[i].i_pre;   }

	inline T * get_recs(void)  {  return buf_;  }

	/// show log
	void show(int fd);
	void show(FILE * pf);

public:
	int		amount_;/// 缓冲元素个数
	int 	count_;	/// 使用总数

	T *		buf_;	/// 存储缓冲

	xslm_index	* indexs_;	/// 队列索引
	int	free_,	/// 空闲队列索引位置
		used_;	/// 使用队列索引位置

protected:
	virtual	int compare(const T *t1, const T *t2) = 0;
	virtual void show_t(FILE * pf, const T * t) = 0;
};

template<class T>
xslm_deque<T>::xslm_deque()
	: amount_(0)
	, count_(0)
{
	buf_ = nullptr;
	indexs_ = nullptr;
	
	free_ = used_ = -1;
}

template<class T>
xslm_deque<T>::~xslm_deque()
{
	close();
}

template<class T>
int xslm_deque<T>::open(int max_item_count)
{
	T * buf = (T *)calloc(max_item_count, sizeof(T));
	if (buf == nullptr)
		return -1;
	
	indexs_ = (xslm_index * )malloc(max_item_count * sizeof(xslm_index));
	if (indexs_ == nullptr){
		free(buf);
		return -2;
	}
	
	amount_ = max_item_count;  buf_ = buf;

	// 初始化free_队列索引
	int i;
	free_ = 0;	// 空闲队列开始项，是0
	for (i = 0; i < amount_; ++i){
		indexs_[i].i_pre = i - 1;
		indexs_[i].i_post= i + 1;
	}
	indexs_[0].i_pre = amount_ - 1;
	indexs_[amount_-1].i_post = 0;
	
	used_ = -1;	// 使用列尚无项
	count_= 0;

	return amount_;
}

template<class T>
void xslm_deque<T>::close(void)
{
	if (buf_){
		free(buf_);  buf_ = nullptr;
	}
	if (indexs_){
		free(indexs_);  indexs_ = nullptr;
	}

	free_ = used_ = -1;
	
	count_ = 0;
}

template<class T>
int xslm_deque<T>::get(T ** x)
{
	if (is_full())
		return -1;
	
	int i = free_;	// free 的头一个index
	if (x)
		*x = buf_ + i;	// 取得项
	
	int i_post = indexs_[i].i_post,
		i_pre  = indexs_[i].i_pre;

	if (i_pre == i){
		free_ = -1;
	}else{
		xslm_index	*index_post = indexs_ + i_post,	// 取头一个的后项
					*index_pre  = indexs_ + i_pre;	// 取头一个的尾项
		index_post->i_pre = i_pre;
		index_pre->i_post = i_post;
		
		free_ = i_post;
	}

	indexs_[i].i_pre = indexs_[i].i_post = i;

	return i;
}

template<class T>
int xslm_deque<T>::add(int i, const T *x)
{
	if (i < 0 || i >= amount_){
		return -1;
	}

	if (x != nullptr && x != buf_ + i)
		memcpy(buf_+ i, x, sizeof(T));

	if (is_empty()){
		count_ = 1;
		used_ = i;
		indexs_[i].i_pre = indexs_[i].i_post = i;
		
		return count_;
	}

	int i_post = indexs_[used_].i_pre;
	indexs_[i].i_post= used_;
	indexs_[i_post].i_post = i;
	indexs_[i].i_pre = i_post;
	indexs_[used_].i_pre = i;

	++count_;
	
	return count_;
}

template<class T>
int xslm_deque<T>::del(int i)
{
	if (i < 0 || i >= amount_ || is_empty())
		return 0;

	xslm_index * x = indexs_ + i;
	// 从原队列拿下

	// 找到使用项
	int j;
	xslm_index * y = indexs_ + used_;
	for (j = 0; j < count_; ++j){
		if (y == x)
			break;
		y = indexs_ + y->i_post;
	}
		
	if (y != x)
		return 0;

	if (count_ == 1) {
		used_ = -1;
	}else{
		if (used_ == i)
			used_ = x->i_post;
		indexs_[x->i_post].i_pre = x->i_pre;
		indexs_[x->i_pre].i_post = x->i_post;
	}
	--count_;

	if (free_ == -1){
		free_ = i;
		indexs_[i].i_post = indexs_[i].i_pre = i;
	}else{
		// 放入free队列中
		int i_post = indexs_[free_].i_pre;	// free 的尾
		indexs_[i_post].i_post = i;
		indexs_[i].i_pre = i_post;
		indexs_[i].i_post= free_;
		indexs_[free_].i_pre = i;
	}

	return 1;
}

template<class T>
int xslm_deque<T>::del(const T * x)
{
	int i;
	int i_y = used_;
	for (i = 0; i < count_; i++){
		T * y = buf_ + i_y;
		if (compare(x, y) == 0)
			return del(i_y);
		
		i_y = indexs_[i_y].i_post;
	}
	
	return 0;
}

template<class T>
void xslm_deque<T>::show(int fd)
{
	FILE * pf = fdopen(fd, "w");
	show(pf);
	fclose(pf);
}

template<class T>
void xslm_deque<T>::show(FILE * f)
{
	if (f == nullptr){
		fputs("file open error.\n", f);  return;
	}

	fprintf(f, "empty: %d, full: %d, amount_=%d, count=%d\n\n", (is_empty()? 1:0), (is_full()? 1: 0), amount_, count_);

	int i;
	// show all items
	for (i = 0; i < amount_; i++){
		show_t(f, buf_ + i);
		fputc('\n', f);
	}
	fputc('\n', f);
	
	// show all free items
	fprintf(f, "free(%d):\n", amount_-count_);
	int k = free_;
	if (k >= 0) {
		i = 0;	// 计数
		do {
			xslm_index* idx = indexs_ + k;
			fprintf(f, "<%d, %d, %d> ", idx->i_pre, k, idx->i_post);
			if (++i % 10 == 0)
				fputc('\n', f);
			k = idx->i_post;
		} while (k != free_);
		fprintf(f, "all = %d\n", i);  fflush(f);
	}
	
	fprintf(f, "used(%d):\n", count_);
	k = used_;
	if (count_ > 0) {
		i = 0;
		do{
			xslm_index* idx = indexs_ + k;
			fprintf(f, "<%d, %d, %d> ", idx->i_pre, k, idx->i_post);
			if (++i % 10 == 0)
				fputc('\n', f);
			k = idx->i_post;
		} while (k != used_ && i < count_);
		fprintf(f, "all = %d\n", i);  fflush(f);
	}
}

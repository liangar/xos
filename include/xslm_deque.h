#pragma once
// #include <stdio.h>
// #include <stdlib.h>

/*! \class xslm_deque
 *  �̶����䳤���ڴ��˫�����
 *  
 *  ���ఴ�������������̶��ռ䣬����2��˫�����:
 *  free / used
 *  ��ʼʱ����������� free�����У�
 *  ʹ��ʱ���ȵ��� get �� free ��ȡ 1 ����
 *          �޸�ֵ
 *          Ȼ���� add ���뵽 used ������
 *  �ͷ�ʱ���� del ���������Դ���������Ҳ������ֵƥ��
 *
 */
template<typename T>
class  xslm_deque{

	struct xslm_index{
		int i_pre, i_post;	// ǰ1������1��
	};

public:
	xslm_deque();
	~xslm_deque();

	/// �򿪶���
	/// @param max_item_count: [in], ���е�����
	/// @return ����������
	int open(int max_item_count);
	/// �ͷ�
	void close(void);

	/// �ӿ��ж�����ȡ��һ����
	/// @param x: [out] ���������ָ�룬�Ա���ʹ��
	/// @return �������λ��, -1 ûȡ��
	int	get(T ** x = nullptr);

	/// ��ȡ������ŵ�ʹ�ö��е�β��
	/// @param i: [in], get ��ȡ�õ�����λ��
	/// @param x: [in], get ��ȡ�õ����û�գ���������ֵ����copy��iλ��
	/// @return ���е�ǰ��ʹ����, <0 δ���
	int	add(int i, const T * x = nullptr);

	/***
     * ����һ���ⲿ��
	 * ��ͬ get ֮�� add
	 * @param x: [in], get ��ȡ�õ����û�գ���������ֵ����copy��iλ��
	 * @return ���е�ǰ��ʹ����, <0 δ���
	 */ 
	int	add(T * x){
		T * p = nullptr;
		int i = get(&p);
		if (i < 0)  return i;
		return add(i, x);
	}

	/// ɾ���ڲ���
	/// @param i: [in], ��Ҫɾ��������λ��
	/// @return 0/1 = ����λ�÷�ʹ���� / ��ʹ�ö�����ɾ��
	int	del(int i);

	/// ɾ����
	/// ��ʹ�ö����У��ҵ�ɾ���ɾ��
	/// @param x: [in], ��Ҫɾ������
	/// @return 0/1 = δ�ҵ�ɾ���� / ��ʹ�ö�����ɾ��
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
	int		amount_;/// ����Ԫ�ظ���
	int 	count_;	/// ʹ������

	T *		buf_;	/// �洢����

	xslm_index	* indexs_;	/// ��������
	int	free_,	/// ���ж�������λ��
		used_;	/// ʹ�ö�������λ��

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

	// ��ʼ��free_��������
	int i;
	free_ = 0;	// ���ж��п�ʼ���0
	for (i = 0; i < amount_; ++i){
		indexs_[i].i_pre = i - 1;
		indexs_[i].i_post= i + 1;
	}
	indexs_[0].i_pre = amount_ - 1;
	indexs_[amount_-1].i_post = 0;
	
	used_ = -1;	// ʹ����������
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
	
	int i = free_;	// free ��ͷһ��index
	if (x)
		*x = buf_ + i;	// ȡ����
	
	int i_post = indexs_[i].i_post,
		i_pre  = indexs_[i].i_pre;

	if (i_pre == i){
		free_ = -1;
	}else{
		xslm_index	*index_post = indexs_ + i_post,	// ȡͷһ���ĺ���
					*index_pre  = indexs_ + i_pre;	// ȡͷһ����β��
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
	// ��ԭ��������

	// �ҵ�ʹ����
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
		// ����free������
		int i_post = indexs_[free_].i_pre;	// free ��β
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
		i = 0;	// ����
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

#pragma once

#include <xipc.h>
#include <xcirc_q.h>

struct xmsg_buf_use{
	long	id;		/// ��ʶ,ʹ�����Զ���
	char *	p;		/// ʹ�ÿռ����ʼ��ַ
	int		len;	/// ����
};

/// ˳��ʹ�õĻ�������
class xqueue_msg
{
public:
	xqueue_msg();
	~xqueue_msg();

	/// bufsize : ��������С��K Ϊ��λ
	/// users : �����Ϣ��
	int init(int bufsize, int msg_count);
	int done(void);

	bool isempty(void);
	bool isfull(void);
	
	bool clear(void);

	int put(long id, const char * s, int timeout_ms = -1);	/// ������
	int put(long id, const void * s, int len, int timeout_ms = -1);

	int get(long * id, void ** d, int timeout_ms = -1);	/// ȡ������,������ռ�
	int get(long * id, void * d, int timeout_ms = -1);

protected:
	void init_vars(void);
	int get(xmsg_buf_use * msg);
	/// ȡ���ݵĴ洢�ṹ
	int free_use(xmsg_buf_use &msg);


protected:
	xcirc_q<xmsg_buf_use>	msgs_;	/// �Ի�������ʹ��

	const char * m_last_alloc;

	char * pbuf_;		int buf_size_;			/// ������,����ʹ��β,��������ͷ
	char * phead_free_;	int head_free_size_;	/// ����ͷ
	char * ptail_free_;	int tail_free_size_;	/// ����β

	xmutex		* hmutex_;	/// ʹ�û�����ٽ���
	xsemaphore	* psem_;
};

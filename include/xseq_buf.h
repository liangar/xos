#pragma once

#include <xsys.h>
#include <xcirc_simple.h>

struct xseq_buf_use{
	long	id;		/// ��ʶ,ʹ�����Զ���
	char *	p;		/// ʹ�ÿռ����ʼ��ַ
	int		len;	/// ����
};

/// ˳��ʹ�õĻ�������
class xseq_buf
{
public:
	xseq_buf();
	~xseq_buf();

	/// bufsize : ��������С��K Ϊ��λ
	/// users : �����Ϣ��
	int init(int bufsize, int uses);
	int down(void);

	bool isempty(void);
	bool isfull(void);

	int put(long id, const char * s);	/// ������
	int put(long id, const void * s, int len);
	int get(long * id, void ** d);	/// ȡ������
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
	xcirc_simple<xseq_buf_use>	m_uses;	/// �Ի�������ʹ��

	const char * m_last_alloc;

	char * m_pbuf;			int m_buf_size;			/// ������,����ʹ��β,��������ͷ
	char * m_p_head_free;	int m_head_free_size;	/// ����ͷ
	char * m_p_tail_free;	int m_tail_free_size;	/// ����β

protected:
	xsys_mutex * m_hmutex;	/// ʹ�û�����ٽ���
	xsys_semaphore	* m_psem;

	int		m_timeout_ms;	/// ȡ���ݵȴ���ʱ������,ȱʡ 2000
};

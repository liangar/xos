#ifndef _xdev_comm_H_
#define _xdev_comm_H_

#define ERR_DEV_OFFLINE	-200001 // 设备不在线

class xdev_comm{
public:
	xdev_comm(const char * name, int recv_max_len);
	~xdev_comm();

	virtual int  init(const char * parms);
	virtual int  down(void);

	virtual int  open(const char * parms) = 0;
	virtual int  close(void) = 0;
	virtual bool isopen(void) = 0;

	virtual int send(const char* buf, int len) = 0;
	int  send_bytes(const char * buf, int len);

	int  sendstring(const char* buf);
	int  recv(char* buf, int maxlen, int msec);
	int  peek(char & c);
	int  peek(char * buf, int maxlen);
	void skip(int bytes);
	int  recv_bytes(char* buf, int maxlen, int seconds);
	int  recv_byend (char * buf, int maxlen, char * endstr, int seconds);
	int  recv_byend (char * buf, int maxlen, char endch, int seconds);

	void clear_all_recv(void);

	int  unread_bytes(void)  {  return m_recv_len;  }

	void set_brief_log(void) {  m_use_full_log = false;  }
	char m_name[64];

protected:
	void save_data(const char * buf, int len);
	void lock(void);
	void unlock(void);

protected:
	xsys_mutex  m_buf_lock;
	xsys_event  m_has_data;

	bool m_use_full_log;

	// 串口数据接收Buffer
	int  m_recv_len;
	int  m_recv_max_len;
	char * m_precv_buf;
};

#endif

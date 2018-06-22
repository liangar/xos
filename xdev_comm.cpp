#include <xsys.h>

#include <ss_service.h>
#include "xdev_comm.h"

xdev_comm::xdev_comm(const char * name, int recv_max_len)
{
	strcpy(m_name, name);
	m_recv_len = 0;
	m_recv_max_len = recv_max_len;
	m_precv_buf = 0;
	m_use_full_log = true;
}

xdev_comm::~xdev_comm()
{
}

int xdev_comm::init(const char * parms)
{
	if (m_precv_buf == 0)
		m_precv_buf = (char *)malloc(m_recv_max_len);

	if (!m_buf_lock.isopen())
		m_buf_lock.init();

	if (!m_has_data.isopen())
		m_has_data.init(1);

	return 0;
}

int xdev_comm::down(void)
{
	if (m_precv_buf){
		::free(m_precv_buf);  m_precv_buf = 0;
	}

	m_buf_lock.down();
	m_has_data.down();

	m_recv_len = 0;
	m_recv_max_len = 0;

	return 0;
}

int xdev_comm::sendstring(const char* buf)
{
	int l = strlen(buf);
	return send_bytes(buf, l);
}

int xdev_comm::send_bytes(const char * buf, int len)
{
	int i, r;

	if (len <= 0)
		return 0;

	for (i = r = 0; i < len; ){
		if ((r = send(buf + i, len-i)) <= 0){
			break;
		}
		i += r;

		xsys_sleep_ms(300);
	}

	return i;
}

void xdev_comm::save_data(const char * buf, int len)
{
	if (buf == NULL || len < 1){
		WriteToEventLog("%s: can not save null or %d bytes", m_name, len);
		return;
	}

	m_buf_lock.lock(1);

	if (len + m_recv_len > m_recv_max_len)
		m_recv_len = 0;
	memcpy(m_precv_buf + m_recv_len, buf, len);

	m_recv_len += len;

	if (m_use_full_log)
		write_buf_log(m_name, (unsigned char *)m_precv_buf, m_recv_len);

	m_has_data.set();

	m_buf_lock.unlock();
}

int xdev_comm::peek(char & c)
{
	if (!isopen())  return -1;

	int r;
	m_buf_lock.lock(2);
	if (m_recv_len > 0){
		c = m_precv_buf[0];
		r = 1;
	}else
		r = 0;
	m_buf_lock.unlock();

	return r;
}

int xdev_comm::peek(char * buf, int maxlen)
{
	if (!isopen())  return -1;

	int l;
	m_buf_lock.lock(2);
	l = min(maxlen, m_recv_len);
	if (l > 0){
		memcpy(buf, m_precv_buf, l);
	}
	m_buf_lock.unlock();

	return l;
}

void xdev_comm::skip(int bytes)
{
	if (isopen()){
		m_buf_lock.lock(2);
		if (bytes >= m_recv_len)
			m_recv_len = 0;
		else{
			m_recv_len -= bytes;
			memmove(m_precv_buf, m_precv_buf+bytes, m_recv_len);
		}
		m_buf_lock.unlock();
	}
}

int xdev_comm::recv_bytes(char *buf, int len, int seconds)
{
	memset(buf, 0, len);

	int i , r;
	if ((r = recv(buf, 1, seconds*1000)) <= 0){
		return r;
	}
	for (i = r = 1; i < len; ){
		if ((r = recv(buf + i, len-i, 150)) <= 0){
			break;
		}
		i += r;
	}
	WriteToEventLog("%s - recv_bytes(%d): recved %d bytes.", m_name, len, i);
	if (i > 0 && m_use_full_log)
		EL_WriteHexString(buf, i);

	return i;
}

int xdev_comm::recv_byend(char * buf, int maxlen, char * endstr, int seconds)
{
	memset(buf, 0, maxlen);

	int i , r;
	if ((r = recv(buf, 1, seconds*1000)) <= 0){
		return ERR_TIMEOUT;
	}
	for (i = r = 1; i < maxlen; ){
		if ((r = recv(buf + i, maxlen-i, 30)) <= 0){
			break;
		}
		i += r;

		// is the end?
		if (endstr != NULL && *endstr && strstr(buf, endstr) != 0){
			break;
		}
	}
	WriteToEventLog("%s - recv_bytes(%d): recved %d bytes.", m_name, maxlen, i);
	if (i > 0 && m_use_full_log)
		EL_WriteHexString(buf, i);

	if (strstr(buf, endstr) == 0)
		return ERR_TIMEOUT;

	return i;
}

int xdev_comm::recv_byend(char * buf, int maxlen, char endch, int seconds)
{
	memset(buf, 0, maxlen);

	int i , r;
	if ((r = recv(buf, 1, seconds)) <= 0){
		return ERR_TIMEOUT;
	}

	for (i = r = 1; i < maxlen; ){
		if ((r = recv(buf + i, maxlen-i, 30)) <= 0){
			return ERR_TIMEOUT;
		}
		i += r;

		if (buf[i-1] == endch){
			break;
		}
	}
	return i;
}


void xdev_comm::clear_all_recv(void)
{
	m_buf_lock.lock(2);

	m_recv_len = 0;
	m_has_data.reset();

	m_buf_lock.unlock();
}

int xdev_comm::recv(char *buf, int len, int msec)
{
	if (!isopen())  return -1;
	if (len == 0)  return 0;

	if (m_recv_len == 0 &&
		m_has_data.wait_ms(msec) == ERR_TIMEOUT)
	{
		return 0;
	}

	lock();

	if (len > m_recv_len)
		len = m_recv_len;

	memcpy(buf, m_precv_buf , len);

	// move buf
	m_recv_len -= len;
	if (m_recv_len)
		memmove(m_precv_buf, m_precv_buf + len, m_recv_len);

	if (m_recv_len <= 0)
		m_has_data.reset();

	unlock();

	return len;
}

void xdev_comm::lock(void)
{
	m_buf_lock.lock(3);
}

void xdev_comm::unlock(void)
{
	m_buf_lock.unlock();
}

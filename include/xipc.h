#pragma once

#include <condition_variable>
#include <thread>
#include <chrono>

class xmutex : public std::recursive_timed_mutex {
public:
	std::cv_status lock(int msecs) {
		using MS = std::chrono::milliseconds;
		if (msecs < 0) {
			std::recursive_timed_mutex::lock();
			return std::cv_status::no_timeout;
		}

		return (
			std::recursive_timed_mutex::try_lock_for(MS(msecs)) ?
			std::cv_status::no_timeout :
			std::cv_status::timeout
		);
	}
};


class xwait_t {
protected:
	xmutex	m_;
	std::condition_variable_any	c_;

public:
	void signal(void)		/// 通知
	{
		c_.notify_one();
	}

	void broadcast(void)	/// 广播通知唤醒
	{
		c_.notify_all();
	}

	std::cv_status wait(int msec)		/// 等待，毫秒，负数为永远等待
	{
		std::unique_lock<std::recursive_timed_mutex>	lk(m_);
		if (msec < 0) {
			c_.wait(lk);
			return std::cv_status::no_timeout;
		}

		return c_.wait_for(lk, std::chrono::milliseconds(msec));
	}
};

class xevent : public xwait_t
{
protected:
	volatile bool	flag_;			/// 有事件标志
	bool			manual_reset_;	/// 是否手工清除事件，自动清除的话，则wait之后，立即清除事件标志

public:
	xevent(bool manual_reset) { manual_reset_ = manual_reset; flag_ = false; }

	int wait(int msec)	/// 若无事件，则等待（毫秒），不然立即返回
	{
		auto pred = [this] { return flag_; };
		std::unique_lock<std::recursive_timed_mutex>	lk(m_);

		std::cv_status r;
		if (msec < 0) {
			c_.wait(lk, pred);
			r = std::cv_status::no_timeout;
		}else {
			r = c_.wait_for(lk, std::chrono::milliseconds(msec), pred) ?
				std::cv_status::no_timeout :
				std::cv_status::timeout
			;
		}

		if (manual_reset_)
			reset();

		return int(r);
	}

	void set(void) { flag_ = true; signal(); }		/// 设置有事件
	void reset(void) { flag_ = false; } 	/// 清除事件
};

class xsemaphore : public xwait_t
{
protected:
	std::atomic_int	count_;	/// 信号数量
	int	 max_count_;		/// 最大数量

public:
	int init(int init_count, int max_count)  	/// 在创建之初调用
	{
		count_ = init_count;  max_count_ = max_count;
		return 0;
	}

	std::cv_status P(int msec)	/// 等待，有信号时，等待结束，信号量减1
	{
		auto pred = [this] { int i = count_; return i; };
		std::unique_lock<std::recursive_timed_mutex>	lk(m_);

		std::cv_status r;
		if (msec < 0) {
			c_.wait(lk, pred);
			r = std::cv_status::no_timeout;
		}else {
			r = c_.wait_for(lk, std::chrono::milliseconds(msec), pred) ?
				std::cv_status::no_timeout :
				std::cv_status::timeout
			;
		}

		if (r == std::cv_status::no_timeout)
			count_--;

		return r;
	}

	void V(int count = 1)	/// 增加信号量
	{
		if (count_ < max_count_) {
			++count_;
			signal();
		}
	}
};

inline void xsleep_sec(int secs)
{
	std::this_thread::sleep_for(std::chrono::seconds(secs));
}

inline void xsleep_ms(int millisecs)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
}

// 取得 type 类型变量的地址，它的 member 所在地址是 ptr 的指针
#ifndef container_of
#define container_of(ptr, type, member) \
	(type *) ((char *) ptr - offsetof(type, member))
#endif	// container_of

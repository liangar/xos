#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <xstr.h>
#include <xslm_deque.h>

#ifndef container_of
#define container_of(ptr, type, member) \
	(type *) ((char *) ptr - offsetof(type, member))
#endif	// container_of
	
struct asc_s{
	int 	id;		/// id

	int 	sock;	/// 通讯所用socket,为系统分配的文件句柄

	struct sockaddr sock_addr;
	struct sockaddr peer_addr;

	time_t	create_time;	/// 建立连接的时间
	volatile time_t	last_trans_time;/// 最近通讯时间
	volatile time_t	last_recv_time;	/// 最近接收时间

	volatile int idle_secs;		/// 空闲秒数，到达空闲时间，执行on_idle

	volatile int state;			/// 状态

	char* precv_buf;		/// 接收缓冲区
	int		recv_size;		/// 当前所用的缓冲空间
	int		recv_bytes; 	/// 当前缓冲长度

	int 	recv_amount;	/// 接收总数

	int 	sent_len;		/// 最近发送长度

	int		pkg_amount;		/// 接收包数

#ifdef _WIN32
	OVERLAPPED	ovlp;	/// ovlp for recv
#endif

	/// 以下给应用扩展使用
	int		app_id;		/// 连接对方的 id 标识, 给*应用扩展设置*使用
	char* app_data;	/// 给*应用扩展设置*使用 
};

class asc_sq : public xslm_deque<asc_s>{
protected:
	int compare(const asc_s *t1, const asc_s *t2){
		return (t1->id - t2->id);
	}
	
	void show_t(FILE * pf, const asc_s * t)
	{
		asc_s * p = container_of(&(t->state), asc_s, state);
		int n = offsetof(asc_s, state);
		fprintf(pf, "id=%d %p %p %d", t->id, p, t, p->state);
	}
};

using namespace std;

int test_slm_q(void)
{
#ifdef WIN32
#ifdef _DEBUG
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );
#endif // _DEBUG
#endif // WIN32

	{
	char b[128];
	int r;
	asc_s * pses = nullptr;
	asc_sq	sess;

	cout << "please enter a command or 'exit' to exit: ";
	while(1)
	{
		cin.getline(b, 512);
		
		if (strncmp(b, "open:", 5) == 0){
			r = sess.open(atoi(b+5));
		}else if (strcmp("close", b) == 0){
			sess.close();
		}else if (strncmp("get:", b, 4) == 0){
			r = sess.get(&pses);
			if (r >= 0) {
				pses->id = atoi(b+4);
				pses->sock = pses->id;
				pses->create_time = pses->last_recv_time = pses->last_trans_time = int(time(0));
				pses->recv_size = pses->create_time;
				pses->state = pses->id;
				pses->app_id = -1;
				r = sess.add(r, pses);
			}
		}else if (strncmp("del_i:", b, 6) == 0){
			r = sess.del(atoi(b + 6));
		}else if (strncmp("del_x:", b, 6) == 0){
			asc_s x;
			x.id = atoi(b + 6);
			r = sess.del(&x);
		}else if (stricmp("show", b) == 0){
			sess.show(stderr);
		}else if (stricmp("exit", b) == 0){
			break;
		}else{
			// usage();
		}
		
		printf("r = %d\n", r);
	}
	
	sess.close();
	}
	

#ifdef WIN32
#ifdef _DEBUG
	_CrtMemCheckpoint( &s2 );
	if ( _CrtMemDifference( &s3, &s1, &s2) ){
		_CrtMemDumpStatistics( &s3 );
		_CrtDumpMemoryLeaks();
	}
#endif // _DEBUG
#endif // WIN32

	return 0;
}

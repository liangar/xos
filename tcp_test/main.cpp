#include <iostream>
#include <algorithm>

#include <xslm_deque.h>
#include <xstr.h>

#include "test_tcp.h"

//
#include <xqueue_msg.h>

#ifdef WIN32
#include <crtdbg.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif // _DEBUG
#endif // WIN32

extern int test_slm_q(void);

char b[16*1024];

template<long FROM, long TO>
class Range {
public:
	// member typedefs provided through inheriting from std::iterator
	class iterator : public std::iterator<
		std::input_iterator_tag,   // iterator_category
		long,                      // value_type
		long,                      // difference_type
		const long*,               // pointer
		long                       // reference
	> {
		long num = FROM;
	public:
		explicit iterator(long _num = 0) : num(_num) {}
		iterator& operator++() { num = TO >= FROM ? num + 1 : num - 1; return *this; }
		iterator operator++(int) { iterator retval = *this; ++(*this); return retval; }
		bool operator==(iterator other) const { return num == other.num; }
		bool operator!=(iterator other) const { return !(*this == other); }
		reference operator*() const { return num; }
	};
	iterator begin() { return iterator(FROM); }
	iterator end() { return iterator(TO >= FROM ? TO + 1 : TO - 1); }
};

void test_iterator(void)
{
	auto range = Range<15, 25>();
	auto itr = std::find(range.begin(), range.end(), 18);
	std::cout << *itr << '\n'; // 18

	// Range::iterator also satisfies range-based for requirements
	for (long l : Range<3, 5>()) {
		std::cout << l << ' '; // 3 4 5
	}
	std::cout << '\n';
}

using namespace std;

static unsigned int recv_show(void * pvoid);
xqueue_msg g_queue;

int g_port;

int main(int argc, char **argv)
{
#ifdef WIN32
#ifdef _DEBUG
	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint( &s1 );
//	_CrtSetBreakAlloc(251);
#endif // _DEBUG
#endif // WIN32

	if (argc < 2){
		printf("usage: tcp_test <listen port> [<queue len>]\n");
		return -1;
	}

	g_port = atoi(argv[1]);
	int queue_len = 100;
	if (argc == 3)
		queue_len = atoi(argv[2]);
	if (queue_len < 5 || queue_len > 5000)
		queue_len = 5;

	net_evn_init();
	log_open("test_tcp.log");

	test_iterator();

	g_queue.init(32, 1000);

	std::thread t1([] {recv_show(nullptr); });

	test_tcp * ptcp = new test_tcp();	///测试tcp服务

	int i=1;
	cout << "please enter a command or '|' to exit: ";
	while(1)
	{
		cin.getline(b, 512);
		if (strcmp(b, "q") == 0) {
			test_slm_q();
		}else if(strcmp(b,"start")==0)
		{
			cout<< "command:start" <<endl;
			// port, sessions, most recv len, max len, send len
			ptcp->open(g_port, 20, 300, 128, 256, 256);
			ptcp->start();
		}else if (strcmp(b,"stop")==0)
		{
			cout<< "command:stop" <<endl;
			ptcp->stop();
			ptcp->close();
		}else if (strncmp(b, "put", 3)==0)
		{
			if (b[3] == ':'){
				const char * p = getaword(b, b+4, ',');
				g_queue.put(atoi(b), p, 2000);
			}else{
				cout << "put:" << i << endl;
				char temp[10];
				sprintf(temp, "TestData%d", i);
				g_queue.put(i, temp, 2000);
			}
			i++;
		}else if (strcmp(b,"get")==0)
		{
			long id;
			int r = g_queue.get(&id, b, 2000);
			if (r > 0){
				cout << "get:(id=" << id <<") " << b  <<endl;
				i--;
			}else{
				cout << "no data" << endl;
			}
		}else if (strncmp(b,"send:", 5) == 0){
			// send:1:this is a test string for 1 session
			int i = atoi(b+5);
			char * s = strchr(b + 5, ':');
			int l = c2string(b, s+1);
			ptcp->send(i, b);
			printf("send (%d) bytes\n", l);
		}else if (strncmp(b, "close:", 6) == 0){
			ptcp->notify_close_session(atoi(b+6));
		}else if (strcmp(b,"|")==0){
			break;
		}else{
			cout << "=================command list ====================" << endl;
			cout<<"1 'start'    启动tcp服务" << endl;
			cout<<"2 'stop'     结束tcp服务" << endl;
			cout<<"3 'send:<i>' 发送数据" << endl;
			cout<<"4 'put'      测试队列，往队列放数据" << endl;
			cout<<"5 'get'      测试队列，从队列取数据" << endl;
			cout<<"6 'close:<i>'关闭会话"  << endl;
			cout<<"7 '|'  退出" << endl;
		}
	}
	g_queue.put(-1, "exit");
	t1.join();

	g_queue.done();
	delete ptcp;

	log_close();

	net_env_done();

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


static unsigned int recv_show(void * pvoid)
{
	char aline[512];

	printf("recver: in.\n");

	int l = 1, i;
	for (;;) {
		l = g_queue.get((long*)(&i), aline);
		if (l > 0) {
			if (i < 0)  break;
			printf("%d <-: %d\n", i, l);
			xsleep_sec(3);
		}
	}
	printf("recver: out.\n");

	return 0;
}

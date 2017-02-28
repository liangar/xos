#include "xsys.h"

int g = 0;
unsigned int f2(void *)
{
	g++;
	printf("this is f2(%d)\n", g);
	g--;
	return 0;
}

unsigned int f1(void * p)
{
	xsys_socket * pc = (xsys_socket *)p;
	char b[128];
	int r = pc->recvblob(b, 32);
	if (r > 0){
		printf("recv ok(%d)\n%s\n", r, b);
	}else{
		printf("recv error(%d)\n%s\n", r, xlasterror());
	}
	printf("f1 in(%d)\n", g);
	g++;
	xsys_thread t;
	printf("this is f1\n");
	t.init(f2, 0);
	t.wait(-1);
	printf("exit f1\n");
	g--;

	return 0;
}


int main(void)
{
	xsys_init(true);
	xsys_socket s;
	int r = s.listen(8097);
	if (r < 0){
		printf("listen error(%d)\n", r);
	}else{
		xsys_socket c;
		r = s.accept(c);
		if (r < 0){
			printf("accept error(%d)\n%s\n", r, xlasterror());
		}else{
			const char * p = "hello, welcome to here.\r\n";
			c.send(p, strlen(p));
///*
	xsys_thread	t;
	t.init(f1, &c);
	t.wait(-1);
//*/
			c.close();
		}
		s.close();
	}
	xsys_down();
	printf("exit main(%d)\n", g);

	return 0;
}

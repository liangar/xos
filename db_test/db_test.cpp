#include <xsys_oci.h>
#include <iostream>
#include <l_str.h>
#include <lfile.h>

#include <httpclient.h>

using namespace oralib;
using namespace std;

int select1(char * p);
void executes1(const char * p);
void show_mes();

connection db;

int main(void)
{
	char b[4039];
	char * p1;

	int r;
	if ((r = xsys_init(true)) != 0){
		return r;
	}
	oralib_init();

	try{
		while (1){			
			cin.getline(b, sizeof(b)-1);
			if (stricmp(b, "exit") == 0)
				break;

			p1 = getaword(b, b, ':');
			if (*p1 == 0){
				show_mes();
				continue;
			}

			switch (b[0])
			{
			case 'o':db.open(p1);	break;
			case '?':show_mes();	break;
			case 's':select1(p1);	break;
			case 'e':executes1(p1);	break;
			case 'x':db.close();	break;
			case 'h':
				{
					HTTP_REQUEST req;
					httpInitReq(&req, p1, 0);
					r = httpRequest(&req);
					if (r < 0){
						printf("error: %d", r);
					}else{
						r = write_file("t.html", req.buffer, b, 512);
						printf("%s\n", req.buffer);
					}
					httpClean(&req);
				}
			default :show_mes();	break;
			}
		}
	}catch (oralib::error &e){
		printf("%s\n", e.details().c_str());
		db.close();
	}catch (...){
		printf("unrecognized error occured!\n");
		db.close();
	}

	oralib_down();
	xsys_down();

	return 0;
}

int select1(char * p){

	resultset * r = db.select(p);
	if (r->eod()){
		printf("no data\n");
		r->release();
		return 0;
	}

	int n = r->columns_count();
	int i;
	for (i = 0;  i < n;  i++){
		column & col = (*r)[FIRST_COLUMN_NO + i];
		printf("%s | ", col.col_name.c_str());
	}
	printf("\n");

	while(!r->eod()){
		for (i = 0;  i < n;  i++) {
			column & col = (*r)[FIRST_COLUMN_NO + i];
			if (col.is_null()){
				printf("(null) | ");
				continue;
			}
			switch(col.col_type){
				case DT_NUMBER:	printf("%f | ",double(col));		break;
				case DT_TEXT:	printf("%s | ",Pstr(col));		break;
				case DT_DATE:
					{
						char v[32];
						datetime(col).toSimpleTime(v);
						printf("%s | ", v);
						break;
					}
			}
		}
		printf("\n");
		r->next();
	}
	r->release();
	return 0;
}

void show_mes()
{
	printf("o: 连接数据库字符(dsn=dbname;uid=..;pwd=..)\n"
		   "s: sql select 语句\n"
		   "e: sql insert/update 语句\n"
		   "x: exit(0)\n"
		   "?: 提示命令应用说明\n"
	);
}

void executes1(const char * p)
{
	db.execute(p);
	db.commit();
}

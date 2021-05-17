// test case comes from: http://zetcode.com/db/postgresqlc/

// #include <iostream>
#include "xpgdb.h"

/*
int add_nums(int a, int count, ...) {
	int result = 0;
	va_list args;						// 指向可变参数列表
	va_start(args, count);				// args：可变参数第一个参数地址，count：可变参数的第一个参数
	for (int i = 0; i < count; ++i) {
		const char * p =  va_arg(args, const char *);	// args：可变参数的当前参数，int：可变参数的下个参数类型
		result += atoi(p);
	}
	va_end(args);						// 清空可变参数
	return result;
}

int main() {
	std::cout << add_nums(1, 4, "111", "222", "2", "55") << '\n';

	system("pause");
	return 0;
}
*/

/*
#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

void do_exit(PGconn* conn) {

    PQfinish(conn);
    exit(1);
}

int main(void)
{
    PGconn* conn = PQconnectdb("port = 50021 user = postgres password = pgdb11 dbname = postgres");

    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "Connection to database failed: %s\n",
            PQerrorMessage(conn));
        do_exit(conn);
    }

    int ver = PQserverVersion(conn);

    printf("Server version: %d\n", ver);

    const char* stm = "SELECT COUNT(*) FROM ha_records WHERE cmdid=$1";
    unsigned int id = htonl(uint32_t(2088445));

    const char* paramValues[1];
    int paramLen[1], paramFormats[1];
    Oid paramTypes[1];

    paramFormats[0] = 1;
    paramTypes[0] = 23;
    paramValues[0] = (const char *)&id;
    paramLen[0] = sizeof(id);

    PGresult* res = PQprepare(conn, "", stm, 1, paramTypes);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("No data retrieved\n");
        PQclear(res);
        do_exit(conn);
    }
    PQclear(res);

    res = PQexecPrepared(conn, "", 1, paramValues, paramLen, paramFormats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data retrieved\n");
        PQclear(res);
        do_exit(conn);
    }

    printf("rows=%d, cols=%d, value=%s\n", PQntuples(res), PQnfields(res), PQgetvalue(res, 0, 0));
    PQclear(res);

    PQfinish(conn);

    int lib_ver = PQlibVersion();

    printf("Version of libpq: %d\n", lib_ver);

    return 0;
}
//*/
static void t_get_dbver(xpg_conn& db, xpg_sql& sql);
static int t_create_table_car(xpg_conn &db, xpg_sql & sql);
static int t_list_cars(xpg_conn& db, xpg_sql& sql);

//*
int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("usage: test 'host port user pwd dbname'\n\n"
            "for example:\n\t\"host=192.168.1.61 port=50021 user=postgres password=pgdb11 dbname=postgres\"\n"
        );
        return -1;
    }

    xpg_conn conn;
	int r;

	if ((r = conn.open(argv[1])) < 0){
        fprintf(stderr, "Connection to [%s] failed(%d): %s\n", argv[1], r,
            conn.get_error());
        return r;
    }
    printf("connect ok\n");
    printf("ver: %d\n", conn.get_ver());
    printf("database: %s\n", conn.get_dbname());

    xpg_sql sql;
    sql.open(&conn);

    t_get_dbver(conn, sql);

	r = t_create_table_car(conn, sql);

	if (r >= 0){
		r = t_list_cars(conn, sql);
	}

    sql.close();

    conn.close();

    return 0;
}
//*/

static void do_exit(xpg_conn &db, xpg_sql & sql, int r)
{
	if (r < 0)
		printf("Error: %s", sql.get_error());
	sql.close();

	db.trans_end(r >= 0);
	db.close();

	exit(r);
}

static int t_create_table_car(xpg_conn &db, xpg_sql & sql)
{
    int r;

	do {
	db.trans_begin();

	if ((r = sql.exec("DROP TABLE IF EXISTS Cars")) != XSQL_OK)
		break;

    r = sql.exec("CREATE TABLE Cars(Id INTEGER PRIMARY KEY, Name VARCHAR(20), Price INT)");

    if (r != XSQL_OK)
		break;

	int key = 1, price = 52642;
    char car[32] = "Audi";
    r = sql.exec("INSERT INTO Cars VALUES($1, $2, $3)",
		XPARM_IN  | XPARM_INT, &key,
					XPARM_STR, car, 0,
					XPARM_INT, &price,
		XPARM_END
	);
    if (r != XSQL_OK)
		break;

    key = 2; price = 57127;  strcpy(car, "Mercedes");
	if ((r = sql.exec()) != XSQL_OK)
		break;

    key = 3; price = 9000;  strcpy(car, "Skoda");
	if ((r = sql.exec()) != XSQL_OK)
		break;

    key = 4; price = 29000; strcpy(car, "Volvo");
	if ((r = sql.exec()) != XSQL_OK)
		break;

    key = 5; price = 350000;strcpy(car, "Bentley");
	if ((r = sql.exec()) != XSQL_OK)
		break;

    key = 6; price = 21000; strcpy(car, "Citroen");
	if ((r = sql.exec()) != XSQL_OK)
		break;

    key = 7; price = 41400; strcpy(car, "Hummer");
	if ((r = sql.exec()) != XSQL_OK)
		break;

    key = 8; price = 21600; strcpy(car, "Volkswagen");
    r = sql.exec();
	}while(false);

    if (r != XSQL_OK)
        do_exit(db, sql, r);

	r = db.trans_end(r >= 0);

    return r;
}

static void t_get_dbver(xpg_conn& db, xpg_sql& sql)
{
    int r = sql.exec("SELECT VERSION()");
    if (r == XSQL_OK && (r = sql.fetch()) == XSQL_OK) {
        printf("db ver = %s\n", sql.get_col(0));
    }
}


static int t_list_cars(xpg_conn& db, xpg_sql& sql)
{
	int key, price; char car[32];
    long id = 6;
    int r = sql.exec("SELECT id, price, name FROM Cars WHERE id <= $1::int4",
		XPARM_IN  | XPARM_LONG,&id,
		XPARM_COL | XPARM_INT, &key,
					XPARM_INT, &price,
                    XPARM_STR, car, sizeof(car),
        XPARM_END
	);

    if (r < 0) {
        do_exit(db, sql, r);
    }

	int i;
	printf("No.");
	for (i = 0; i < sql.res_columns(); i++){
		printf("\t%s", sql.column_name(i));
	}
	printf("\n");

	i = 1;
    while((r = sql.fetch()) == XSQL_OK) {
        printf("%d:\t%d\t%d\t%s\n", i++, key, price, car);
    }

	if (r < 0)
		do_exit(db, sql, r);

	r = sql.res_rows();

	sql.end_exec();

	return r;
}



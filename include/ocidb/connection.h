// OraLib 0.0.3 / 2002-06-30
//	connection.h
//
//	http://606u.dir.bg/
//	606u@dir.bg

#ifndef	_CONNECTION_H
#define	_CONNECTION_H


namespace oralib {


class statement;
class resultset;

class connection
{
	// friends
	friend class statement;
	friend class parameter;
	friend class resultset;
	friend class column;

private:
	OCIEnv		*environment_handle;
	OCIServer	*server_handle;
	mutable OCIError	*error_handle;	// because it could be changed by most oracle APIs
	OCISession	*session_handle;
	OCISvcCtx	*svc_context_handle;

	bool		is_available;			// (used for connection pooling)?
	bool		is_blocking;			// mode (a call could return OCI_STILL_EXECUTING)

private:
	// private copy-constructor and assignment operator - class could not be copied
	inline connection (
		IN const connection& /* cn */) { /* could not be copy-constructed */ };
	inline connection& operator = (
		IN const connection& /* cn */) { return (*this); /* could not be copy-constructed */ };

	// initialize private data
	void initialize (void);

	// free resources allocated
	inline void cleanup (void) { close (); };

public:
	connection (void);

	// create an instance and open the connection
	connection (
		IN const char *service_name,
		IN const char *login,
		IN const char *password,
		IN OPTIONAL unsigned long env_mode = OCI_OBJECT | OCI_THREADED,
		IN OPTIONAL bool non_blocking_mode = false);

	connection (IN const char * dsn);

	~connection ();

	// connects to an Oracle server
	void open (
		IN const char *service_name,
		IN const char *login,
		IN const char *password,
		IN OPTIONAL unsigned long env_mode = OCI_OBJECT | OCI_THREADED,
		IN OPTIONAL bool non_blocking_mode = false);

	void open (IN const char * dsn);

	// closes the connection
	void close (void);

public:
	// executes a sql statement with no result
	void execute (
		IN const char *sql_block,
		IN OPTIONAL int sql_len = -1);

	// prepares (and returns) a sql statement for execution
	statement *prepare (
		IN const char *sql_block,
		IN OPTIONAL int sql_len = -1);

	// executes a select sql statement and return the result set
	resultset *select (
		IN const char *select,
		IN OPTIONAL int select_len = -1);

	// commits changes
	inline void commit (void)
		{ execute ("commit", 6); };

	// rollbacks changes
	inline void rollback (void)
		{ execute ("rollback", 8); };

	long geta(const char * sqlstring, long & r);
	long geta(const char * sqlstring);
	const char * geta(const char * sqlstring, char * d, int buflength);

public:
	bool	is_opened;
	long	lastTime;
	string	lastsql;
}; // connection class


}; // oralib namespace

void oralib_init();
void oralib_down();

#endif	// _CONNECTION_H

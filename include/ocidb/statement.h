// OraLib 0.0.3 / 2002-06-30
//	statement.h
//
//	http://606u.dir.bg/
//	606u@dir.bg

#ifndef	_STATEMENT_H
#define	_STATEMENT_H


namespace oralib {


// statement type - select statements and pl/sql blocks are handled with care
enum StatementTypesEnum
{
	ST_UNKNOWN,
	ST_SELECT = OCI_STMT_SELECT,
	ST_UPDATE = OCI_STMT_UPDATE,
	ST_DELETE = OCI_STMT_DELETE,
	ST_INSERT = OCI_STMT_INSERT,
	ST_CREATE = OCI_STMT_CREATE,
	ST_DROP = OCI_STMT_DROP,
	ST_ALTER = OCI_STMT_ALTER,
	ST_BEGIN = OCI_STMT_BEGIN,
	ST_DECLARE = OCI_STMT_DECLARE
};


class statement
{
	// friends
	friend class parameter;
	friend class connection;

private:
	connection		*conn;			// being used
	OCIStmt			*stmt_handle;
	string		sql;			// being executed
	StatementTypesEnum	type;		// of the statement

	typedef vector <parameter *> Parameters;
	typedef map <string, parameter *> ParametersMap;
	Parameters		parameters;		// an array with bound parameters
	ParametersMap	parameters_map;	// a map with parameters against their names

	bool			is_prepared;
	bool			is_executed;

private:
	// public not creatable; use connection.execute, .prepare or .select
	// prepares an sql statement for execution
	statement (
		 connection &use,
		 const char *sql_block,
		 int sql_len = -1);

	// public not deletable; use release method instead
	~statement ();

	// private copy-constructor and assignment operator - class could not be copied
	statement (
		 const statement& /* st */) { /* could not be copy-constructed */ };
	statement& operator = (
		 const statement& /* st */) { return (*this); /* could not be copy-constructed */ };

	// initialize private data
	void initialize (void);

	// free resources allocated
	void cleanup (void);

	// prepares an sql statement for execution
	void prepare (
		 const char *sql_block,
		 int sql_len = -1);

	// executes already prepared statement
	void execute_prepared (void);

public:
	// binds a named variable to the statement
	// when type is set to DT_UNKNOWN type is taken from name's prefix
	parameter &bind (
		 const char *name,
		 DataTypesEnum type = DT_UNKNOWN);

	// executes a prepared statement with no output parameters
	inline void execute (void) { execute_prepared (); };

	// executes a prepared select sql statement and returns the result set
	resultset *select (void);

	// releases statement
	inline void release (void) { delete this; };

	// returns a bound parameter by name or index
	parameter& operator [] (
		const char *name);
	parameter& operator [] (
		ub2 parameter_index);
}; // statement class


}; // oralib namespace


#endif	// _STATEMENT_H

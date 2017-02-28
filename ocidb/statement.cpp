// OraLib 0.0.3 / 2002-06-30
//	statement.cpp
//
//	http://606u.dir.bg/
//	606u@dir.bg

#include <xsys_oci.h>

namespace oralib {

statement::statement (
	IN connection &use,
	IN const char *sql_block,
	IN OPTIONAL int sql_len)	// = -1
{
	conn = &use;
	initialize ();
	try
	{
		prepare (
			sql_block,
			sql_len);
	}
	catch (...)
	{
		cleanup ();
		throw;
	}
}


statement::~statement ()
{
	cleanup ();
}


void
statement::initialize (void)
{
	stmt_handle = 0;
	is_prepared = false;
	is_executed = false;
	type = ST_UNKNOWN;
}


void
statement::cleanup (void)
{
	if (stmt_handle)
	{
		// ignore return code - returns either OCI_SUCCESS or OCI_INVALID_HANDLE
		OCIHandleFree (
			stmt_handle,
			OCI_HTYPE_STMT);
		stmt_handle = NULL;
	}

	// remove bound parameters
	for (Parameters::iterator i=parameters.begin (); i!=parameters.end (); ++i)
		delete (*i);
	parameters.clear ();
}


void
statement::prepare (
	IN const char *sql_block,
	IN OPTIONAL int sql_len)	// = -1
{
	sword	result;

	conn->lastTime = time(0);
	conn->lastsql = sql_block;

	// allocate statement handle
	result = OCIHandleAlloc (
		conn->environment_handle,
		(void **) &stmt_handle,
		OCI_HTYPE_STMT,
		0,		// extra memory to allocate
		NULL);	// pointer to user-memory

	if (result == OCI_SUCCESS)
	{
		if (sql_len == -1) sql_len = strlen (sql_block);

		result = OCIStmtPrepare (
			stmt_handle,
			conn->error_handle,
			(text *) sql_block,
			sql_len,
			OCI_NTV_SYNTAX,
			OCI_DEFAULT);
	}
	else
		throw (oralib::error (result, conn->environment_handle, __FILE__, __LINE__));

	if (result == OCI_SUCCESS)
	{
		ub2	stmt_type = 0;
		result = OCIAttrGet (
			stmt_handle,
			OCI_HTYPE_STMT,
			&stmt_type,
			NULL,	// ptr to storage size; required for strings, only
			OCI_ATTR_STMT_TYPE,
			conn->error_handle);
		// returns 0 (ST_UNKNOWN) if sql statement is wrong
		type = (StatementTypesEnum) stmt_type;
	}

	if (result == OCI_SUCCESS)
	{
		is_prepared = true;
		is_executed = false;
	}
	else
		throw (oralib::error (result, conn->error_handle, __FILE__, __LINE__));
}


void
statement::execute_prepared (void)
{
	conn->lastTime = time(0);

	// direct select statement?
	ub4 iters = (type == ST_SELECT) ? 0 : 1;
	sword result = OCIStmtExecute (
		conn->svc_context_handle,
		stmt_handle,
		conn->error_handle,
		iters,	// number of iterations
		0,		// starting index from which the data in an array bind is relevant
		NULL,	// input snapshot descriptor
		NULL,	// output snapshot descriptor
		OCI_DEFAULT);

	if (result == OCI_SUCCESS)
		is_executed = true;
	else
		throw (oralib::error (result, conn->error_handle, __FILE__, __LINE__));
}


parameter&
statement::bind (
	IN const char *name,
	IN OPTIONAL DataTypesEnum type)	// = DT_UNKNOWN
{
	// could throw an exception
	parameter *p = new parameter (
		this,
		name,
		type,
		FETCH_SIZE);
	try
	{
		parameters.push_back (p);
		parameters_map [p->param_name] = p;
	}
	catch (...) // STL exception, perhaps
	{
		p->release ();
		throw;
	}
	return (*p);
}


resultset*
statement::select (void)
{
	execute ();
	resultset *r = new resultset (
		stmt_handle,
		conn);
	try
	{
		r->fetch_rows ();
		return (r);
	}
	catch (...)
	{
		if (r) delete r;
		throw;
	}
}


parameter&
statement::operator [] (const char *name)
{
	ParametersMap::iterator i = parameters_map.find (string (name));
	if (i == parameters_map.end ())
		// name not found in parameters
		throw (oralib::error (EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__, name));
	return (*(i->second));
}


parameter&
statement::operator [] (ub2 parameter_index)
{
	if (parameter_index < FIRST_PARAMETER_NO ||
		parameter_index > parameters.size ())
		// no parameter with such index
		throw (oralib::error (EC_PARAMETER_NOT_FOUND, __FILE__, __LINE__, "%d", (int) parameter_index));
	return (*(parameters.at (parameter_index - FIRST_PARAMETER_NO)));
}


}; // oralib namespace

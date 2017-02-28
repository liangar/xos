// OraLib 0.0.3 / 2002-06-30
//	connection.cpp
//
//	http://606u.dir.bg/
//	606u@dir.bg

#include <xsys_oci.h>
#include <l_str.h>

// set to 1 to track OCI's memory usage
#if (0)
	dvoid *malloc_func (dvoid * /* ctxp */, size_t size)
		{ return (malloc (size)); }
	dvoid *realloc_func (dvoid * /* ctxp */, dvoid *ptr, size_t size)
		{ return (realloc (ptr, size)); }
	void free_func (dvoid * /* ctxp */, dvoid *ptr)
		{ free (ptr); }
#else
#	define	malloc_func		NULL
#	define	realloc_func	NULL
#	define	free_func		NULL
#endif

static xsys_mutex xsys_dblock;

void oralib_init(){  
	xsys_dblock.init();  
}

void oralib_down(){  
	xsys_dblock.down();  
}

namespace oralib {


connection::connection (void)
{
	initialize ();
}


connection::connection (
	IN const char *service_name,
	IN const char *login,
	IN const char *password,
	IN OPTIONAL unsigned long env_mode,	// = OCI_OBJECT
	IN OPTIONAL bool non_blocking_mode)	// = false
{
	initialize ();
	try
	{
		open (
			service_name,
			login,
			password,
			env_mode,
			non_blocking_mode);
	}catch (oralib::error e){
		close();
		throw e;
	}catch (...) {
		close();
		throw "unknown error";
	}
}

connection::connection (IN const char * dsn)
{
	initialize();
	try{
		open (dsn);
	}catch (oralib::error e){
		close();
		throw e;
	}catch (...) {
		close();
		throw "unknown error";
	}
}

connection::~connection ()
{
	cleanup (); // calls close
}


void
connection::initialize (void)
{
	environment_handle = NULL;
	server_handle = NULL;
	error_handle = NULL;
	session_handle = NULL;
	svc_context_handle = NULL;

	is_opened = false;
	is_available = false;
	is_blocking = false;
}

void
connection::open(IN const char * dsn)
{
	char server[64], uid[64], pwd[64], t[64];
	server[0] = '\0';  uid[0] = '\0';  pwd[0] = '\0';

	char * pdsn = new char[strlen(dsn) + 1];
	strcpy(pdsn, dsn);

	const char * p = pdsn;
	while(p && *p){
		p = getaword(t, p, '=');
		if (stricmp(t, "server") == 0 || stricmp(t, "dsn") == 0){
			p = getaword(server, p, ';');
		}else if (stricmp(t, "uid") == 0 || stricmp(t, "user") == 0) {
			p = getaword(uid, p, ';');
		}else if (stricmp(t, "pwd") == 0 || stricmp(t, "password") == 0) {
			p = getaword(pwd, p, ';');
		}
	}
	delete[] pdsn;

	open(server, uid, pwd);
}

void
connection::open (
	IN const char *service_name,
	IN const char *login,
	IN const char *password,
	IN OPTIONAL unsigned long env_mode,	// = OCI_OBJECT
	IN OPTIONAL bool non_blocking_mode)	// = false
{
	try{
		if (xsys_dblock.lock(2) != 0){
			throw "connection::open : cannot lock";
		}
		xsys_sleep_ms(500);

		if (is_opened)
			close();

		sword	result;

		// allocate an environment handle
		result = OCIEnvCreate (
			&environment_handle,
			env_mode,
			NULL,		// context
			malloc_func,	// malloc
			realloc_func,	// realloc
			free_func,		// free
			0,			// extra memory to allocate
			NULL);		// pointer to user-memory

		// allocate a server handle
		if (result == OCI_SUCCESS)
			result = OCIHandleAlloc (
				environment_handle,
				(void **) &server_handle,
				OCI_HTYPE_SERVER,
				0,		// extra memory to allocate
				NULL);	// pointer to user-memory
		else
			throw (oralib::error (EC_ENV_CREATE_FAILED, __FILE__, __LINE__));

		// allocate an error handle
		if (result == OCI_SUCCESS)
			result = OCIHandleAlloc (
				environment_handle,
				(void **) &error_handle,
				OCI_HTYPE_ERROR,
				0,		// extra memory to allocate
				NULL);	// pointer to user-memory

		// create a server context
		if (result == OCI_SUCCESS)
			result = OCIServerAttach (
				server_handle,
				error_handle,
				(text *) service_name,
				strlen (service_name),
				OCI_DEFAULT);
		else
			throw (oralib::error (result, environment_handle, __FILE__, __LINE__));

		// allocate a service handle
		if (result == OCI_SUCCESS)
			result = OCIHandleAlloc (
				environment_handle,
				(void **) &svc_context_handle,
				OCI_HTYPE_SVCCTX,
				0,		// extra memory to allocate
				NULL);	// pointer to user-memory
		else
			throw (oralib::error (result, error_handle, __FILE__, __LINE__));

		// set the server attribute in the service context handle
		if (result == OCI_SUCCESS)
			result = OCIAttrSet (
				svc_context_handle,
				OCI_HTYPE_SVCCTX,
				server_handle,
				sizeof (OCIServer *),
				OCI_ATTR_SERVER,
				error_handle);
		else
			throw (oralib::error (result, environment_handle, __FILE__, __LINE__));

		// allocate a user session handle
		if (result == OCI_SUCCESS)
			result = OCIHandleAlloc (
				environment_handle,
				(void **) &session_handle,
				OCI_HTYPE_SESSION,
				0,		// extra memory to allocate
				NULL);	// pointer to user-memory
		else
			throw (oralib::error (result, error_handle, __FILE__, __LINE__));

		// set username and password attributes in user session handle
		if (result == OCI_SUCCESS)
			result = OCIAttrSet (
				session_handle,
				OCI_HTYPE_SESSION,
				(text *) login,
				strlen (login),
				OCI_ATTR_USERNAME,
				error_handle);
		else
			throw (oralib::error (result, environment_handle, __FILE__, __LINE__));

		if (result == OCI_SUCCESS)
			result = OCIAttrSet (
				session_handle,
				OCI_HTYPE_SESSION,
				(text *) password,
				strlen (password),
				OCI_ATTR_PASSWORD,
				error_handle);

		// start the session
		if (result == OCI_SUCCESS)
			result = OCISessionBegin (
				svc_context_handle,
				error_handle,
				session_handle,
				OCI_CRED_RDBMS,
				OCI_DEFAULT);

		// set the user session attribute in the service context handle
		if (result == OCI_SUCCESS)
			result = OCIAttrSet (
				svc_context_handle,
				OCI_HTYPE_SVCCTX,
				session_handle,
				sizeof (OCISession *),
				OCI_ATTR_SESSION,
				error_handle);

		// switch to non-blocking mode?
		if (result == OCI_SUCCESS &&
			non_blocking_mode)
		{
			ub1	attr_value;

			attr_value = 1;
			result = OCIAttrSet (
				server_handle,
				OCI_HTYPE_SERVER,
				&attr_value,
				sizeof (attr_value),
				OCI_ATTR_NONBLOCKING_MODE,
				error_handle);
		}

		if (result == OCI_SUCCESS)
		{
			is_opened = true;
			is_available = true;
			is_blocking = !non_blocking_mode;
		}
		else
			throw (oralib::error (result, error_handle, __FILE__, __LINE__));
	}catch (oralib::error &e){
		xsys_dblock.unlock();
		throw e;
	}catch (...){
		xsys_dblock.unlock();
		throw "unknown error";
	}
	xsys_dblock.unlock();

	lastTime = long(time(0));
}


void
connection::close (void)
{
	sword	result;

	// no prerequisites

	// just in case switch server to blocking mode
	if (server_handle != NULL){
		ub1	attr_value;

		attr_value = 0;
		result = OCIAttrSet (
			server_handle,
			OCI_HTYPE_SERVER,
			&attr_value,
			sizeof (attr_value),
			OCI_ATTR_NONBLOCKING_MODE,
			error_handle);
	}
	else
		result = OCI_SUCCESS;

	// end session
	if (session_handle && svc_context_handle && error_handle)
	{
		result = OCISessionEnd (
			svc_context_handle,
			error_handle,
			session_handle,
			OCI_DEFAULT);

		// detach from the oracle server
		result = OCIServerDetach (
			server_handle,
			error_handle,
			OCI_DEFAULT);
	}
	else
		result = OCI_SUCCESS;

	// free handles
	if (svc_context_handle != NULL){
		result = OCIHandleFree (svc_context_handle, OCI_HTYPE_SVCCTX);
	}

	if (session_handle != NULL){
		result = OCIHandleFree (session_handle, OCI_HTYPE_SESSION);
	}
	
	if (error_handle != NULL){
		result = OCIHandleFree (error_handle, OCI_HTYPE_ERROR);
	}
	
	if (server_handle != NULL){
		result = OCIHandleFree (server_handle, OCI_HTYPE_SERVER);
	}

	
	if (environment_handle != NULL){
		result = OCIHandleFree (environment_handle, OCI_HTYPE_ENV);
	}

	initialize();
}


void
connection::execute (
	IN const char *sql_block,
	IN OPTIONAL int sql_len)	// = -1
{
	statement st (*this, sql_block, sql_len);
	st.execute_prepared ();
}


statement*
connection::prepare (
	IN const char *sql_block,
	IN OPTIONAL int sql_len)	// = -1
{
	return (new statement (
		*this,
		sql_block,
		sql_len));
}


resultset*
connection::select (
	IN const char *select,
	IN OPTIONAL int select_len)	// = -1
{
	statement	*s = prepare (select, select_len);
	try
	{
		oralib::resultset	*r = s->select ();
		r->attach_statement (s);
		return (r);
	}
	catch (...)
	{
		s->release ();
		throw;
	}
}

long connection::geta(const char * sqlstring, long & r)
{
	return (r = geta(sqlstring));
}

long connection::geta(const char * sqlstring)
{
	long r;
	resultset* presult = select(sqlstring);
	if (presult->eod() || (*presult)[FIRST_COLUMN_NO].is_null()){
		r = 0;
	}else{
		r = (*presult)[FIRST_COLUMN_NO];
	}
	presult->release();
	return r;
}

const char * connection::geta(const char * sqlstring, char * d, int buflength)
{
	resultset* presult = select(sqlstring);
	if (presult->eod() || (*presult)[FIRST_COLUMN_NO].is_null()){
		*d = '\0';
	}else{
		if (buflength > 0) {
			strncpy(d, (char *)Pstr((*presult)[FIRST_COLUMN_NO]), buflength);
		}else
			strcpy(d, (char *)Pstr((*presult)[FIRST_COLUMN_NO]));
	}
	presult->release();
	return d;
}


};	// oralib namespace

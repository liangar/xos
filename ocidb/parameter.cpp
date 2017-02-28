// OraLib 0.0.3 / 2002-06-30
//	parameter.cpp
//
//	http://606u.dir.bg/
//	606u@dir.bg

#include <xsys_oci.h>

namespace oralib
{

parameter::parameter (
	IN statement *to,
	IN const char *name,
	IN OPTIONAL DataTypesEnum type,	// = DT_UNKNOWN
	IN OPTIONAL ub2 fetch_size)		// = FETCH_SIZE
{
	initialize ();
	try
	{
		attach (
			to,
			name,
			type,
			fetch_size);
	}
	catch (...)
	{
		cleanup ();
		throw;
	}
}


parameter::~parameter ()
{
	cleanup ();
}


void
parameter::initialize (void)
{
	param_type = DT_UNKNOWN;
	oci_type = 0;
	size = 0;
	indicator = 0;
	data_len = 0;
	fetch_buffer = NULL;
	is_array = false;
	stmt = NULL;
	bind_handle = NULL;
	result_set = NULL;
	rs_handle = NULL;
}


void
parameter::cleanup (void)
{
	// set all to null to be save to call cleanup more than once for a single instance

	// bind handle cannot be freed
	if (bind_handle) bind_handle = NULL;

	// resultset class for bound cursor variable
	if (result_set) delete result_set, result_set = NULL;

	if (rs_handle)
	{
		// free statement handle if result set
		OCIHandleFree (
			rs_handle,
			OCI_HTYPE_STMT);
		rs_handle = NULL;
	}

	if (fetch_buffer) delete [] fetch_buffer, fetch_buffer = NULL;
}


void
parameter::attach (
	IN statement *to,
	IN const char *name,
	IN OPTIONAL DataTypesEnum type, // = DT_UNKNOWN
	IN OPTIONAL ub2 fetch_size)	// = FETCH_SIZE
{
	param_name = name;

	// setup type, oci_type, size, is_input, is_array
	setup_type (name, type);

	indicator = -1; // null by default
	data_len = 0;

	fetch_buffer = NULL;

	stmt = NULL;
	bind_handle = NULL;
	rs_handle = NULL;
	result_set = NULL;

	if (param_type == DT_RESULT_SET)
		bind_result_set (to, fetch_size);
	else
		bind (to);
}


void
parameter::setup_type (
	IN const char *param_name,
	IN OPTIONAL DataTypesEnum type)	// = DT_UNKNOWN
{
	const char	*p = param_name;

	if (p [0] == ':')
		p++;

	// an array?
	if (p [0] == PP_ARRAY)
	{
		is_array = true;
		p++;
	}

	// determine type
	if (type == DT_NUMBER || (type == DT_UNKNOWN && p [0] == PP_NUMERIC))
	{
		param_type = DT_NUMBER;
		oci_type = SQLT_VNU;
		size = sizeof (OCINumber);
	}
	else if (type == DT_DATE || (type == DT_UNKNOWN && p [0] == PP_DATE))
	{
		param_type = DT_DATE;
		oci_type = SQLT_ODT;
		size = sizeof (OCIDate);
	}
	else if (type == DT_TEXT || (type == DT_UNKNOWN && p [0] == PP_TEXT))
	{
		param_type = DT_TEXT;
		oci_type = SQLT_STR;
		size = MAX_OUTPUT_TEXT_BYTES;
	}
	else if (type == DT_RESULT_SET || (type == DT_UNKNOWN && p [0] == PP_RESULT_SET))
	{
		param_type = DT_RESULT_SET;
		oci_type = SQLT_RSET;
		size = sizeof (OCIBind *);
	}
	else
		// unrecognized data type
		throw (oralib::error (EC_BAD_PARAM_PREFIX, __FILE__, __LINE__, param_name));
}


void
parameter::bind (
	IN statement *to)
{
	sword	result;

	// allocate and initialize memory for the result
	fetch_buffer = new char [size];
	switch (param_type)
	{
	case	DT_NUMBER:
		OCINumberSetZero (
			to->conn->error_handle,
			(OCINumber *) fetch_buffer);
		break;

	case	DT_DATE:
		OCIDateSysDate (
			to->conn->error_handle,
			(OCIDate *) fetch_buffer);
		break;

	case	DT_TEXT:
		*((wchar_t *) fetch_buffer) = L'\0';
		break;

	default:
		throw (oralib::error (EC_INTERNAL, __FILE__, __LINE__, "Unsupported internal type"));
	}

	// bind
	data_len = static_cast<sb2> (size);
	result = OCIBindByName (
		to->stmt_handle,
		&bind_handle,
		to->conn->error_handle,
		(text *) (param_name.data ()),
		param_name.length (),
		fetch_buffer,
		size,
		oci_type,
		&indicator,
		&data_len,
		NULL,	// pointer to array of column-level return codes
		0,		// maximum possible number of elements of type m_nType
		NULL,	// a pointer to the actual number of elements (PL/SQL binds)
		OCI_DEFAULT);

#if defined (UNICODE)
	// request/pass text parameters as unicode (2.0)
	if (result == OCI_SUCCESS)
	{
		ub4	value = OCI_UCS2ID;
		result = OCIAttrSet (
			bind_handle,
			OCI_HTYPE_BIND,
			&value,
			sizeof (value),
			OCI_ATTR_CHARSET_ID,
			to->conn->error_handle);
	}
#endif // UNICODE defined?

	if (result != OCI_SUCCESS)
		throw (oralib::error (result, to->conn->error_handle, __FILE__, __LINE__, param_name.c_str ()));
	stmt = to;
}


void
parameter::bind_result_set (
	IN statement *to,
	IN OPTIONAL ub2 fetch_size)	// = FETCH_SIZE
{
	sword	result;

	// allocate a statement handle for the result set
	result = OCIHandleAlloc (
		to->conn->environment_handle,
		reinterpret_cast <void **> (&rs_handle),
		OCI_HTYPE_STMT,
		0,		// extra memory to allocate
		NULL);	// pointer to user-memory

	// bind statement handle as result set
	if (result == OCI_SUCCESS)
		result = OCIBindByName (
			to->stmt_handle,
			&bind_handle,
			to->conn->error_handle,
			(text *) param_name.data (),
			param_name.length (),
			&rs_handle,
			size,
			oci_type,
			NULL,	// pointer to array of indicator variables
			NULL,	// pointer to array of actual length of array elements
			NULL,	// pointer to array of column-level return codes
			0,		// maximum possible number of elements of type m_nType
			NULL,	// a pointer to the actual number of elements (PL/SQL binds)
			OCI_DEFAULT);
	else
		throw (oralib::error (result, to->conn->environment_handle, __FILE__, __LINE__));

	if (result != OCI_SUCCESS)
		throw (oralib::error (result, to->conn->error_handle, __FILE__, __LINE__, param_name.c_str ()));
	stmt = to;
}


parameter&
parameter::operator = (Pstr text)
{
	if (!text)
		indicator = -1; // set to null
	else if (param_type == DT_TEXT)
	{
#if defined (UNICODE)
		data_len = static_cast <ub2> (wcslen (text) * 2);
#else
		data_len = static_cast <ub2> (strlen (text));
#endif
		if (data_len > size)
			data_len = static_cast <ub2> ((size - 2) & ~1);
		memcpy (fetch_buffer, text, data_len);
#if defined (UNICODE)
		// zero-terminate
		*((wchar_t *) fetch_buffer + data_len / 2) = L'\0';
		// data len should include terminating zero
		data_len += sizeof (wchar_t);
#else
		*((char *) fetch_buffer + data_len) = '\0';
		data_len += sizeof (char);
#endif
		indicator = 0; // not null
	}
	else
		throw (oralib::error (EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
	return (*this);
}


parameter&
parameter::operator = (double value)
{
	if (param_type == DT_NUMBER)
	{
		sword result = OCINumberFromReal (
			stmt->conn->error_handle,
			&value,
			sizeof (double),
			reinterpret_cast <OCINumber *> (fetch_buffer));
		if (result != OCI_SUCCESS)
			throw (oralib::error (result, stmt->conn->error_handle, __FILE__, __LINE__));
		indicator = 0; // not null
	}
	else
		throw (oralib::error (EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
	return (*this);
}


parameter&
parameter::operator = (long value)
{
	if (param_type == DT_NUMBER)
	{
		sword result = OCINumberFromInt (
			stmt->conn->error_handle,
			&value,
			sizeof (long),
			OCI_NUMBER_SIGNED,
			reinterpret_cast <OCINumber *> (fetch_buffer));
		if (result != OCI_SUCCESS)
			throw (oralib::error (result, stmt->conn->error_handle, __FILE__, __LINE__));
		indicator = 0; // not null
	}
	else
		throw (oralib::error (EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
	return (*this);
}


parameter&
parameter::operator = (const datetime& d)
{
	if (param_type == DT_DATE)
	{
		d.set (*reinterpret_cast <OCIDate*> (fetch_buffer));
		indicator = 0; // not null
	}
	else
		throw (oralib::error (EC_BAD_INPUT_TYPE, __FILE__, __LINE__));
	return (*this);
}


Pstr
parameter::as_string (void) const
{
	if (param_type == DT_TEXT
		&& indicator != -1) // not null?
		return (reinterpret_cast <Pstr> (fetch_buffer));
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


double
parameter::as_double (void) const
{
	if (param_type == DT_NUMBER
		&& indicator != -1) // not null?)
	{
		double	value;
		sword result = OCINumberToReal (
			stmt->conn->error_handle, // could be changed
			reinterpret_cast <OCINumber *> (fetch_buffer),
			sizeof (double),
			&value);
		if (result == OCI_SUCCESS)
			return (value);
		else
			throw (oralib::error (result, stmt->conn->error_handle, __FILE__, __LINE__));
	}
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


long
parameter::as_long (void) const
{
	if (param_type == DT_NUMBER
		&& indicator != -1) // not null?)
	{
		long	value;
		sword result = OCINumberToInt (
			stmt->conn->error_handle, // could be changed
			reinterpret_cast <OCINumber *> (fetch_buffer),
			sizeof (long),
			OCI_NUMBER_SIGNED,
			&value);
		if (result == OCI_SUCCESS)
			return (value);
		else
			throw (oralib::error (result, stmt->conn->error_handle, __FILE__, __LINE__));
	}
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


datetime
parameter::as_datetime (void) const
{
	if (param_type == DT_DATE
		&& indicator != -1) // not null?)
		return (datetime (*reinterpret_cast <OCIDate *> (fetch_buffer)));
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


resultset&
parameter::as_resultset (void)
{
	if (param_type == DT_RESULT_SET)
	{
		if (!result_set)
		{
			// initialize
			// both could throw an exception
			result_set = new resultset (
				rs_handle,
				stmt->conn);
			result_set->fetch_rows ();
		}
		return (*result_set);
	}
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


}; // oralib namespace

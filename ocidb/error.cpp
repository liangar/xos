// OraLib 0.0.3 / 2002-06-30
//	error.cpp
//
//	http://606u.dir.bg/
//	606u@dir.bg

#include <xsys_oci.h>

namespace oralib {

error::error (
	IN sword ora_err,
	IN OCIError *error_handle,
	IN OPTIONAL const char *source_name,	// = NULL
	IN OPTIONAL long line_number,			// = -1
	IN OPTIONAL const char *format,			// = NULL
	...)
{
	// sets-up ora_code and description
	oracle_error (
		ora_err,
		error_handle,
		0);
	
	// concat user-specified details
	if (format)
	{
		va_list	va;
		va_start (va, format);
		concat_message (format, va);
		va_end (va);
	}

	type = ET_ORACLE;
	source = source_name;
	line_no = line_number;
}


error::error (
	IN sword ora_err,
	IN OCIEnv *env_handle,
	IN OPTIONAL const char *source_name,	// = NULL
	IN OPTIONAL long line_number,			// = -1
	IN OPTIONAL const char *format,			// = NULL
	...)
{
	// sets-up ora_code and description
	oracle_error (
		ora_err,
		NULL,
		env_handle);

	// concat user-specified details
	if (format)
	{
		va_list	va;
		va_start (va, format);
		concat_message (format, va);
		va_end (va);
	}

	type = ET_ORACLE;
	source = source_name;
	line_no = line_number;
}


error::error (
	IN sword oralib_err,
	IN OPTIONAL const char *source_name,	// = NULL
	IN OPTIONAL long line_number,			// = -1
	IN OPTIONAL const char *format,			// = NULL
	...)
{
	// sets-up code and description
	oralib_error (oralib_err);

	// concat user-specified details
	if (format)
	{
		va_list	va;
		va_start (va, format);
		concat_message (format, va);
		va_end (va);
	}

	type = ET_ORALIB;
	ora_code = 0;
	source = source_name;
	line_no = line_number;
}


// copy constructor
error::error (
	IN const error& err)
	:
	type (err.type),
	code (err.code),
	ora_code (err.ora_code),
	description (err.description),
	source (err.source),
	line_no (err.line_no)
{
	type = err.type;

}


error::~error ()
{
}


void
error::oracle_error (
	IN sword ora_err,
	IN OCIError *error_handle,
	IN OCIEnv *env_handle)
{
	bool	get_details = false;

	code = ora_err;
	switch (ora_err)
	{
	case	OCI_SUCCESS:
		description = "(OCI_SUCCESS)";
		break;

	case	OCI_SUCCESS_WITH_INFO:
		description = "(OCI_SUCCESS_WITH_INFO)";
		get_details = true;
		break;
	
	case	OCI_ERROR:
		description = "(OCI_ERROR)";
		get_details = true;
		break;
	
	case	OCI_NO_DATA:
		description = "(OCI_NO_DATA)";
		get_details = true;
		break;
	
	case	OCI_INVALID_HANDLE:
		description = "(OCI_INVALID_HANDLE)";
		break;
	
	case	OCI_NEED_DATA:
		description = "(OCI_NEED_DATA)";
		break;
	
	case	OCI_STILL_EXECUTING:
		description = "(OCI_STILL_EXECUTING)";
		get_details = true;
		break;
	
	case	OCI_CONTINUE:
		description = "(OCI_CONTINUE)";
		break;
	
	default:
		description = "unknown";
	}

	// get detailed error description
	if (get_details)
	{
		const int	max_text_len = 4000;
		char	*error_text = new char [max_text_len];

		if (error_text)
		{
			*error_text = '\0';
			if (error_handle)
				OCIErrorGet (
					error_handle,		// error handle (or environment handle)
					1,					// record to seek for (first is 1)
					NULL,				// sqlstate; not supported in 8.x
					&ora_code,			// ORA-xxxx error returned
					reinterpret_cast<text *> (error_text),
					max_text_len,
					OCI_HTYPE_ERROR);	// handle type
			else
				OCIErrorGet (
					env_handle,
					1,
					NULL,
					&ora_code,
					reinterpret_cast<text *> (error_text),
					max_text_len,
					OCI_HTYPE_ENV);

			description += " ";
			description += error_text;
			delete [] error_text;
		}
	}
}


void
error::oralib_error (
	IN sword oralib_err)
{
	code = oralib_err;
	switch (oralib_err)
	{
	case	EC_ENV_CREATE_FAILED:
		description = "(EC_ENV_CREATE_FAILED) Environment handle creation failed";
		break;

	case	EC_TIMEOUT:
		description = "(EC_TIMEOUT) Statement took too long to complete and has been aborted";
		break;

	case	EC_NO_MEMORY:
		description = "(EC_NO_MEMORY) Memory allocation request has failed";
		break;

	case	EC_BAD_PARAM_TYPE:
		description = "(EC_BAD_PARAM_TYPE) Parameter type is incorrect";
		break;

	case	EC_POOL_NOT_SETUP:
		description = "(EC_POOL_NOT_SETUP) Connection pool has not been setup yet";
		break;

	case	EC_BAD_INPUT_TYPE:
		description = "(EC_BAD_INPUT_TYPE) Input data doesn't have expected type";
		break;

	case	EC_BAD_OUTPUT_TYPE:
		description = "(EC_BAD_OUTPUT_TYPE) Cannot convert to requested type";
		break;

	case	EC_BAD_TRANSFORM:
		description = "(EC_BAD_TRANSFORM) Requested transformation is not possible";
		break;

	case	EC_BAD_PARAM_PREFIX:
		description = "(EC_BAD_PARAM_PREFIX) Parameter prefix is not known";
		break;

	case	EC_INTERNAL:
		description = "(EC_INTERNAL) Internal library error. Please, report to developers";
		break;

	case	EC_UNSUP_ORA_TYPE:
		description = "(EC_UNSUP_ORA_TYPE) Unsupported Oracle type - cannot be converted to numeric, date or text";
		break;

	case	EC_PARAMETER_NOT_FOUND:
		description = "(EC_PARAMETER_NOT_FOUND) Name not found in statement's parameters";
		break;

	case	EC_COLUMN_NOT_FOUND:
		description = "(EC_COLUMN_NOT_FOUND) Result set doesn't contain column with such name";
		break;

	default:
		description = "unknown";
	}
}

string
error::details (void) const
{
	static const char *error_types_text [] =
	{
		"unknown", "Oracle", "OraLib", "Win32 API"
	};

	return (
		"Error type: " + string (error_types_text [type]) + "\n"
		"Description: " + description + "\n");
}


void
error::concat_message (
	IN const char *format,
	IN va_list va)
{
	char	*buffer = new char [ERROR_FORMAT_MAX_MSG_LEN];
	if (buffer)
	{
		// vsnprintf - printf-like, with max buffer length and va_list input
		xsnprintf (
			buffer,
			ERROR_FORMAT_MAX_MSG_LEN - 1,
			format,
			va);
		description += ": ";
		description += buffer;
		delete [] buffer;
	}
}


}; // oralib namespace

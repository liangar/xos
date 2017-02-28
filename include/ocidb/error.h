// OraLib 0.0.3 / 2002-06-30
//	error.h
//
//	http://606u.dir.bg/
//	606u@dir.bg

#ifndef	_ERROR_H
#define	_ERROR_H


namespace oralib {


// error codes thrown from the library
enum ErrorCodesEnum
{
	EC_OCI_ERROR = -1,
	EC_ENV_CREATE_FAILED = 1000,
	EC_TIMEOUT,
	EC_NO_MEMORY,
	EC_BAD_PARAM_TYPE,
	EC_POOL_NOT_SETUP,
	EC_BAD_INPUT_TYPE,
	EC_BAD_OUTPUT_TYPE,
	EC_BAD_TRANSFORM,
	EC_BAD_PARAM_PREFIX,
	EC_UNSUP_ORA_TYPE,
	EC_PARAMETER_NOT_FOUND,
	EC_COLUMN_NOT_FOUND,
	EC_INTERNAL
};


// error types
enum ErrorTypesEnum
{
	ET_UNKNOWN = 0,
	ET_ORACLE,
	ET_ORALIB,
	ET_WINAPI
};


class error
{
public:
	ErrorTypesEnum	type;		// type
	sword		code;			// error code if library error or -1 if Oracle error
	sb4			ora_code;		// Oracle's error code - ORA-xxxxx
	string	description;	// error description as a text
	string	source;			// source file, where error was thrown (optional)
	long		line_no;		// line number, where error was thrown (optional)

private:
	// sets-up an oracle error details
	void oracle_error (
		IN sword ora_err,
		IN OCIError *error_handle,
		IN OCIEnv *env_handle);

	// sets-up a library error details
	void oralib_error (
		IN sword oralib_err);

	// formats printf-like message and concats it to the description
	void concat_message (
		IN const char *format,
		IN va_list va);

private:
	// private assignment operator - class could not be copied
	error& operator = (
		IN const error& /* err */) { return (*this); /* could not be copy-constructed */ };

public:
	// oracle error via error handle
	error (
		IN sword ora_err,
		IN OCIError *error_handle,
		IN OPTIONAL const char *source_name = NULL,
		IN OPTIONAL long line_number = -1,
		IN OPTIONAL const char *format = NULL,
		...);

	// oracle error via environment handle
	error (
		IN sword ora_err,
		IN OCIEnv *env_handle,
		IN OPTIONAL const char *source_name = NULL,
		IN OPTIONAL long line_number = -1,
		IN OPTIONAL const char *format = NULL,
		...);

	// library error
	error (
		IN sword oralib_err,
		IN OPTIONAL const char *source_name = NULL,
		IN OPTIONAL long line_number = -1,
		IN OPTIONAL const char *format = NULL,
		...);

	// copy constructor
	error (
		IN const error& err);

	~error ();

	// return error details (in a format, similar to display method output)
	string	details (void) const;

}; // error class


}; // oralib namespace


#endif	// _ERROR_H

// OraLib 0.0.3 / 2002-06-30
//	oralib.h
//
//	http://606u.dir.bg/
//	606u@dir.bg

#ifndef	_ORALIB_H
#define	_ORALIB_H


namespace oralib {


// L I B R A R Y   C O N F I G


// uncomment next line if you wish not to have error.display method
// (usable, because it forces linker to include iostream library)
#define	ORALIB_NO_ERROR_DISPLAY

// maximal length (in characters) of a text input and/or output parameter
const ub2 MAX_OUTPUT_TEXT_BYTES = 1024;

// number of rows to request on each fetch
const ub2 FETCH_SIZE = 20;

// maximal length (in ansi characters) of user-supplied error message to format
const ub2 ERROR_FORMAT_MAX_MSG_LEN = 1024;

// index of the first column in a result set (one of 0, 1)
const ub2 FIRST_COLUMN_NO = 1;

// index of the first parameter in a statement (one of 0, 1)
const ub2 FIRST_PARAMETER_NO = 1;

// unicode/ansi
// keep in mind, that those are valid for data values only
// parameter and column names are still in ansi
#if defined (UNICODE)
	typedef const wchar_t *Pstr;
	const int CHAR_SIZE = sizeof (wchar_t);
#else
	typedef const char *Pstr;
	const int CHAR_SIZE = sizeof (char);
#endif


// IN means an input parameter; OUT is an output parameter
// IN OUT is both input and output parameter
// OPTIONAL means, that value is not required - a default will be used instead
#if	!defined (IN)
#	define	IN
#endif
#if	!defined (OUT)
#	define	OUT
#endif
#if	!defined (OPTIONAL)
#	define	OPTIONAL
#endif


// internal data types are 4 only: number, date, text and result set
enum DataTypesEnum
{
	DT_UNKNOWN,
	DT_NUMBER,
	DT_DATE,
	DT_TEXT,
	DT_RESULT_SET
};


// parameter prefixes (for example: :n1 is a number, :sName is a text)
enum ParameterPrefixesEnum
{
	PP_ARRAY = 't',
	PP_NUMERIC = 'n',
	PP_DATE = 'd',
	PP_TEXT = 's',
	PP_RESULT_SET = 'c'
};


}; // oralib namespace


#endif	// _ORALIB_H

#include "error.h"
#include "datetime.inl"
#include "connection.h"
#include "column.h"
#include "resultset.h"
#include "statement.h"
#include "parameter.h"

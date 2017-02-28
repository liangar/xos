// OraLib 0.0.3 / 2002-06-30
//	column.cpp
//
//	http://606u.dir.bg/
//	606u@dir.bg

#include <xsys_oci.h>

namespace oralib {

column::column (
	IN resultset *rs,
	IN const char *name,
	IN ub4 name_len,
	IN ub2 oci_data_type,
	IN ub4 max_data_size,
	IN OPTIONAL int fetch_size)	// = FETCH_SIZE
{
	initialize ();

	col_name = string (name, name_len);
	oci_type = oci_data_type;

	// decide the format we want oci to return data in (oci_type member)
	switch (oci_data_type)
	{
	case	SQLT_INT:	// integer
	case	SQLT_LNG:	// long
	case	SQLT_UIN:	// unsigned int

	case	SQLT_NUM:	// numeric
	case	SQLT_FLT:	// float
	case	SQLT_VNU:	// numeric with length
	case	SQLT_PDN:	// packed decimal
		oci_type = SQLT_VNU;
		col_type = DT_NUMBER;
		size = sizeof (OCINumber);
		break;

	case	SQLT_DAT:	// date
	case	SQLT_ODT:	// oci date - should not appear?
		oci_type = SQLT_ODT;
		col_type = DT_DATE;
		size = sizeof (OCIDate);
		break;

	case	SQLT_CHR:	// character string
	case	SQLT_STR:	// zero-terminated string
	case	SQLT_VCS:	// variable-character string
	case	SQLT_AFC:	// ansi fixed char
	case	SQLT_AVC:	// ansi var char
	case	SQLT_VST:	// oci string type
		oci_type = SQLT_STR;
		col_type = DT_TEXT;
		size = (max_data_size + 1) * CHAR_SIZE; // + 1 for terminating zero!
		break;

	default:
		throw (oralib::error (EC_UNSUP_ORA_TYPE, __FILE__, __LINE__, name));
	}

	// allocate memory for indicators, column lengths and fetched data
	indicators = new sb2 [fetch_size];

	if (col_type == DT_TEXT)
		// only text columns requires length
		data_lens = new ub2 [fetch_size];
	else
		data_lens = NULL;

	fetch_buffer = new char [size * fetch_size];

	define_handle = NULL;

	if (!indicators || !fetch_buffer)
	{
		cleanup (); // because there could be some memory allocated
		// no memory
		throw (oralib::error (EC_NO_MEMORY, __FILE__, __LINE__));
	}

	result_set = rs;
}


column::~column ()
{
	cleanup ();
}


void
column::initialize (void)
{
	col_type = DT_UNKNOWN;
	oci_type = 0;
	size = 0;
	indicators = NULL;
	data_lens = NULL;
	fetch_buffer = NULL;
	define_handle = NULL;
	result_set = NULL;
}


void
column::cleanup (void)
{
	// set all to null to be save to call cleanup more than once for a single instance
	if (indicators) delete [] indicators, indicators = NULL;
	if (data_lens) delete [] data_lens, data_lens = NULL;
	if (fetch_buffer) delete [] fetch_buffer, fetch_buffer = NULL;
}


bool
column::is_null (void) const
{
	ub2	row_no = static_cast <ub2> (result_set->current_row % result_set->fetch_count);
	return (indicators [row_no] == -1);
}


Pstr
column::as_string (void) const
{
	ub2	row_no = static_cast <ub2> (result_set->current_row % result_set->fetch_count);
	if (indicators [row_no] == -1)
		return "";

	if (col_type == DT_TEXT &&
		indicators [row_no] != -1)
		return (reinterpret_cast <Pstr> (fetch_buffer + size * row_no));
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


double
column::as_double (void) const
{
	ub2	row_no = static_cast <ub2> (result_set->current_row % result_set->fetch_count);
	if (indicators [row_no] == -1)
		return 0;

	if (col_type == DT_NUMBER &&
		indicators [row_no] != -1)
	{
		double	value;
		sword result = ::OCINumberToReal (
			result_set->conn->error_handle, // could be changed
			reinterpret_cast <OCINumber *> (fetch_buffer) + row_no,
			sizeof (double),
			&value);
		if (result == OCI_SUCCESS)
			return (value);
		else
			throw (oralib::error (result, result_set->conn->error_handle, __FILE__, __LINE__));
	}
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


long
column::as_long (void) const
{
	ub2	row_no = static_cast <ub2> (result_set->current_row % result_set->fetch_count);
	if (indicators [row_no] == -1)
		return 0;

	if (col_type == DT_NUMBER &&
		indicators [row_no] != -1)
	{
		long	value;
		sword result = OCINumberToInt (
			result_set->conn->error_handle, // could be changed
			reinterpret_cast <OCINumber *> (fetch_buffer) + row_no,
			sizeof (long),
			OCI_NUMBER_SIGNED,
			&value);
		if (result == OCI_SUCCESS)
			return (value);
		else
			throw (oralib::error (result, result_set->conn->error_handle, __FILE__, __LINE__));
	}
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


datetime
column::as_datetime (void) const
{
	ub2	row_no = static_cast <ub2> (result_set->current_row % result_set->fetch_count);
	if (indicators [row_no] == -1)
		return datetime(1, jan, 1900);

	if (col_type == DT_DATE &&
		indicators [row_no] != -1)
		return (datetime (*(reinterpret_cast <OCIDate *> (fetch_buffer) + row_no)));
	else
		throw (oralib::error (EC_BAD_OUTPUT_TYPE, __FILE__, __LINE__));
}


}; // oralib namespace

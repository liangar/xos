// OraLib 0.0.3 / 2002-06-30
//	resultset.cpp
//
//	http://606u.dir.bg/
//	606u@dir.bg

#include <xsys_oci.h>

#include <ss_service.h>

namespace oralib {

resultset::resultset (
	IN OCIStmt *rs,
	IN connection *use,
	IN OPTIONAL ub2 fetch_size) // = FETCH_SIZE
{
	initialize ();
	try
	{
		attach (
			rs,
			use,
			fetch_size);
	}
	catch (...)
	{
		cleanup ();
		throw;
	}
}


resultset::~resultset ()
{
	cleanup ();
}


void
resultset::initialize (void)
{
	conn = 0;
	rs_handle = 0;
	stmt = 0;
	fetch_count = 0;
	rows_fetched = 0;
	current_row = 0;
	is_eod = false;
	is_described = false;
	is_defined = false;
}


void
resultset::cleanup (void)
{
	// free columns
	for (Columns::iterator i=columns.begin (); i!=columns.end (); ++i)
		delete (*i);
	columns.clear ();

	// when resultset is created by statement.select - freed by statement object
	// when created by parameter.as_resultset - freed by parameter object
	if (rs_handle) rs_handle = NULL;

	// statement attached to be released?
	if (stmt) stmt->release (), stmt = NULL;
}


void
resultset::attach (
	IN OCIStmt *rs,
	IN connection *use,
	IN OPTIONAL ub2 fetch_size)	// = FETCH_SIZE
{
	conn = use;
	rs_handle = rs;
	stmt = NULL;
	fetch_count = fetch_size;

	rows_fetched = 0;
	current_row = 0;
	is_eod = false;

	is_described = false;
	is_defined = false;

	describe ();
	define ();
}


ub4
resultset::columns_count (void)
{
	sword	result;
	ub4		count;

	count = 0;
	result = OCIAttrGet (
		rs_handle,
		OCI_HTYPE_STMT,
		&count,
		NULL,
		OCI_ATTR_PARAM_COUNT,
		conn->error_handle);
	if (result == OCI_SUCCESS)
		return (count);
	else
		throw (oralib::error (result, conn->error_handle, __FILE__, __LINE__));
}


ub4
resultset::rows_count (void)
{
	sword	result;
	ub4		count;

	count = 0;
	result = OCIAttrGet (
		rs_handle,
		OCI_HTYPE_STMT,
		&count,
		NULL,
		OCI_ATTR_ROW_COUNT,
		conn->error_handle);
	if (result == OCI_SUCCESS)
		return (count);
	else
		throw (oralib::error (result, conn->error_handle, __FILE__, __LINE__));
}


void
resultset::describe (void)
{
	sword		result;
	ub4			count, i;

	count = columns_count ();
	columns.reserve (count);
	for (i=0; i<count; i++)
	{
		// get next column info
		OCIParam	*param_handle = NULL;
		text		*param_name = NULL;
		ub4			name_len = 0;
		ub2			oci_type = 0;
		ub2			size = 0;

		result = OCIParamGet (
			rs_handle,
			OCI_HTYPE_STMT,
			conn->error_handle,
			reinterpret_cast <void **> (&param_handle),
			i + 1);	// first is 1

		if (result == OCI_SUCCESS)
		{
			// column name
			result = OCIAttrGet (
				param_handle,
				OCI_DTYPE_PARAM,
				&param_name,
				&name_len,
				OCI_ATTR_NAME,
				conn->error_handle);
		}

		if (result == OCI_SUCCESS)
		{
			// oci data type
			result = OCIAttrGet (
				param_handle,
				OCI_DTYPE_PARAM,
				&oci_type,
				NULL,
				OCI_ATTR_DATA_TYPE,
				conn->error_handle);
		}

		if (result == OCI_SUCCESS)
		{
			// maximum data size in bytes
			result = OCIAttrGet (
				param_handle,
				OCI_DTYPE_PARAM,
				&size,
				NULL,
				OCI_ATTR_DATA_SIZE,
				conn->error_handle);
		}
		if (size >= 2047) {
			WriteToEventLog("%s: too large size column(%d)", param_name, size);
			size = 2048;
		}

		if (param_handle)
			// ignore result code
			OCIDescriptorFree (
				param_handle,
				OCI_DTYPE_PARAM);

		// error situation?
		if (result != OCI_SUCCESS)
			throw (oralib::error (result, conn->error_handle, __FILE__, __LINE__));

		// setup new column, alloc memory for fetch buffer, indicators and data lens;
		// column.constructor could possibly throw an out-of-memory exception
		// in this case resultset will be partially initialized
		column	*c = new column (
			this,
			reinterpret_cast <const char *> (param_name),
			name_len,
			oci_type,
			size,
			fetch_count);
		
		// add to array AND to map
		// (makes possible to address columns by name AND index)
		columns.push_back (c);
		columns_map [c->col_name] = c;
	}

	is_described = true;
	is_defined = false;
}


void
resultset::define (void)
{
	Columns::iterator	i;
	sword	result;
	ub4		position;

	// define all columns
	position = 1;
	result = OCI_SUCCESS;
	for (i=columns.begin (); result == OCI_SUCCESS && i!=columns.end (); ++i)
	{
		result = OCIDefineByPos (
			rs_handle,
			&((*i)->define_handle),
			conn->error_handle,
			position++,
			(*i)->fetch_buffer,
			(*i)->size,			// fetch size for a single row (NOT for several)
			(*i)->oci_type,
			(*i)->indicators,
			(*i)->data_lens,	// will be NULL for non-text columns
			NULL,				// ptr to array of column-level return codes
			OCI_DEFAULT);

#if defined (UNICODE)
		// request text columns as unicode (2.0)
		if (result == OCI_SUCCESS &&
			(*i)->col_type == DT_TEXT)
		{
			ub4	value = OCI_UCS2ID;
			result = OCIAttrSet (
				(*i)->define_handle,
				OCI_HTYPE_DEFINE,
				&value,
				sizeof (value),
				OCI_ATTR_CHARSET_ID,
				conn->error_handle);
		}
#endif // UNICODE defined?

		if (result != OCI_SUCCESS)
			throw (oralib::error (result, conn->error_handle, __FILE__, __LINE__, (*i)->col_name.c_str ()));
	}
	is_defined = true;
}


void
resultset::attach_statement (
	IN statement *select)
{
	stmt = select;
}


void
resultset::fetch_rows (void)
{
	sword	result;
	ub4		old_rows_count = rows_fetched;

	result = OCIStmtFetch (
		rs_handle,
		conn->error_handle,
		fetch_count,
		OCI_FETCH_NEXT,
		OCI_DEFAULT);
	if (result == OCI_SUCCESS ||
		result == OCI_NO_DATA ||
		result == OCI_SUCCESS_WITH_INFO)
	{
		rows_fetched = rows_count ();
		if (rows_fetched - old_rows_count != fetch_count)
			is_eod = true;
	}
	else
		throw (oralib::error (result, conn->error_handle, __FILE__, __LINE__));
}


bool
resultset::next (void)
{
	current_row++;
	if (current_row >= rows_fetched)
		if (!is_eod)
			// fetch new block of rows; fetch_rows will set is_eod on true
			// when last block if rows has been fetched; will also update rows_fetched
			fetch_rows ();
		else
			return (false);
	if (current_row >= rows_fetched)
		return (false);

	return (true);
}


column&
resultset::operator [] (const char *column_name)
{
	ColumnsMap::iterator i = columns_map.find (column_name);
	if (i == columns_map.end ())
		// column with such name is not found
		throw (oralib::error (EC_COLUMN_NOT_FOUND, __FILE__, __LINE__, column_name));
	return (*(i->second));
}


column&
resultset::operator [] (ub2 column_index)
{
	if (column_index < FIRST_COLUMN_NO ||
		column_index > columns.size ())
		// no column with such index
		throw (oralib::error (EC_COLUMN_NOT_FOUND, __FILE__, __LINE__, "%d", (int) column_index));
	return (*(columns.at (column_index - FIRST_COLUMN_NO)));
}


}; // oralib namespace

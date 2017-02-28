// OraLib 0.0.3 / 2002-06-30
//	parameter.h
//
//	http://606u.dir.bg/
//	606u@dir.bg

#ifndef	_PARAMETER_H
#define	_PARAMETER_H


namespace oralib {


class resultset;

class parameter
{
	// friends
	friend class statement;

private:
	string		param_name;		// in the exact case, including leading ':'
	DataTypesEnum	param_type;		// as it will be returned
	ub2				oci_type;		// oracle's data type
	ub2				size;			// number of bytes required for

	sb2				indicator;		// 0 - ok; -1 - null
	ub2				data_len;		// number of bytes returned (used for text)
	char			*fetch_buffer;	// where data is returned

	bool			is_array;		// with values?

	statement		*stmt;			// parameter is bound to
	OCIBind			*bind_handle;
	OCIStmt			*rs_handle;		// if parameter is a result set
	resultset		*result_set;	// if parameter is a result set

private:
	// public not creatable; use statement.bind instead
	// attaches parameter to a statement
	// when type is set to DT_UNKNOWN type is taken from name's prefix
	parameter (
		IN statement *to,
		IN const char *name,
		IN OPTIONAL DataTypesEnum type = DT_UNKNOWN,
		IN OPTIONAL ub2 fetch_size = FETCH_SIZE);

	~parameter ();

	// private copy-constructor and assignment operator - class could not be copied
	parameter (
		IN const parameter& /* var */) { /* could not be copy-constructed */ };
	parameter& operator = (
		IN const parameter& /* var */) { return (*this); /* could not be copy-constructed */ };

	// initialize private data
	void initialize (void);

	// free resources allocated
	void cleanup (void);

private:
	// attaches parameter to a statement
	// when type is set to DT_UNKNOWN type is taken from name's prefix
	void attach (
		IN statement *to,
		IN const char *name,
		IN OPTIONAL DataTypesEnum type = DT_UNKNOWN,
		IN OPTIONAL ub2 fetch_size = FETCH_SIZE);

	// sets-up name, type, oci_type and size, depending on type value
	// when type is set to DT_UNKNOWN type is taken from name's prefix
	void setup_type (
		IN const char *param_name,
		IN OPTIONAL DataTypesEnum type = DT_UNKNOWN);

	// binds an input and/or output parameter to the statement to
	void bind (
		IN statement *to);

	// binds a result set - fetch_size rows will be retrieved on each step
	void bind_result_set (
		IN statement *to,
		IN OPTIONAL ub2 fetch_size = FETCH_SIZE);

public:
	// sets parameter value to null
	inline void to_null (void) { indicator = -1; };

	// sets parameter value to some text
	parameter& operator = (Pstr text);

	// sets parameter value to some double
	parameter& operator = (double value);

	// sets parameter value to some long
	parameter& operator = (long value);

	// sets parameter value to some date/time
	parameter& operator = (const datetime& d);

	// returns whether parameter value is null
	inline bool is_null (void) const { return (indicator == -1); };

	// returns parameter value as a text
	inline operator Pstr (void) const { return (as_string ()); };
	Pstr as_string (void) const;

	// returns parameter value as a double
	inline operator double (void) const { return (as_double ()); };
	double as_double (void) const;

	// returns parameter value as a long
	inline operator long (void) const { return (as_long ()); };
	long as_long (void) const;

	// returns parameter value as a date/time helper object
	inline operator datetime (void) const { return (as_datetime ()); };
	datetime as_datetime (void) const;

	// returns a resultset for a cursor bound variable
	inline operator resultset& (void) { return (as_resultset ()); };
	resultset& as_resultset (void);

	// (parameter is freed, when it's statement is released)
	inline void release (void) { };
}; // parameter class


}; // oralib namespace


#endif	// _PARAMETER_H

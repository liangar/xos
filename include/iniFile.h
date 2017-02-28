#ifndef _XINIFILE_H_
#define _XINIFILE_H_

// GetPrivateProfileInt
int ini_get_int(
	const char * sectionName,
	const char * keyName,
	int defaultValue,
	const char * fileName
);

// GetPrivateProfileString
int ini_get_string(
	const char * sectionName,
	const char * keyName,
	const char * defaultValue,
	char * resultString,
	int bufferSize,
	const char * fileName
);

// WritePrivateProfileString
int ini_write_string(
	const char * sectionName,
	const char * keyName,
	const char * keyValue,
	const char * fileName
);


#endif // _XINIFILE_H_


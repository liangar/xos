#include <xsys.h>

#include <lfile.h>

int read_file(char ** d, const char * filename, char * msg, int lmsg)
{
	return read_file(d, filename, 0, msg, lmsg);
}

int read_file(char * d, const char * filename, char * msg, int lmsg)
{
	return read_file(d, filename, 0, msg, lmsg);
}

int read_file (char ** d, const char * filename, long startpos, char * msg, int lmsg)
{
	if (d == 0 || filename == 0 || *filename == '\0')
		return -100;

	long l = get_file_size(filename);
	if (l < 0){
		if (msg && lmsg > 0){
			strncpy(msg, strerror(errno), lmsg);  msg[lmsg-1] = '\0';
		}
		return -1;
	}

	if (startpos > l) {
		return 0;
	}

#ifdef WIN32
        int h = _open(filename, O_RDONLY|O_BINARY);
#else
        int h = _open(filename, O_RDONLY);
#endif
	if (h == 0){
		if (msg && lmsg > 0)
			xsnprintf(msg, lmsg, "rf_error[%d] : cannot open the file[%s].", h, filename);
		return -2;
	}

	char * buffer = (char *)malloc(l+1);

	if (buffer  == NULL) {
		strncpy(msg, strerror(errno), lmsg);
		_close(h);
		return -3;
	}

	if (startpos > 0) {
		if (_lseek(h, startpos, SEEK_SET) < 0){
			xsnprintf(msg, lmsg, "rf_error[%d] : cannot seek the file[%s](%d).", filename, startpos);
			return -2;
		}
		l -= startpos;
	}

	long rc;
	if ((rc = _read(h, buffer, l)) != l) {
		if (msg && lmsg > 0)
			sprintf(msg, "fread fail, only read %d bytes\n", rc);
		free(buffer);
		_close(h);
		return -4;
	}
	_close(h);
	buffer[l] = '\0';	// add a '\0' to the tail
	*d = buffer;

	return l;
}

int read_file (char  * d, const char * filename, long startpos, char * msg, int lmsg)
{
	if (d == 0 || filename == 0 || *filename == '\0')
		return -100;

	long l = get_file_size(filename);
	if (l < 0){
		if (msg && lmsg > 0){
			strncpy(msg, strerror(errno), lmsg);  msg[lmsg-1] = '\0';
		}
		return -1;
	}

	if (startpos > l) {
		return 0;
	}

#ifdef WIN32
        int h = _open(filename, O_RDONLY|O_BINARY);
#else
        int h = _open(filename, O_RDONLY);
#endif
	if (h == 0){
		if (msg && lmsg > 0)
			xsnprintf(msg, lmsg, "rf_error[%d] : cannot open the file[%s].", h, filename);
		return -2;
	}

	if (startpos > 0) {
		if (_lseek(h, startpos, SEEK_SET) < 0){
			xsnprintf(msg, lmsg, "rf_error[%d] : cannot seek the file[%s](%d).", filename, startpos);
			return -2;
		}
		l -= startpos;
	}

	long rc;
	if ((rc = _read(h, d, l)) != l) {
		if (msg && lmsg > 0)
			sprintf(msg, "fread fail, only read %d bytes\n", rc);
		_close(h);
		return -3;
	}
	_close(h);
	d[l] = '\0';	// add a '\0' to the tail

	return l;
}

int write_file(const char * filename, const char * s, char * msg, int lmsg)
{
	int l = int(strlen(s));
	return write_file(filename, s, l, msg, lmsg) - l;
}

int write_file(const char * filename, const char * s, int len, char * msg, int lmsg)
{
	static const char szFunctionName[] = "write_file";

	// write file
	FILE * h;
	if (*filename == '+')
		h = fopen(filename+1, "ab");
	else
		h = fopen(filename, "wb");

	if (h == 0){
		if (msg != NULL && lmsg > 0)
			xsnprintf(msg, lmsg, "%s : cannot open the file[%s], errcode = %d.", szFunctionName, filename, h);
		return -1;
	}
	int i = 0;
	if (len > 0)
		i = (fwrite(s, 1, len, h));

	fclose(h);

	if (i != len){
		if (msg != NULL && lmsg > 0)
			xsnprintf(msg, lmsg, "%s : write %d bytes, and return %d.", szFunctionName, i);
	}
	return i;
}

time_t get_modify_time(char * timestring, const char * filename)
{
	SYS_FILE_INFO finfo;

	int r = xsys_file_info(filename, finfo);
	if (r < 0){
		if (timestring) {
			*timestring = 0;
		}
		return 0;
	}

	time_t tmodify = finfo.tMod;

	if (timestring) {
		struct tm *ltime;
		ltime = localtime(&tmodify);
		sprintf(timestring, "%04d%02d%02d%02d%02d%02d", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	}

	return tmodify;
}

long get_file_size(const char * filename)
{
	return xsys_file_size(filename);
}

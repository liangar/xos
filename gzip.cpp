#include <xsys.h>

#include <ss_service.h>
#include <l_str.h>

#include <gzip.h>

static	xsys_mutex m_mutex;

xsys_gzip::xsys_gzip()
{
	m_type[0] = 0;
	m_lastmsg[0] = 0 ;
	m_zip_path[0] = 0 ;

	m_files.init(ZIP_FILE_NUM, 8) ;

	if (!m_mutex.isopen()){
		m_mutex.init();
	}
}

xsys_gzip::~xsys_gzip()
{
	m_files.down();
}

bool xsys_gzip::set_zip_path(const char *path)
{
	if (!xsys_exist_path(path)){
		sprintf(m_lastmsg, "path not exist: %s", path);
		return false;
	}

	m_zip_path_len = strlen(path);
	if (m_zip_path_len >= sizeof(m_zip_path)){
		sprintf(m_lastmsg, "too long path name: %s", path);
		return false;
	}
	if (path[m_zip_path_len-1] == '\\' || path[m_zip_path_len-1] == '/')
		m_zip_path_len--;

	memcpy(m_zip_path, path, m_zip_path_len);
	m_zip_path[m_zip_path_len] = 0;

	m_files.init(ZIP_FILE_NUM, 8) ;
	
	return true;
}


void xsys_gzip::set_type(const char * ziptype)
{
	m_files.init(ZIP_FILE_NUM, 8);

	if (ziptype){
		strcpy(m_type, ziptype);
		if (*ziptype == 'g')
			m_zip_type = 2;	// gzip
		else
			m_zip_type = 1;	// tar
	}else
		m_zip_type = 1;
}

bool xsys_gzip::add(const char * filename, const char * fullname)
{
	if (filename == 0){
		strcpy(m_lastmsg, "param : <filename> is null");
		return false;
	}
	if (fullname == 0){
		strcpy(m_lastmsg, "param : <fullname> is null");
		return false;
	}

	xsys_zipfile afile;
	if (!xsys_exist_file(fullname)){
		sprintf(m_lastmsg, "file not exist : %s \n", fullname);
		afile.state = 1;
	}else
		afile.state = 0;
	strcpy(afile.filename, filename);
	strcpy(afile.fullname, fullname);
	m_files.add(&afile) ;

	return (afile.state == 0);
}

int  xsys_gzip::zip(char * resultname, const char * zipfile)
{
	static const char szFunctionName[] = "xsys_gzip::unzip";

	// 先删除文件
	xsys_clear_path(m_zip_path);

	char   fullname[MAX_PATH];

	int i, n;

	for (i = n = 0; i < m_files.m_count; i++){
		if (m_files[i].state == 0){
			sprintf(fullname, "%s/%s", m_zip_path, m_files[i].filename);
			xsys_cp(fullname, m_files[i].fullname);
			n++;
		}
	}
	sprintf(fullname, "%s/%s.tar", m_zip_path, zipfile);
	sprintf(m_cmd, "tar -cvf \"%s\" *", fullname);

	WriteToEventLog("run cmd : %s \n", m_cmd) ;

	if (m_mutex.lock(10000) != 0){
		sprintf(m_lastmsg, "%s: lock(10) path error", szFunctionName);
		return -2;
	}

	try{
	_chdir(m_zip_path);
	system(m_cmd);	

	if (m_zip_type == 2) // zip 
	{
		// write out file is : zipfile.tar.gz, .tar file is deleted by gzip cmd
		sprintf(m_cmd, "gzip -f \"%s\"", fullname);

		WriteToEventLog("run cmd : %s \n", m_cmd) ;
		system(m_cmd);

		sprintf(resultname, "%s.tar.gz", zipfile);
	}else{
		sprintf(resultname, "%s.tar", zipfile);
	}
	}catch(...){
		sprintf(m_lastmsg, "%s: unknown exception occured", szFunctionName);
		n = -3;
	}
	m_mutex.unlock();

	for (i = 0; i < m_files.m_count; i++){
		if (m_files[i].state == 0){
			sprintf(fullname, "%s/%s", m_zip_path, m_files[i].filename);
			xsys_rm(fullname);
		}
	}
	
	m_files.init(ZIP_FILE_NUM, 8);
	
	return n;		
}

int xsys_gzip::unzip(const char * zipfile, const char * temp_path, bool ignore_ext)
{
	static const char szFunctionName[] = "xsys_gzip::unzip";

	if (zipfile == 0){
		sprintf(m_lastmsg, "%s: param zipfile = 0", szFunctionName) ;
		return -1;
	}

	char unzip_path[MAX_PATH] ;
	strcpy(
		unzip_path,
		((temp_path == 0)? m_zip_path : temp_path)
		);
	
	xsys_clear_path(unzip_path);

	// use lastmsg get the tar output
	m_lastmsg[0] = 0;

	// 备份文件到temp目录, 因为gunzip会把原来的.gz删除
	char zip_file[MAX_PATH];		// 要解码的文件
	char unzip_file_temp[MAX_PATH];	// 要解码的临时文件

	sprintf(zip_file, "%s/%s",
		m_zip_path,
		zipfile
	);
	sprintf(unzip_file_temp, "%s/%s",
		unzip_path,
		zipfile
	);
	char * pext = strrchr(unzip_file_temp, '.');
	if (ignore_ext){	// 去文件尾部扩展名
		if (pext){
			*pext = 0;
			pext = strrchr(unzip_file_temp, '.');
		}
	}

	if (pext){
		if ((pext[1] == 'g' && pext[2] == 'z') ||
			(pext[1] == 't' && pext[2] == 'a' && pext[3] == 'r'))
		{
			if (pext[1] == 't')
				m_zip_type = 1;
			else
				m_zip_type = 2;
		}
	}else{	// 无扩展名的，增加扩展名
		if (m_zip_type == 1){
			strcat(unzip_file_temp, ".tar");
		}else if (m_zip_type == 2){
			strcat(unzip_file_temp, ".tar.gz");
		}
	}
	xsys_cp(unzip_file_temp, zip_file);

	if (m_mutex.lock(10000) != 0){
		sprintf(m_lastmsg, "%s: lock(10) path error", szFunctionName);
		return -2;
	}

	try{
	// 解压
	if (m_zip_type == 1){  // untar
			sprintf(m_cmd, "tar -xvf \"%s\"", unzip_file_temp);
		WriteToEventLog("run cmd : %s \n", m_cmd) ;
			_chdir(unzip_path);
			xsys_do_cmd(m_cmd, ".", m_lastmsg, sizeof(m_lastmsg));
	}else if(m_zip_type == 2){ // unzip
		// 解压缩文件到m_zip_path下，原来的.tar.gz文件被删除，产生.tar文件
			sprintf(m_cmd, "gunzip -f \"%s\"", unzip_file_temp);
		WriteToEventLog("run cmd : %s \n", m_cmd);
			_chdir(unzip_path);
		system(m_cmd) ;

			xsys_rm(unzip_file_temp);

			char * p = strrchr(unzip_file_temp, '.');
		*p = 0;

		// 解压文件到指定的目录
		sprintf(m_cmd, "tar -xvf \"%s\"",
				unzip_file_temp
		);
		WriteToEventLog("run cmd : %s \n", m_cmd) ;
			_chdir(unzip_path);
		xsys_do_cmd(m_cmd, ".", m_lastmsg, sizeof(m_lastmsg));
	}
	}catch(...){
		m_mutex.unlock();
		return -1;
	}
	m_mutex.unlock();

	// 把临时文件删除
	xsys_rm(unzip_file_temp);
	WriteToEventLog("remove : %s \n", unzip_file_temp) ;

	// 取得文件列表
	m_files.init(ZIP_FILE_NUM, 8);
	for (char * p = m_lastmsg; *p;){
		xsys_zipfile f;

		p = (char *)getaline(m_cmd, p, '\n');
		trim_all(m_cmd);
		char * q = strrchr(m_cmd, '/');
		if (!q)
			q = strrchr(m_cmd, '\\');
		if (q){
			strcpy(f.fullname, m_cmd);
			strcpy(f.filename, q+1);
		}else{
			strcpy(f.filename, m_cmd);
			sprintf(f.fullname, "%s/%s", temp_path, f.filename);
		}
		f.state = 0;
		m_files.add(&f);
	}
	return m_files.m_count;
}

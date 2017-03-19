#include <xsys.h>

#include <l_str.h>
#include <lfile.h>
#include <iniFile.h>

// GetPrivateProfileInt
int ini_get_int(
	const char * sectionName,
	const char * keyName,
	int defaultValue,
	const char * fileName
)
{
	char d[64], v[64];

	strcpy(d, xsys_ltoa(defaultValue));

	// 用GetPrivateProfileString取得string -> defaultString
	if (ini_get_string(sectionName, keyName, d, v, 64, fileName))
		return atoi(v);
	return defaultValue;
}

// GetPrivateProfileString
int ini_get_string(
	const char * sectionName,
	const char * keyName,
	const char * defaultValue,
	char * resultString,
	int bufferSize,
	const char * fileName
)
{
	*resultString = '\0';

	// 读入文件入缓冲区
	char *buf = 0;
	int l = read_file(&buf, fileName, 0, 0);
	if (l < 0)
		return 0;

	// 查找[<sectionName>]段
	char v[512];
	sprintf(v, "[%s]", sectionName);
	const char * p = strstr(buf, v);

	if (p == 0){
		::free(buf);
		return 0;
	}

	// 查找段中的\n<keyName>=<value>
	// 定位下一个段
	const char * pend = strstr(p+1, "\n[");
	if (pend == 0)
		pend = buf + l;

	if (keyName && *keyName){
		sprintf(v, "\n%s", keyName);
		const char * pkey = strstr(p+1, v);
		// 返回单个值
		if (pkey && pkey < pend){
			const char * pp = getaword(v, pkey, " \t", "=");
			getaline(resultString, pp+1);
			::free(buf);
			return int(strlen(resultString));
		}
		::free(buf);
		return 0;
	}

	// 解析所有的行并将=号之前的并入到返回值之中
	p = getaline((char *)p, p);
	char *pp;
	for (pp = resultString; p < pend; ){
		p = getaline(v, p);
		trimstr(v);
		if (v[0]){
			getaword(pp, v, '=');

			pp += strlen(pp) + 1;
		}
	}
	*pp = 0;
	::free(buf);
	// 返回
	return int(pp - resultString);
}

// WritePrivateProfileString
int ini_write_string(
	const char * sectionName,
	const char * keyName,
	const char * keyValue,
	const char * fileName
)
{
	// 读入文件入缓冲区
	char *buf;
	int l = read_file(&buf, fileName, 0, 0);
	if (l < 0)
		return 0;

	// 查找[<sectionName>]段
	char v[512];
	int l0 = sprintf(v, "[%s]", sectionName);
	char * p = strstr(buf, v);

	// 用q存放之后写入的字符串
	// 用pos存放要写入的位置
	char *	q = buf + l;// 不写入
	int		pos = l;	// 写入位置在最后
	if (p){
		p += l0;
		// 查找段中的\n[<keyName>]
		q = strstr(p, "\n[");
		if (q == 0)
			q = buf + l;
		else
			++q;

		sprintf(v, "\n%s=", keyName);
		char * pkey = strstr(p, v);

		// 找到位置
		if (pkey && pkey < q){
			// 在段内
			pos = int(pkey - buf) + 1;
			q = (char *)getaline(v, pkey+1);
		}else{
			// 段内没有,需要在段末添加
			pos = int(q - buf);
		}
	}

	FILE * out = fopen(fileName,"rb+");

	// 将文件走到地址
	if (out == 0){
		::free(buf);
		return 0;
	}

	int r = fseek(out, pos, SEEK_SET);
	if (r < 0){
		::free(buf);
		fclose(out);
		return 0;
	}

	// 写入key=value
	fprintf(out, "%s=%s\r\n", keyName, keyValue);

	// 写入改行之后的字符串
	if (*q)
		fputs(q, out);

	fclose(out);
	::free(buf);

	return 1;
}

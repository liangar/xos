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

	// ��GetPrivateProfileStringȡ��string -> defaultString
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

	// �����ļ��뻺����
	char *buf = 0;
	int l = read_file(&buf, fileName, 0, 0);
	if (l < 0)
		return 0;

	// ����[<sectionName>]��
	char v[512];
	sprintf(v, "[%s]", sectionName);
	const char * p = strstr(buf, v);

	if (p == 0){
		::free(buf);
		return 0;
	}

	// ���Ҷ��е�\n<keyName>=<value>
	// ��λ��һ����
	const char * pend = strstr(p+1, "\n[");
	if (pend == 0)
		pend = buf + l;

	if (keyName && *keyName){
		sprintf(v, "\n%s", keyName);
		const char * pkey = strstr(p+1, v);
		// ���ص���ֵ
		if (pkey && pkey < pend){
			const char * pp = getaword(v, pkey, " \t", "=");
			getaline(resultString, pp+1);
			::free(buf);
			return int(strlen(resultString));
		}
		::free(buf);
		return 0;
	}

	// �������е��в���=��֮ǰ�Ĳ��뵽����ֵ֮��
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
	// ����
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
	// �����ļ��뻺����
	char *buf;
	int l = read_file(&buf, fileName, 0, 0);
	if (l < 0)
		return 0;

	// ����[<sectionName>]��
	char v[512];
	int l0 = sprintf(v, "[%s]", sectionName);
	char * p = strstr(buf, v);

	// ��q���֮��д����ַ���
	// ��pos���Ҫд���λ��
	char *	q = buf + l;// ��д��
	int		pos = l;	// д��λ�������
	if (p){
		p += l0;
		// ���Ҷ��е�\n[<keyName>]
		q = strstr(p, "\n[");
		if (q == 0)
			q = buf + l;
		else
			++q;

		sprintf(v, "\n%s=", keyName);
		char * pkey = strstr(p, v);

		// �ҵ�λ��
		if (pkey && pkey < q){
			// �ڶ���
			pos = int(pkey - buf) + 1;
			q = (char *)getaline(v, pkey+1);
		}else{
			// ����û��,��Ҫ�ڶ�ĩ���
			pos = int(q - buf);
		}
	}

	FILE * out = fopen(fileName,"rb+");

	// ���ļ��ߵ���ַ
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

	// д��key=value
	fprintf(out, "%s=%s\r\n", keyName, keyValue);

	// д�����֮����ַ���
	if (*q)
		fputs(q, out);

	fclose(out);
	::free(buf);

	return 1;
}

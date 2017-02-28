// name_value.cpp: implementation of the name_value class.
//
//////////////////////////////////////////////////////////////////////
#include <xsys.h>
#include <name_value.h>

static void ex_dtoa(char * s, double d)
{
	sprintf(s, "%f", d);
	s += strlen(s) - 1;
	while (*s == '0') s--;
	if (*s == '.')  *s = '\0';
	else
		s[1] = '\0';
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

name_value::name_value()
	:m_i(0)
{
}

name_value::~name_value()
{
    close();
}

int name_value::open(const char *s)
{
	int n = countlines(s, '\n');
    close();
    m_values.init(n + 8, 16);

    return merge(s);
}

int name_value::merge(const char * s)
{
	int l = strlen(s) + 1;

    char * buf = new char[l];
	strcpy(buf, s);

	for (const char * p = buf; p && *p;){
        char * pvalue, * pcomment, name[64];

		p = (const char *)skipblanks((char *)p);
		if (*p == '\0')  break;

		pcomment = (char *)p;
		p = getaword(name, p, " \t\r\n", "<=");  // parse comment

		if (*p == '<'){
            p = getaword(pcomment, p+1, " \t\r\n", ">=");
			if(*p != '>')
				goto parse_error;

			trim_all(pcomment, pcomment);
			pvalue = skipblanks((char *)(p + 1));
		}else if (*p == '='){
			pvalue = (char *)p;
			* pcomment = '\0';
		}else
			goto parse_error;

		if (*pvalue != '=')
			goto parse_error;

        p = (const char *)skipchs(pvalue + 1, " \t");        // parse value
        if (*p == '\'' || *p == '"'){
            char ch = *p;
			p = getaword(pvalue, p + 1, ch);
			if (*(p - 1) != ch){
				goto parse_error;
			}
		}else{
			p = getaline(pvalue, p);
			trim_all(pvalue, pvalue);
		}

		createvalue(name, pvalue, XPARM_STR, pcomment);
    }
    delete[] buf;

	return 0;

parse_error:
	m_values.free_all();  delete[] buf;
	return -1;
}

int name_value::merge(name_value * pnv)
{
	const char * pname, * pvalue, * pcomment;
	for (pnv->gohead(); pnv->getnext(&pvalue, &pname, &pcomment) == 0;){
		createvalue(pname, (void*)pvalue, XPARM_STR, pcomment);
    }

    return 0;
}

int name_value::putvalue(const void *pv, const char *name, int type)
{
    s_name_value * pvalue = findvalue(name);
	if (pvalue == 0)  return -1;

    char v[32];
	const char * s;
    if (XPARM_TYPE(type) == XPARM_LONG){
		sprintf(v, "%d", *((long *)pv));
        s = v;
    }else if (XPARM_TYPE(type) == XPARM_DOUBLE){
        ex_dtoa(v, *((double *)pv));
        s = v;
    }else
        s = (char *)pv;
	int l = strlen(s);

	if (XPARM_TYPE(type) == XPARM_STR){
        if (l >= pvalue->len){
            // 重新分配空间
            int len = int(pvalue->value - pvalue->name);
            char * p = new char[len + l * 2 + 1];
            memmove(p, pvalue->name, len);
            delete[] pvalue->name;
            pvalue->name    = p;
            pvalue->comment = p + strlen(p) + 1;
            pvalue->value   = pvalue->comment + strlen(pvalue->comment) + 1;
            pvalue->len = l * 2 + 1;
        }
	}
    strcpy(pvalue->value, s);
	return 0;
}

int name_value::createvalue(const char *name, const void *pv, int type, const char * pcomment)
{
    s_name_value * pvalue = findvalue(name);

	if (pvalue)
		return putvalue(pv, (char *)name, type);

    char v[32];
	const char * s;
    if (XPARM_TYPE(type) == XPARM_LONG){
		sprintf(v, "%d", *((long *)pv));
        s = v;
    }else if (XPARM_TYPE(type) == XPARM_DOUBLE){
        ex_dtoa(v, *((double *)pv));
        s = v;
    }else
        s = (const char *)pv;

	int l0 = strlen(name);
    int l1 = strlen(s);
    int l2 = (pcomment != 0) ? strlen(pcomment) : 0;
    s_name_value snv;
    snv.len = l1*2 + 1;
    snv.name = new char[l0 + l2 + 2 + snv.len];
    snv.comment = snv.name + l0 + 1;
    snv.value = snv.comment + l2 + 1;
    strcpy(snv.name, name);
    strcpy(snv.value, s);
	if (l2 > 0)
		strcpy(snv.comment, pcomment);
	else
		snv.comment[0] = '\0';

    m_values.add(&snv);
	return 0;
}

//Add by Rain, 2002-12-09
int name_value::getcomment(void *pv, const char *name)
{
    if (pv == 0)  return -2;

    s_name_value * pvalue = findvalue(name);

	if (pvalue == 0)  return -1;
	strcpy((char *)pv, pvalue->comment);

	return 0;
}

int name_value::getvalue(void *pv, const char *name, int type)
{
    if (pv == 0)  return -2;

    s_name_value * pvalue = findvalue(name);

	if (pvalue == 0)  return -1;
	if (XPARM_TYPE(type) == XPARM_STR){
		strcpy((char *)pv, pvalue->value);
	}else if (XPARM_TYPE(type) == XPARM_DOUBLE)
		*((double *)pv) = atof(pvalue->value);
    else
		*((long *)pv) = atol(pvalue->value);

	return 0;
}

char * name_value::getvalue(const char * name)
{
    s_name_value * pvalue = findvalue(name);

	if (pvalue == 0)  return 0;
	return pvalue->value;
}

int name_value::getvalue(void **pv, const char *name, int type)
{
    if (pv == 0)  return -2;

    s_name_value * pvalue = findvalue(name);
	if (pvalue == 0)  return -1;

	if (XPARM_TYPE(type) == XPARM_STR){
		*pv = new char[strlen(pvalue->value) + 1];
		if (*pv == NULL)
			return -2;
		strcpy((char *)(*pv), pvalue->value);
	}else if (XPARM_TYPE(type) == XPARM_DOUBLE)
		*((double *)pv) = atof(pvalue->value);
    else
		*((long *)pv) = atol(pvalue->value);

	return 0;
}

name_value::s_name_value * name_value::findvalue(const char *name)
{
	for (int i = 0; i < m_values.m_count; i++){
        if (strcmp(name, m_values[i].name) == 0){
            return m_values.m_phandles + i;
		}
	}
    return 0;
}


int name_value::close()
{
    for (int i = 0; i < m_values.m_count; i++){
        delete[] m_values[i].name;
    }
    m_values.free_all();

	return 0;
}

void name_value::gohead(void)
{
    m_i = 0;
}

int name_value::getnext(const char **pv, const char **name, const char **comment)
{
    if (m_i >= m_values.m_count || m_values.m_count < 1)  return 1;

    s_name_value * pvalue = m_values.m_phandles + m_i++;

    *name = (const char *)pvalue->name;
    *pv = (const char *)pvalue->value;
	if (comment)
		*comment = (const char *)pvalue->comment;

    return 0;
}

int name_value::getnext(void *pv, char *name, char * comment, int type)
{
    if (m_i >= m_values.m_count || m_values.m_count < 1)  return 1;

    s_name_value * pvalue = m_values.m_phandles + m_i++;

    strcpy(name, pvalue->name);

	if (comment)
		strcpy(comment, pvalue->comment);

	if (pvalue == 0)  return -1;
	if (XPARM_TYPE(type) == XPARM_STR){
		strcpy((char *)pv, pvalue->value);
	}else if (XPARM_TYPE(type) == XPARM_DOUBLE)
		*((double *)pv) = atof(pvalue->value);
    else
		*((long *)pv) = atol(pvalue->value);

	return 0;
}

int name_value::getbuffer(char * d, int len)
{
	if (d == 0)  return -1;

	try{
		s_name_value * p = m_values.m_phandles;
		char * s = d;

		for (int i = 0; i < m_values.m_count && len > 0; i++, p++){
			int l;
			char * r = strchr(p->value, '"');
			char ch = (r != 0) ? '\'' : '"';
			if (p->comment && *p->comment != '\0')
				l = xsnprintf(s, len, "%s<%s>=%c%s%c \t\n", p->name, p->comment, ch, p->value, ch);
			else
				l = xsnprintf(s, len, "%s=%c%s%c\n", p->name, ch, p->value, ch);
			len -= l;
			s += l;
		}
		*s = '\0';
	}catch(...){
		return -1;
	}

	return 0;
}

int name_value::getbuffer(char **d)
{
	if (d == 0)  return -1;

	// caculate the buffer length
	s_name_value * p = m_values.m_phandles;

	int i, l;
	for (i = 0, l = 1; i < m_values.m_count; i++, p++){
		l += strlen(p->name) + strlen(p->value) + 4;
		if (p->comment && *p->comment != '\0')
			l += strlen(p->comment) + 3;
	}

	// allocate
	char * s = new char[l];

	// get string
	if ((i = getbuffer(s, l)) != 0){
		delete[] s;  return i;
	}

	*d = s;

	return 0;
}

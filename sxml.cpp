#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <l_str.h>
#include <lfile.h>

#include <sxml_parse.h>

struct ximl_item{
	unsigned short wtype;  int  nlen;  char * p;
};

static char * perrmsgs[] = {
    "缺少'<'",
    "缺少名字定义",
    "缺少属性值定义",
    "缺少项结束符号'</'",
    "项结束名称与开始定义的不同",
    "缺少'>'",
    "项定义缺少结束标志",
    "缺少'='",
    "缺少值定义",
    "项定义是空",
	"文件头定义出错"
};

static ximl_errmsg errmsg[XIML_MAX_ERR];
static int i_err = 0;

static char * pstart, * s, * pitems;
static int    ll, cc,
			  itags, iprops, ivalues, itts,
			  ntags, nprops, nvalues, ntts;

static char    * g_pbuf = 0;
static ximl_T  * g_pT;
static ximl_P  * g_pP;
static ximl_TT * g_pTT;

static char * pTname[XIML_MAX_DEEP];
static int nTlen[XIML_MAX_DEEP], i_tname = 0;

static int add_error(int errcode)
{
	errmsg[i_err].code = errcode;
	errmsg[i_err].col  = cc;
	errmsg[i_err].line = ll;
	errmsg[i_err++].pmsg = perrmsgs[(-errcode)];
	return errcode;
}

void X_geterror(char * msg)
{
	char * p = msg;
	for (int i = 0; i < i_err; i++){
		sprintf(p, "[line = %4d, col = %4d] : %s\n", errmsg[i].line, errmsg[i].col, errmsg[i].pmsg);
		p += strlen(p);
	}
}

static char * add_item(unsigned short type, char * p, int len)
{
	int l;	unsigned short w;	char * pitem;

	if (type == XT_TAG_END){
		pitem = pitems;
		*((unsigned short *)pitems) = XT_TAG_END | 0;  pitems += sizeof(unsigned short);
		return pitem;
	}

	if (len < 0 || len > 4*1024)
		return NULL;

	for (pitem = pstart; pitem < pitems;){	// 查找相同的项
		w = *((unsigned short *)pitem);						// 取得类型
		if (w & XT_REF){							// 引用类型的无需比较
			pitem += sizeof(unsigned short) + sizeof(int);  continue;
		}
		if (w & XT_TAG_END){
			pitem += sizeof(unsigned short);  continue;
		}
		l = w & XT_LEN_MASK;						// 取得长度
		if ((w & XT_TYPE_MASK) == type && l == len+1 && _strnicmp(p, pitem+sizeof(unsigned short), len) == 0){
			*((unsigned short *)pitems) = w | XT_REF;			// 添加引用类型的项
			*((int *)(pitems+sizeof(unsigned short))) = int(pitem + sizeof(unsigned short) - pstart);
			pitem = pitems;
			pitems += sizeof(unsigned short) + sizeof(int);
			return pitem;
		}
		pitem += sizeof(unsigned short) + l;
	}
	// 添加新的项
	pitem = pitems;
	*((unsigned short *)pitems) = type | ((unsigned short)len+1);	pitems += sizeof(unsigned short);
	if (len)
		memmove(pitems, p, len);
	pitems[len] = '\0';  pitems += len + 1;
	return pitem;
}

static char * get_item(ximl_item * d, char * s)
{
	bool isref;
	d->wtype = *(unsigned short *)s;
	d->nlen  = XT_LEN_MASK & d->wtype;
	isref    = (XT_REF & d->wtype) == XT_REF;
	d->wtype &= (XT_TYPE_MASK & ~XT_REF);
	s += sizeof(unsigned short);
	if (isref)
		d->p = pstart + *((int*)s);
	else
		d->p = s;
	if (d->wtype == XT_TAG_END)
		return s;
	return s+((isref)?sizeof(int):d->nlen);
}

static char skip_b_blank(void);
static char * skip_f_blank(char *p);
static bool goto_char(char ch);

static int  get_name  (char **p);   // return name length (0..n)
static int  get_t_prop(char **p);   // return prop length (0..n)
static int  get_p_prop(char **p);   // return prop length (0..n)

static char get_n_char(void)
{
    if (*s){
        s++;
        if (*s == '\xa')  { ll++;  cc = 0;  }
        if (*s == '\xd')  cc = 0; else  cc++;
    }
    return *s;
}

static char get_char(void) {  char ch = *s;  get_n_char();  return ch;  }
static char * get_p(void)  {  char * p = s;  get_n_char();  return p;  }

static char skip_b_blank(void)
{
    while (*s && ISBLANKCH(*s)){
        get_n_char();
    }
    return *s;
}

static char * skip_f_blank(char * p)
{
    do{  --p;  }while (p != pstart && ISBLANKCH(*p));
    return p;
}

static bool goto_char(char ch)
{
    while (*s && *s != ch) get_n_char();
    return (*s == ch);
}

static int get_name(char **p)
{
    int i;

    if (!isalpha(skip_b_blank()))  return 0;
    for (*p = get_p(), i = 1; *s == '_' || isalnum(*s); i++, get_n_char())
        ;
	ivalues++;
    return i;
}

static int get_t_prop(char **p)
{
    int i = 0;  char * t, * q;  int l;

    *p = s;
    while (goto_char('<')){
        q = get_p();
        if (skip_b_blank() != '/')  continue;
        get_n_char();
        l = get_name(&t);
		ivalues--;
        if (l == nTlen[i_tname-1] && _strnicmp(t, pTname[i_tname-1], l) == 0)
            break;
    }
    if (*s){
		ivalues ++;
        s = q;
        return q-(*p);
    }
    *p = NULL;
    return 0;
}

static int get_p_prop(char **p)
{
    int i = 0;

    if (isalpha(skip_b_blank())){
        i = get_name(p);
    }else if (*s == '\'' || *s == '"'){
        char ch = get_char();
        *p = s;
        if (goto_char(ch)){
            i = s - *p;
            get_n_char();
        }else
            i = -1;
    }else if (*s == '#'){
		*p = s;
        while (isxdigit(get_n_char()))
			;
		if (*s != '/' && *s != '>' && (!ISBLANKCH(*s))){
			i = -1;
		}else
			i = s - *p;
    }else if (isdigit(*s)){
		*p = s;
		while (isdigit(get_n_char()) || *s == '.')
			;
		if (*s != '/' && *s != '>' && (!ISBLANKCH(*s))){
			i = -1;
		}else
			i = s - *p;
	}else{
		*p = NULL;  i = -1;
	}

	if (i < 0)
		ivalues++;

    return i;
}

int head_valid(void)
{
	char * p;

    if (skip_b_blank() != '<')
        return XE_NO_TAGSTART;

	get_n_char();
	if (skip_b_blank() != '?'  ||
		get_n_char() == 0 ||
		get_name(&p) != 3      ||
		_strnicmp(p, "xml", 3))
		return XE_HEADER;
	ivalues --;
	if (skip_b_blank() == 'v' || *s == 'V'){
		if (get_name(&p) != 7 || _strnicmp(p, "version", 7) || skip_b_blank() != '=')
			return XE_HEADER;
		ivalues --;
		get_n_char();
		if (get_p_prop(&p))
			ivalues --;
		else
			return XE_HEADER;
	}
	p = strstr(s, "?>");
	if (p)
		s = p+2;
	else
		return XE_HEADER;
	return 0;
}

int T(void), Ps(void), C(void);

int X(char * p)
{
    int ret;

	i_err = 0;
	s = pitems = pstart = p;

	itags = iprops = ivalues = itts = 0;
	ntags = nprops = nvalues = ntts = 0;

	if (ret = head_valid())
		return add_error(ret);
	while (skip_b_blank()){
        if (ret = T())
            break;
    }
	if (ret)
		add_error(ret);	
	ntags = itags;  nprops = iprops;  ntts = itts;

    return ret;
}

void X_free(void)
{
	if (g_pbuf){
		delete[] g_pbuf;  g_pbuf = 0;
	}
	g_pT = 0;
	g_pP = 0;
	g_pTT= 0;
}

int T()
{
    char * pname, * pitem;    int i, props;

    // get tag name
    if (skip_b_blank() != '<')
        return XE_NO_TAGSTART;
	get_n_char();
    if ((i = get_name(&pname)) < 1)
        return XE_NAME;
    
	// add a ximl item to the table
	pitem = add_item(XT_TAG_NAME, pname, i);

	if (*((unsigned short *)pitem) & XT_REF)
		pTname[i_tname] = pstart + (*(int *)(pitem + sizeof(unsigned short)));
	else
		pTname[i_tname] = pitem+sizeof(unsigned short);
	nTlen[i_tname++] = i;

    // get props
    if ((props = Ps()) < 0)
        return XE_PROP;

    if (skip_b_blank() == '>'){
        get_n_char();
        if ((i = C()) < 0)
            if (i != XE_NO_TAG_VALUE || props == 0)
	            return i;
		if (i == XO_VALUE && props == 0){
			*((unsigned short *)pitem) = *((unsigned short *)pitem) & ~XT_TAG_NAME | XT_PROP_NAME;
			iprops++;
		}else{
			if ((*((unsigned short *)pitem) & XT_REF) == 0)
				itts++;
			itags++;
		}
        if (skip_b_blank() != '<' || *s++ == '\0' || skip_b_blank() != '/')
            return XE_END_TAGSTART;
		get_n_char();
        if (get_name(&pname) != nTlen[i_tname-1] ||
		    _strnicmp(pname, pTname[i_tname-1], nTlen[i_tname-1]))
            return XE_END_TAGNAME;
        if (skip_b_blank() != '>')
            return XE_END_TAGNAME_END;
		ivalues--;
        i_tname--;
		if ((*((unsigned short *)pitem) & XT_TAG_NAME) == XT_TAG_NAME)
			add_item(XT_TAG_END, NULL, 0);
    }else if (*s == '/'){
    	get_n_char();
        if (skip_b_blank() != '>')
            return XE_END_TAGNAME_END;

		if (props == 0)
			return XE_NO_PVALUE;

		if ((*((unsigned short *)pitem) & XT_REF) == 0)
			itts++;
		itags++;
        add_item(XT_TAG_END, NULL, 0);
		i_tname--;
    }else{
		i_tname--;
        return XE_END_TAG;
    }
    get_n_char();
    return 0;
}

int Ps()
{
    char * pname, * pvalue;  int lname, lvalue, i = 0;

    while ((lname = get_name(&pname)) > 0){
        if (skip_b_blank() != '=')
            return XE_NO_EQU;
		add_item(XT_PROP_NAME, pname, lname);
        get_n_char();
        if ((lvalue = get_p_prop(&pvalue)) < 0)
            return XE_NO_PVALUE;
        i++;
		iprops++;
		add_item(XT_PROP_VALUE, pvalue, lvalue);
    }
    return i;
}

static int strtrimall(char * d, char * s, int l)
{
	char ch = s[l];
	s[l] = '\0';
	int len = trim_all(d, s);
	s[l] = ch;

	return len;

/*
	int i, j;	bool instring;	char ch;
	for (i = j = 0, instring = false; j < l; j++){
		if ((i && d[i-1] != ' ') || !ISBLANKCH(s[j]) || instring == true){
			if (instring == false && ISBLANKCH(s[j]))
				d[i] = ' ';
			else{
				if (d+i != s+j)
					d[i] = s[j];
				if (instring){
					if (ch == s[j])
						instring = false;
				}else if (s[j] == '\'' || s[j] == '"'){
					ch = s[j];
					instring = true;
				}
			}
			i++;
		}
	}
	if (d[i-1] == ' ')
		i--;
	return i;
//*/
}

int C()
{
    char * p;  int l, ret;

    if (skip_b_blank() != '<'){
        if ((l = get_t_prop(&p)) <= 0)
            return XE_NO_TAG_VALUE;
		l = strtrimall(p, p, l);
		add_item(XT_PROP_VALUE, p, l);
        return XO_VALUE;
    }

	l = 0;
    while (skip_b_blank() == '<'){
        p = get_p();
        if (skip_b_blank() == '/'){
            s = p;  break;
        }
        s = p;
        if (ret = T())
            return add_error(ret);
    	l++;
    }

    if (l == 0)
        return XO_NO_CONTENT;

    return XO_TAGS;
}

void X_show(char * d)
{
	int lev;	char * p;	ximl_item item;

	for (p = pstart, lev = 0; p < pitems;){
		// get type, length and value
		if ((p = get_item(&item, p)) == NULL)
			break;
		if (item.wtype == XT_TAG_END){
			lev --;  continue;
		}
		// show
		for (int i = 0; i < lev; i++)
			*d++ = '\t';
		switch (item.wtype){
		case XT_TAG_NAME:
			lev ++;
			memcpy(d, "TAG : [", 7);  d += 7;
			break;
		case XT_PROP_NAME:
			memcpy(d, "PROP: [", 7);  d += 7;
			break;
		case XT_PROP_VALUE:
			*d++ = '\t';  *d++ = '[';
			break;
		}
		memcpy(d, item.p, item.nlen-1);  d += item.nlen-1;
		*d++ = ']';  *d++ = '\xd';  *d++ = '\xa';
	}
	*d = '\0';
	delete[] g_pbuf;  g_pbuf = 0;
}

static ximl_TT * add_tagtype(char * p)
{
	for (int i = 0; i < itts; i++){
		if (g_pTT[i].pname == p){
			g_pTT[i].n ++;
			return g_pTT + i;
		}
	}
	g_pTT[itts].pname = p;
	g_pTT[itts++].n     = 1;
	return (g_pTT + itts - 1);
}

int X_gen(ximl_T * p_t, ximl_TT ** p_tt)
{
	int i, l;	char * p;

	itags = iprops = ivalues = itts = 0;
	memset(p_t, 0, sizeof(ximl_T));

	// allocate buffer for intercode
	l = sizeof(ximl_T) * ntags + sizeof(ximl_P) * nprops + sizeof(ximl_TT) * ntts;
	i = int(pitems - pstart);

	p = g_pbuf;
	g_pbuf = new char[l+i];
	memcpy(g_pbuf, pstart, i);
	if (p)
		delete[] p;
	p = g_pbuf;
	pstart = p;  pitems = p + i;  p = pitems;

	memset(p, 0, l);
	g_pT  = (ximl_T  *)p;
	g_pP  = (ximl_P  *)(p + sizeof(ximl_T) * ntags);
	g_pTT = (ximl_TT *)(p + sizeof(ximl_T) * ntags + sizeof(ximl_P) * nprops);

	ximl_item item;
	ximl_T *  t = p_t, * pt;	ximl_P * pp;

	for (p = pstart; p < pitems;){
		// get type, length and value
		if ((p = get_item(&item, p)) == NULL)
			goto err_end;
		if (item.wtype == XT_TAG_END){
			t = t->pparent;  continue;
		}
		switch (item.wtype){
		case XT_TAG_NAME:
			g_pT[itags].pparent = t;
			g_pT[itags].ptype   = add_tagtype(item.p);
			if (t->nchildren == 0){
				t->pchildren = g_pT + itags;
				g_pT[itags].next = g_pT + itags;
				g_pT[itags].prev = g_pT + itags;
			}else{
				pt = t->pchildren;
				g_pT[itags].next = pt;
				g_pT[itags].prev = pt->prev;
				pt->prev->next = g_pT + itags;
				pt->prev = g_pT + itags;
			}
			t->nchildren++;
			t = g_pT + itags;
			itags ++;
			break;

		case XT_PROP_NAME:
			g_pP[iprops].pname = item.p;
			if ((p = get_item(&item, p)) == NULL || item.wtype != XT_PROP_VALUE)
				goto err_end;
			g_pP[iprops].pvalue= item.p;
			if (t->nprop == 0){
				t->prop = g_pP + iprops;
				g_pP[iprops].next = g_pP + iprops;
				g_pP[iprops].prev = g_pP + iprops;
			}else{
				pp = t->prop;
				g_pP[iprops].next = pp;
				g_pP[iprops].prev = pp->prev;
				pp->prev->next = g_pP + iprops;
				pp->prev = g_pP + iprops;
			}
			t->nprop ++;
			iprops++;
			break;
		}
	}
	*p_tt = g_pTT;
	return ntts;

err_end:
	if (g_pbuf){
		delete[] g_pbuf;  g_pbuf = 0;
	}
	return -1;
}

static int show_level;
char * X_Disp(char * d, ximl_T * pt);
char * show_lev(char * d, int i)  {  for (int j = 0; j < i; j++) for(int k = 0; k < 6; k++)  *d++ = ' ';  return d;  }
char * show_add(char * d, char * s)  {  int l = strlen(s);  memcpy(d, s, l);  return d+l; }
void X_D(char * d, ximl_T * pt)
{
	show_level = 0;
	for (int i = 0; i < ntts; i++){
		d = show_add(d, "TAG_T:[");  sprintf(d, "%s, %i]\r\n", g_pTT[i].pname, g_pTT[i].n);  d += strlen(d);
	}
	d = X_Disp(d, pt->pchildren);
	*d = '\0';
}
char * X_Disp(char * d, ximl_T * pt)
{
	for (; pt;  pt = pt->next){
		d = show_lev(d, show_level);
		d = show_add(d, "TAG  :[");  d = show_add(d, pt->ptype->pname);  d = show_add(d, "]\r\n");
		for (ximl_P * pp = pt->prop; pp; pp = pp->next){
			d = show_lev(d, show_level+1);
			d += sprintf(d, "PROP :[%s] [%s]\r\n", pp->pname, pp->pvalue);
			if (pp->next == pt->prop)
				break;
		}
		if (pt->pchildren){
			show_level ++;
			d = X_Disp(d, pt->pchildren);
			show_level --;
		}
		if (pt->next == pt->pparent->pchildren)
			break;
	}
	return d;
}

ximl_P * X_get_prop(ximl_T * t, const char * pname)
{
	if (!t)
		return NULL;

	for (ximl_P * p = t->prop; p; p = p->next){
		if (stricmp(pname, p->pname) == 0)  return p;
		if (p->next == t->prop)  break;
	}
	return NULL;
}

ximl_P * X_get_prop(const char * pname)
{
	for (int i = 0; i < nprops; i++){
		if (stricmp(g_pP[i].pname, pname) == 0){
			return g_pP+i;
		}
	}
	return NULL;
}

ximl_T * X_get_tag(const char * path)
{
	ximl_T * t = g_pT;
	char tags[512], * p = tags;


	memset(tags, 0, sizeof(tags));
	strcpy(tags, path);
	if (*p == '/')
		++p;
	splitstr(p, '/');

	for (;;){
		ximl_T * t0 = t;
		bool isfind = false;
		for (int i = 0; i < t->pparent->nchildren; i++, t0 = t0->next){
			if (stricmp(t0->ptype->pname, p) == 0){
				isfind = true;
				t = t0;
				break;
			}
		}
		if (!isfind)
			return NULL;
		
		p += strlen(p)+1;
		if (*p == 0)
			break;

		t = t->pchildren;
	}

	return t;
}

ximl_P * X_get_prop(const char * path, const char * pname)
{
	return X_get_prop(X_get_tag(path), pname);
}

int X_parse(ximl_T * t, ximl_TT ** tt, char * buf)
{
	int l;

	if (l = X(buf)){
		X_free();  return l;
	}
	return X_gen(t, tt);
}

int X_parsefile(ximl_T * t, ximl_TT ** tt, char * filename)
{
	int l;

	X_free();

	// read the file to memory
	{
		char msg[256];
		if ((l = read_file(&g_pbuf, filename, msg, 256)) < 0){
			X_free();
			return -1;
		}
		g_pbuf[l] = '\0';
	}
	return X_parse(t, tt, g_pbuf);
}

int X_count(char * pt_name)
{
	for (int i = 0; i < ntts; i++){
		if (strcmp(pt_name, g_pTT[i].pname) == 0)
			return g_pTT[i].n;
	}
	return 0;
}

ximl_P * X_get_prop_head(int &pcount)
{
	pcount = nprops;
	return g_pP;
}

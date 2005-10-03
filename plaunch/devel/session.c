/*
 * PuTTY saved session tree implementation.
 * This file is a part of SESSION_FOLDERS patch tree for PuTTY,
 * also is used in Pageant and PLaunch.
 *
 * (c) 2004 dwalin <dwalin@dwalin.ru>.
 * Portions (c) 1997-2004 Simon Tatham.
 *
 * This software and any part of it is distributed under MIT licence.
 */

#include <windows.h>
#include <stdio.h>

#include "session.h"

#define REGROOT			"Software\\SimonTatham\\PuTTY\\Sessions"
#define ISFOLDER		"IsFolder"
#define ISEXPANDED		"IsExpanded"
#define HOTKEY			"PLaunchHotKey"

#define BUFSIZE	2048
static const char hex[16] = "0123456789ABCDEF";
static int id = 0;

/*
static char *only_name(const char *name) {
	int i;
	char *p;

	if (!name)
		return NULL;

	p = name;

	while (*p) { p++; };

	if (p == name)
		return NULL;

	while (*p && p != name) {
		if (*p == "\\") {
			p++;
			return p;
		};
		p--;
	};

	return name;
};
*/

static char *mangle(const char *in)
{
    char *buf, *out, *ret;
    int candot, sz;

    candot = 0;
    sz = strlen(in) * 3 + 1;

    buf = malloc(sz);
    memset(buf, 0, sz);
    out = buf;

    while (*in) {
	if (*in == ' ' || *in == '*' || *in == '?' ||
	    *in == '%' || *in < ' ' || *in > '~' || (*in == '.'
						     && !candot)) {
	    *out++ = '%';
	    *out++ = hex[((unsigned char) *in) >> 4];
	    *out++ = hex[((unsigned char) *in) & 15];
	} else
	    *out++ = *in;
	in++;
	candot = 1;
    };

    ret = malloc(strlen(buf) + 1);
    strcpy(ret, buf);
    free(buf);

    return ret;
}

static char *unmangle(const char *in)
{
    char *buf, *out, *ret;
    int outlen;

    outlen = strlen(in);

    buf = malloc(outlen * 3 + 1);
    memset(buf, 0, outlen * 3 + 1);
    out = buf;

    while (*in) {
	if (*in == '%' && in[1] && in[2]) {
	    int i, j;

	    i = in[1] - '0';
	    i -= (i > 9 ? 7 : 0);
	    j = in[2] - '0';
	    j -= (j > 9 ? 7 : 0);

	    *out++ = (i << 4) + j;
	    in += 3;
	} else {
	    *out++ = *in++;
	};
    };
    ret = malloc(strlen(buf) + 1);
    strcpy(ret, buf);
    free(buf);

    return ret;
}

int session_compare(const struct session *s1, const struct session *s2)
{
    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(s1->name, DEFAULTSETTINGS))
	return -1;		/* a comes first */
    if (!strcmp(s2->name, DEFAULTSETTINGS))
	return +1;		/* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(s1->name, s2->name);	/* otherwise, compare normally */
}

struct session **session_insert(struct session **sl, int *snum,
				struct session *s)
{
    int i, j, sz;
    struct session **tmp;

    j = *snum;
    sz = sizeof(struct session *);

    /*
     * If slist is empty, just allocate it and make
     * sess the first and only entry.
     */
    if (sl == NULL) {
	tmp = malloc(sz);
	memset(tmp, 0, sz);
	tmp[0] = s;
	*snum = 1;
	return tmp;
    };

    /*
     * If slist is not empty, insert sess into it.
     */
    for (i = 0; i < j; i++) {
	if (session_compare(sl[i], s) < 0) {
	    tmp = malloc(sz * (j + 1));
	    memmove(tmp, sl, sz * i);
	    tmp[i] = s;
	    memmove(tmp + (i + 1), sl + i, sz * (j - i));
	    free(sl);
	    (*snum)++;
	    return tmp;
	};
    };

    /*
     * And if sess wasn't inserted, append it to the list.
     */
    tmp = malloc(sz * (j + 1));
    memmove(tmp, sl, sz * j);
    tmp[j] = s;
    (*snum)++;
    free(sl);

    return tmp;
};

struct session **session_remove(struct session **sl, int *snum,
				struct session *s)
{
    int i, j, pos;

    j = *snum;

    if (!sl || !j || !s)
	return NULL;

    /*
     * firstly check whether s is in session list or not
     */

    pos = 0;
    for (i = 0; i < j; i++) {
	if (sl[i] == s) {
	    pos = i;
	    break;
	};
    };
    if (!pos)
	return sl;

    /*
     * if s is the last entry, just null it.
     */
    if (pos == (j - 1)) {
	sl[pos] = NULL;
	j--;
	*snum = j;
	return sl;
    };

    /*
     * otherwise use magical memmove.
     */

    memmove(sl[pos], sl[pos + 1], (j - pos) * sizeof(struct session *));
    j--;
    *snum = j;

    return sl;
};

static char *lastname(char *in)
{
    char *p;

    if (in == "")
	return in;

    p = &in[strlen(in)];

    while (p >= in && *p != '\\')
	*p--;

    return p + 1;
};

char *session_get_full_path(struct session *s)
{
    char *buf, *buf2, *ret;

    if (!s || s->name[0] == '\0')
	return NULL;

    if (s->parent)
	buf2 = session_get_full_path(s->parent);

    buf = malloc(BUFSIZE);
    memset(buf, 0, BUFSIZE);

    if (buf2) {
	strcpy(buf, buf2);
	strcat(buf, "\\");
    };

    strcat(buf, s->name);

    ret = malloc(strlen(buf) + 1);
    strcpy(ret, buf);

    return ret;
};

int session_set_expanded(struct session *s, int isexpanded)
{
    char *p, *mp, *buf;
    HKEY key;
    int ret;
    DWORD exp;

    ret = FALSE;

    if (!s)
	return ret;

    p = session_get_full_path(s);

    if (!p)
	return ret;

    mp = mangle(p);
    free(p);

    if (!mp)
	return ret;

    buf = malloc(strlen(mp) + strlen(REGROOT) + 2);
    sprintf(buf, "%s\\%s", REGROOT, mp);
    free(mp);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, buf, 0,
		     KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
	exp = isexpanded;
	if (RegSetValueEx(key, ISEXPANDED, 0,
			  REG_DWORD, (LPBYTE) & exp,
			  sizeof(exp)) == ERROR_SUCCESS) {
	    RegCloseKey(key);
	    ret = TRUE;
	};
    };

    free(buf);

    return ret;
};

int session_set_hotkey(struct session *s, int index, int hotkey)
{
    char *p, *mp, *buf;
    HKEY key;
    int ret;
    DWORD hk;

    ret = FALSE;

    if (!s)
	return ret;

    p = session_get_full_path(s);

    if (!p)
	return ret;

    mp = mangle(p);
    free(p);

    if (!mp)
	return ret;

    buf = malloc(strlen(mp) + strlen(REGROOT) + 2);
    sprintf(buf, "%s\\%s", REGROOT, mp);
    free(mp);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, buf, 0,
		     KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
	hk = hotkey;
	p = (char *) malloc(BUFSIZE);
	sprintf(p, "%s%d", HOTKEY, index);
	if (hk) {
	    if (RegSetValueEx(key, p, 0,
			      REG_DWORD, (LPBYTE) & hk,
			      sizeof(hk)) == ERROR_SUCCESS) {
		RegCloseKey(key);
		ret = TRUE;
	    };
	} else {
	    if (RegDeleteValue(key, p) == ERROR_SUCCESS) {
		RegCloseKey(key);
		ret = TRUE;
	    };
	};
	free(p);
    };

    free(buf);

    return ret;
};

struct session *session_find_by_id(struct session *root, int id)
{
    int i;
    struct session *tmp;

    if (!root)
	return NULL;

    if (root->id == id)
	return root;

    for (i = 0; i < root->nchildren; i++) {
	if (root->children[i]->type) {
	    tmp = session_find_by_id(root->children[i], id);
	    if (tmp)
		return tmp;
	} else {
	    if (root->children[i]->id == id)
		return root->children[i];
	};
    };

    return NULL;
};

struct session *session_find_by_name(struct session *root, char *name,
				     int recurse)
{
    int i;
    struct session *tmp;

    if (!root || !name)
	return NULL;

    if (root->name && !strcmp(root->name, name))
	return root;

    for (i = 0; i < root->nchildren; i++) {
	if (recurse && root->children[i] && root->children[i]->type) {
	    tmp = session_find_by_name(root->children[i], name, recurse);
	    if (tmp)
		return tmp;
	} else {
	    if (root->children[i] && root->children[i]->name &&
		!strcmp(root->children[i]->name, name))
		return root->children[i];
	};
    };

    return NULL;
};

struct session *session_find_by_hotkey(struct session *root, int hotkey,
				       int recurse)
{
    int i, j;
    struct session *tmp;

    if (!root || !hotkey)
	return NULL;

    for (i = 0; i < root->nchildren; i++) {
	if (recurse && root->children[i] && root->children[i]->type) {
	    tmp =
		session_find_by_hotkey(root->children[i], hotkey, recurse);
	    if (tmp)
		return tmp;
	} else {
	    if (root->children[i] && !root->children[i]->type &&
		root->children[i]->nhotkeys) {
		for (j = 0; j < root->children[i]->nhotkeys; j++) {
		    if (root->children[i]->hotkeys[j] == hotkey)
			return root->children[i];
		};
	    };
	};
    };

    return NULL;
};

struct session *session_get_tree(char *name)
{
    HKEY key, key2;
    DWORD i, j, subkeys, values, size, isfolder, isexpanded;
    char *subkey, *subname, *unmangled;
    struct session *tmp, *tmp2;

    subkey = (char *) malloc(BUFSIZE);
    memset(subkey, 0, BUFSIZE);
    subname = (char *) malloc(BUFSIZE);
    memset(subname, 0, BUFSIZE);

    if (name == "") {
	sprintf(subname, "%s", REGROOT);
	id = 0;
    } else {
	sprintf(subname, "%s\\%s", REGROOT, name);
    };

    if ((RegOpenKeyEx(HKEY_CURRENT_USER, subname, 0,
		      KEY_READ, &key) != ERROR_SUCCESS) ||
	(RegQueryInfoKey(key, NULL, NULL, NULL, &subkeys,
			 NULL, NULL, &values, NULL, NULL,
			 NULL, NULL) != ERROR_SUCCESS)) {
	free(subkey);
	free(subname);

	return NULL;
    };

    unmangled = unmangle(lastname(name));
    tmp = session_new((subkeys > 0) ? STYPE_FOLDER : STYPE_SESSION,
		      unmangled, NULL);
    free(unmangled);

    size = sizeof(isexpanded);
    if (RegQueryValueEx(key, ISEXPANDED, NULL, NULL,
			(LPBYTE) & isexpanded, &size) == ERROR_SUCCESS)
	tmp->isexpanded = (int) isexpanded;

    size = sizeof(isfolder);
    if (RegQueryValueEx(key, ISFOLDER, NULL, NULL,
			(LPBYTE) & isfolder, &size) == ERROR_SUCCESS)
	tmp->type = isfolder;

    size = BUFSIZE;
    for (i = 0; i < subkeys; i++) {
	if (RegEnumKeyEx(key, i, (LPTSTR) subkey, &size,
			 NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
	    if (name == "") {
		sprintf(subname, "%s\\%s", REGROOT, subkey);
	    } else {
		sprintf(subname, "%s\\%s\\%s", REGROOT, name, subkey);
	    };
	    size = sizeof(isfolder);
	    if (RegOpenKeyEx(HKEY_CURRENT_USER, subname, 0,
			     KEY_READ, &key2) == ERROR_SUCCESS &&
		RegQueryValueEx(key2, ISFOLDER, NULL, NULL,
				(LPBYTE) & isfolder,
				&size) == ERROR_SUCCESS
		&& isfolder == TRUE) {
		if (name == "")
		    sprintf(subname, "%s", subkey);
		else
		    sprintf(subname, "%s\\%s", name, subkey);
		tmp2 = session_get_tree(subname);
		if (tmp2) {
		    tmp->children =
			session_insert(tmp->children, &tmp->nchildren,
				       tmp2);
		    tmp2->parent = tmp;
		};
		RegCloseKey(key2);
	    } else {
		unmangled = unmangle(subkey);
		tmp2 = session_new(STYPE_SESSION, unmangled, tmp);
		free(unmangled);
		for (j = 0; j < 2; j++) {
		    sprintf(subname, "%s%d", HOTKEY, j);
		    if (RegQueryValueEx(key2, subname, NULL, NULL,
					(LPBYTE) & isexpanded, &size) ==
			ERROR_SUCCESS) {
			tmp2->hotkeys[j] = (int) isexpanded;
			tmp2->nhotkeys++;
		    };
		};
		tmp->children =
		    session_insert(tmp->children, &tmp->nchildren, tmp2);
		RegCloseKey(key2);
	    };
	};
	size = BUFSIZE;
    };
    RegCloseKey(key);

    free(subkey);
    free(subname);

    return tmp;
};

static int new_session_record(char *name, int type)
{
    HKEY key;
    DWORD disp, value, size;

    if (!name ||
	RegCreateKeyEx(HKEY_CURRENT_USER, name, 0, NULL,
		       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key,
		       &disp) != ERROR_SUCCESS
	|| disp != REG_CREATED_NEW_KEY)
	return FALSE;

    if (type) {
	size = sizeof(DWORD);
	value = type;
	RegSetValueEx(key, ISFOLDER, 0, REG_DWORD, (LPBYTE) & value, size);
	value = 0;
	RegSetValueEx(key, ISEXPANDED, 0, REG_DWORD, (LPBYTE) & value,
		      size);
    };

    RegCloseKey(key);

    return TRUE;
};

struct session *session_create(int type, char *name,
			       struct session *parent)
{
    struct session *s;
    char *path, *mangled, *buf;

    s = session_new(type, name, parent);
    path = session_get_full_path(s);
    mangled = mangle(path);
    free(path);
    buf = (char *) malloc(BUFSIZE);
    sprintf(buf, "%s\\%s", REGROOT, mangled);
    free(mangled);

    if (!new_session_record(buf, type))
	s = session_free(s);

    free(buf);

    return s;
};

struct session *session_new(int type, char *name, struct session *parent)
{
    struct session *s;

    s = malloc(sizeof(struct session));
    memset(s, 0, sizeof(struct session));
    s->id = ++id;
    s->type = type;
    if (name) {
	s->name = (char *) malloc(strlen(name) + 1);
	strcpy(s->name, name);
    } else {
	s->name = (char *) malloc(1);
	s->name[0] = '\0';
    };
    s->parent = parent;

    return s;
};

struct session *session_free(struct session *s)
{
    int i;

    if (!s)
	return s;

    for (i = 0; i < s->nchildren; i++) {
	if (s->children[i]->type)
	    session_free(s->children[i]);
	else {
	    if (s->name)
		free(s->children[i]->name);
	    if (s->children[i]->children)
		free(s->children[i]->children);
	};
    };
    if (s->name)
	free(s->name);
    if (s->children)
	free(s->children);
    free(s);
    s = NULL;

    return s;
};

struct session *session_duplicate(struct session *s)
{
    struct session *snew;
    int i;

    if (!s)
	return NULL;

    snew = session_new(s->type, s->name, s->parent);
    snew->isexpanded = s->isexpanded;
    snew->nhotkeys = s->nhotkeys;
    memmove(snew->hotkeys, s->hotkeys, s->nhotkeys * sizeof(int));
    snew->nchildren = s->nchildren;
    if (snew->nchildren) {
	snew->children =
	    (struct session **) malloc(s->nchildren *
				       sizeof(struct session *));
	for (i = 0; i < s->nchildren; i++)
	    snew->children[i] = session_duplicate(s->children[i]);
    };

    return snew;
};

static int copy_session_tree(char *from, char *to)
{
    HKEY key1, key2;
    char *f, *t, *name;
    LPBYTE data;
    DWORD err, size, dsize, subkeys, values;
    int i;

    if (!from || !to)
	return FALSE;

    if ((err =
	 RegOpenKeyEx(HKEY_CURRENT_USER, from, 0, KEY_READ,
		      &key1)) != ERROR_SUCCESS)
	return FALSE;

    if ((err =
	 RegCreateKeyEx(HKEY_CURRENT_USER, to, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
			&key2, &size)) != ERROR_SUCCESS) {
	RegCloseKey(key1);
	return FALSE;
    };

    if ((err = RegQueryInfoKey(key1, NULL, NULL, NULL, &subkeys,
			       NULL, NULL, &values, NULL, NULL, NULL,
			       NULL)) != ERROR_SUCCESS) {
	RegCloseKey(key1);
	RegCloseKey(key2);
	return FALSE;
    };

    f = (char *) malloc(BUFSIZE);
    memset(f, 0, BUFSIZE);
    t = (char *) malloc(BUFSIZE);
    memset(t, 0, BUFSIZE);
    name = (char *) malloc(BUFSIZE);
    memset(name, 0, BUFSIZE);

    for (i = 0; i < (int) subkeys; i++) {
	size = BUFSIZE;
	if ((err = RegEnumKeyEx(key1, i, (LPSTR) name, &size,
				NULL, NULL, NULL,
				NULL)) == ERROR_SUCCESS) {
	    sprintf(f, "%s\\%s", from, name);
	    sprintf(t, "%s\\%s", to, name);
	    if (!copy_session_tree(f, t)) {
		free(f);
		free(t);
		free(name);
		RegCloseKey(key1);
		RegCloseKey(key2);
		return FALSE;
	    };
	}
    };

    data = (LPBYTE) malloc(BUFSIZE);

    for (i = 0; i < (int) values; i++) {
	size = dsize = BUFSIZE;
	if (((err = RegEnumValue(key1, i, (LPSTR) name, &size, NULL,
				 &subkeys, data, &dsize)) != ERROR_SUCCESS)
	    || ((err = RegSetValueEx(key2, name, 0, subkeys, data, dsize))
		!= ERROR_SUCCESS)) {
	    free(data);
	    free(f);
	    free(t);
	    free(name);
	    RegCloseKey(key1);
	    RegCloseKey(key2);
	    return FALSE;
	};
    };

    free(data);
    free(f);
    free(t);
    free(name);

    return TRUE;
};

struct session *session_copy(struct session *s, char *name)
{
    char *path, *mangled, *from, *to;
    int ret;
    struct session *snew;

    if (!s || !name)
	return FALSE;

    path = session_get_full_path(s);

    if (!path)
	return FALSE;

    from = (char *) malloc(BUFSIZE);
    to = (char *) malloc(BUFSIZE);

    mangled = mangle(path);
    sprintf(from, "%s\\%s", REGROOT, mangled);
    free(mangled);
    mangled = mangle(name);
    sprintf(to, "%s\\%s", REGROOT, mangled);
    free(mangled);

    ret = copy_session_tree(from, to);

    free(path);
    free(from);
    free(to);
/*
	if (ret) {
		snew = session_duplicate(s);
		if (snew->name)
			free(snew->name);
		to = only_name(name);
		snew->name = (char *)malloc(strlen(to) + 1);
		strcpy(snew->name, to);
		snew->parent = ses_parent;
		ses_parent->children = session_insert(ses_parent->children, 
											&ses_parent->nchildren, 
											snew);
	} else
	*/
    return NULL;
};

static int delete_session_record(char *name)
{
    HKEY key;
    DWORD i, subkeys, values, size;
    char *keyname, *subkey, *subname;

    keyname = malloc(BUFSIZE);
    memset(keyname, 0, BUFSIZE);
    subkey = malloc(BUFSIZE);
    memset(subkey, 0, BUFSIZE);
    subname = malloc(BUFSIZE);
    memset(subname, 0, BUFSIZE);

    if (name == "") {
	sprintf(keyname, "%s", REGROOT);
	id = 0;
    } else {
	sprintf(keyname, "%s\\%s", REGROOT, name);
    };

    if (RegOpenKeyEx(HKEY_CURRENT_USER, keyname, 0,
		     KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
	free(keyname);
	free(subkey);
	free(subname);
	return FALSE;
    };
    if (RegQueryInfoKey(key, NULL, NULL, NULL, &subkeys,
			NULL, NULL, &values, NULL, NULL,
			NULL, NULL) != ERROR_SUCCESS) {
	free(keyname);
	free(subkey);
	free(subname);
	return FALSE;
    };

    for (i = 0; i < subkeys; i++) {
	size = BUFSIZE;
	if (RegEnumKeyEx(key, 0, (LPTSTR) subkey, &size,
			 NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
	    if (name == "") {
		sprintf(subname, "%s", subkey);
	    } else {
		sprintf(subname, "%s\\%s", name, subkey);
	    };

	    if (!delete_session_record(subname)) {
		RegCloseKey(key);
		free(subkey);
		free(subname);
		free(keyname);

		return FALSE;
	    };
	};
    };
    RegCloseKey(key);

    RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0, KEY_ALL_ACCESS, &key);

    RegDeleteKey(key, keyname);

    RegCloseKey(key);

    free(keyname);
    free(subkey);
    free(subname);

    return TRUE;
};

int session_delete(struct session *s)
{
    char *path, *mangled;

    path = session_get_full_path(s);
    mangled = mangle(path);
    free(path);

    return delete_session_record(mangled);
};

struct session *session_rename(struct session *s, char *name)
{
    struct session *tmp;
    char *path, *buf;

    if (!s || !name)
	return FALSE;

    tmp = session_find_by_name(s->parent, name, FALSE);

    if (tmp)
	return FALSE;

    path = session_get_full_path(s->parent);
    buf = (char *) malloc(BUFSIZE);
    if (path) {
	strcpy(buf, path);
	strcat(buf, "\\");
    } else
	strcpy(buf, "");
    strcat(buf, name);

    if (!session_copy(s, buf) || !session_delete(s)) {
	free(buf);
	return FALSE;
    };

    free(buf);

    if (s->name)
	free(s->name);
    s->name = (char *) malloc(strlen(name) + 1);
    strcpy(s->name, name);

    return TRUE;
};

struct session *session_get_root(void)
{
    struct session *root;

    root = session_get_tree("");
    if (root) {
	root->isexpanded = 1;	/* root is always expanded */
	root->type = STYPE_FOLDER;	/* root is always a folder */
	root->parent = NULL;	/* and root is root. */
    };

    return root;
};

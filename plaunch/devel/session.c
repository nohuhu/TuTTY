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

#define BUFSIZE	2048
static const char hex[16] = "0123456789ABCDEF";
static int id = 0;

static char *mangle(const char *in) {
	char *buf, *out, *ret;
    int candot, sz;

    candot = 0;
    sz = strlen(in) * 3 + 1;

    buf = malloc(sz);
	memset(buf, 0, sz);
    out = buf;

    while (*in) {
		if (*in == ' ' || *in == '*' || *in == '?' ||
			*in == '%' || *in < ' ' || *in > '~' || (*in == '.' && !candot)) {
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

static char *unmangle(const char *in) {
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

static int sessioncmp(const char *a, const char *b) {
    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a, DEFAULTSETTINGS))
		return -1;		       /* a comes first */
    if (!strcmp(b, DEFAULTSETTINGS))
		return +1;		       /* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);	       /* otherwise, compare normally */
}

void *insert_session(void *slist, int *snum, void *sess) {
    struct session **sl;
    struct session *s;
    int i, j, sz;
    struct session **tmp;

    sl = (struct session **)slist;
    s = (struct session *)sess;
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
		if (sessioncmp(sl[i]->name, s->name) < 0) {
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
    tmp[j] = sess;
    (*snum)++;
    free(sl);

    return tmp;
};

char *lastname(char *in) {
    char *p;

    if (in == "")
		return in;
    
    p = &in[strlen(in)];

    while (p >= in && *p != '\\')
		*p--;

    return p + 1;
};

char *get_full_path(struct session *s) {
    char *buf, *buf2, *ret;

    if (!s)
		return NULL;

    if (s->name[0] == '\0')
		return NULL;

    if (s->parent)
		buf2 = get_full_path(s->parent);

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

int set_expanded(struct session *s, int isexpanded) {
    char *p, *mp, *buf;
    HKEY key;
    int ret;
    DWORD exp;

    ret = FALSE;

    if (!s)
		return ret;

    p = get_full_path(s);

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
			REG_DWORD, (LPBYTE)&exp, sizeof(exp)) == ERROR_SUCCESS) {
			RegCloseKey(key);
			ret = TRUE;
		};
    };

    free(buf);

    return ret;
};

struct session *find_session_by_id(struct session *root, int id) {
    int i;
    struct session *tmp;

    if (!root)
		return NULL;

    if (root->id == id)
		return root;

    for (i = 0; i < root->nchildren; i++) {
		if (root->children[i]->type) {
			tmp = find_session_by_id(root->children[i], id);
			if (tmp)
				return tmp;
		} else {
			if (root->children[i]->id == id)
				return root->children[i];
		};
    };

    return NULL;
};

struct session *find_session_by_name(struct session *root, char *name, int recurse) {
	int i;
	struct session *tmp;

	if (!root || !name)
		return NULL;

	if (root->name && !strcmp(root->name, name))
		return root;

	for (i = 0; i < root->nchildren; i++) {
		if (recurse && root->children[i] && root->children[i]->type) {
			tmp = find_session_by_name(root->children[i], name, recurse);
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

struct session *get_sessions(char *name) {
    HKEY key, key2;
    DWORD i, subkeys, values, size, isfolder, isexpanded;
    char *subkey, *subname, *unmangled;
    struct session *tmp, *tmp2;

    subkey = malloc(BUFSIZE);
	memset(subkey, 0, BUFSIZE);
    subname = malloc(BUFSIZE);
	memset(subname, 0, BUFSIZE);

    if (name == "") {
		sprintf(subname, "%s", REGROOT);
		id = 0;
	} else {
		sprintf(subname, "%s\\%s", REGROOT, name);
    };

    if (RegOpenKeyEx(HKEY_CURRENT_USER, subname, 0, 
					 KEY_READ, &key) != ERROR_SUCCESS)
		return NULL;
    if (RegQueryInfoKey(key, NULL, NULL, NULL, &subkeys,
						NULL, NULL, &values, NULL, NULL, 
						NULL, NULL) != ERROR_SUCCESS)
		return NULL;

	unmangled = unmangle(lastname(name));
	tmp = new_session((subkeys > 0) ? STYPE_FOLDER : STYPE_SESSION, 
		unmangled, NULL);
	free(unmangled);

    size = sizeof(isexpanded);
    if (RegQueryValueEx(key, ISEXPANDED, NULL, NULL,
						(LPBYTE)&isexpanded, &size) == 
						ERROR_SUCCESS)
		tmp->isexpanded = (int) isexpanded;

    size = BUFSIZE;
    for (i = 0; i < subkeys; i++) {
		if (RegEnumKeyEx(key, i, (LPTSTR)subkey, &size,
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
							(LPBYTE)&isfolder, &size) == ERROR_SUCCESS &&
				isfolder == TRUE) {
				if (name == "")
					sprintf(subname, "%s", subkey);
				else
					sprintf(subname, "%s\\%s", name, subkey);
				tmp2 = get_sessions(subname);
				if (tmp2) {
					tmp->children = insert_session(tmp->children, &tmp->nchildren, tmp2);
					tmp2->parent = tmp;
				};
				RegCloseKey(key2);
			} else {
				unmangled = unmangle(subkey);
				tmp2 = new_session(STYPE_SESSION, unmangled, tmp);
				free(unmangled);
				tmp->children = insert_session(tmp->children, &tmp->nchildren, tmp2);
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

struct session *new_session(int type, char *name, struct session *parent) {
	struct session *s;

	s = malloc(sizeof(struct session));
	s->id = ++id;
	s->type = type;
	s->isexpanded = FALSE;
	if (name) {
		s->name = (char *)malloc(strlen(name) + 1);
		strcpy(s->name, name);
	} else {
		s->name = (char *)malloc(1);
		s->name[0] = '\0';
	};
	s->nchildren = 0;
	s->children = NULL;
	s->parent = parent;

	return s;
};

struct session *free_session(struct session *s) {
    int i;

    if (!s)
		return s;

    for (i = 0; i < s->nchildren; i++) {
		if (s->children[i]->type)
			free_session(s->children[i]);
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

int rename_session(struct session *s, char *name) {
	struct session *tmp;

	if (!s || !name)
		return FALSE;

	tmp = find_session_by_name(s->parent, name, FALSE);

	if (tmp)
		return FALSE;

	if (s->name)
		free(s->name);
	s->name = (char *)malloc(strlen(name) + 1);
	strcpy(s->name, name);

	return TRUE;
};

struct session *get_root(void)
{
    struct session *root;

    root = get_sessions("");
	if (root) {
		root->isexpanded = 1;      /* root is always expanded */
		root->type = STYPE_FOLDER; /* root is always a folder */
		root->parent = NULL;       /* and root is root. */
	};

    return root;
};

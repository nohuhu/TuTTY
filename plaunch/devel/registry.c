#include <windows.h>
#include <stdio.h>
#include "entry.h"
#include "registry.h"

#define BUFSIZE			512

const char *const LIMIT_SEARCHFOR_STRINGS[] = {
    "last", "first", "second", "third", "fourth", "fifth",
    "sixth", "seventh", "eighth", "ninth", "tenth", "eleventh",
    "twelfth", "%dth"
};

const char *const LIMIT_ACTION_STRINGS[] = {
    "do nothing with", "hide", "show", "minimize", "maximize", "center",
    "kill", "murder", "run another"
};

const char *const AUTORUN_WHEN_STRINGS[] = {
    "start", "network start", "quit"
};

const char *const AUTORUN_ACTION_STRINGS[] = {
    "do nothing with", "hide", "show", "minimize", "maximize", "center"
};

static const char hex[16] = "0123456789ABCDEF";

static void mungestr(const char *in, char *out)
{
    int candot = 0;

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
    }
    *out = '\0';
    return;
};

static void unmungestr(const char *in, char *out, int outlen)
{
    while (*in) {
	if (*in == '%' && in[1] && in[2]) {
	    int i, j;

	    i = in[1] - '0';
	    i -= (i > 9 ? 7 : 0);
	    j = in[2] - '0';
	    j -= (j > 9 ? 7 : 0);

	    *out++ = (i << 4) + j;
	    if (!--outlen)
		return;
	    in += 3;
	} else {
	    *out++ = *in++;
	    if (!--outlen)
		return;
	}
    }
    *out = '\0';
    return;
};

int sessioncmp(const char *a, const char *b)
{
    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a, "Default Settings"))
	return -1;		/* a comes first */
    if (!strcmp(b, "Default Settings"))
	return +1;		/* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);	/* otherwise, compare normally */
};

static void stupid_sort(char **strings, unsigned int count)
{
    unsigned int i, j;
    char *tmp;

    for (i = 0; i < count; i++) {
	for (j = 0; j < count - 1; j++) {
	    if (sessioncmp(strings[j], strings[j + 1]) < 0) {
		tmp = strings[j];
		strings[j] = strings[j + 1];
		strings[j + 1] = tmp;
	    };
	};
    };
};

unsigned int reg_make_path(char *parent, char *path, char *buffer)
{
    if (parent && parent[0])
	sprintf(buffer, "%s\\%s\\%s", REGROOT, parent, path);
    else if (path && path[0])
	sprintf(buffer, "%s\\%s", REGROOT, path);
    else
	strcpy(buffer, REGROOT);

    return TRUE;
};

int reg_read_i(char *keyname, char *valname, int defval, int *value)
{
    HKEY key = 0;
    DWORD ret, type, size;
    char munge[BUFSIZE];

    ret = FALSE;
    *value = defval;

    mungestr(keyname, munge);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, munge, 0, KEY_READ, &key) !=
	ERROR_SUCCESS) {
	RegCloseKey(key);
	return ret;
    };

    type = REG_DWORD;
    size = sizeof(DWORD);

    if (RegQueryValueEx(key, valname, NULL, &type, (LPBYTE) value, &size)
	== ERROR_SUCCESS)
	ret = TRUE;

    RegCloseKey(key);

    return ret;
};

unsigned int reg_write_i(char *keyname, char *valname, int value)
{
    HKEY key;
    DWORD val, disp;
    char munge[BUFSIZE];

    mungestr(keyname, munge);
    if (RegCreateKeyEx
	(HKEY_CURRENT_USER, munge, 0, NULL, REG_OPTION_NON_VOLATILE,
	 KEY_ALL_ACCESS, NULL, &key, &disp) != ERROR_SUCCESS)
	return FALSE;

    val = value;
    if (RegSetValueEx
	(key, valname, 0, REG_DWORD, (LPBYTE) & val,
	 sizeof(DWORD)) == ERROR_SUCCESS)
	val = TRUE;
    else
	val = FALSE;

    RegCloseKey(key);

    return val;
};

unsigned int reg_read_s(char *keyname, char *valname, char *defval,
			char *buffer, unsigned int bufsize)
{
    HKEY key = 0;
    DWORD type, size;
    char munge[BUFSIZE];
    unsigned int ret;

    mungestr(keyname, munge);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, munge, 0, KEY_READ, &key) !=
	ERROR_SUCCESS) {
	RegCloseKey(key);
	return FALSE;
    };

    type = REG_SZ;
    size = bufsize;

    if (RegQueryValueEx(key, valname, NULL, &type, (LPBYTE) buffer, &size)
	== ERROR_SUCCESS && size > 0)
	ret = TRUE;
    else
	ret = FALSE;

    RegCloseKey(key);

    return ret;
};

unsigned int reg_write_s(char *keyname, char *valname, char *value)
{
    HKEY key;
    DWORD disp;
    char munge[BUFSIZE];
    int ret;

    mungestr(keyname, munge);
    if (RegCreateKeyEx
	(HKEY_CURRENT_USER, munge, 0, NULL, REG_OPTION_NON_VOLATILE,
	 KEY_ALL_ACCESS, NULL, &key, &disp) != ERROR_SUCCESS)
	return FALSE;

    ret =
	!RegSetValueEx(key, valname, 0, REG_SZ, (LPBYTE) value,
		       strlen(value));

    RegCloseKey(key);

    return ret;
};

unsigned int reg_delete_v(char *keyname, char *valname)
{
    HKEY key;
    DWORD ret;
    char munge[BUFSIZE];

    mungestr(keyname, munge);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, munge, 0, KEY_ALL_ACCESS, &key) !=
	ERROR_SUCCESS)
	return FALSE;

    if (RegDeleteValue(key, valname) == ERROR_SUCCESS)
	ret = TRUE;
    else
	ret = FALSE;

    RegCloseKey(key);

    return ret;
};

unsigned int reg_delete_k(char *keyname)
{
    HKEY key;
    char munge[BUFSIZE];
    DWORD err;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0,
		     KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
	return FALSE;

    mungestr(keyname, munge);

    err = RegDeleteKey(key, munge);
    RegCloseKey(key);

    return (err == ERROR_SUCCESS);
};

static unsigned int _reg_copy_tree(char *from, char *to)
{
    HKEY key1, key2;
    char f[BUFSIZE], t[BUFSIZE], name[BUFSIZE];
    BYTE data[16384];
    DWORD i, size, dsize, subkeys, values;

    if (!from || !to)
	return FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, from, 0, KEY_READ, &key1) !=
	ERROR_SUCCESS)
	return FALSE;

    if (RegCreateKeyEx
	(HKEY_CURRENT_USER, to, 0, NULL, REG_OPTION_NON_VOLATILE,
	 KEY_ALL_ACCESS, NULL, &key2, &size) != ERROR_SUCCESS) {
	RegCloseKey(key1);
	return FALSE;
    };

    if (RegQueryInfoKey(key1, NULL, NULL, NULL, &subkeys,
			NULL, NULL, &values, NULL,
			NULL, NULL, NULL) != ERROR_SUCCESS) {
	RegCloseKey(key1);
	RegCloseKey(key2);
	return FALSE;
    };

    for (i = 0; i < subkeys; i++) {
	size = BUFSIZE;
	if (RegEnumKeyEx(key1, i, (LPSTR) name, &size,
			 NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
	    sprintf(f, "%s\\%s", from, name);
	    sprintf(t, "%s\\%s", to, name);
	    if (!_reg_copy_tree(f, t)) {
		RegCloseKey(key1);
		RegCloseKey(key2);
		return FALSE;
	    };
	};
    };

    for (i = 0; i < values; i++) {
	size = dsize = 16384;
	if ((RegEnumValue(key1, i, (LPSTR) name, &size, NULL,
			  &subkeys, data, &dsize) != ERROR_SUCCESS) ||
	    (RegSetValueEx(key2, name, 0, subkeys, data, dsize) !=
	     ERROR_SUCCESS)) {
	    RegCloseKey(key1);
	    RegCloseKey(key2);
	    return FALSE;
	};
    };

    return TRUE;
};

unsigned int reg_copy_tree(char *from, char *to)
{
    char f[BUFSIZE], t[BUFSIZE];

    mungestr(from, f);
    mungestr(to, t);
    return _reg_copy_tree(f, t);
};

static int reg_delete_callback(char *name, char *path, int isfolder,
			       int mode, void *priv1, void *priv2,
			       void *priv3)
{
    char buf[512];

    switch (mode) {
    case REG_MODE_PREPROCESS:
	if (isfolder)
	    return 0;
	break;
    case REG_MODE_POSTPROCESS:
	if (!isfolder)
	    return 0;
	break;
    };

    reg_make_path(NULL, path, buf);
    return reg_delete_k(buf);
};

unsigned int reg_delete_tree(char *keyname)
{
    char buf[BUFSIZE];
    reg_walk_over_tree(keyname, reg_delete_callback, NULL, NULL, NULL);
    reg_make_path(NULL, keyname, buf);
    return reg_delete_k(buf);
};

unsigned int reg_move_tree(char *keyfrom, char *keyto)
{
    return (reg_copy_tree(keyfrom, keyto) && reg_delete_tree(keyfrom));
};

unsigned int reg_walk_over_tree(char *root, reg_callback cb,
				void *priv1, void *priv2, void *priv3)
{
    char str1[BUFSIZE], str2[BUFSIZE];
    HKEY key;
    DWORD err, i, subkeys, size, isfolder;
    char **slist;
    int ret;

    mungestr(root, str2);
    reg_make_path(NULL, str2, str1);

    if ((err =
	 RegOpenKeyEx(HKEY_CURRENT_USER, str1, 0, KEY_READ,
		      &key)) != ERROR_SUCCESS)
	return FALSE;

    if (RegQueryInfoKey(key, NULL, NULL, NULL, &subkeys, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS ||
	subkeys == 0)
	return FALSE;

    slist = (char **) malloc(subkeys * sizeof(char *));
    memset(slist, 0, subkeys * sizeof(char *));

    for (i = 0; i < subkeys; i++) {
	size = BUFSIZE;
	if (RegEnumKeyEx(key, i, str1, &size, NULL, NULL, NULL, NULL) ==
	    ERROR_SUCCESS) {
	    unmungestr(str1, str2, BUFSIZE);
	    slist[i] = (char *) malloc(strlen(str2) + 1);
	    strcpy(slist[i], str2);
	};
    };

    stupid_sort(slist, subkeys);

    for (i = 0; i < subkeys; i++) {
	reg_make_path(root, slist[i], str1);
	if (root && root != "")
	    sprintf(str2, "%s\\%s", root, slist[i]);
	else
	    strcpy(str2, slist[i]);
	reg_read_i(str1, ISFOLDER, 0, &isfolder);
	ret =
	    cb(slist[i], str2, isfolder, REG_MODE_PREPROCESS, priv1, priv2,
	       priv3);
	if (isfolder)
	    reg_walk_over_tree(str2, cb, (void *) ret, priv2, priv3);
	ret =
	    cb(slist[i], str2, isfolder, REG_MODE_POSTPROCESS,
	       (void *) ret, priv2, priv3);

	free(slist[i]);
    };

    free(slist);

    return TRUE;
};

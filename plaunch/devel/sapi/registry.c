/*
 * Session abstraction layer for TuTTY and PLaunch.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2005, 2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Windows-specific registry storage implementation file.
 */

#include <windows.h>
#include <stdio.h>
#if !defined(_DEBUG) && defined(MINIRTL)
#include "entry.h"
#endif
#include "registry.h"

static const char hex[16] = "0123456789ABCDEF";

static void mungestr(const char *in, char *out, int outlen)
{
    int candot = 0, ptr = 0;

    while (*in) {
	if ((*in == ' ' || *in == '*' || *in == '?' ||
	    *in == '%' || *in < ' ' || *in > '~' || 
	    (*in == '.' && !candot))) {
		if (ptr < (outlen - 4)) {
		    *out++ = '%';
		    *out++ = hex[((unsigned char) *in) >> 4];
		    *out++ = hex[((unsigned char) *in) & 15];
		    ptr += 3;
		} else
		    break;
	} else {
	    if (ptr < (outlen - 1)) {
		*out++ = *in;
		ptr++;
	    };
	};
	in++;
	candot = 1;
    };

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

int reg_make_path(char *parent, char *path, char *buffer,
		  int bufsize)
{
    return reg_make_path_specific(REGROOT, parent, path, 
		buffer, bufsize);
};

int reg_make_path_specific(char *root, char *parent, char *path,
			   char *buffer, int bufsize)
{
    int ret = 0;

    if (!root || !path || !buffer || !bufsize)
	return FALSE;

    if (parent && parent[0])
	ret = _snprintf(buffer, bufsize, "%s\\%s\\%s", root, 
		    parent, path);
    else if (path && path[0])
	ret = _snprintf(buffer, bufsize, "%s\\%s", root, path);
    else
	strncpy(buffer, root, bufsize);

    return ret < 0 ? FALSE : TRUE;
};

void *reg_open_session_r(session_root_t *root, char *keyname)
{
    char munge[BUFSIZE];

    mungestr(keyname, munge, BUFSIZE);

    return reg_open_key_r(root, munge);
};

void *reg_open_session_w(session_root_t *root, char *keyname)
{
    char munge[BUFSIZE];

    mungestr(keyname, munge, BUFSIZE);

    return reg_open_key_w(root, munge);
};

void *reg_open_key_r(session_root_t *root, char *keyname)
{
    HKEY key = 0;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, keyname, 0, KEY_READ, &key) !=
	ERROR_SUCCESS) {
	RegCloseKey(key);
	return NULL;
    };
    
    return (void *) key;
};

void *reg_open_key_w(session_root_t *root, char *keyname)
{
    HKEY key = 0;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, keyname, 0, 
		     KEY_ALL_ACCESS, &key) != ERROR_SUCCESS &&
	RegCreateKeyEx(HKEY_CURRENT_USER, keyname, 0, 
		     NULL, REG_OPTION_NON_VOLATILE,
		     KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS)
	return NULL;

    return (void *) key;
};

void reg_close_key(session_root_t *root, void *handle)
{
    RegCloseKey((HKEY) handle);
};

int reg_read_i(session_root_t *root, void *handle, char *valname, 
	       int defval, int *value)
{
    DWORD type, size;

    *value = defval;

    type = REG_DWORD;
    size = sizeof(DWORD);

    return RegQueryValueEx((HKEY) handle, valname, 
			    NULL, &type, (LPBYTE) value, 
			    &size) == ERROR_SUCCESS;
};

int reg_write_i(session_root_t *root, void *handle, char *valname, 
		int value)
{
    DWORD val;

    val = value;
    return RegSetValueEx((HKEY) handle, valname, 0, 
			 REG_DWORD, (LPBYTE) & val, 
			 sizeof(DWORD)) == ERROR_SUCCESS;
};

int reg_read_s(session_root_t *root, void *handle, char *valname, 
	       char *defval, char *buffer, int bufsize)
{
    DWORD type, size;

    type = REG_SZ;
    size = bufsize;

    return RegQueryValueEx((HKEY) handle, valname, 
			    NULL, &type, (LPBYTE) buffer, 
			    &size) == ERROR_SUCCESS && size > 0;
};

int reg_write_s(session_root_t *root, void *handle, char *valname, 
		char *value)
{
    return !RegSetValueEx((HKEY) handle, valname, 0, 
			    REG_SZ, (LPBYTE) value, 
			    strlen(value));
};

int reg_delete_v(session_root_t *root, void *handle, char *valname)
{
    return RegDeleteValue((HKEY) handle, valname) == ERROR_SUCCESS;
};

int reg_delete_k(session_root_t *root, char *keyname)
{
    HKEY key;
    char munge[BUFSIZE];
    DWORD err;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0,
		     KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
	return FALSE;

    mungestr(keyname, munge, BUFSIZE);

    err = RegDeleteKey(key, munge);
    RegCloseKey(key);

    return (err == ERROR_SUCCESS);
};

int reg_copy_session(session_root_t *root, char *frompath, char *topath)
{
    HKEY key1, key2;
    char from[BUFSIZE], to[BUFSIZE], name[BUFSIZE];
    BYTE data[16384];
    DWORD i, size, dsize, subkeys, values;

    if (!frompath || !topath)
	return FALSE;

    mungestr(frompath, from, BUFSIZE);
    mungestr(topath, to, BUFSIZE);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, from, 0, KEY_READ, &key1) !=
	ERROR_SUCCESS)
	return FALSE;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, to, 0, NULL, 
	REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
	&key2, &size) != ERROR_SUCCESS) {
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

    RegCloseKey(key1);
    RegCloseKey(key2);

    return TRUE;
};

typedef struct _reg_enumsettings_t {
    HKEY key;
    int i;
} reg_enumsettings_t;

void *reg_enum_settings_start(session_root_t *root, char *path)
{
    reg_enumsettings_t *ret;
    HKEY key;
    char munge[BUFSIZE];

    mungestr(path, munge, BUFSIZE);

    if (RegOpenKey(HKEY_CURRENT_USER, munge, &key) != ERROR_SUCCESS) {
	return NULL;
    };

    ret = (reg_enumsettings_t *) malloc(sizeof(reg_enumsettings_t));
    if (ret) {
	ret->key = key;
	ret->i = 0;
    }

    return ret;
}

int reg_enum_settings_count(session_root_t *root, void *handle)
{
    reg_enumsettings_t *e = (reg_enumsettings_t *)handle;
    DWORD subkeys;

    RegQueryInfoKey(e->key, NULL, NULL, NULL, &subkeys, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL);

    return (int) subkeys;
};

char *reg_enum_settings_next(session_root_t *root, void *handle, 
			     char *buffer, int buflen)
{
    reg_enumsettings_t *e = (reg_enumsettings_t *)handle;
    char *otherbuf;

    otherbuf = (char *)malloc(3 * buflen);
    if (RegEnumKey(e->key, e->i++, otherbuf, 3 * buflen) == ERROR_SUCCESS) {
	unmungestr(otherbuf, buffer, buflen);
	free(otherbuf);
	return buffer;
    } else {
	free(otherbuf);
	return NULL;
    }
}

void reg_enum_settings_finish(session_root_t *root, void *handle)
{
    reg_enumsettings_t *e = (reg_enumsettings_t *) handle;

    RegCloseKey(e->key);
    free(e);
};

typedef struct _reg_enumvalues_t {
    HKEY key;
    int i;
    int type;
} reg_enumvalues_t;

void *reg_enum_values_start(session_root_t *root, void *session)
{
    reg_enumvalues_t *e;

    if (!session)
	return NULL;

    e = (reg_enumvalues_t *) malloc(sizeof(reg_enumvalues_t));
    memset(e, 0, sizeof(reg_enumvalues_t));

    e->key = (HKEY) session;

    return e;
};

int reg_enum_values_count(session_root_t *root, void *handle)
{
    reg_enumvalues_t *e = (reg_enumvalues_t *) handle;
    DWORD values;

    if (e && RegQueryInfoKey(e->key, NULL, NULL, NULL, NULL, NULL, NULL,
	&values, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	return values;
    else
	return 0;
};

int reg_enum_values_type(session_root_t *root, void *handle)
{
    reg_enumvalues_t *e = (reg_enumvalues_t *) handle;

    return e->type;
};

char *reg_enum_values_next(session_root_t *root, void *handle,
			   char *buffer, int buflen)
{
    reg_enumvalues_t *e = (reg_enumvalues_t *) handle;
    DWORD size, type;

    size = buflen;

    if (e && RegEnumValue(e->key, e->i++, (LPTSTR) buffer, &size, NULL,
	&type, NULL, NULL) == ERROR_SUCCESS) {
	switch (type) {
	case REG_DWORD:
	    e->type = SES_VALUE_INTEGER;
	    break;
	case REG_SZ:
	    e->type = SES_VALUE_STRING;
	    break;
	default:
	    e->type = SES_VALUE_UNDEFINED;
	};
	return buffer;
    } else
	return NULL;
};

void reg_enum_values_finish(session_root_t *root, void *handle)
{
    reg_enumsettings_t *e = (reg_enumsettings_t *) handle;

    RegCloseKey(e->key);
    free(e);
};
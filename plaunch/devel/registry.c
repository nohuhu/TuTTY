#include <stdio.h>
#include "registry.h"
#include "misc.h"

#define BUFSIZE			2048

#define snew(type) ((type *)malloc(sizeof(type)))
#define snewn(n, type) ((type *)malloc((n)*sizeof(type)))
#define sresize(ptr, n, type) ((type *)realloc(ptr, (n)*sizeof(type)))
#define sfree(n) (free(n))

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
        return -1;                     /* a comes first */
    if (!strcmp(b, "Default Settings"))
        return +1;                     /* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);               /* otherwise, compare normally */
};

static void stupid_sort(char **strings, int count) {
	int i, j;
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

int reg_read_i(char *keyname, char *valname, int defval) {
	HKEY key;
	DWORD ret, type, value, size;
	char *munge;

	ret = defval;

	munge = (char *)malloc(3 * strlen(keyname) + 1);
	mungestr(keyname, munge);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, munge, 0, KEY_READ, &key) != ERROR_SUCCESS) {
		free(munge);
		return ret;
	};
	free(munge);

	type = REG_DWORD;
	value = 0;
	size = sizeof(DWORD);

	if (RegQueryValueEx(key, valname, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS)
		ret = value;

	RegCloseKey(key);

	return ret;
};

int reg_write_i(char *keyname, char *valname, int value) {
	HKEY key;
	DWORD val, disp;
	char *munge;

	munge = (char *)malloc(3 * strlen(keyname) + 1);
	mungestr(keyname, munge);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, munge, 0, NULL, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, NULL, &key, &disp) != ERROR_SUCCESS) {
		free(munge);
		return FALSE;
	};
	free(munge);

	val = value;
	if (RegSetValueEx(key, valname, 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD)) == ERROR_SUCCESS)
		val = TRUE;
	else
		val = FALSE;

	RegCloseKey(key);

	return val;
};

char *reg_read_s(char *keyname, char *valname, char *defval) {
	HKEY key;
	DWORD type, size;
	char *munge, *ret, *value;

	ret = defval;

	munge = (char *)malloc(3 * strlen(keyname) + 1);
	mungestr(keyname, munge);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, munge, 0, KEY_READ, &key) != ERROR_SUCCESS) {
		free(munge);
		return ret;
	};
	free(munge);

	type = REG_SZ;
	value = (char *)malloc(BUFSIZE);
	size = BUFSIZE;

	if (RegQueryValueEx(key, valname, NULL, &type, (LPBYTE)value, &size) == ERROR_SUCCESS &&
		size > 0)
		ret = dupstr(value);
	else
		ret = NULL;

	free(value);

	RegCloseKey(key);

	return ret;
};

int reg_write_s(char *keyname, char *valname, char *value) {
	HKEY key;
	DWORD disp;
	char *munge;
	int ret;

	munge = (char *)malloc(3 * strlen(keyname) + 1);
	mungestr(keyname, munge);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, munge, 0, NULL, REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, NULL, &key, &disp) != ERROR_SUCCESS) {
		free(munge);
		return FALSE;
	};
	free(munge);

	ret = !RegSetValueEx(key, valname, 0, REG_SZ, (LPBYTE)value, strlen(value));

	RegCloseKey(key);

	return ret;
};

int reg_delete_v(char *keyname, char *valname) {
	HKEY key;
	DWORD ret;
	char *munge;

	munge = (char *)malloc(3 * strlen(keyname) + 1);
	mungestr(keyname, munge);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, munge, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
		free(munge);
		return FALSE;
	};
	free(munge);

	if (RegDeleteValue(key, valname) == ERROR_SUCCESS)
		ret = TRUE;
	else
		ret = FALSE;

	RegCloseKey(key);

	return ret;
};

int reg_delete_k(char *keyname) {
	HKEY key;
	char *munge;
	DWORD err;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0, 
		KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) {
		return FALSE;
	};

	munge = (char *)malloc(3 * strlen(keyname) + 1);
	mungestr(keyname, munge);

	err = RegDeleteKey(key, munge);
	RegCloseKey(key);

	free(munge);

	return (err == ERROR_SUCCESS);
};

int reg_copy_tree(char *from, char *to) {
	HKEY key1, key2;
	char *f, *t, *name;
	LPBYTE data;
	DWORD i, size, dsize, subkeys, values;

	if (!from || !to)
		return FALSE;

	f = (char *)malloc(BUFSIZE);
	mungestr(from, f);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, f, 0, KEY_READ, &key1) != ERROR_SUCCESS) {
		free(f);
		return FALSE;
	};

	mungestr(to, f);

	if (RegCreateKeyEx(HKEY_CURRENT_USER, f, 0, NULL, REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS, NULL, &key2, &size) != ERROR_SUCCESS) {
		free(f);
		RegCloseKey(key1);
		return FALSE;
	};

	if (RegQueryInfoKey(key1, NULL, NULL, NULL, &subkeys, 
						NULL, NULL, &values, NULL, 
						NULL, NULL, NULL) != ERROR_SUCCESS) {
		free(f);
		RegCloseKey(key1);
		RegCloseKey(key2);
		return FALSE;
	};

	memset(f, 0, BUFSIZE);
	t = (char *)malloc(BUFSIZE);
	memset(t, 0, BUFSIZE);
	name = (char *)malloc(BUFSIZE);
	memset(name, 0, BUFSIZE);

	for (i = 0; i < (int)subkeys; i++) {
		size = BUFSIZE;
		if (RegEnumKeyEx(key1, i, (LPSTR)name, &size, 
						NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			sprintf(f, "%s\\%s", from, name);
			sprintf(t, "%s\\%s", to, name);
			if (!reg_copy_tree(f, t)) {
				free(f);
				free(t);
				free(name);
				RegCloseKey(key1);
				RegCloseKey(key2);
				return FALSE;
			};
		};
	};

	data = (LPBYTE)malloc(BUFSIZE);

	for (i = 0; i < values; i++) {
		size = dsize = BUFSIZE;
		if ((RegEnumValue(key1, i, (LPSTR)name, &size, NULL,
						&subkeys, data, &dsize) != ERROR_SUCCESS) ||
			(RegSetValueEx(key2, name, 0, subkeys, data, dsize) != ERROR_SUCCESS)) {
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

static int reg_delete_callback(char *name, char *path, int isfolder, int mode,
							   void *priv1, void *priv2, void *priv3) {
	char *buf;
	int ret;

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

	buf = (char *)malloc(BUFSIZE);
	sprintf(buf, "%s\\%s", REGROOT, path);
	ret = reg_delete_k(buf);
	free(buf);

	return ret;
};

int reg_delete_tree(char *keyname) {
	reg_walk_over_tree(keyname, reg_delete_callback, NULL, NULL, NULL);
	return reg_delete_k(keyname);
};

int reg_move_tree(char *keyfrom, char *keyto) {
	return 
		(reg_copy_tree(keyfrom, keyto) && reg_delete_tree(keyfrom));
};

int reg_walk_over_tree(char *root, reg_callback cb, 
					   void *priv1, void *priv2, void *priv3) {
	char *str1, *str2;
	HKEY key;
	DWORD err, i, subkeys, size, isfolder;
	char **slist;
	int ret;

	str1 = (char *)malloc(BUFSIZE);
	str2 = (char *)malloc(BUFSIZE);

	if (root && root != "") {
		mungestr(root, str2);
		sprintf(str1, "%s\\%s", REGROOT, str2);
	} else
		sprintf(str1, "%s", REGROOT);

	if ((err = RegOpenKeyEx(HKEY_CURRENT_USER, str1, 0, KEY_READ, &key)) != ERROR_SUCCESS) {
		free(str1);
		free(str2);

		return FALSE;
	};

	if (RegQueryInfoKey(key, NULL, NULL, NULL, &subkeys, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS ||
						subkeys == 0) {
		free(str1);
		free(str2);
		return FALSE;
	};

	slist = (char **)malloc(subkeys * sizeof(char *));
	memset(slist, 0, subkeys * sizeof(char *));

	for (i = 0; i < subkeys; i++) {
		size = BUFSIZE;
		if (RegEnumKeyEx(key, i, str1, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			unmungestr(str1, str2, BUFSIZE);
			slist[i] = (char *)malloc(strlen(str2) + 1);
			strcpy(slist[i], str2);
		};
	};

	stupid_sort(slist, subkeys);

	for (i = 0; i < subkeys; i++) {
		if (root && root != "") {
			sprintf(str1, "%s\\%s\\%s", REGROOT, root, slist[i]);
			sprintf(str2, "%s\\%s", root, slist[i]);
		} else {
			sprintf(str1, "%s\\%s", REGROOT, slist[i]);
			sprintf(str2, "%s", slist[i]);
		};
		isfolder = reg_read_i(str1, ISFOLDER, 0);
		ret = cb(slist[i], str2, isfolder, REG_MODE_PREPROCESS, priv1, priv2, priv3);
		if (isfolder)
			reg_walk_over_tree(str2, cb, (void *)ret, priv2, priv3);
		ret = cb(slist[i], str2, isfolder, REG_MODE_POSTPROCESS, (void *)ret, priv2, priv3);

		free(slist[i]);
	};

	free(slist);
	free(str1);
	free(str2);

	return TRUE;
};


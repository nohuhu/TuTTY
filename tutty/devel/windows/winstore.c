/*
 * winstore.c: Windows-specific implementation of the interface
 * defined in storage.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "putty.h"
#include "storage.h"
#include "session.h"

typedef struct _session_handle_t {
    session_root_t *root;
    void *handle;
} session_handle_t;

static const char *const puttystr = PUTTY_REG_POS "\\Sessions";

static char seedpath[2 * MAX_PATH + 10] = "\0";

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
}

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
}

void *open_settings_w(session_root_t *root, const char *sessionname, char **errmsg)
{
    session_handle_t *handle;

    *errmsg = NULL;

    if (!sessionname || !*sessionname)
	sessionname = "Default Settings";

    handle = snew(session_handle_t);

    if (!handle) {
	*errmsg = "Not enough memory";
	return NULL;
    };

    handle->root = root;
    handle->handle = (void *) ses_open_session_w(root, (char *) sessionname);

    return handle;
}

void write_setting_s(void *handle, const char *key, const char *value)
{
    session_handle_t *sh = (session_handle_t *) handle;

    if (handle)
	ses_write_handle_s(sh->root, sh->handle, (char *) key, (char *) value);
}

void write_setting_i(void *handle, const char *key, int value)
{
    session_handle_t *sh = (session_handle_t *) handle;

    if (handle)
	ses_write_handle_i(sh->root, sh->handle, (char *) key, value);
}

void close_settings_w(void *handle)
{
    session_handle_t *sh = (session_handle_t *) handle;

    if (handle) {
	ses_close_session(sh->root, sh->handle);
	sfree(sh);
    };
}

void *open_settings_r(session_root_t *root, const char *sessionname)
{
    session_handle_t *handle;

    if (!sessionname || !*sessionname)
	sessionname = "Default Settings";

    handle = snew(session_handle_t);

    if (!handle)
	return NULL;

    handle->root = root;
    handle->handle = (void *) ses_open_session_r(root, (char *) sessionname);

    return handle;
}

char *read_setting_s(void *handle, const char *key, char *buffer,
		     int buflen)
{
    session_handle_t *sh = (session_handle_t *) handle;

    if (!handle ||
	!ses_read_handle_s(sh->root, sh->handle, (char *) key, NULL, buffer, buflen))
	return NULL;
    else
	return buffer;
}

int read_setting_i(void *handle, const char *key, int defvalue)
{
    session_handle_t *sh = (session_handle_t *) handle;
    int val = 0;

    if (!handle ||
	!ses_read_handle_i(sh->root, sh->handle, (char *) key, defvalue, &val))
	return defvalue;
    else
	return val;
}

int read_setting_fontspec(void *handle, const char *name,
			  FontSpec * result)
{
    char *settingname;
    FontSpec ret;

    if (!read_setting_s(handle, name, ret.name, sizeof(ret.name)))
	return 0;
    settingname = dupcat(name, "IsBold", NULL);
    ret.isbold = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (ret.isbold == -1)
	return 0;
    settingname = dupcat(name, "CharSet", NULL);
    ret.charset = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (ret.charset == -1)
	return 0;
    settingname = dupcat(name, "Height", NULL);
    ret.height = read_setting_i(handle, settingname, INT_MIN);
    sfree(settingname);
    if (ret.height == INT_MIN)
	return 0;
    *result = ret;
    return 1;
}

void write_setting_fontspec(void *handle, const char *name, FontSpec font)
{
    char *settingname;

    write_setting_s(handle, name, font.name);
    settingname = dupcat(name, "IsBold", NULL);
    write_setting_i(handle, settingname, font.isbold);
    sfree(settingname);
    settingname = dupcat(name, "CharSet", NULL);
    write_setting_i(handle, settingname, font.charset);
    sfree(settingname);
    settingname = dupcat(name, "Height", NULL);
    write_setting_i(handle, settingname, font.height);
    sfree(settingname);
}

int read_setting_filename(void *handle, const char *name,
			  Filename * result)
{
    return !!read_setting_s(handle, name, result->path,
			    sizeof(result->path));
}

void write_setting_filename(void *handle, const char *name,
			    Filename result)
{
    write_setting_s(handle, name, result.path);
}

void close_settings_r(void *handle)
{
    session_handle_t *sh = (session_handle_t *) handle;

    if (handle) {
	ses_close_session(sh->root, sh->handle);
	sfree(sh);
    };
}

void del_settings(session_root_t *root, const char *sessionname)
{
    ses_delete_tree(root, (char *) sessionname);
}

typedef struct _enum_settings_t {
    session_root_t *root;
    void *handle;
} enum_settings_t;

void *enum_settings_start(session_root_t *root, char *path)
{
    enum_settings_t *ret;

    ret = snew(enum_settings_t);

    if (!ret)
	return NULL;

    ret->root = root;
    ret->handle = ses_enum_settings_start(root, path);

    return ret;
}

char *enum_settings_next(void *handle, char *buffer, int buflen)
{
    enum_settings_t *e = (enum_settings_t *) handle;

    return ses_enum_settings_next(e->root, e->handle, buffer, buflen);
}

void enum_settings_finish(void *handle)
{
    enum_settings_t *e = (enum_settings_t *) handle;
    ses_enum_settings_finish(e->root, e->handle);
    sfree(e);
}

static void hostkey_regname(char *buffer, const char *hostname,
			    int port, const char *keytype)
{
    int len;
    strcpy(buffer, keytype);
    strcat(buffer, "@");
    len = strlen(buffer);
    len += sprintf(buffer + len, "%d:", port);
    mungestr(hostname, buffer + strlen(buffer));
}

int verify_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{
    char *otherstr, *regname;
    int len;
    HKEY rkey;
    DWORD readlen;
    DWORD type;
    int ret, compare;

    len = 1 + strlen(key);

    /*
     * Now read a saved key in from the registry and see what it
     * says.
     */
    otherstr = snewn(len, char);
    regname = snewn(3 * (strlen(hostname) + strlen(keytype)) + 15, char);

    hostkey_regname(regname, hostname, port, keytype);

    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS "\\SshHostKeys",
		   &rkey) != ERROR_SUCCESS)
	return 1;		/* key does not exist in registry */

    readlen = len;
    ret = RegQueryValueEx(rkey, regname, NULL, &type, otherstr, &readlen);

    if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA &&
	!strcmp(keytype, "rsa")) {
	/*
	 * Key didn't exist. If the key type is RSA, we'll try
	 * another trick, which is to look up the _old_ key format
	 * under just the hostname and translate that.
	 */
	char *justhost = regname + 1 + strcspn(regname, ":");
	char *oldstyle = snewn(len + 10, char);	/* safety margin */
	readlen = len;
	ret = RegQueryValueEx(rkey, justhost, NULL, &type,
			      oldstyle, &readlen);

	if (ret == ERROR_SUCCESS && type == REG_SZ) {
	    /*
	     * The old format is two old-style bignums separated by
	     * a slash. An old-style bignum is made of groups of
	     * four hex digits: digits are ordered in sensible
	     * (most to least significant) order within each group,
	     * but groups are ordered in silly (least to most)
	     * order within the bignum. The new format is two
	     * ordinary C-format hex numbers (0xABCDEFG...XYZ, with
	     * A nonzero except in the special case 0x0, which
	     * doesn't appear anyway in RSA keys) separated by a
	     * comma. All hex digits are lowercase in both formats.
	     */
	    char *p = otherstr;
	    char *q = oldstyle;
	    int i, j;

	    for (i = 0; i < 2; i++) {
		int ndigits, nwords;
		*p++ = '0';
		*p++ = 'x';
		ndigits = strcspn(q, "/");	/* find / or end of string */
		nwords = ndigits / 4;
		/* now trim ndigits to remove leading zeros */
		while (q[(ndigits - 1) ^ 3] == '0' && ndigits > 1)
		    ndigits--;
		/* now move digits over to new string */
		for (j = 0; j < ndigits; j++)
		    p[ndigits - 1 - j] = q[j ^ 3];
		p += ndigits;
		q += nwords * 4;
		if (*q) {
		    q++;	/* eat the slash */
		    *p++ = ',';	/* add a comma */
		}
		*p = '\0';	/* terminate the string */
	    }

	    /*
	     * Now _if_ this key matches, we'll enter it in the new
	     * format. If not, we'll assume something odd went
	     * wrong, and hyper-cautiously do nothing.
	     */
	    if (!strcmp(otherstr, key))
		RegSetValueEx(rkey, regname, 0, REG_SZ, otherstr,
			      strlen(otherstr) + 1);
	}
    }

    RegCloseKey(rkey);

    compare = strcmp(otherstr, key);

    sfree(otherstr);
    sfree(regname);

    if (ret == ERROR_MORE_DATA ||
	(ret == ERROR_SUCCESS && type == REG_SZ && compare))
	return 2;		/* key is different in registry */
    else if (ret != ERROR_SUCCESS || type != REG_SZ)
	return 1;		/* key does not exist in registry */
    else
	return 0;		/* key matched OK in registry */
}

void store_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{
    char *regname;
    HKEY rkey;

    regname = snewn(3 * (strlen(hostname) + strlen(keytype)) + 15, char);

    hostkey_regname(regname, hostname, port, keytype);

    if (RegCreateKey(HKEY_CURRENT_USER, PUTTY_REG_POS "\\SshHostKeys",
		     &rkey) != ERROR_SUCCESS)
	return;			/* key does not exist in registry */
    RegSetValueEx(rkey, regname, 0, REG_SZ, key, strlen(key) + 1);
    RegCloseKey(rkey);
}

/*
 * Find the random seed file path and store it in `seedpath'.
 */
static void get_seedpath(void)
{
    HKEY rkey;
    DWORD type, size;

    size = sizeof(seedpath);

    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS, &rkey) ==
	ERROR_SUCCESS) {
	int ret = RegQueryValueEx(rkey, "RandSeedFile",
				  0, &type, seedpath, &size);
	if (ret != ERROR_SUCCESS || type != REG_SZ)
	    seedpath[0] = '\0';
	RegCloseKey(rkey);
    } else
	seedpath[0] = '\0';

    if (!seedpath[0]) {
	int len, ret;

	len =
	    GetEnvironmentVariable("HOMEDRIVE", seedpath,
				   sizeof(seedpath));
	ret =
	    GetEnvironmentVariable("HOMEPATH", seedpath + len,
				   sizeof(seedpath) - len);
	if (ret == 0) {		/* probably win95; store in \WINDOWS */
	    GetWindowsDirectory(seedpath, sizeof(seedpath));
	    len = strlen(seedpath);
	} else
	    len += ret;
	strcpy(seedpath + len, "\\PUTTY.RND");
    }
}

void read_random_seed(noise_consumer_t consumer)
{
    HANDLE seedf;

    if (!seedpath[0])
	get_seedpath();

    seedf = CreateFile(seedpath, GENERIC_READ,
		       FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL, OPEN_EXISTING, 0, NULL);

    if (seedf != INVALID_HANDLE_VALUE) {
	while (1) {
	    char buf[1024];
	    DWORD len;

	    if (ReadFile(seedf, buf, sizeof(buf), &len, NULL) && len)
		consumer(buf, len);
	    else
		break;
	}
	CloseHandle(seedf);
    }
}

void write_random_seed(void *data, int len)
{
    HANDLE seedf;

    if (!seedpath[0])
	get_seedpath();

    seedf = CreateFile(seedpath, GENERIC_WRITE, 0,
		       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (seedf != INVALID_HANDLE_VALUE) {
	DWORD lenwritten;

	WriteFile(seedf, data, len, &lenwritten, NULL);
	CloseHandle(seedf);
    }
}

/*
 * Recursively delete a registry key and everything under it.
 */
static void registry_recursive_remove(HKEY key)
{
    DWORD i;
    char name[MAX_PATH + 1];
    HKEY subkey;

    i = 0;
    while (RegEnumKey(key, i, name, sizeof(name)) == ERROR_SUCCESS) {
	if (RegOpenKey(key, name, &subkey) == ERROR_SUCCESS) {
	    registry_recursive_remove(subkey);
	    RegCloseKey(subkey);
	}
	RegDeleteKey(key, name);
    }
}

void cleanup_all(void)
{
    HKEY key;
    int ret;
    char name[MAX_PATH + 1];

    /* ------------------------------------------------------------
     * Wipe out the random seed file.
     */
    if (!seedpath[0])
	get_seedpath();
    remove(seedpath);

    /* ------------------------------------------------------------
     * Destroy all registry information associated with PuTTY.
     */

    /*
     * Open the main PuTTY registry key and remove everything in it.
     */
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS, &key) ==
	ERROR_SUCCESS) {
	registry_recursive_remove(key);
	RegCloseKey(key);
    }
    /*
     * Now open the parent key and remove the PuTTY main key. Once
     * we've done that, see if the parent key has any other
     * children.
     */
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_PARENT,
		   &key) == ERROR_SUCCESS) {
	RegDeleteKey(key, PUTTY_REG_PARENT_CHILD);
	ret = RegEnumKey(key, 0, name, sizeof(name));
	RegCloseKey(key);
	/*
	 * If the parent key had no other children, we must delete
	 * it in its turn. That means opening the _grandparent_
	 * key.
	 */
	if (ret != ERROR_SUCCESS) {
	    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_GPARENT,
			   &key) == ERROR_SUCCESS) {
		RegDeleteKey(key, PUTTY_REG_GPARENT_CHILD);
		RegCloseKey(key);
	    }
	}
    }
    /*
     * Now we're done.
     */
}

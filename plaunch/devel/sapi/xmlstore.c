/*
 * Session abstraction layer for TuTTY and PLaunch.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2005, 2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Platform-independent XML file storage implementation file.
 */

#include <string.h>
#include "xmlstore.h"
#include "ezxml.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define XML_NODE_KEY	"key"
#define XML_NODE_VALUE	"value"
#define XML_ATTR_NAME	"name"

typedef struct _xml_handle_t {
    session_root_t *root;
    ezxml_t node;
    int write;
} xml_handle_t;

void *xml_init(session_root_t *root, char *errmsg, int errsize)
{
    FILE *f;
    char *buf;
    int bsize, read;
    ezxml_t root_node;

    if (!root || !root->root_location || !root->root_location[0])
	return NULL;

    f = fopen(root->root_location, "rbS");

    if (!f) {
	strncpy(errmsg, "Cannot open file", errsize);
	return NULL;
    };

    fseek(f, 0, SEEK_END);
    bsize = ftell(f);

    buf = (char *) malloc(bsize + 1);
    memset(buf, 0, bsize);

    read = fread(buf, sizeof(char), bsize, f);
    fclose(f);

    if (read != bsize) {
	free(buf);
	strncpy(errmsg, "Cannot read from file", errsize);
	return NULL;
    };

    root_node = ezxml_parse_str(buf, bsize);

    return root_node;
};

int xml_write(session_root_t *root, char *errmsg, int errsize)
{
    FILE *f;
    char *buf;
    int bsize, wrote;
    ezxml_t root_node;

    if (!root || !root->root_location || !root->root_location[0] || !root->data)
	return 0;

    root_node = (ezxml_t) root->data;

    buf = ezxml_toxml(root_node);

    if (!buf)
	return 0;

    f = fopen(root->root_location, "wbS");

    if (!f) {
	free(buf);
	strncpy(errmsg, "Cannot open file", errsize);
	return 0;
    };

    bsize = strlen(buf);

    wrote = fwrite(buf, sizeof(char), bsize, f);
    fclose(f);
    free(buf);

    if (wrote != bsize) {
	strncpy(errmsg, "Cannot write to file", errsize);
	return 0;
    };

    return wrote;
};

void xml_finish(session_root_t *root)
{
    ezxml_t root_node;

    if (!root || !root->data)
	return;

    root_node = (ezxml_t) root->data;

    ezxml_free(root_node);
};

int xml_make_path(char *parent, char *path, char *buffer,
		  int bufsize)
{
    int ret = 0;

    if (!path || !buffer || !bufsize)
	return FALSE;

    if (parent && parent[0])
	ret = _snprintf(buffer, bufsize, "%s\\%s", parent, path);
    else if (path && path[0])
	strncpy(buffer, path, bufsize);
    else
	strcpy(buffer, "");

    return ret < 0 ? FALSE : TRUE;
};

char *second_name(char *in)
{
    char *p;

    p = in;
    while (*p != '\\' && *p != '\0') *p++;
    return p;
};

ezxml_t xml_find_key(ezxml_t root_node, char *path)
{
    ezxml_t node;
    char name[BUFSIZE], *cpath;

    if (!path || !path[0])
	return NULL;

    ses_firstname(path, name, BUFSIZE);
    cpath = second_name(path);

    for (node = root_node; node; node = ezxml_next(node)) {
	if (strcmp(ezxml_name(node), XML_NODE_KEY) == 0 &&
	    strcmp(ezxml_attr(node, XML_ATTR_NAME), name) == 0) {
	    if (cpath)
		return xml_find_key(node, cpath);
	    else
		return node;
	};
    };

    return NULL;
};

ezxml_t xml_find_value(ezxml_t node, char *valname)
{
    ezxml_t value;

    for (value = ezxml_child(node, XML_NODE_VALUE); value; value = ezxml_next(value)) {
	if (ezxml_attr(value, valname))
	    return value;
    };

    return NULL;
};

void *xml_open_session_r(session_root_t *root, char *keyname)
{
    ezxml_t root_node, sessions;
    xml_handle_t *handle;
    
    if (!root || !root->data || !keyname)
	return NULL;

    handle = (xml_handle_t *) malloc(sizeof(xml_handle_t));
    memset(handle, 0, sizeof(xml_handle_t));

    handle->write = FALSE;
    handle->root = root;

    root_node = (ezxml_t) root->data;

    sessions = ezxml_child(root_node, XML_SESSIONROOT);

    handle->node = xml_find_key(sessions, keyname);

    return handle;
};

void *xml_open_session_w(session_root_t *root, char *keyname)
{
    xml_handle_t *handle;

    if (!root || !root->data || !keyname || 
	root->readonly)
	return NULL;

    handle = xml_open_session_r(root, keyname);
    handle->write = TRUE;

    return handle;
};

void xml_close_key(void *handle)
{
    xml_handle_t *h = (xml_handle_t *) handle;

    free(h);

    return;
};

int xml_delete_v(void *handle, char *valname)
{
    xml_handle_t *h = (xml_handle_t *) handle;
    ezxml_t value;

    if (!h || !h->node || !valname || !h->root->readonly)
	return FALSE;

    value = xml_find_value(h->node, valname);

    if (value) {
	h->root->modified = TRUE;
	ezxml_remove(value);
	return TRUE;
    } else
	return FALSE;
};

int xml_delete_k(void *handle, char *keyname)
{
    xml_handle_t *h = (xml_handle_t *) handle;
    ezxml_t key;

    if (!h || !h->node || !keyname || !h->root->readonly)
	return FALSE;

    key = xml_find_key(h->node, keyname);

    if (key) {
	h->root->modified = TRUE;
	ezxml_remove(key);
	return TRUE;
    } else
	return FALSE;
};

int xml_read_i(void *handle, char *valname, int defval, int *value)
{
    xml_handle_t *h = (xml_handle_t *) handle;
    ezxml_t val;

    if (!h || !h->node || !valname) {
	*value = defval;
	return defval;
    };
    
    val = xml_find_value(h->node, valname);

    if (val)
	*value = atoi(ezxml_txt(val));
    else
	*value = defval;

    return *value;
};

int xml_write_i(void *handle, char *valname, int value)
{
    xml_handle_t *h = (xml_handle_t *) handle;
    ezxml_t val;
    char buf[BUFSIZE];

    if (!h || !h->node || !valname || h->root->readonly)
	return 0;

    sprintf(buf, "%d", value);

    val = xml_find_value(h->node, valname);

    if (val)
	ezxml_set_txt_d(val, buf);
    else {
	val = ezxml_add_child_d(h->node, XML_NODE_VALUE, 0);
	ezxml_set_txt_d(val, buf);
    };

    h->root->modified = TRUE;

    return TRUE;
};

int xml_read_s(void *handle, char *valname, char *defval, 
	       char *buffer, int bufsize)
{
    xml_handle_t *h = (xml_handle_t *) handle;
    ezxml_t val;

    if (!h || !h->node || !valname) {
	strncpy(buffer, defval, bufsize);
	return FALSE;
    };

    val = xml_find_value(h->node, valname);

    if (val && ezxml_txt(val))
	strncpy(buffer, ezxml_txt(val), bufsize);
    else
	strncpy(buffer, defval, bufsize);

    return TRUE;
};

int xml_write_s(void *handle, char *valname, char *value)
{
    xml_handle_t *h = (xml_handle_t *) handle;
    ezxml_t val;

    if (!h || !h->node || !valname || !value || h->root->readonly)
	return FALSE;

    val = xml_find_value(h->node, valname);

    if (val)
	ezxml_set_txt_d(val, value);
    else {
	val = ezxml_add_child_d(h->node, XML_NODE_VALUE, 0);
	ezxml_set_txt_d(val, value);
    };

    return TRUE;
};

typedef struct _xml_enumsettings_t {
    session_root_t *root;
    ezxml_t root_node;
    ezxml_t current;
} xml_enumsettings_t;

void *xml_enum_settings_start(session_root_t *root, char *path)
{
    xml_enumsettings_t *s;

    if (!root || !root->data || !path)
	return NULL;

    s = (xml_enumsettings_t *) malloc(sizeof(xml_enumsettings_t));
    memset(s, 0, sizeof(xml_enumsettings_t));

    s->root = root;
    s->root_node = xml_find_key((ezxml_t) root->data, path);
    s->current = s->root_node;

    return s;
};

int xml_enum_settings_count(void *handle)
{
    xml_enumsettings_t *h = (xml_enumsettings_t *) handle;
    ezxml_t node;
    int i = 0;

    if (!h)
	return 0;

    for (node = h->root_node; node; node = ezxml_next(node))
	i++;

    return i;
};

char *xml_enum_settings_next(void *handle, char *buffer, int buflen)
{
    xml_enumsettings_t *h = (xml_enumsettings_t *) handle;

    if (!h)
	return NULL;

    h->current = ezxml_next(h->current);

    if (h->current) {
	strncpy(buffer, ezxml_attr(h->current, XML_ATTR_NAME), buflen);
	return buffer;
    } else {
	buffer[0] = '\0';
	return NULL;
    };
};

void xml_enum_settings_finish(void *handle)
{
    xml_enumsettings_t *h = (xml_enumsettings_t *) handle;

    free(h);
};

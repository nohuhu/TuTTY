#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "session.h"
#include "registry.h"

#ifndef FALSE
#define FALSE	0
#define	TRUE	1
#endif

char *ses_lastname(char *in)
{
    char *p;

    if (!in || !in[0])
	return in;

    p = &in[strlen(in)];

    while (p >= in && *p != '\\' && *p != '/')
	*p--;

    return p + 1;
};

int ses_pathname(char *in, char *buffer, unsigned int bufsize)
{
    char *p;

    if (!in || !in[0] || !buffer || !bufsize)
	return FALSE;

    p = ses_lastname(in);

    if (p == in)
	buffer[0] = '\0';
    else {
	strncpy(buffer, in, (p - in) - 1);
	buffer[p - in - 1] = '\0';
    };

    return TRUE;
};

int ses_make_path(char *parent, char *path, char *buffer,
		  unsigned int bufsize) 
{
    if (!path || !path[0] || !buffer || !bufsize)
	return FALSE;

    if (parent && parent[0])
#ifdef _WINDOWS
	_snprintf(buffer, bufsize, "%s\\%s", parent, path);
#else
	snprintf(buffer, bufsize, "%s\\%s", parent, path);
#endif /* _WINDOWS */
    else
	strncpy(buffer, path, bufsize);

    return TRUE;
};    

int ses_is_folder(session_root_t *root, char *path)
{
    char regpath[BUFSIZE];
    int isfolder = FALSE;

    if (!root || !path || !path[0])
	return FALSE;

    if (reg_make_path(NULL, path, regpath, BUFSIZE))
	reg_read_i(regpath, ISFOLDER, 0, &isfolder);

    return isfolder;
};

int ses_make_folder(session_root_t *root, char *path) {
    char regpath[BUFSIZE];
    int ret = FALSE;

    if (!root || !path || !path[0])
	return FALSE;

    if (reg_make_path(NULL, path, regpath, BUFSIZE))
	ret = reg_write_i(regpath, ISFOLDER, TRUE);

    return ret;
};

int ses_copy_tree(session_root_t *root, char *frompath, char *topath)
{
    return reg_copy_tree(frompath, topath);
};

int ses_move_tree(session_root_t *root, char *frompath, char *topath)
{
    return reg_move_tree(frompath, topath);
};

int ses_delete_value(session_root_t *root, char *spath, char *valname) 
{
    char regpath[BUFSIZE];
    int ret = FALSE;

    if (reg_make_path(NULL, spath, regpath, BUFSIZE))
	ret = reg_delete_v(regpath, valname);

    return ret;
};

int ses_delete_session(session_root_t *root, char *spath)
{
    char regpath[BUFSIZE];
    int ret = FALSE;

    if (reg_make_path(NULL, spath, regpath, BUFSIZE))
	ret = reg_delete_k(regpath);

    return ret;
};

int ses_delete_tree(session_root_t *root, char *spath)
{
    return reg_delete_tree(spath);
};

int ses_read_i(session_root_t *root, char *spath, char *valname, 
	       int defval, int *value)
{
    char regpath[BUFSIZE];
    int ret = 0;

    if (reg_make_path(NULL, spath, regpath, BUFSIZE))
	ret = reg_read_i(regpath, valname, defval, value);

    return ret;
};

int ses_write_i(session_root_t *root, char *spath, char *valname, int value)
{
    char regpath[BUFSIZE];
    int ret = FALSE;

    if (reg_make_path(NULL, spath, regpath, BUFSIZE))
	ret = reg_write_i(regpath, valname, value);

    return ret;
};

int ses_read_s(session_root_t *root, char *spath, char *valname, 
	       char *defval, char *buffer, int bufsize)
{
    char regpath[BUFSIZE];
    int ret = FALSE;

    if (reg_make_path(NULL, spath, regpath, BUFSIZE))
	ret = reg_read_s(regpath, valname, defval, buffer, bufsize);

    return ret;
};

int ses_write_s(session_root_t *root, char *spath, char *valname, 
		char *value)
{
    char regpath[BUFSIZE];
    int ret = FALSE;

    if (reg_make_path(NULL, spath, regpath, BUFSIZE))
	ret = reg_write_s(regpath, valname, value);

    return ret;
};

static int ses_compare_default(const session_t *a, const session_t *b)
{
    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a->name, "Default Settings"))
	return -1;		/* a comes first */
    if (!strcmp(b->name, "Default Settings"))
	return +1;		/* b comes first */

    /*
     * Next step: determine if one (or both) of the compared items
     * is a folder. If one, a folder has precedence, if both, comparing
     * in alphabetical order as usual.
     * Additional check is made for ".." directory name, it always comes first.
     */
    if (!strcmp(a->name, ".."))
	return -1;
    if (!strcmp(b->name, ".."))
	return +1;

    if (a->isfolder && !b->isfolder)
	return -1;
    else if (!a->isfolder && b->isfolder)
	return +1;

    return strcmp(a->name, b->name);	/* otherwise, compare normally */
}

#ifdef MINIRTL
/*
 * implements the smallest sorting routine i've been able
 * to find. yes, it's an infamous bubble sort. :)
 * since this routine will never be used for sorting lists
 * of any notable size, any performance impact would be
 * negligible.
 */
static void ses_sort_list(session_t **array, ses_compare_f cmpfunc,
			  unsigned int count)
{
    unsigned int i, j;
    session_t *tmp;
    ses_compare_f compare;

    compare = cmpfunc ? cmpfunc : ses_compare_default;

    for (i = 0; i < count; i++) {
	for (j = 0; j < count - 1; j++) {
	    if (compare(array[j], array[j + 1]) > 0) {
		tmp = array[j];
		array[j] = array[j + 1];
		array[j + 1] = tmp;
	    };
	};
    };
};
#else
/*
 * use more appropriate quick sort routine when we don't need to
 * tighten out every unnecessary byte.
 */
static void _quick_sort(session_t **array, ses_compare_f cmpfunc,
			int lower_b, int upper_b)
{
    session_t *pivot, *tmp;
    int up, down;

    if (lower_b >= upper_b)
	return;

    pivot = array[lower_b];
    up = upper_b;
    down = lower_b;

    while (down < up) {
	while (cmpfunc(array[down], pivot) <= 0 && down < upper_b)
	    down++;
	while (cmpfunc(array[up], pivot) > 0 && up > 0)
	    up--;
	if (down < up) {
	    tmp = array[down];
	    array[down] = array[up];
	    array[up] = tmp;
	};
    };

    array[lower_b] = array[up];
    array[up] = pivot;

    _quick_sort(array, cmpfunc, lower_b, up - 1);
    _quick_sort(array, cmpfunc, up + 1, upper_b);
};

static void ses_sort_list(session_t **array, ses_compare_f cmpfunc,
			  unsigned int count)
{
    _quick_sort(array, cmpfunc ? cmpfunc : ses_compare_default, 0, count - 1);
};
#endif /* MINIRTL */

static void ses_free_slist(session_t **slist, int count) {
    int i;

    if (!slist)
	return;

    /* 
     * note that we won't free session_t->root because mostly
     * it will not be allocated specifically for the instance of
     * session_t but will be just assigned.
     */
    for (i = 0; i < count; i++) {
	if (slist[i]) {
	    if (slist[i]->name)
		free(slist[i]->name);
	    if (slist[i]->path)
		free(slist[i]->path);
	    free(slist[i]);
	};
    };
    free(slist);
};

int ses_walk_over_tree(session_walk_t *sw)
{
    int i, sessions;
    void *handle;
    char name[BUFSIZE], path[BUFSIZE];
    session_t **slist;
    session_callback_t scb;
    session_walk_t swalk;

    if (!sw)
	return FALSE;

    if (sw->depth < 0)
	return FALSE;

    handle = reg_enum_settings_start(sw->root_path);

    if (!handle)
	return FALSE;

    sessions = reg_enum_settings_count(handle);

    slist = (session_t **) malloc(sessions * sizeof(session_t *));

    if (!slist) {
	reg_enum_settings_finish(handle);
	return FALSE;
    };

    memset(slist, 0, sessions * sizeof(session_t *));

    for (i = 0; i < sessions; i++) {
	reg_enum_settings_next(handle, name, BUFSIZE);

	if (name[0]) {
	    int len;
	    session_t *session;

	    session = (session_t *) malloc(sizeof(session_t));
	    if (!session) {
		ses_free_slist(slist, i);
		reg_enum_settings_finish(handle);
		return FALSE;
	    };
	    memset(session, 0, sizeof(session_t));

	    len = strlen(name) + 1;
	    session->name = (char *) malloc(len);
	    if (!session->name) {
		free(session);
		ses_free_slist(slist, i);
		reg_enum_settings_finish(handle);
		return FALSE;
	    };
	    memset(session->name, 0, len);
	    strcpy(session->name, name);

	    memset(path, 0, BUFSIZE);
	    ses_make_path(sw->root_path, name, path, BUFSIZE);
	    len = strlen(path) + 1;
	    session->path = (char *) malloc(len);
	    if (!session->path) {
		free(session->name);
		free(session);
		ses_free_slist(slist, i);
		reg_enum_settings_finish(handle);
		return FALSE;
	    };
	    memset(session->path, 0, len);
	    strcpy(session->path, path);

	    session->root = sw->root;
	    session->isfolder = ses_is_folder(&sw->root, path);

	    slist[i] = session;
	};
    };

    reg_enum_settings_finish(handle);

    ses_sort_list(slist, sw->compare, sessions);

    for (i = 0; i < sessions; i++) {
	memset(&scb, 0, sizeof(scb));
	scb.root = sw->root;
	scb.session = slist[i];
	scb.retval = sw->retval;
	scb.p1 = sw->p1;
	scb.p2 = sw->p2;
	scb.p3 = sw->p3;
	scb.p4 = sw->p4;
	scb.mode = SES_MODE_PREPROCESS;

	sw->callback(&scb);

	if (scb.session->isfolder && sw->depth > 0) {
	    memset(&swalk, 0, sizeof(swalk));
	    memmove(&swalk, sw, sizeof(swalk));
	    swalk.root_path = scb.session->path;
	    swalk.retval = scb.retval;
	    swalk.depth--;
	    ses_walk_over_tree(&swalk);
	};

	scb.mode = SES_MODE_POSTPROCESS;
	sw->callback(&scb);
    };

    ses_free_slist(slist, sessions);

    return TRUE;
};

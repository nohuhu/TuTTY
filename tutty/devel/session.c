#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "session.h"

/*
 * platform-specific includes
 */
#ifdef _WINDOWS
#include "registry.h"
#endif /* _WINDOWS */

#ifndef FALSE
#define FALSE	0
#define	TRUE	1
#endif /* FALSE */

/*
 * handlers for platform-specific default functions
 * for windows, these are registry manipulation functions,
 * for unix we use legacy file-based ones.
 */
static int (* default_make_path)(char *, char *, char *, int) =
#ifdef _WINDOWS
    reg_make_path;
#endif /* _WINDOWS */

static int (* default_copy_session)(char *, char *) =
#ifdef _WINDOWS
    reg_copy_session;
#endif /* _WINDOWS */

static int (* default_delete_value)(char *, char *) =
#ifdef _WINDOWS
    reg_delete_v;
#endif /* _WINDOWS */

static int (* default_delete_session)(char *) =
#ifdef _WINDOWS
    reg_delete_k;
#endif /* _WINDOWS */

static int (* default_delete_folder)(char *) =
#ifdef _WINDOWS
    reg_delete_k;
#endif /* _WINDOWS */

static int (* default_read_i)(char *, char *, int, int *) =
#ifdef _WINDOWS
    reg_read_i;
#endif /* _WINDOWS */

static int (* default_write_i)(char *, char *, int) =
#ifdef _WINDOWS
    reg_write_i;
#endif /* _WINDOWS */

static int (* default_read_s)(char *, char *, char *,
			      char *, int) =
#ifdef _WINDOWS
    reg_read_s;
#endif /* _WINDOWS */

static int (* default_write_s)(char *, char *, char *) =
#ifdef _WINDOWS
    reg_write_s;
#endif /* _WINDOWS */

static void * (* default_enum_settings_start)(char *) =
#ifdef _WINDOWS
    reg_enum_settings_start;
#endif /* _WINDOWS */

static int (* default_enum_settings_count)(void *) =
#ifdef _WINDOWS
    reg_enum_settings_count;
#endif /* _WINDOWS */

static char * (* default_enum_settings_next)(void *, char *, int) =
#ifdef _WINDOWS
    reg_enum_settings_next;
#endif /* _WINDOWS */

static void (* default_enum_settings_finish)(void *) =
#ifdef _WINDOWS
    reg_enum_settings_finish;
#endif /* _WINDOWS */

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
    char ppath[BUFSIZE];
    int isfolder = FALSE;

    if (!path || !path[0])
	return FALSE;

    if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    default_read_i(ppath, ISFOLDER, 0, &isfolder);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (root->root_location &&
		reg_make_path_specific(root->root_location, NULL, 
		    path, ppath, BUFSIZE))
		reg_read_i(ppath, ISFOLDER, 0, &isfolder);
	    break;
#endif /* _WINDOWS */
	};
    };

    return isfolder;
};

int ses_make_folder(session_root_t *root, char *path) {
    char ppath[BUFSIZE];
    int ret = FALSE;

    if (!path || !path[0])
	return FALSE;

    if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    ret = default_write_i(ppath, ISFOLDER, TRUE);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (root->root_location &&
		reg_make_path_specific(root->root_location, NULL, 
		    path, ppath, BUFSIZE))
		reg_write_i(ppath, ISFOLDER, TRUE);
	    break;
#endif /* _WINDOWS */
	};
    };

    return ret;
};

int ses_copy_session(session_root_t *root, char *frompath, char *topath)
{
    char fppath[BUFSIZE], tppath[BUFSIZE];

    if (!frompath || !topath)
	return FALSE;

    if (!root || !root->root_type) {
	if (default_make_path(NULL, frompath, fppath, BUFSIZE) &&
	    default_make_path(NULL, topath, tppath, BUFSIZE))
	    return default_copy_session(fppath, tppath);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, frompath, fppath, BUFSIZE) &&
		reg_make_path_specific(root->root_location,
		    NULL, frompath, fppath, BUFSIZE))
		return reg_copy_session(fppath, tppath);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

static void ses_callback_copy_tree(session_callback_t *scb)
{
    char tppath[BUFSIZE];
    char *parent;

    if (!scb || !scb->session)
	return;

    parent = (int) scb->protected1 ?
	(char *) scb->protected2 :
	(char *) scb->public1;

    switch (scb->mode) {
    case SES_MODE_PREPROCESS:
	if (!scb->session->isfolder)
	    return;
	else {
	    if (ses_make_path(parent, scb->session->name, tppath, BUFSIZE) &&
		ses_make_folder(&scb->session->root, tppath)) {
		scb->private1 = (void *) TRUE;
		scb->private2 = malloc(strlen(parent) + 1);
		strcpy(scb->private2, parent);
		scb->protected1 = (void *) TRUE;
		strcpy(scb->protected2, tppath);
	    };
	};
	break;
    case SES_MODE_POSTPROCESS:
	if (scb->session->isfolder) {
	    if ((int) scb->private1) {
		scb->protected1 = (void *) TRUE;
		strcpy(scb->protected2, scb->private2);
		free(scb->private2);
		scb->private2 = NULL;
		scb->private1 = FALSE;
	    };
	} else {
	    if (ses_make_path(parent, scb->session->name, tppath, BUFSIZE))
		ses_copy_session(&scb->session->root,
				 scb->session->path,
				 tppath);
	};
    };
};

int ses_copy_tree(session_root_t *root, char *frompath, char *topath)
{
    session_walk_t swalk;
    char protected2[BUFSIZE];

    if (!root || !frompath || !topath)
	return FALSE;

    if (!ses_is_folder(root, frompath)) {
	/*
	 * just copy a session
	 */
	return ses_copy_session(root, frompath, topath);
    } else {
	/*
	 * create destination folder first
	 */
	if (ses_make_folder(root, topath)) {
	    /*
	     * fill session_walk_t structure and copy everything
	     * below the root which in this case is from.
	     */
	    memset(&swalk, 0, sizeof(swalk));
	    swalk.root = *root;
	    swalk.root_path = frompath;
	    swalk.callback = ses_callback_copy_tree;
	    swalk.depth = SES_MAX_DEPTH;
	    swalk.public1 = (void *) topath;
	    /*
	     * a special case, we need to have a buffer for return value
	     * and we don't want to allocate/deallocate it in callback
	     * function. so we assign it here and callbacks just copy
	     * data into it.
	     */
	    memset(protected2, 0, BUFSIZE);
	    swalk.protected2 = (void *) protected2;

	    return ses_walk_over_tree(&swalk);
	};
    };

    return FALSE;
};

int ses_delete_value(session_root_t *root, char *path, char *valname) 
{
    char ppath[BUFSIZE];

    if (!path || !valname)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_delete_value(ppath, valname);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_delete_v(ppath, valname);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

int ses_delete_session(session_root_t *root, char *path)
{
    char ppath[BUFSIZE];

    if (!path)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_delete_session(ppath);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_delete_k(ppath);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

int ses_delete_folder(session_root_t *root, char *path)
{
    char ppath[BUFSIZE];

    if (!path)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_delete_folder(ppath);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_delete_k(ppath);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

static void ses_callback_delete_tree(session_callback_t *scb) {
    char ppath[BUFSIZE];

    if (!scb || !scb->session)
	return;

    switch (scb->mode) {
    case SES_MODE_PREPROCESS:
	if (!scb->session->isfolder &&
	    ses_make_path(NULL, scb->session->path, ppath, BUFSIZE))
	    ses_delete_session(&scb->session->root, ppath);
	break;
    case SES_MODE_POSTPROCESS:
	if (scb->session->isfolder &&
	    ses_make_path(NULL, scb->session->path, ppath, BUFSIZE))
	    ses_delete_folder(&scb->session->root, ppath);
	break;
    };
};

int ses_delete_tree(session_root_t *root, char *path)
{
    session_walk_t swalk;

    if (!root || !path)
	return FALSE;

    if (!ses_is_folder(root, path)) {
	/*
	 * delete this session
	 */
	return ses_delete_session(root, path);
    } else {
	/*
	 * first we need to delete everything below a folder
	 * specified by treepath
	 */
	memset(&swalk, 0, sizeof(swalk));
	swalk.root = *root;
	swalk.root_path = path;
	swalk.callback = ses_callback_delete_tree;
	swalk.depth = SES_MAX_DEPTH;
	swalk.public1 = (void *) path;

	if (ses_walk_over_tree(&swalk))
	    return ses_delete_folder(root, path);
    };

    return FALSE;
};

int ses_move_tree(session_root_t *root, char *frompath, char *topath)
{
    return ses_copy_tree(root, frompath, topath) &&
	   ses_delete_tree(root, frompath);
};

int ses_read_i(session_root_t *root, char *path, char *valname, 
	       int defval, int *value)
{
    char ppath[BUFSIZE];

    if (!path || !valname)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_read_i(ppath, valname, defval, value);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_read_i(ppath, valname, defval, value);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

int ses_write_i(session_root_t *root, char *path, char *valname, int value)
{
    char ppath[BUFSIZE];

    if (!path || !valname)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_write_i(ppath, valname, value);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_write_i(ppath, valname, value);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

int ses_read_s(session_root_t *root, char *path, char *valname, 
	       char *defval, char *buffer, int bufsize)
{
    char ppath[BUFSIZE];

    if (!path || !valname)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_read_s(ppath, valname, defval, buffer, bufsize);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_read_s(ppath, valname, defval, buffer, bufsize);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

int ses_write_s(session_root_t *root, char *path, char *valname, 
		char *value)
{
    char ppath[BUFSIZE];

    if (!path || !valname)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_write_s(ppath, valname, value);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_write_s(ppath, valname, value);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
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

    handle = ses_enum_settings_start(&sw->root, sw->root_path);

    if (!handle)
	return FALSE;

    sessions = ses_enum_settings_count(&sw->root, handle);

    slist = (session_t **) malloc(sessions * sizeof(session_t *));

    if (!slist) {
	ses_enum_settings_finish(&sw->root, handle);
	return FALSE;
    };

    memset(slist, 0, sessions * sizeof(session_t *));

    for (i = 0; i < sessions; i++) {
	ses_enum_settings_next(&sw->root, handle, name, BUFSIZE);

	if (name[0]) {
	    int len;
	    session_t *session;

	    session = (session_t *) malloc(sizeof(session_t));
	    if (!session) {
		ses_free_slist(slist, i);
		ses_enum_settings_finish(&sw->root, handle);
		return FALSE;
	    };
	    memset(session, 0, sizeof(session_t));

	    len = strlen(name) + 1;
	    session->name = (char *) malloc(len);
	    if (!session->name) {
		free(session);
		ses_free_slist(slist, i);
		ses_enum_settings_finish(&sw->root, handle);
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
		ses_enum_settings_finish(&sw->root, handle);
		return FALSE;
	    };
	    memset(session->path, 0, len);
	    strcpy(session->path, path);

	    session->root = sw->root;
	    session->isfolder = ses_is_folder(&sw->root, path);

	    slist[i] = session;
	};
    };

    ses_enum_settings_finish(&sw->root, handle);

    ses_sort_list(slist, sw->compare, sessions);

    for (i = 0; i < sessions; i++) {
	memset(&scb, 0, sizeof(scb));
	scb.root = sw->root;
	scb.session = slist[i];
	scb.public1 = sw->public1;
	scb.public2 = sw->public2;
	scb.public3 = sw->public3;
	scb.public4 = sw->public4;
	scb.protected1 = sw->protected1;
	scb.protected2 = sw->protected2;
	scb.protected3 = sw->protected3;
	scb.protected4 = sw->protected4;
	scb.mode = SES_MODE_PREPROCESS;

	sw->callback(&scb);

	if (scb.session->isfolder && sw->depth > 0) {
	    memset(&swalk, 0, sizeof(swalk));
	    memmove(&swalk, sw, sizeof(swalk));
	    swalk.root_path = scb.session->path;
	    swalk.protected1 = scb.protected1;
	    swalk.protected2 = scb.protected2;
	    swalk.protected3 = scb.protected3;
	    swalk.protected4 = scb.protected4;
	    swalk.depth--;
	    ses_walk_over_tree(&swalk);
	};

	scb.mode = SES_MODE_POSTPROCESS;
	sw->callback(&scb);
    };

    ses_free_slist(slist, sessions);

    return TRUE;
};

void *ses_enum_settings_start(session_root_t *root, char *path)
{
    char ppath[BUFSIZE];

    if (!path)
	return FALSE;

     if (!root || !root->root_type) {
	if (default_make_path(NULL, path, ppath, BUFSIZE))
	    return default_enum_settings_start(ppath);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    if (reg_make_path_specific(root->root_location,
		    NULL, path, ppath, BUFSIZE))
		return reg_enum_settings_start(ppath);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

int ses_enum_settings_count(session_root_t *root, void *handle)
{
     if (!root || !root->root_type) {
	return default_enum_settings_count(handle);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    return reg_enum_settings_count(handle);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

char *ses_enum_settings_next(session_root_t *root, void *handle, 
			     char *buffer, int buflen)
{
    if (!root || !root->root_type) {
	return default_enum_settings_next(handle, buffer, buflen);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    return reg_enum_settings_next(handle, buffer, buflen);
	    break;
#endif /* _WINDOWS */
	};
    };

    return FALSE;
};

void ses_enum_settings_finish(session_root_t *root, void *handle)
{
    if (!root || !root->root_type) {
	default_enum_settings_finish(handle);
    } else {
	switch (root->root_type) {
#ifdef _WINDOWS
	case SES_ROOT_REGISTRY:
	    reg_enum_settings_finish(handle);
	    break;
#endif /* _WINDOWS */
	};
    };
};

int ses_init_session_root(session_root_t *root, char *cmdline, char *errmsg)
{
    char *command, *url = NULL, *path;

    memset(root, 0, sizeof(session_root_t));

    if (cmdline) {
	command = strstr(cmdline, "-sr");

	if (command) {
	    command += 3;

	    while ((*command == ' ' ||
		   *command == '\t') &&
		   *command != '\0')
		   command++;

	    url = command;
	} else {
	    command = strstr(cmdline, "--session-root");

	    if (command) {
		command += 15;

		url = command;
	    };
	};
    };

    if (!url)
        url = getenv(PUTTY_SESSION_ROOT);

    if (url) {
	if (strstr(url, "registry://")) {
	    root->root_type = SES_ROOT_REGISTRY;
	    url += 11;
	} else if (strstr(url, "file://")) {
	    root->root_type = SES_ROOT_DISKXML;
	    url += 7;
	} else if (strstr(url, "legacy://")) {
#ifdef _WINDOWS
	    strcpy(errmsg, "Legacy file type session root is not supported in Windows.");
	    return FALSE;
#endif /* _WINDOWS */
	    root->root_type = SES_ROOT_DISKLEGACY;
	    url += 9;
	} else if (strstr(url, "http://")) {
	    root->root_type = SES_ROOT_URLXML;
	    url += 7;
	} else if (strstr(url, "ftp://")) {
	    root->root_type = SES_ROOT_URLXML;
	    url += 6;
	};

	path = url;

	while (*url != ' ' && *url != '\t' && *url != '\0') {
#ifdef _WINDOWS
	    if ((root->root_type == SES_ROOT_REGISTRY ||
		root->root_type == SES_ROOT_DISKXML) &&
		*url == '/')
#endif /* _WINDOWS */
		*url = '\\';
	    url++;
	};

	root->root_location = (char *) malloc(url - path + 1);
	memset(root->root_location, 0, url - path + 1);
	strncpy(root->root_location, path, url - path);
    };

    return TRUE;
};

int ses_finish_session_root(session_root_t *root) {
    if (root->root_location)
	free(root->root_location);

    return TRUE;
};

/*
 * PuTTY saved session tree structures, definitions & functions.
 * This file is a part of SESSION_FOLDERS patch tree for PuTTY,
 * also is used in Pageant and PLaunch.
 *
 * (c) 2004 dwalin <dwalin@dwalin.ru>.
 * Portions (c) 1997-2004 Simon Tatham.
 *
 * This software and any part of it is distributed under MIT licence.
 */

#ifndef SESSION_H
#define SESSION_H

#if !defined(_DEBUG) && defined(MINIRTL)
#include "entry.h"
#endif /* _DEBUG && MINIRTL */

#define REGROOT "Software\\SimonTatham\\PuTTY\\Sessions"
#define ISFOLDER "IsFolder"
#define ISEXPANDED "IsExpanded"
#define DEFAULTSETTINGS "Default Settings"

/*
 * session item structure. pretty simple, as it goes.
 */

#define STYPE_SESSION	0
#define STYPE_FOLDER	1

struct session {
    int id;
    int type;				   /* 0: a session, 1: a folder */
    int isexpanded;			   /* is this folder view expanded by default? (used in PLaunch) */
    char *name;				   /* session/folder name without path */
    int nchildren;			   /* number of children */
    struct session **children; /* array of children sessions */
    struct session *parent;    /* parent to this session */
};

/*
 * insert session into session list.
 */

void *insert_session(void *slist, int *snum, void *sess);

/*
 * get the session's full path within a tree.
 */

char *get_full_path(struct session *s);

/*
 * set folder's 'expanded' attribute. used only by plaunch.
 */

int set_expanded(struct session *s, int isexpanded);

/*
 * find a session by its id.
 */

struct session *find_session_by_id(struct session *root, int id);

/*
 * find a session by its name. if recurse == TRUE walk over the tree,
 * otherwise search only among level 1 children.
 */

struct session *find_session_by_name(struct session *root, char *name, int recurse);

/*
 * get all folders and sessions, starting with 'name' as a root.
 * this function goes recursively over a session tree and returns
 * one struct session pointer for root item.
 */

struct session *get_sessions(char *name);

/*
 * create new session.
 */

struct session *new_session(int type, char *name, struct session *parent);

/*
 * free all session tree, starting with s.
 */

struct session *free_session(struct session *s);

/*
 * rename session, modifying registry. returns TRUE if all ok, FALSE on error.
 */

int rename_session(struct session *s, char *name);

/*
 * this is the main wrapper: it returns pointer to
 * root struct session item.
 */

struct session *get_root(void);

#endif /* SESSION_H */

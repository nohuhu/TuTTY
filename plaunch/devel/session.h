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
#endif				/* _DEBUG && MINIRTL */

#define DEFAULTSETTINGS	"Default Settings"

/*
 * session item structure. pretty simple, as it goes.
 */

#define STYPE_SESSION	0
#define STYPE_FOLDER	1

struct session {
    int id;
    int type;			/* 0: a session, 1: a folder */
    int isexpanded;		/* is this folder view expanded by default? (used in PLaunch) */
    int nhotkeys;		/* number of this session's hot keys. */
    int hotkeys[2];		/* hotkeys for this session. 0th for launch, 1st for edit */
    char *name;			/* session/folder name without path */
    int nchildren;		/* number of children */
    struct session **children;	/* array of children sessions */
    struct session *parent;	/* parent to this session */
};

/*
 * compare two sessions to determine which one goes first in alphabetical order.
 */

int session_compare(const struct session *s1, const struct session *s2);

/*
 * insert session into session list.
 */

struct session **session_insert(struct session **sl, int *snum,
				struct session *s);

/*
 * remove session from a session list.
 */

struct session **session_remove(struct session **sl, int *snum,
				struct session *s);

/*
 * get the session's full path within a tree.
 */

char *session_get_full_path(struct session *s);

/*
 * set folder's 'expanded' attribute. used only by plaunch.
 */

int session_set_expanded(struct session *s, int isexpanded);

/*
 * set session's 'hotkey' attribute. used only by plaunch.
 */

int session_set_hotkey(struct session *, int index, int hotkey);

/*
 * find a session by its id.
 */

struct session *session_find_by_id(struct session *root, int id);

/*
 * find a session by its name. if recurse == TRUE walk over the tree,
 * otherwise search only among level 1 children.
 */

struct session *session_find_by_name(struct session *root, char *name,
				     int recurse);

/*
 * find a session by its hotkey. if recurse == TRUE walk over the tree,
 * otherwise search only among level 1 children.
 */

struct session *session_find_by_hotkey(struct session *root, int hotkey,
				       int recurse);

/*
 * get all folders and sessions, starting with 'name' as a root.
 * this function goes recursively over a session tree and returns
 * one struct session pointer for root item.
 */

struct session *session_get_tree(char *name);

/*
 * allocate new struct session.
 */

struct session *session_new(int type, char *name, struct session *parent);

/*
 * free all session tree, starting with s.
 */

struct session *session_free(struct session *s);

/*
 * duplicate session
 */

struct session *session_duplicate(struct session *s);

/*
 * create new session or folder in registry
 */

struct session *session_create(int type, char *name,
			       struct session *parent);

/*
 * delete session. returns TRUE on success, FALSE on error.
 */

int session_delete(struct session *s);

/*
 * copy session to another name/folder. returns pointer to new session if
 * all ok, NULL on failure.
 */

struct session *session_copy(struct session *s, char *name);

/*
 * rename session, modifying registry. returns pointer to new session if
 * all ok, NULL on error.
 */

struct session *session_rename(struct session *s, char *name);

/*
 * this is the main wrapper: it returns pointer to
 * root struct session item.
 */

struct session *session_get_root(void);

#endif				/* SESSION_H */

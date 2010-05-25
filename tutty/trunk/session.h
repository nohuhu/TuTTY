#ifndef SESSION_H
#define SESSION_H

#ifdef PLAUNCH
#include "plaunch.h"
#endif

#ifndef BUFSIZE
#define BUFSIZE			2048
#endif /* BUFSIZE */

#define SES_MODE_PREPROCESS	1
#define	SES_MODE_POSTPROCESS	2

#define SES_MAX_DEPTH		65535	/* ought to be enough for anybody */

#define	SES_ROOT_DEFAULT	0   /* 
				     * default method: registry for windows,
				     * legacy disk file for unix
				     */
#define	SES_ROOT_REGISTRY	1   /*
				     * windows only: registry with non-default
				     * path
				     */
#define	SES_ROOT_DISKLEGACY	2   /*
				     * windows/unix: legacy disk file
				     */
#define	SES_ROOT_DISKXML	3   /*
				     * windows/unix: xml disk file
				     */
#define	SES_ROOT_URLXML		4    /*
				     * windows/unix: xml file on remote server
				     */

typedef struct _session_root_t {
    unsigned int root_type;	    /* session root type */
    char *root_location;	    /* 
				     * session root location: valid only for
				     * non-default root types
				     */
} session_root_t;

typedef struct _session_t {
    session_root_t root;
    char *name;
    char *path;
    unsigned int isfolder;
} session_t;

typedef struct _session_callback_t {
    session_root_t root;
    session_t *session;
    int mode;			    /* session callback mode: pre/post process */
    void *retval;		    /* 
				     * any custom return value that should be passed
				     * further 
				     */
    void *p1;			    /* any private data */
    void *p2;
    void *p3;
    void *p4;
} session_callback_t;

/*
 * a prototype for a callback function. ses_walk_over_tree()
 * will call this callback on every session and folder it finds,
 * passing it a pointer to filled sessioncallback structure.
 */
typedef void (*ses_callback_f)(session_callback_t *scb);

/*
 * a prototype for comparison function. ses_walk_over_tree()
 * will use this function to compare items in the session list
 * when sorting it.
 * unfortunately we can't use existing sessioncmp() because of
 * two reasons: first, it's not aware of tree-like structure
 * and will sort anything alphabetically, and second, because of
 * the first we need to pass it additional data.
 */
typedef int (*ses_compare_f)(const session_t *a, const session_t *b);

typedef struct _session_walk_t {
    session_root_t root;
    char *root_path;		    /* where to start, relative to root_location */
    signed int depth;		    /*
				     * recursion depth:
				     * 0 means only current folder, no recursion
				     * > 0 means n levels of recursion
				     */
    ses_callback_f callback;	    /* call back function */
    ses_compare_f compare;	    /* comparison function. use default if NULL */
    void *retval;		    /* 
				     * any custom return value that should be passed
				     * further 
				     */
    void *p1;			    /* any private data */
    void *p2;			     
    void *p3;			     
    void *p4;
} session_walk_t;

char *ses_lastname(char *in);
int ses_pathname(char *in, char *buffer, unsigned int bufsize);
int ses_make_path(char *parent, char *path, char *buffer,
		  unsigned int bufsize);

int ses_is_folder(session_root_t *root, char *path);

int ses_make_folder(session_root_t *root, char *path);

int ses_copy_tree(session_root_t *root, char *frompath, char *topath);
int ses_move_tree(session_root_t *root, char *frompath, char *topath);

int ses_delete_value(session_root_t *root, char *spath, char *valname);
int ses_delete_session(session_root_t *root, char *spath);
int ses_delete_tree(session_root_t *root, char *spath);

int ses_read_i(session_root_t *root, char *spath, char *valname, 
	       int defval, int *value);
int ses_write_i(session_root_t *root, char *spath, char *valname, 
		int value);

int ses_read_s(session_root_t *root, char *spath, char *valname, 
	       char *defval, char *buffer, int bufsize);
int ses_write_s(session_root_t *root, char *spath, char *valname, 
		char *value);

int ses_walk_over_tree(session_walk_t *sw);

#endif				/* SESSION_H */

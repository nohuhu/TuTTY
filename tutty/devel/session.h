#ifndef SESSION_H
#define SESSION_H

#ifndef BUFSIZE
#define BUFSIZE			2048
#endif /* BUFSIZE */

#ifndef PUTTY_SESSION_ROOT
#define	PUTTY_SESSION_ROOT	"PUTTY_SESSION_ROOT"
#endif /* PUTTY_SESSION_ROOT */

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
#define	SES_ROOT_HTTPXML	4   /*
				     * windows/unix: xml file on remote http server
				     */
#define SES_ROOT_FTPXML		5   /*
				     * windows/unix: xml file on remote ftp server
				     */

typedef struct _session_root_t {
    unsigned int root_type;	    /* session root type */
    char *root_location;	    /* 
				     * session root location: valid only for
				     * non-default root types
				     */
    int readonly;		    /*
				     * is the root read only?
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
    void *public1;		    /*
				     * this is a public data passed to callback
				     * function as an input. these pointers
				     * are guaranteed to never be changed by the 
				     * callback function itself.
				     */
    void *public2;
    void *public3;
    void *public4;
    void *protected1;		    /*
				     * these pointers are used for passing
				     * data deeper into the tree. we can provide
				     * allocated storage space from the outside
				     * but should never ever try to preset any values.
				     */
    void *protected2;
    void *protected3;
    void *protected4;
    void *private1;		    /* 
				     * any private data passed between callbacks
				     * on the same level. it is not available for
				     * manipulating from outside in any way.
				     */
    void *private2;
    void *private3;
    void *private4;
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
    void *public1;		    /*
				     * this is a public data passed to callback
				     * function as an input. these pointers
				     * are guaranteed to never be changed by the 
				     * callback function itself.
				     */

    void *public2;			     
    void *public3;			     
    void *public4;
    void *protected1;		    /*
				     * these pointers are used for passing
				     * data deeper into the tree. we can provide
				     * allocated storage space from the outside
				     * but should never ever try to preset any values.
				     */
    void *protected2;
    void *protected3;
    void *protected4;
} session_walk_t;

char *ses_lastname(char *in);
int ses_pathname(char *in, char *buffer, unsigned int bufsize);
int ses_make_path(char *parent, char *path, char *buffer,
		  unsigned int bufsize);

int ses_is_folder(session_root_t *root, char *path);

int ses_make_folder(session_root_t *root, char *path);

int ses_copy_session(session_root_t *root, char *frompath, char *topath);
int ses_copy_tree(session_root_t *root, char *frompath, char *topath);

int ses_delete_value(session_root_t *root, char *path, char *valname);
int ses_delete_session(session_root_t *root, char *path);
int ses_delete_folder(session_root_t *root, char *path);
int ses_delete_tree(session_root_t *root, char *path);

int ses_move_tree(session_root_t *root, char *frompath, char *topath);

int ses_read_i(session_root_t *root, char *path, char *valname, 
	       int defval, int *value);
int ses_write_i(session_root_t *root, char *path, char *valname, 
		int value);

int ses_read_s(session_root_t *root, char *path, char *valname, 
	       char *defval, char *buffer, int bufsize);
int ses_write_s(session_root_t *root, char *path, char *valname, 
		char *value);

int ses_walk_over_tree(session_walk_t *sw);

void *ses_open_session_r(session_root_t *root, char *path);
void *ses_open_session_w(session_root_t *root, char *path);
void ses_close_session(session_root_t *root, void *handle);

int ses_read_handle_i(session_root_t *root, void *handle, char *valname,
		      int defval, int *value);
int ses_write_handle_i(session_root_t *root, void *handle, char *valname,
		       int value);
int ses_read_handle_s(session_root_t *root, void *handle, char *valname,
		      char *defval, char *buffer, int bufsize);
int ses_write_handle_s(session_root_t *root, void *handle, char *valname,
		       char *value);
int ses_delete_value_handle(session_root_t *root, void *handle, char *valname);

void *ses_enum_settings_start(session_root_t *root, char *path);
int ses_enum_settings_count(session_root_t *root, void *handle);
char *ses_enum_settings_next(session_root_t *root, void *handle, 
			     char *buffer, int buflen);
void ses_enum_settings_finish(session_root_t *root, void *handle);

int ses_init_session_root(session_root_t *root, char *cmdline, char *errmsg, 
			  int bufsize);
int ses_cmdline_from_session_root(session_root_t *root, char *cmdline, 
				  int bufsize);
int ses_finish_session_root(session_root_t *root);

#endif				/* SESSION_H */

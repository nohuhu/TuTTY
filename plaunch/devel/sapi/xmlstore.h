/*
 * Session abstraction layer for TuTTY and PLaunch.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2005, 2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Platform independent XML file storage header file.
 */

#ifndef XMLSTORE_H
#define XMLSTORE_H

#include "session.h"

#define XML_ROOT			"PuTTY"
#define XML_SESSIONROOT			"Sessions"
#define XML_SSHHOSTKEYS			"SshHostKeys"
#define XML_PLAUNCHROOT			"PLaunch"

#define DEFAULTSETTINGS			"Default Settings"
#define SESSION				"Session"
#define ISFOLDER			"IsFolder"

void *xml_init(session_root_t *root, char *errmsg, int errsize);
int xml_write(session_root_t *root, char *errmsg, int errsize);
void xml_finish(session_root_t *root);

int xml_make_path(char *parent, char *path, char *buffer,
		  int bufsize);

void *xml_open_session_r(session_root_t *root, char *keyname);
void *xml_open_session_w(session_root_t *root, char *keyname);

void *xml_open_key_r(session_root_t *root, char *keyname);
void *xml_open_key_w(session_root_t *root, char *keyname);

void xml_close_key(session_root_t *root, void *handle);

int xml_copy_session(session_root_t *root, char *from, char *to);

int xml_delete_v(session_root_t *root, void *handle, char *valname);
int xml_delete_k(session_root_t *root, void *handle, char *keyname);

int xml_read_i(session_root_t *root, void *handle, char *valname, 
	       int defval, int *value);
int xml_write_i(session_root_t *root, void *handle, char *valname, 
		int value);

int xml_read_s(session_root_t *root, void *handle, char *valname, 
	       char *defval, char *buffer, int bufsize);

int xml_write_s(session_root_t *root, void *handle, char *valname, 
		char *value);

void *xml_enum_settings_start(session_root_t *root, char *path);
int xml_enum_settings_count(session_root_t *root, void *handle);
char *xml_enum_settings_next(session_root_t *root, void *handle, 
			     char *buffer, int buflen);
void xml_enum_settings_finish(session_root_t *root, void *handle);

void *xml_enum_values_start(session_root_t *root, void *session);
int xml_enum_values_count(session_root_t *root, void *handle);
int xml_enum_values_type(session_root_t *root, void *handle);
char *xml_enum_values_next(session_root_t *root, void *handle, 
			   char *buffer, int buflen);
void xml_enum_values_finish(session_root_t *root, void *handle);

#endif /* XMLSTORE_H */

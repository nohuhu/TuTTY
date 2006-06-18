/*
 * Session abstraction layer for TuTTY and PLaunch.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2005, 2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Windows-specific registry storage header file.
 */

#ifndef REGISTRY_H
#define REGISTRY_H

#include "session.h"

#define DEFAULTSETTINGS			"Default Settings"
#define REGROOT				"Software\\SimonTatham\\PuTTY\\Sessions"
#define ISFOLDER			"IsFolder"

int reg_make_path(char *parent, char *path, char *buffer,
		  int bufsize);

int reg_make_path_specific(char *root, char *parent, char *path,
			   char *buffer, int bufsize);

void *reg_open_session_r(session_root_t *root, char *keyname);
void *reg_open_session_w(session_root_t *root, char *keyname);

void *reg_open_key_r(session_root_t *root, char *keyname);
void *reg_open_key_w(session_root_t *root, char *keyname);

void reg_close_key(session_root_t *root, void *handle);

int reg_copy_session(session_root_t *root, char *from, char *to);

int reg_delete_v(session_root_t *root, void *handle, char *valname);
int reg_delete_k(session_root_t *root, char *keyname);

int reg_read_i(session_root_t *root, void *handle, char *valname, 
	       int defval, int *value);
int reg_write_i(session_root_t *root, void *handle, char *valname, 
		int value);

int reg_read_s(session_root_t *root, void *handle, char *valname, 
	       char *defval, char *buffer, int bufsize);

int reg_write_s(session_root_t *root, void *handle, char *valname, 
		char *value);

void *reg_enum_settings_start(session_root_t *root, char *path);
int reg_enum_settings_count(session_root_t *root, void *handle);
char *reg_enum_settings_next(session_root_t *root, void *handle, 
			     char *buffer, int buflen);
void reg_enum_settings_finish(session_root_t *root, void *handle);

void *reg_enum_values_start(session_root_t *root, void *session);
int reg_enum_values_count(session_root_t *root, void *handle);
int reg_enum_values_type(session_root_t *root, void *handle);
char *reg_enum_values_next(session_root_t *root, void *handle,
			   char *buffer, int buflen);
void reg_enum_values_finish(session_root_t *root, void *handle);

#endif /* REGISTRY_H */

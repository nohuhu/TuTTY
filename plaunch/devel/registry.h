#ifndef REGISTRY_H
#define REGISTRY_H

#define DEFAULTSETTINGS			"Default Settings"
#define REGROOT				"Software\\SimonTatham\\PuTTY\\Sessions"
#define ISFOLDER			"IsFolder"

int reg_make_path(char *parent, char *path, char *buffer,
		  int bufsize);

int reg_make_path_specific(char *root, char *parent, char *path,
			   char *buffer, int bufsize);

void *reg_open_session_r(char *keyname);
void *reg_open_session_w(char *keyname);
void reg_close_session(void *handle);

int reg_copy_session(char *from, char *to);

int reg_delete_v(void *handle, char *valname);
int reg_delete_k(char *keyname);

int reg_read_i(void *handle, char *valname, int defval, int *value);
int reg_write_i(void *handle, char *valname, int value);

int reg_read_s(void *handle, char *valname, char *defval, 
	       char *buffer, int bufsize);

int reg_write_s(void *handle, char *valname, char *value);

void *reg_enum_settings_start(char *path);
int reg_enum_settings_count(void *handle);
char *reg_enum_settings_next(void *handle, char *buffer, int buflen);
void reg_enum_settings_finish(void *handle);

#endif /* REGISTRY_H */

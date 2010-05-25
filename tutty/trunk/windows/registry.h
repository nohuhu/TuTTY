#ifndef REGISTRY_H
#define REGISTRY_H

#define DEFAULTSETTINGS			"Default Settings"
#define REGROOT				"Software\\SimonTatham\\PuTTY\\Sessions"
#define ISFOLDER			"IsFolder"
#define ISEXPANDED			"IsExpanded"
#define	SESSIONICON			"WindowIcon"
#define HOTKEY				"PLaunchHotKey"
#define PLAUNCH_LIMIT_ENABLE		"PLaunchInstanceLimitEnable"
#define PLAUNCH_LIMIT_NUMBER		"PLaunchInstanceLimitNumber"
#define PLAUNCH_LIMIT_SEARCHFOR		"PLaunchInstanceLimitSearchFor"
#define	PLAUNCH_LIMIT_ACTION1		"PLaunchInstanceLimitAction1"
#define PLAUNCH_LIMIT_ACTION2		"PLaunchInstanceLimitAction2"
#define	PLAUNCH_AUTORUN_ENABLE		"PLaunchAutoRunEnable"
#define	PLAUNCH_AUTORUN_WHEN		"PLaunchAutoRunCondition"
#define	PLAUNCH_AUTORUN_ACTION		"PLaunchAutoRunAction"

#define	LIMIT_SEARCHFOR_LAST		0
#define	LIMIT_SEARCHFOR_FIRST		1
#define	LIMIT_SEARCHFOR_SECOND		2
#define	LIMIT_SEARCHFOR_THIRD		3
#define	LIMIT_SEARCHFOR_FOURTH		4
#define	LIMIT_SEARCHFOR_FIFTH		5
#define	LIMIT_SEARCHFOR_SIXTH		6
#define	LIMIT_SEARCHFOR_SEVENTH		7
#define	LIMIT_SEARCHFOR_EIGHTH		8
#define	LIMIT_SEARCHFOR_NINTH		9
#define	LIMIT_SEARCHFOR_TENTH		10
#define	LIMIT_SEARCHFOR_ELEVENTH	11
#define	LIMIT_SEARCHFOR_TVELFTH		12
#define	LIMIT_SEARCHFOR_UMPTEENTH	13
#define	LIMIT_SEARCHFOR_MAX		14

extern const char *const LIMIT_SEARCHFOR_STRINGS[];

#define	LIMIT_ACTION_NOTHING		0
#define	LIMIT_ACTION_HIDE		1
#define	LIMIT_ACTION_SHOW		2
#define	LIMIT_ACTION_MINIMIZE		3
#define	LIMIT_ACTION_MAXIMIZE		4
#define	LIMIT_ACTION_CENTER		5
#define	LIMIT_ACTION_KILL		6
#define	LIMIT_ACTION_MURDER		7
#define	LIMIT_ACTION_RUN		8
#define LIMIT_ACTION_MAX		9

extern const char *const LIMIT_ACTION_STRINGS[];

#define	AUTORUN_WHEN_START		0
#define	AUTORUN_WHEN_NETWORKSTART	1
#define	AUTORUN_WHEN_QUIT		2
#define	AUTORUN_WHEN_MAX		3

extern const char *const AUTORUN_WHEN_STRINGS[];

#define	AUTORUN_ACTION_NOTHING		0
#define	AUTORUN_ACTION_HIDE		1
#define	AUTORUN_ACTION_SHOW		2
#define	AUTORUN_ACTION_MINIMIZE		3
#define	AUTORUN_ACTION_MAXIMIZE		4
#define	AUTORUN_ACTION_CENTER		5
#define	AUTORUN_ACTION_MAX		6

extern const char *const AUTORUN_ACTION_STRINGS[];

#define REG_MODE_PREPROCESS		1
#define	REG_MODE_POSTPROCESS		2

int sessioncmp(const char *a, const char *b);

int reg_make_path(char *parent, char *path, char *buffer,
		  unsigned int bufsize);

int reg_copy_tree(char *keyfrom, char *keyto);

int reg_delete_v(char *keyname, char *valname);
int reg_delete_k(char *keyname);
int reg_delete_tree(char *keyname);

int reg_move_tree(char *keyfrom, char *keyto);

int reg_read_i(char *keyname, char *valname, int defval, int *value);
int reg_write_i(char *keyname, char *valname, int value);

int reg_read_s(char *keyname, char *valname, char *defval,
	       char *buffer, unsigned int bufsize);

int reg_write_s(char *keyname, char *valname, char *value);

typedef int (*reg_callback) (char *name, char *path,
			     int isfolder, int mode,
			     void *priv1, void *priv2, void *priv3);

int reg_walk_over_tree(char *root, reg_callback cb,
		       void *priv1, void *priv2, void *priv3);

void *reg_enum_settings_start(char *path);
int reg_enum_settings_count(void *handle);
char *reg_enum_settings_next(void *handle, char *buffer, int buflen);
void reg_enum_settings_finish(void *handle);

#endif				/* REGISTRY_H */

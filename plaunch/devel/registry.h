#ifndef REGISTRY_H
#define REGISTRY_H

#include <windows.h>
#include "entry.h"

int sessioncmp(const char *a, const char *b);

#define DEFAULTSETTINGS	"Default Settings"
#define REGROOT			"Software\\SimonTatham\\PuTTY\\Sessions"
#define ISFOLDER		"IsFolder"
#define ISEXPANDED		"IsExpanded"
#define HOTKEY			"PLaunchHotKey"

int reg_copy_tree(char *keyfrom, char *keyto);

int reg_delete_v(char *keyname, char *valname);
int reg_delete_k(char *keyname);
int reg_delete_tree(char *keyname);

int reg_move_tree(char *keyfrom, char *keyto);

int reg_read_i(char *keyname, char *valname, int defval);
int reg_write_i(char *keyname, char *valname, int value);

char *reg_read_s(char *keyname, char *valname, char *defval);
int reg_write_s(char *keyname, char *valname, char *value);

#define REG_MODE_PREPROCESS		1
#define	REG_MODE_POSTPROCESS	2

typedef int (*reg_callback)(char *name, char *path, 
							int isfolder, int mode, 
							void *priv1, void *priv2, void *priv3);
int reg_walk_over_tree(char *root, reg_callback cb,
					   void *priv1, void *priv2, void *priv3);

#endif /* REGISTRY_H */
#ifndef HOTKEY_H
#define HOTKEY_H

#include <windows.h>

#define	HOTKEY_ACTION_NONE		0
#define HOTKEY_ACTION_MESSAGE	1
#define	HOTKEY_ACTION_LAUNCH	2
#define	HOTKEY_ACTION_EDIT		3
#define	HOTKEY_ACTION_HIDE		4
#define	HOTKEY_ACTION_KILL		5

#define	HOTKEY_MAX_ACTION		5

extern const char * const HOTKEY_STRINGS[];

#define HK_CHANGE	0x9000

void key_name(UINT modifiers, UINT vkey, char *buffer, unsigned int bufsize);

unsigned int make_hotkey(HWND editbox, LONG defhotkey);
unsigned int unmake_hotkey(HWND editbox);

LONG get_hotkey(HWND editbox);
LONG set_hotkey(HWND editbox, LONG newhotkey);

#endif /* HOTKEY_H */
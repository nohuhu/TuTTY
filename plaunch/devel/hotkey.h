#ifndef HOTKEY_H
#define HOTKEY_H

#include <windows.h>

#if !defined(_DEBUG) && defined(MINIRTL)
#include "entry.h"
#else
#include <stdio.h>
#endif

#define HK_CHANGE	0x9000

char *key_name(UINT modifiers, UINT vkey);

int make_hotkey(HWND editbox, LONG defhotkey);
int unmake_hotkey(HWND editbox);

LONG get_hotkey(HWND editbox);
LONG set_hotkey(HWND editbox, LONG newhotkey);

#endif /* HOTKEY_H */
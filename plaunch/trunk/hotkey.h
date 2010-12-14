/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004-2006 dwalin <dwalin@dwalin.ru>
 *
 * Advanced Hotkey Control header file.
 */

#ifndef HOTKEY_H
#define HOTKEY_H

#include <windows.h>

#define HK_CHANGE	0x9000

void key_name(UINT modifiers, UINT vkey, char *buffer,
	      unsigned int bufsize);

unsigned int make_hotkey(HWND editbox, LONG defhotkey);
unsigned int unmake_hotkey(HWND editbox);

LONG get_hotkey(HWND editbox);
LONG set_hotkey(HWND editbox, LONG newhotkey);

#endif				/* HOTKEY_H */

/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004-2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Custom window menu manipulation routines header file.
 */

#ifndef WINMENU_H
#define WINMENU_H
#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#ifndef SESSIONICON
#define SESSIONICON "WindowIcon"
#endif /* SESSIONICON */

HMENU menu_addsession(HMENU menu, char *root);
HMENU menu_refresh(HMENU menu, char *root);

#endif				/* WINMENU_H */

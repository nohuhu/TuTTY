/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Header file.
 */

#ifndef PLAUNCH_H
#define PLAUNCH_H

#include <windows.h>

#define IMG_FOLDER_OPEN	    4
#define IMG_FOLDER_CLOSED   3

#define BUFSIZE 2048

#define HOTKEY_LAUNCHBOX	1
#define HOTKEY_WINDOWLIST	2

#define WM_XUSER     (WM_USER + 0x2000)
#define WM_SYSTRAY   (WM_XUSER + 6)
#define WM_SYSTRAY2  (WM_XUSER + 7)

#define IDM_CLOSE	    0x0010
#define IDM_LAUNCHBOX	    0x0020
//#define IDM_ADDKEY	    0x0030
#define IDM_HELP	    0x0040
#define IDM_ABOUT	    0x0050
#define	IDM_WINDOWLIST	0x0060
#define IDM_CTXM_COPY	0x0070
#define IDM_CTXM_CUT	0x0071
#define IDM_CTXM_PASTE	0x0072
#define IDM_CTXM_DELETE	0x0073
#define IDM_CTXM_UNDO	0x0074
#define IDM_CTXM_CRTFLD	0x0075
#define IDM_CTXM_CRTSES	0x0076
#define IDM_CTXM_RENAME	0x0077
#define IDM_CTXW_SHOW	0x0080
#define IDM_CTXW_HIDE	0x0081
#define IDM_CTXW_KILL	0x0082

#define IDM_SESSION_BASE    0x0300
#define IDM_SESSION_MAX	    0x0500

#define IDI_MAINICON	100
#define ID_LICENSE		101
#define ID_NEWFOLDER	102
#define ID_NEWSESSION	103
#define ID_EDIT			104
#define ID_RENAME		105
#define ID_DELETE		106
#define ID_OPTIONS		107

#define	ID_WINDOWLISTSTATIC		110
#define	ID_WINDOWLISTLISTBOX	111
#define	ID_WINDOWLISTHIDE		112
#define	ID_WINDOWLISTSHOW		113
#define	ID_WINDOWLISTKILL		114

#define ID_OPTIONS_GROUPBOX			120
#define ID_OPTIONS_PUTTYPATH_STATIC	121
#define	ID_OPTIONS_PUTTYPATH_EDIT	122
#define ID_OPTIONS_PUTTYPATH_BUTTON	123
#define	ID_OPTIONS_WL_HOTKEY_STATIC	124
#define ID_OPTIONS_WL_HOTKEY_EDIT	125
#define ID_OPTIONS_LB_HOTKEY_STATIC	126
#define	ID_OPTIONS_LB_HOTKEY_EDIT	127

#define APPNAME "PLaunch"
#define APPVERSION "0.2"
#define PUTTY "PuTTY"
#define PUTTYTEL "PuTTYtel"

#ifdef _DEBUG
#define PL_HOTKEY_WINDOWLIST 'Q'
#define PL_HOTKEY_LAUNCHBOX 'O'
#else
#define PL_HOTKEY_WINDOWLIST 'W'
#define PL_HOTKEY_LAUNCHBOX 'P'
#endif /* _DEBUG */

HINSTANCE hinst;
HWND hwnd;
HIMAGELIST image_list;
HMENU systray_menu;
char *putty_path;
int img_open, img_closed, img_session;
static int iconx = 0, icony = 0;

typedef BOOL (WINAPI * SHGIL_PROC)	(HIMAGELIST *phLarge, HIMAGELIST *phSmall);
typedef BOOL (WINAPI * FII_PROC)	(BOOL fFullInit);

#endif /* PLAUNCH_H */

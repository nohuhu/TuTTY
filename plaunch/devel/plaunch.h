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

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <commctrl.h>

#define BUFSIZE 512

#define WM_XUSER			(WM_USER + 0x2000)
#define WM_SYSTRAY			(WM_XUSER + 6)
#define WM_SYSTRAY2			(WM_XUSER + 7)
#define WM_REFRESHTV		(WM_XUSER + 8)
#define WM_REFRESHBUTTONS	(WM_XUSER + 9)
#define WM_HOTKEYCHANGE		(WM_XUSER + 10)
#define WM_LAUNCHBOX		(WM_XUSER + 11)
#define WM_WINDOWLIST		(WM_XUSER + 12)
#define	WM_LAUNCHPUTTY		(WM_XUSER + 13)
#define	WM_NETWORKSTART		(WM_XUSER + 14)
#define	WM_HIDEWINDOW		(WM_XUSER + 15)

#define IDM_CLOSE	    0x0010
#define IDM_LAUNCHBOX	0x0020
//#define IDM_ADDKEY    0x0030
#define IDM_HELP	    0x0040
#define IDM_ABOUT	    0x0050
#define	IDM_WINDOWLIST	0x0060
#define IDM_OPTIONSBOX	0x0070
#define IDM_BACKREST	0x0080

#define	IDM_EMPTY		0x0200

#define IDM_SESSION_BASE    0x0300
#define IDM_SESSION_MAX	    0x0500

#define	IDM_RUNNING_BASE	0x0501
#define	IDM_RUNNING_MAX		0x0700

#define IDI_MAINICON	100

#define APPNAME "PLaunch"
#ifdef WINDOWS_NT351_COMPATIBLE
#define WINDOWTITLE "PuTTY Launcher"
#else
#define WINDOWTITLE "PLaunch"
#endif				/* WINDOWS_NT351_COMPATIBLE */
#define APPVERSION "0.56"
#define PUTTY "PuTTY"
#define PUTTYTEL "PuTTYtel"

#define OPTION_ENABLEDRAGDROP	0x0001
#define OPTION_ENABLESAVECURSOR	0x0002
#define	OPTION_SHOWONQUIT		0x0004
#define	OPTION_MENUSESSIONS		0x0008
#define	OPTION_MENURUNNING		0x0010

struct _config {
    HINSTANCE hinst;
    HWND hwnd_mainwindow;
    HIMAGELIST image_list;
    HMENU systray_menu;
    HICON main_icon;
    unsigned int img_open;
    unsigned int img_closed;
    unsigned int img_session;
    unsigned int iconx, icony;
    char putty_path[BUFSIZE];
    unsigned int nhotkeys;
    struct _hotkey_action {
	LONG hotkey;
	unsigned int action;
	char *destination;
    } hotkeys[256];
    unsigned int options;
    unsigned int version_major;
    unsigned int version_minor;
#ifdef WINDOWS_NT351_COMPATIBLE
    unsigned int have_shell;
#endif				/* WINDOWS_NT351_COMPATIBLE */
} *config;

/*
 * import from aboutbox.c
 */

void do_aboutbox(void);

/*
 * import from licensebox.c
 */
void do_licensebox(void);

/*
 * import from launchbox.c
 */
static HWND hwnd_launchbox = NULL;
int do_launchbox(void);

/*
 * import from windowlistbox.c
 */
struct windowlist {
    unsigned int nhandles;
    unsigned int current;
    HWND *handles;
};

void do_windowlistbox(void);

/*
 * import from optionsbox.c
 */
void do_optionsbox(void);

#endif				/* PLAUNCH_H */

/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004-2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Main implementation file.
 */

#include "plaunch.h"
#include <tchar.h>
#include <wininet.h>
#include <stdio.h>
#include "entry.h"
#include "dlgtmpl.h"
#include "misc.h"
#include "hotkey.h"
#include "winmenu.h"
#include "session.h"

#define IMG_FOLDER_OPEN	    4
#define IMG_FOLDER_CLOSED   3

static char *lastname(char *in)
{
    char *p;

    if (in == "")
	return in;

    p = &in[strlen(in)];

    while (p >= in && *p != '\\')
	*p--;

    return p + 1;
};

extern BOOL CALLBACK CountPuTTYWindows(HWND hwnd, LPARAM lParam);
extern BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam);

static unsigned int add_tray_icon(HWND hwnd)
{
    unsigned int ret;
    NOTIFYICONDATA tnid;
    HICON hicon;

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hwnd;
    tnid.uID = 1;		/* unique within this systray use */
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = WM_SYSTRAY;
    tnid.hIcon = hicon = config->main_icon;
#ifdef _DEBUG
    strcpy(tnid.szTip,
	   "(DEBUG) PuTTY Launcher version " APPVERSION " built " __DATE__
	   " " __TIME__);
#else
    strcpy(tnid.szTip, "PuTTY Launcher");
#endif				/* _DEBUG */

    ret = Shell_NotifyIcon(NIM_ADD, &tnid);

    if (hicon)
	DestroyIcon(hicon);

    return ret;
};

/*
 * Main window (hidden): window function.
 */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
    static unsigned int menuinprogress;
    static UINT msgTaskbarCreated = 0;
    static UINT iconx = 0, icony = 0, cxmenucheck = 0, cymenucheck = 0;

    switch (message) {
    case WM_CREATE:
	msgTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
	iconx = config->iconx;
	icony = config->icony;
	cxmenucheck = GetSystemMetrics(SM_CXMENUCHECK);
	cymenucheck = GetSystemMetrics(SM_CYMENUCHECK);
	break;
    default:
	if (message == msgTaskbarCreated) {
	    /*
	     * Explorer has been restarted, so the tray icon will
	     * have been lost.
	     */
	    add_tray_icon(hwnd);
	}
	break;
#ifdef WINDOWS_NT351_COMPATIBLE
    case WM_INITMENUPOPUP:
	if (!config->have_shell && (HMENU) wParam == config->systray_menu) {
	    config->systray_menu = menu_refresh(config->systray_menu, "");
	};
	break;
#endif				/* WINDOWS_NT351_COMPATIBLE */
    case WM_SYSTRAY:
	if (lParam == WM_RBUTTONUP) {
	    POINT cursorpos;
	    GetCursorPos(&cursorpos);
	    SendMessage(hwnd, WM_SYSTRAY2, cursorpos.x, cursorpos.y);
	} else if (lParam == WM_LBUTTONDBLCLK) {
	    SendMessage(hwnd, WM_COMMAND, IDM_LAUNCHBOX, 0);
	}
	return TRUE;
    case WM_SYSTRAY2:
	if (!menuinprogress) {
	    int ret;

	    menuinprogress = TRUE;

	    config->systray_menu = menu_refresh(config->systray_menu, "");

	    SetForegroundWindow(hwnd);
	    ret = TrackPopupMenu(config->systray_menu,
				 TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
				 TPM_RIGHTBUTTON | TPM_RETURNCMD,
				 wParam, lParam, 0, hwnd, NULL);
	    PostMessage(hwnd, WM_NULL, 0, 0);

	    if (ret >= IDM_SESSION_BASE && ret <= IDM_SESSION_MAX) {
		MENUITEMINFO mii;

		memset(&mii, 0, sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA;
		if (GetMenuItemInfo(config->systray_menu, ret, FALSE, &mii) &&
		    mii.dwItemData != 0)
		    SendMessage(hwnd, WM_LAUNCHPUTTY,
				(WPARAM) HOTKEY_ACTION_LAUNCH,
				(LPARAM) mii.dwItemData);
	    } else if (ret >= IDM_RUNNING_BASE && ret <= IDM_RUNNING_MAX) {
		MENUITEMINFO mii;
		HWND window;

		memset(&mii, 0, sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA;
		if (GetMenuItemInfo(config->systray_menu, ret, FALSE, &mii) &&
		    mii.dwItemData != 0) {
		    window = (HWND) mii.dwItemData;
		    ShowWindow(window, SW_NORMAL);
		    SetForegroundWindow(window);
		};
	    } else
		SendMessage(hwnd, WM_COMMAND, (WPARAM) ret, 0);

	    menuinprogress = FALSE;
	}
	return TRUE;
    case WM_LAUNCHPUTTY:
	{
	    char *path;
	    HWND pwin;
	    int limit;

	    if (!lParam)
		return FALSE;

	    path = (char *) lParam;

	    /*
	     * is there an instance limit for this session?
	     * if it is, work over the upper threshold (if it exists).
	     */

	    ses_read_i(&config->sessionroot, path, PLAUNCH_LIMIT_ENABLE, 0, &limit);

	    if (limit) {
		int enable, threshold, active;

		/*
		 * work over the upper threshold
		 */

		ses_read_i(&config->sessionroot, path, PLAUNCH_LIMIT_UPPER_ENABLE, 
		    0, &enable);

		if (enable) {
		    ses_read_i(&config->sessionroot, path, PLAUNCH_LIMIT_UPPER_THRESHOLD,
			0, &threshold);

		    active = enum_process_records(process_records, nprocesses, path);

		    if (active >= threshold)
			work_over_actions(config, path, LIMIT_UPPER_STRINGS);
		    else
			pwin = launch_putty(wParam - HOTKEY_ACTION_LAUNCH, path);
		};
	    } else
		pwin = launch_putty(wParam - HOTKEY_ACTION_LAUNCH, path);
	};
	return FALSE;
    case WM_HOTKEY:
	{
	    int i;

	    for (i = config->nhotkeys - 1; i >= 0; i--) {
		if (config->hotkeys[i].hotkey == lParam) {
		    switch (config->hotkeys[i].action) {
		    case HOTKEY_ACTION_MESSAGE:
			SendMessage(config->hwnd_mainwindow,
				    (UINT) config->hotkeys[i].destination,
				    0, 0);
			return FALSE;
		    case HOTKEY_ACTION_LAUNCH:
		    case HOTKEY_ACTION_EDIT:
			SendMessage(hwnd, WM_LAUNCHPUTTY,
				    (WPARAM) config->hotkeys[i].action,
				    (LPARAM) config->hotkeys[i].
				    destination);
			return FALSE;
		    };
		};
	    };
	};
	return FALSE;
    case WM_HOTKEYCHANGE:
	{
	    UnregisterHotKey(hwnd, wParam);
	    if (config->hotkeys[wParam].hotkey)
		RegisterHotKey(hwnd, wParam,
			       LOWORD(config->hotkeys[wParam].hotkey),
			       HIWORD(config->hotkeys[wParam].hotkey));
	};
	return FALSE;
    case WM_HIDEWINDOW:
	{
	    HWND fwin;
	    char buf[BUFSIZE];

	    fwin = GetForegroundWindow();

	    if (!fwin)
		break;

	    GetClassName(fwin, buf, BUFSIZE);

	    if (!strcmp(buf, PUTTY) ||
		!strcmp(buf, TUTTY) ||
		!strcmp(buf, PUTTYTEL) ||
		!strcmp(buf, TUTTYTEL))
		ShowWindow(fwin, SW_HIDE);
	};
	break;
    case WM_CYCLEWINDOW:
	{
	    struct windowlist wl;
	    HWND pwin;

	    memset(&wl, 0, sizeof(struct windowlist));

	    EnumWindows(CountPuTTYWindows, (LPARAM) &wl.nhandles);

	    if (!wl.nhandles)
		break;

	    wl.handles = (HWND *) malloc(wl.nhandles * sizeof(HWND));
	    memset(wl.handles, 0, wl.nhandles * sizeof(HWND));

	    EnumWindows(EnumPuTTYWindows, (LPARAM) &wl);

	    pwin = wl.handles[wl.nhandles - 1];

	    ShowWindow(pwin, SW_SHOW);
	    SetForegroundWindow(pwin);
	};
	break;
    case WM_NETWORKSTART:
	launch_autoruns("", AUTORUN_WHEN_NETWORKUP);
	break;
    case WM_LAUNCHBOX:
	do_launchbox();

	return FALSE;
    case WM_WINDOWLIST:
	do_windowlistbox();

	return FALSE;
    case WM_MEASUREITEM:
	{
	    LPMEASUREITEMSTRUCT mis;
	    HDC hdc;
	    SIZE size;
	    char *name, *visible, buf[BUFSIZE], buf2[BUFSIZE];
	    HWND pwin;
	    UINT y, y1;

	    mis = (LPMEASUREITEMSTRUCT) lParam;

	    if (mis->itemID >= IDM_RUNNING_BASE &&
		mis->itemID <= IDM_RUNNING_MAX) {
		pwin = (HWND) mis->itemData;
		GetWindowText(pwin, buf, BUFSIZE);
		visible = IsWindowVisible(pwin) ? "" : " [Hidden]";
		sprintf(buf2, "%s%s", buf, visible);
		name = buf;
	    } else
		name = lastname((char *) mis->itemData);

	    if (!name)
		return FALSE;

	    hdc = GetDC(hwnd);
	    GetTextExtentPoint32(hdc, name, strlen(name), &size);
	    ReleaseDC(hwnd, hdc);

	    mis->itemWidth = size.cx + iconx + 7 + cxmenucheck;
	    y1 = icony > (UINT) size.cy ? icony : size.cy;
	    y = cymenucheck > y1 ? cymenucheck : y1;
	    mis->itemHeight = y + 2;

	    return TRUE;
	};
	break;
    case WM_DRAWITEM:
	{
	    LPDRAWITEMSTRUCT dis;
	    HICON icon;
	    SIZE size;
	    HWND pwin;
	    char *name, *path, *visible, buf[BUFSIZE], buf2[BUFSIZE];
	    unsigned int selected = 0, x, y, idestroy;

	    dis = (LPDRAWITEMSTRUCT) lParam;

	    if (dis->itemID >= IDM_RUNNING_BASE &&
		dis->itemID <= IDM_RUNNING_MAX) {
		pwin = (HWND) dis->itemData;
		GetWindowText(pwin, buf, BUFSIZE);
		visible = IsWindowVisible(pwin) ? "" : " [Hidden]";
		sprintf(buf2, "%s%s", buf, visible);
		name = buf2;
		path = buf2;
		icon = (HICON) GetClassLong(pwin, GCL_HICONSM);
		
		if (!icon)
		    icon = (HICON) GetClassLong(pwin, GCL_HICON);

		idestroy = FALSE;
	    } else {
		path = (char *) dis->itemData;
		name = lastname(path);

		if (ses_is_folder(&config->sessionroot, path)) {
		    if (selected)
			icon = ImageList_ExtractIcon(0, config->image_list,
						    config->img_open);
		    else
			icon = ImageList_ExtractIcon(0, config->image_list,
						    config->img_closed);
		} else {
		    char sesicon[256];

		    icon = NULL;

		    if (ses_read_s(&config->sessionroot, path, SESSIONICON, 
				    "", sesicon, 255) &&
			sesicon[0]) {
			char iname[256], *comma;
			int iindex;

			iname[0] = '\0';
			iindex = 0;

			comma = strrchr(sesicon, ',');
			if (comma) {
			    comma[0] = '\0';
			    *comma++;
			    iindex = atoi(comma);
			    icon = ExtractIcon(config->hinst, sesicon, iindex);
			};
		    };

		    if (!icon)
			icon = ImageList_ExtractIcon(0, config->image_list,
						    config->img_session);
		};
		idestroy = TRUE;
	    };

	    if (!name)
		return FALSE;

	    if (dis->itemState & ODS_SELECTED) {
		SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
		SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
		selected = TRUE;
	    };

	    GetTextExtentPoint32(dis->hDC, name, strlen(name), &size);

	    x = dis->rcItem.left + config->iconx + 7;
	    y = dis->rcItem.top +
		((dis->rcItem.bottom - dis->rcItem.top - size.cy) / 2);

	    ExtTextOut(dis->hDC, x, y, ETO_OPAQUE, &dis->rcItem, name,
		       strlen(name), NULL);

	    x = dis->rcItem.left + 2;
	    y = dis->rcItem.top + 1;

	    DrawIconEx(dis->hDC, x, y, icon, iconx, icony, 0, NULL,
		       DI_NORMAL);

	    if (idestroy)
		DestroyIcon(icon);

	    if (selected) {
		SetTextColor(dis->hDC, GetSysColor(COLOR_MENUTEXT));
		SetBkColor(dis->hDC, GetSysColor(COLOR_MENU));
	    };

	    return TRUE;
	};
	break;
    case WM_COMMAND:
    case WM_SYSCOMMAND:
	switch (wParam & ~0xF) {	/* low 4 bits reserved to Windows */
	case IDM_WINDOWLIST:
	    do_windowlistbox();
	    break;
	case IDM_LAUNCHBOX:
	    do_launchbox();
	    break;
	case IDM_OPTIONSBOX:
	    do_optionsbox();
	    break;
	case IDM_CLOSE:
	    SendMessage(hwnd, WM_CLOSE, 0, 0);
	    break;
	case IDM_ABOUT:
	    do_aboutbox();
	    break;
	case IDM_HELP:
/*
            if (help_path) {
                WinHelp(main_hwnd, help_path, HELP_COMMAND,
                        (DWORD)"JI(`',`pageant.general')");
                requested_help = TRUE;
            }
*/
	    break;
	}
	break;
    case WM_DESTROY:
/*
        if (requested_help) {
            WinHelp(main_hwnd, help_path, HELP_QUIT, 0);
            requested_help = FALSE;
        }
*/
	PostQuitMessage(0);
	return FALSE;
#ifdef WINDOWS_NT351_COMPATIBLE
    case WM_QUERYOPEN:
	return FALSE;
#endif				/* WINDOWS_NT351_COMPATIBLE */
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

/*
void CALLBACK PLaunchInternetStatusCallback(HINTERNET hInternet, DWORD dwContext,
											DWORD dwInternetStatus, 
											LPVOID lpvStatusInformation,
											DWORD dwStatusInformationLength) {
	if (dwInternetStatus == INTERNET_STATUS_STATE_CHANGE &&
		*((DWORD *)lpvStatusInformation) == INTERNET_STATE_CONNECTED)
		PostMessage(config->hwnd_mainwindow, WM_NETWORKSTART, 0, 0);
};
*/

void main_message_loop(void)
{
    MSG msg;
    DWORD wait;

    while (TRUE) {
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
	    if (msg.message == WM_QUIT)
		return;
	    DispatchMessage(&msg);
	};

	wait = MsgWaitForMultipleObjects(nprocesses, process_handles,
					 FALSE, INFINITE, QS_ALLINPUT);
	if (wait == WAIT_OBJECT_0 + nprocesses)
	    continue;
	else if (wait >= WAIT_OBJECT_0 &&
		 wait < (WAIT_OBJECT_0 + nprocesses)) {
	    HANDLE hprocess;
	    struct process_record *pr;
	    int limit = FALSE;

	    /*
	     * process is dead, we need to clean up after it.
	     */
	    wait -= WAIT_OBJECT_0;
	    hprocess = process_handles[wait];
	    pr = process_records[wait];

	    /*
	     * is there an instance limit for this session?
	     * if it is, work over the lower threshold (if it exists).
	     */

	    if (pr && pr->path)
		ses_read_i(&config->sessionroot, pr->path, PLAUNCH_LIMIT_ENABLE, 0, &limit);

	    if (limit) {
		int enable, threshold, active;

		/*
		 * work over the lower threshold
		 */

		ses_read_i(&config->sessionroot, pr->path, PLAUNCH_LIMIT_LOWER_ENABLE,
		    0, &enable);

		if (enable) {
		    ses_read_i(&config->sessionroot, pr->path, PLAUNCH_LIMIT_LOWER_THRESHOLD,
			0, &threshold);

		    active = enum_process_records(process_records, nprocesses, pr->path) - 1;

		    if (active < threshold)
			work_over_actions(config, pr->path, LIMIT_LOWER_STRINGS);
		};
	    };

	    item_remove((void *) &process_handles, &nprocesses, hprocess);
	    nprocesses++;
	    item_remove((void *) &process_records, &nprocesses, pr);
	    CloseHandle(hprocess);
	    if (pr->path)
		free(pr->path);
	    free(pr);
	};
    };
};

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
    WNDCLASS wndclass;
    OSVERSIONINFO osv;
    HIMAGELIST small_list, large_list;
//      HINTERNET hinternet = NULL;

#ifndef _DEBUG
    {
	HWND plaunch;

	plaunch = FindWindow(APPNAME, NULL);
	if (plaunch) {
	    PostMessage(plaunch, WM_COMMAND, IDM_LAUNCHBOX, 0);
	    return 0;
	};
    };
#endif /* _DEBUG */

    InitCommonControls();

    /*
     * create struct config and check windows version
     */
    config = (struct _config *) malloc(sizeof(struct _config));
    memset(config, 0, sizeof(struct _config));

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osv);

    config->version_major = osv.dwMajorVersion;
    config->version_minor = osv.dwMinorVersion;

#ifdef WINDOWS_NT351_COMPATIBLE
    config->have_shell = (config->version_major >= 4);
#endif /* WINDOWS_NT351_COMPATIBLE */

    config->hinst = inst;

    config->main_icon = LoadIcon(inst, MAKEINTRESOURCE(IDI_MAINICON));

    /*
     * initialize non-default session root
     */
    {
	char errmsg[BUFSIZE];

	if (!ses_init_session_root(&config->sessionroot, cmdline, errmsg, BUFSIZE)) {
	    MessageBox(NULL, errmsg, "PuTTY Launcher Error", MB_OK | MB_ICONERROR);
	    return 0;
	};
    };

    /*
     * set up the config
     */
    read_config(config);

    /*
     * Get system image list handler, then create my own image list
     * and copy needed images from system image list to my own.
     * I need to add one image to the list and I prefer not to mess
     * with system's.
     */
    {
	HICON icon;
	HMODULE shell32 = 0;

	if (
#ifdef WINDOWS_NT351_COMPATIBLE
	       config->have_shell &&
#endif /* WINDOWS_NT351_COMPATIBLE */
	       GetSystemImageLists(&shell32, &large_list, &small_list)) {
	    ImageList_GetIconSize(small_list, &config->iconx,
				  &config->icony);
	    config->image_list =
		ImageList_Create(config->iconx, config->icony,
				 ILC_MASK |
				 (COMPAT_WINDOWSXP
				  (config->version_major,
				   config->
				   version_minor) ? ILC_COLOR32 :
				  ILC_COLOR8), 3, 0);

	    icon = ImageList_ExtractIcon(0, small_list, IMG_FOLDER_CLOSED);
	    config->img_closed =
		ImageList_AddIcon(config->image_list, icon);
	    DestroyIcon(icon);

	    icon = ImageList_ExtractIcon(0, small_list, IMG_FOLDER_OPEN);
	    config->img_open = ImageList_AddIcon(config->image_list, icon);
	    DestroyIcon(icon);

	    config->img_session =
		ImageList_AddIcon(config->image_list, config->main_icon);

	    FreeSystemImageLists(shell32);

	}
#ifdef WINDOWS_NT351_COMPATIBLE
	else {
	    config->image_list =
		ImageList_Create(config->iconx, config->icony, ILC_COLOR8 | ILC_MASK, 3, 3);

	    icon = config->main_icon;
	    config->img_closed =
		config->img_open =
		config->img_session =
		ImageList_AddIcon(config->image_list, icon);
//                      DeleteObject(icon);
	};
#endif /* WINDOWS_NT351_COMPATIBLE */
    }

    if (!prev) {
	wndclass.style = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = inst;
	wndclass.hIcon = config->main_icon;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = APPNAME;

	RegisterClass(&wndclass);
    }

    config->hwnd_mainwindow = CreateWindow(APPNAME, WINDOWTITLE,
					   WS_OVERLAPPED | WS_CAPTION |
					   WS_SYSMENU | WS_ICONIC,
					   CW_USEDEFAULT, CW_USEDEFAULT, 0,
					   0, NULL, NULL, inst, NULL);
//      SetForegroundWindow(config->hwnd_mainwindow);

    /* Set up a system tray icon */
#ifdef WINDOWS_NT351_COMPATIBLE
    if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
	add_tray_icon(config->hwnd_mainwindow);

    /* Create popup menu */
#ifdef WINDOWS_NT351_COMPATIBLE
    if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
	config->systray_menu = CreatePopupMenu();
#ifdef WINDOWS_NT351_COMPATIBLE
    else {
	HMENU system_menu;

	system_menu = GetSystemMenu(config->hwnd_mainwindow, FALSE);
	InsertMenu(system_menu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
	config->systray_menu = CreatePopupMenu();
	InsertMenu(system_menu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING,
		   (UINT) config->systray_menu, WINDOWTITLE "...");
	DrawMenuBar(config->hwnd_mainwindow);
    };
#endif /* WINDOWS_NT351_COMPATIBLE */

#ifdef WINDOWS_NT351_COMPATIBLE
    if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
	ShowWindow(config->hwnd_mainwindow, SW_HIDE);
#ifdef WINDOWS_NT351_COMPATIBLE
    else
	ShowWindow(config->hwnd_mainwindow, SW_SHOWMINIMIZED);
#endif /* WINDOWS_NT351_COMPATIBLE */

    /*
     * register all hot keys.
     */
    {
	int i;

	for (i = config->nhotkeys - 1; i >= 0; i--) {
	    if (!RegisterHotKey(config->hwnd_mainwindow,
				i,
				LOWORD(config->hotkeys[i].hotkey),
				HIWORD(config->hotkeys[i].hotkey))) {
		char err[BUFSIZE], key[32], *action, *destination;
		key_name(LOWORD(config->hotkeys[i].hotkey),
			 HIWORD(config->hotkeys[i].hotkey), key, 32);
		switch (config->hotkeys[i].action) {
		case HOTKEY_ACTION_MESSAGE:
		    action = "bringing up";
		    switch ((UINT) config->hotkeys[i].destination) {
		    case WM_LAUNCHBOX:
			destination = "Launch Box";
			break;
		    case WM_WINDOWLIST:
			destination = "Window List";
			break;
		    case WM_HIDEWINDOW:
			action = "hiding";
			destination = "active window";
			break;
		    case WM_CYCLEWINDOW:
			action = "cycling through";
			destination = "session windows";
			break;
		    };
		    break;
		case HOTKEY_ACTION_LAUNCH:
		    action = "launching";
		    destination = config->hotkeys[i].destination;
		    break;
		case HOTKEY_ACTION_EDIT:
		    action = "editing";
		    destination = config->hotkeys[i].destination;
		    break;
		};
		sprintf(err, "Cannot register '%s' hotkey for %s \"%s\"!",
			key, action, destination);
		MessageBox(config->hwnd_mainwindow, err, "Error",
			   MB_OK | MB_ICONERROR);
	    };
	};
    };
    
    /*
     * find all previously started putty sessions and fill 'em into process
     * records table.
     */
    {
	find_existing_processes();
    };

    /*
     * work over all startup autorun session.
     */
    launch_autoruns("", AUTORUN_WHEN_START);

    /*
     * try to discover if a network connection is available, and if it is,
     * launch all network-start autorun sessions.
     */
#ifdef WINDOWS_NT351_COMPATIBLE
    if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
    {
//              DWORD flags;

/*
		if (InternetGetConnectedState(&flags, 0))
			launch_autoruns("", AUTORUN_WHEN_NETWORKSTART);
		else {
			if (hinternet = InternetOpen(APPNAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL,
				INTERNET_FLAG_ASYNC)) {
				InternetSetStatusCallback(hinternet, PLaunchInternetStatusCallback);
			};
		};
*/
    };

    main_message_loop();

#ifdef WINDOWS_NT351_COMPATIBLE
    if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
//              InternetCloseHandle(hinternet);

	/*
	 * work over all quit auto-run session.
	 */
	launch_autoruns("", AUTORUN_WHEN_STOP);

    /*
     * show up all hidden putty windows if applicable.
     */
    if (config->options & OPTION_SHOWONQUIT) {
	unsigned int i;
	struct process_record *pr;

	for (i = 0; i < nprocesses; i++) {
	    pr = process_records[i];
	    if (pr && pr->window && !IsWindowVisible(pr->window))
		ShowWindow(pr->window, SW_SHOW);
	};
    };

    /*
     * ...and, by the way, it would be rude to leave opened handles
     * to hang all over the system.
     */
    free_process_records();

    /*
     * clean up hot keys.
     */
    {
	int i;

	for (i = config->nhotkeys - 1; i >= 0; i--) {
	    UnregisterHotKey(config->hwnd_mainwindow, i);
	    if (config->hotkeys[i].action != HOTKEY_ACTION_MESSAGE &&
		config->hotkeys[i].destination)
		free(config->hotkeys[i].destination);
	};
    }

    /* Clean up the system tray icon */
#ifdef WINDOWS_NT351_COMPATIBLE
    if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
    {
	NOTIFYICONDATA tnid;

	tnid.cbSize = sizeof(NOTIFYICONDATA);
	tnid.hWnd = config->hwnd_mainwindow;
	tnid.uID = 1;

	Shell_NotifyIcon(NIM_DELETE, &tnid);
    }

    DestroyMenu(config->systray_menu);

    ses_finish_session_root(&config->sessionroot);

    /*
     * clean up the config
     */
    {
	if (config->image_list)
	    ImageList_Destroy(config->image_list);

	if (config->main_icon)
	    DestroyIcon(config->main_icon);

	free(config);
    }

    /*
     * clean up image lists
     */
    {
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT) {
	    ImageList_Destroy(large_list);
	    ImageList_Destroy(small_list);
	};
    };

    return 0;
};

void __cdecl main(void)
{
};

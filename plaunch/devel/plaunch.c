/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Implementation file.
 */

#include "plaunch.h"
#include "entry.h"
#include "dlgtmpl.h"
#include "misc.h"
#include "registry.h"
#include "hotkey.h"

#define IMG_FOLDER_OPEN	    4
#define IMG_FOLDER_CLOSED   3

/*
 * Main window (hidden): window function.
 */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
				WPARAM wParam, LPARAM lParam)
{
	int ret;
	static int menuinprogress;
    static UINT msgTaskbarCreated = 0;

	switch (message) {
	case WM_CREATE:
		msgTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
		ImageList_GetIconSize(config->image_list, 
					&config->iconx, 
					&config->icony);
		break;
	default:
		if (message==msgTaskbarCreated) {
			/*
			 * Explorer has been restarted, so the tray icon will
			 * have been lost.
			 */
			AddTrayIcon(hwnd);
		}
		break;
#ifdef WINDOWS_NT351_COMPATIBLE
	case WM_INITMENUPOPUP:
		if (!config->have_shell && (HMENU)wParam == config->systray_menu) {
			config->systray_menu = menu_refresh(config->systray_menu, "");
		};
		break;
#endif /* WINDOWS_NT351_COMPATIBLE */
	case WM_SYSTRAY:
		if (lParam == WM_RBUTTONUP) {
			POINT cursorpos;
			GetCursorPos(&cursorpos);
			SendMessage(hwnd, WM_SYSTRAY2, cursorpos.x, cursorpos.y);
		} else if (lParam == WM_LBUTTONDBLCLK) {
			SendMessage(hwnd, WM_COMMAND, IDM_LAUNCHBOX, 0);
		}
		break;
	case WM_SYSTRAY2:
		if (!menuinprogress) {
//			struct session *root;

			menuinprogress = 1;
//			root = session_get_root();

			config->systray_menu = menu_refresh(config->systray_menu, "");

			SetForegroundWindow(hwnd);
			ret = TrackPopupMenu(config->systray_menu,
								 TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
								 TPM_RIGHTBUTTON | TPM_RETURNCMD,
								 wParam, lParam, 0, hwnd, NULL);
			PostMessage(hwnd, WM_NULL, 0, 0);

			if (ret > IDM_SESSION_BASE && ret < IDM_SESSION_MAX) {
				MENUITEMINFO mii;

				memset(&mii, 0, sizeof(MENUITEMINFO));
				mii.cbSize = sizeof(MENUITEMINFO);
				mii.fMask = MIIM_DATA;
				if (GetMenuItemInfo(config->systray_menu, ret, FALSE, &mii) &&
					mii.dwItemData != 0) {
					char *s, *buf;

					s = (char *)mii.dwItemData;
					buf = malloc(strlen(s) + 9);
					sprintf(buf, "-load \"%s\"", s);

					ShellExecute(hwnd, "open", config->putty_path,
								 buf, _T(""), SW_SHOWNORMAL);

					free(buf);
				};
			} else
				SendMessage(hwnd, WM_COMMAND, (WPARAM)ret, 0);

			menuinprogress = 0;
		}
		break;
	case WM_HOTKEY:
		{
			int i;

			for (i = 0; i < config->nhotkeys; i++) {
				if (config->hotkeys[i].hotkey == lParam) {
					switch (config->hotkeys[i].action) {
					case HOTKEY_ACTION_MESSAGE:
						SendMessage(config->hwnd_mainwindow, 
							(UINT)config->hotkeys[i].destination, 0, 0);
						break;
					case HOTKEY_ACTION_LAUNCH:
					case HOTKEY_ACTION_EDIT:
						{
							char *buf;

							buf = (char *)malloc(BUFSIZE);
							sprintf(buf, 
								config->hotkeys[i].action == HOTKEY_ACTION_LAUNCH ?
								"-load \"%s\"" :
								"-edit \"%s\"",
								config->hotkeys[i].destination);
							ShellExecute(hwnd, "open", config->putty_path,
										 buf, _T(""), SW_SHOWNORMAL);
							free(buf);
						};
						break;
					};
				};
			};
		};
		break;
	case WM_HOTKEYCHANGE:
		{
			UnregisterHotKey(hwnd, wParam);
			if (config->hotkeys[wParam].hotkey)
				RegisterHotKey(hwnd, wParam, 
					LOWORD(config->hotkeys[wParam].hotkey),
					HIWORD(config->hotkeys[wParam].hotkey));
		};
		break;
	case WM_LAUNCHBOX:
		do_launchbox();

		return TRUE;
	case WM_WINDOWLIST:
		do_windowlistbox();

		return TRUE;
	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT mis;
			HDC hdc;
			SIZE size;
			char *name;

			mis = (LPMEASUREITEMSTRUCT)lParam;
			name = lastname((char *)mis->itemData);

			if (!name)
				return FALSE;

			hdc = GetDC(hwnd);
			GetTextExtentPoint32(hdc, name, strlen(name), &size);
			mis->itemWidth = size.cx + config->iconx + 7 + GetSystemMetrics(SM_CXMENUCHECK);
			if (config->icony > size.cy)
				mis->itemHeight = config->icony + 2;
			else
				mis->itemHeight = size.cy + 2;
			ReleaseDC(hwnd, hdc);

			return TRUE;
		};
		break;
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT dis;
			HICON icon;
			SIZE size;
			char *name, *path;
			int selected = 0, x, y;

			dis = (LPDRAWITEMSTRUCT)lParam;
			path = (char *)dis->itemData;
			name = lastname(path);

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
				(int)((dis->rcItem.bottom - dis->rcItem.top - size.cy) / 2);

			ExtTextOut(dis->hDC, x, y, ETO_OPAQUE, &dis->rcItem, name, strlen(name), NULL);
			
			x = dis->rcItem.left + 2;
			y = dis->rcItem.top + 1;

			if (is_folder(path)) {
				if (selected)
					icon = ImageList_ExtractIcon(0, config->image_list, config->img_open);
				else
					icon = ImageList_ExtractIcon(0, config->image_list, config->img_closed);
			} else
				icon = ImageList_ExtractIcon(0, config->image_list, config->img_session);
			DrawIconEx(dis->hDC, x, y, icon, config->iconx, config->icony, 0, NULL, DI_NORMAL);
			DeleteObject(icon);

			if (selected) {
				SetTextColor(dis->hDC, GetSysColor(COLOR_MENUTEXT));
				SetBkColor(dis->hDC, GetSysColor(COLOR_MENU));
			};

			return TRUE;
		};
		break;
	case WM_COMMAND:
	case WM_SYSCOMMAND:
		switch (wParam & ~0xF) {       /* low 4 bits reserved to Windows */
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
	case WM_QUERYOPEN:
		return FALSE;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
    WNDCLASS wndclass;
    MSG msg;

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
	{
		OSVERSIONINFO osv;

		config = (struct _config *)malloc(sizeof(struct _config));
		memset(config, 0, sizeof(struct _config));

		osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osv);

		config->version_major = osv.dwMajorVersion;
		config->version_minor = osv.dwMinorVersion;

#ifdef WINDOWS_NT351_COMPATIBLE
		config->have_shell = (config->version_major >= 4);
#endif /* WINDOWS_NT351_COMPATIBLE */

		config->hinst = inst;
	}

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
		HIMAGELIST small_list, large_list;
		HICON icon;
		HMODULE shell32 = 0;
		int cx = 0, cy = 0;

		if (
#ifdef WINDOWS_NT351_COMPATIBLE
			config->have_shell &&
#endif /* WINDOWS_NT351_COMPATIBLE */
			GetSystemImageLists(shell32, &large_list, &small_list)) {
			ImageList_GetIconSize(small_list, &cx, &cy);
			config->image_list = ImageList_Create(cx, cy, 
				ILC_MASK | (config->version_major >= 5 ? ILC_COLOR32 : ILC_COLOR8),
				3, 0);

			icon = ImageList_ExtractIcon(0, small_list, IMG_FOLDER_CLOSED);
			config->img_closed = ImageList_AddIcon(config->image_list, icon);
			DestroyIcon(icon);

			icon = ImageList_ExtractIcon(0, small_list, IMG_FOLDER_OPEN);
			config->img_open = ImageList_AddIcon(config->image_list, icon);
			DestroyIcon(icon);

			icon = LoadIcon(config->hinst, MAKEINTRESOURCE(IDI_MAINICON));
			config->img_session = ImageList_AddIcon(config->image_list, icon);
			DeleteObject(icon);

			FreeSystemImageLists(shell32);
		} 
#ifdef WINDOWS_NT351_COMPATIBLE
		else {
			config->image_list = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 3, 3);

			icon = LoadIcon(config->hinst, MAKEINTRESOURCE(IDI_MAINICON));
			config->img_closed = 
			config->img_open = 
			config->img_session = ImageList_AddIcon(config->image_list, icon); 
			DeleteObject(icon);
		};
#endif /* WINDOWS_NT351_COMPATIBLE */
	}

    if (!prev) {
		wndclass.style = 0;
		wndclass.lpfnWndProc = WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = inst;
		wndclass.hIcon = LoadIcon(inst, MAKEINTRESOURCE(IDI_MAINICON));
		wndclass.hCursor = LoadCursor(NULL, IDC_IBEAM);
		wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = APPNAME;

		RegisterClass(&wndclass);
    }

    config->hwnd_mainwindow = CreateWindow(APPNAME, WINDOWTITLE,
						WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_ICONIC,
						CW_USEDEFAULT, CW_USEDEFAULT,
						0, 0, NULL, NULL, inst, NULL);
	SetForegroundWindow(config->hwnd_mainwindow);

    /* Set up a system tray icon */
#ifdef WINDOWS_NT351_COMPATIBLE
	if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
		AddTrayIcon(config->hwnd_mainwindow);

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
			(UINT)config->systray_menu, WINDOWTITLE "...");
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

		for (i = 0; i < config->nhotkeys; i++) {
			if (!RegisterHotKey(config->hwnd_mainwindow, 
								i, 
								LOWORD(config->hotkeys[i].hotkey), 
								HIWORD(config->hotkeys[i].hotkey))) {
				char *err, *key, *action, *destination;
				err = malloc(BUFSIZE);
				key = key_name(LOWORD(config->hotkeys[i].hotkey),
					HIWORD(config->hotkeys[i].hotkey));
				switch (config->hotkeys[i].action) {
				case HOTKEY_ACTION_MESSAGE:
					action = "bringing up";
					switch ((int)config->hotkeys[i].destination) {
					case WM_LAUNCHBOX:
						destination = "Launch Box";
						break;
					case WM_WINDOWLIST:
						destination = "Window List";
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
				MessageBox(config->hwnd_mainwindow, err, "Error", MB_OK | MB_ICONERROR);
				free(err);
			};
		};
	};

    while (GetMessage(&msg, NULL, 0, 0)) {
//		TranslateMessage(&msg);
		DispatchMessage(&msg);
    };

	/*
	 * clean up hot keys.
	 */
	{
		int i;

		for (i = 0; i < config->nhotkeys; i++) {
			UnregisterHotKey(config->hwnd_mainwindow, i);
			if (i > 1 && config->hotkeys[i].destination)
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

	/*
	 * clean up the config
	 */
	{
		if (config->image_list)
			ImageList_Destroy(config->image_list);

		if (config->putty_path)
			free(config->putty_path);

		free(config);
	}

    return 0;
};

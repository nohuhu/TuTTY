#include "plaunch.h"
#include "dlgtmpl.h"
#include "entry.h"
#include "misc.h"

#define	ID_WINDOWLIST_STATIC		100
#define	ID_WINDOWLIST_LISTBOX		101
#define	ID_WINDOWLIST_HIDEBUTTON	102
#define	ID_WINDOWLIST_SHOWBUTTON	103
#define	ID_WINDOWLIST_KILLBUTTON	104

#define IDM_CTXW_SHOW				0x0110
#define IDM_CTXW_HIDE				0x0111
#define IDM_CTXW_KILL				0x0112

/*
 * Window List Box: helper enumeration function.
 * Returns number of PuTTY or PuTTYtel windows in lParam,
 * which is indeed an integer pointer.
 */
BOOL CALLBACK CountPuTTYWindows(HWND hwnd, LPARAM lParam) {
	char *classname;
	int *count = (int *)lParam;

	classname = malloc(BUFSIZE);
	memset(classname, 0, BUFSIZE);

	if (GetClassName(hwnd, classname, BUFSIZE) &&
		(strcmp(classname, PUTTY) == 0 ||
		 strcmp(classname, PUTTYTEL) == 0))
		 (*count)++;

	free(classname);

	return TRUE;
};

static int nhandles;
static HWND hwnd_windowlistbox = NULL;

/*
 * Window List Box: helper enumeration function.
 * Returns an array of PuTTY or PuTTYtel window handles in lParam.
 */
BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam) {
	char *classname;
	HWND *handles = (HWND *)lParam;

	classname = malloc(BUFSIZE);
	memset(classname, 0, BUFSIZE);

	if (GetClassName(hwnd, classname, BUFSIZE) &&
		(strcmp(classname, PUTTY) == 0 ||
		 strcmp(classname, PUTTYTEL) == 0)) {
		handles[nhandles] = hwnd;
		nhandles++;
	};

	free(classname);

	return TRUE;
};

/*
 * Window List Box: helper listbox updation function.
 * It does refresh of listbox content based on the window
 * list but doesn't update the window list itself.
 */
static void update_listbox(HWND listbox, int nwindows, HWND *handles, char **windowlist) {
	int i, j;
	char *buf;

	SendMessage(listbox, (UINT)LB_RESETCONTENT, 0, 0);

	buf = malloc(BUFSIZE);

	for (i = 0; i < nwindows; i++) {
		memset(buf, 0, BUFSIZE);

		if (j = GetWindowText(handles[i], buf, BUFSIZE)) {
			char *title =
				malloc(j + 5);

			if (IsWindowVisible(handles[i]))
				sprintf(title, "[v] %s", buf);
			else
				sprintf(title, "[h] %s", buf);

			if (windowlist[i])
				free(windowlist[i]);

			windowlist[i] = title;

			SendMessage(listbox, (UINT)LB_ADDSTRING, 0, (LPARAM)title);
		} else {
			windowlist[i] = NULL;
		};
	};

	free(buf);
};

/*
 * Window List Box: helper listbox refreshing function.
 * Updates contents of the window list by enumerating PuTTY
 * and PuTTYtel windows then updates listbox content.
 */
static void refresh_listbox(HWND listbox, int *nwindows, HWND **handles, char ***windowlist) {
	int i;

	if (*windowlist && *nwindows > 0) {
		for (i = 0; i < *nwindows; i++)
			free((*windowlist)[i]);
		free(*windowlist);
		*windowlist = NULL;
	};
	if (*handles && *nwindows > 0) {
		free(*handles);
		*handles = NULL;
	};

	*nwindows = 0;
	nhandles = 0;

	SendMessage(listbox, (UINT)LB_RESETCONTENT, 0, 0);

	EnumWindows(CountPuTTYWindows, (LPARAM)nwindows);

	if (*nwindows == 0) {
		*windowlist = NULL;
		*handles = NULL;
	} else {
		char *buf;
		int len;

		*handles = (HWND *)malloc(sizeof(HWND) * (*nwindows));
		memset(*handles, 0, sizeof(HWND) * (*nwindows));
		*windowlist = (char **)malloc(sizeof(char *) * (*nwindows));
		memset(*windowlist, 0, sizeof(char *) * (*nwindows));

		EnumWindows(EnumPuTTYWindows, (LPARAM)(*handles));

		buf = malloc(BUFSIZE);

		for (i = 0; i < *nwindows; i++) {
			memset(buf, 0, BUFSIZE);

			if (len = GetWindowText((*handles)[i], buf, BUFSIZE)) {
				char *title =
					malloc(len + 5);

				if (IsWindowVisible((*handles)[i]))
					sprintf(title, "[v] %s", buf);
				else
					sprintf(title, "[h] %s", buf);

				(*windowlist)[i] = title;

				SendMessage(listbox, (UINT)LB_ADDSTRING, 0, (LPARAM)title);
			} else {
				(*windowlist)[i] = NULL;
			};
		};

		free(buf);
	};
};

/*
 * Window List Box: dialog function.
 */
static int CALLBACK WindowListBoxProc(HWND hwnd, UINT msg,
									  WPARAM wParam, LPARAM lParam) {
	int i = 0;
	static char **windowlist;
	static int nwindows;
	static HWND *handles;
	static HWND listbox;
	static HWND hidebutton, showbutton, killbutton;
	static HMENU context_menu = NULL;

    switch (msg) {
    case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
		if (!config->have_shell)
			center_window(hwnd);
#endif /* WINDOWS_NT351_COMPATIBLE */

		hwnd_windowlistbox = hwnd;
		SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
					(LPARAM) LoadIcon(config->hinst, MAKEINTRESOURCE(IDI_MAINICON)));

		listbox = GetDlgItem(hwnd, ID_WINDOWLIST_LISTBOX);
		hidebutton = GetDlgItem(hwnd, ID_WINDOWLIST_HIDEBUTTON);
		showbutton = GetDlgItem(hwnd, ID_WINDOWLIST_SHOWBUTTON);
		killbutton = GetDlgItem(hwnd, ID_WINDOWLIST_KILLBUTTON);

		handles = NULL;
		windowlist = NULL;

		refresh_listbox(listbox, &nwindows, &handles, &windowlist);

		SendMessage(listbox, (UINT)LB_SETCURSEL, 0, 0);

		if (!context_menu) {
			context_menu = CreatePopupMenu();
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, ID_WINDOWLIST_HIDEBUTTON, "&Hide");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, ID_WINDOWLIST_SHOWBUTTON, "&Show");
			AppendMenu(context_menu, MF_SEPARATOR, 0, "");
			AppendMenu(context_menu, MF_STRING, ID_WINDOWLIST_KILLBUTTON, "&Kill");
		};

		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(listbox);
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, LBN_SELCHANGE), (LPARAM)listbox);

		return FALSE;
	case WM_COMMAND:
		if (HIWORD(wParam) == LBN_DBLCLK &&
			(HWND)lParam == listbox) {
			SendMessage(hwnd, WM_COMMAND, (WPARAM)IDOK, 0);

			return FALSE;
		} else if (HIWORD(wParam) == LBN_SELCHANGE &&
			(HWND)lParam == listbox) {
			int item = SendMessage(listbox, LB_GETCURSEL, 0, 0);

			if (item == LB_ERR) {
				EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
				EnableWindow(hidebutton, FALSE);
				EnableWindow(showbutton, FALSE);
				EnableWindow(killbutton, FALSE);
				EnableMenuItem(context_menu, ID_WINDOWLIST_HIDEBUTTON, MF_GRAYED);
				EnableMenuItem(context_menu, ID_WINDOWLIST_SHOWBUTTON, MF_GRAYED);
				EnableMenuItem(context_menu, ID_WINDOWLIST_KILLBUTTON, MF_GRAYED);
			} else if (IsWindowVisible(handles[item])) {
				EnableWindow(hidebutton, TRUE);
				EnableWindow(showbutton, FALSE);
				EnableMenuItem(context_menu, ID_WINDOWLIST_HIDEBUTTON, MF_ENABLED);
				EnableMenuItem(context_menu, ID_WINDOWLIST_SHOWBUTTON, MF_GRAYED);
			} else {
				EnableWindow(hidebutton, FALSE);
				EnableWindow(showbutton, TRUE);
				EnableMenuItem(context_menu, ID_WINDOWLIST_HIDEBUTTON, MF_GRAYED);
				EnableMenuItem(context_menu, ID_WINDOWLIST_SHOWBUTTON, MF_ENABLED);
			};

			return FALSE;
		};
		switch (wParam) {
		case ID_WINDOWLIST_STATIC:
			SetFocus(listbox);
			break;
		case IDOK:
			if (handles) {
				i = SendMessage(listbox, (UINT)LB_GETCURSEL, 0, 0);

				if (i != LB_ERR) {
					ShowWindow(handles[i], SW_SHOWNORMAL);
					SetForegroundWindow(handles[i]);
					SetFocus(handles[i]);
				};
			};
			SendMessage(hwnd, WM_COMMAND, (WPARAM)IDCANCEL, 0);
			break;
		case ID_WINDOWLIST_HIDEBUTTON:
		case ID_WINDOWLIST_SHOWBUTTON:
			if (handles) {
				i = SendMessage(listbox, (UINT)LB_GETCURSEL, 0, 0);

				if (i != LB_ERR)
					ShowWindow(handles[i], wParam == ID_WINDOWLIST_HIDEBUTTON ?
												SW_HIDE : SW_SHOWNOACTIVATE);

				update_listbox(listbox, nwindows, handles, windowlist);
				SendMessage(listbox, (UINT)LB_SETCURSEL, i, 0);
				SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, LBN_SELCHANGE), (LPARAM)listbox);
			};
			break;
		case ID_WINDOWLIST_KILLBUTTON:
			if (handles) {
				i = SendMessage(listbox, (UINT)LB_GETCURSEL, 0, 0);

				if (i != LB_ERR) {
					SetForegroundWindow(handles[i]);
					SetActiveWindow(handles[i]);
					SendMessage(handles[i], WM_CLOSE, 0, 0);
					if (!IsWindow(handles[i])) {
						int j;

						refresh_listbox(listbox, &nwindows, &handles, &windowlist);
						j = SendMessage(listbox, (UINT)LB_GETCOUNT, 0, 0);
						if (i > (j - 1))
							i = j - 1;
						SendMessage(listbox, (UINT)LB_SETCURSEL, i, 0);
						SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, LBN_SELCHANGE),
							(LPARAM)listbox);
					};
				};
			};
			break;
		case IDCANCEL:
			if (handles) {
				free(handles);
				handles = NULL;
			};
			if (windowlist) {
				for (i = 0; i < nwindows; i++)
					free(windowlist[i]);
				free(windowlist);
				windowlist = NULL;
			};
			nwindows = 0;
			EndDialog(hwnd, 0);
			
			return FALSE;
		};
		break;
	case WM_CONTEXTMENU:
		{
			RECT rc;
			POINT pt;
			DWORD msg;
			int item, top, bottom, flags;

			pt.x = ((int)(short)LOWORD(lParam));
			pt.y = ((int)(short)HIWORD(lParam));

			item = SendMessage(listbox, LB_GETCURSEL, 0, 0);
			top = SendMessage(listbox, LB_GETTOPINDEX, 0, 0);
			GetClientRect(listbox, &rc);
			bottom = top + 
				(int)((rc.bottom - rc.top) / 
					SendMessage(listbox, LB_GETITEMHEIGHT, (WPARAM)0, 0));

			if (pt.x == -1 && pt.y == -1) {
				if (item < top || item > bottom)
					SendMessage(listbox, LB_SETTOPINDEX, (WPARAM)item, 0);
				SendMessage(listbox, LB_GETITEMRECT, (WPARAM)item, (LPARAM)&rc);
				pt.x = rc.left + config->iconx;
				if ((bottom - item) < 6)
					pt.y = rc.top;
				else
					pt.y = rc.bottom;
			} else {
				ScreenToClient(listbox, &pt);
				GetClientRect(listbox, (LPRECT)&rc);

				if (PtInRect((LPRECT)&rc, pt)) {
					item = SendMessage(listbox, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));

					if (HIWORD(item))
						return FALSE;

					if (item < top || item > bottom)
						SendMessage(listbox, LB_SETTOPINDEX, (WPARAM)item, 0);

					SendMessage(listbox, LB_SETCURSEL, item, 0);
					SendMessage(hwnd, WM_COMMAND, 
						MAKEWPARAM(0, LBN_SELCHANGE), (LPARAM)listbox);
				};
			};

			flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
			if ((bottom - item) < 6)
				flags |= TPM_BOTTOMALIGN;
			else
				flags |= TPM_TOPALIGN;

			ClientToScreen(listbox, &pt);

			if (msg = TrackPopupMenu(context_menu, flags, pt.x, pt.y, 0, hwnd, NULL))
				PostMessage(hwnd, WM_COMMAND, msg, MAKELPARAM(item, 0));

			return TRUE;
		}
	case WM_VKEYTOITEM:
		switch (LOWORD(wParam)) {
		case 'H':
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_WINDOWLIST_HIDEBUTTON, 0);
			return -2;
		case 'S':
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_WINDOWLIST_SHOWBUTTON, 0);
			return -2;
		case VK_DELETE:
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_WINDOWLIST_KILLBUTTON, 0);
			SetFocus(listbox);
			return -2;
		default:
			return -1;
		};
	default:
		return FALSE;
	};
	return FALSE;
};

/*
 * Window List Box: setup function.
 */
void do_windowlistbox(void) {
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	if (hwnd_windowlistbox) {
		SetForegroundWindow(hwnd_windowlistbox);
		return;
	};

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE * 2);
	ptr = dialogtemplate_create(tmpl, WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU |
								DS_CENTER | DS_MODALFRAME,
								0, 0, 240, 152, "PuTTY sessions", 7, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE | SS_LEFT,
								7, 3, 169, 8, "&Running PuTTY or PuTTYtel sessions:",
								ID_WINDOWLIST_STATIC);
	ptr = dialogtemplate_addlistbox(ptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | 
								WS_TABSTOP | LBS_HASSTRINGS | LBS_NOTIFY | 
								LBS_DISABLENOSCROLL | LBS_USETABSTOPS | LBS_WANTKEYBOARDINPUT,
								7, 15, 169, 132, NULL, ID_WINDOWLIST_LISTBOX);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								183, 21, 50, 14, "Switch to", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 38, 50, 14, "Cancel", IDCANCEL);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 77, 50, 14, "&Hide", ID_WINDOWLIST_HIDEBUTTON);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 93, 50, 14, "&Show", ID_WINDOWLIST_SHOWBUTTON);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 109, 50, 14, "&Kill", ID_WINDOWLIST_KILLBUTTON);

	SetForegroundWindow(config->hwnd_mainwindow);
	DialogBoxIndirect(config->hinst, (LPDLGTEMPLATE)tmpl, NULL, WindowListBoxProc);

	GlobalFree(tmpl);
	hwnd_windowlistbox = NULL;
};

#include "plaunch.h"
#include "entry.h"
#include "misc.h"
#include "resource.h"
#include "dlgtmpl.h"

#define IDM_CTXW_SHOW				0x0110
#define IDM_CTXW_HIDE				0x0111
#define IDM_CTXW_KILL				0x0112

/*
 * Window List Box: helper enumeration function.
 * Returns number of PuTTY or PuTTYtel windows in lParam,
 * which is indeed an integer pointer.
 */
BOOL CALLBACK CountPuTTYWindows(HWND hwnd, LPARAM lParam)
{
    unsigned char *classname;
    unsigned int *count = (unsigned int *) lParam;

    classname = malloc(BUFSIZE);
    memset(classname, 0, BUFSIZE);

    if (GetClassName(hwnd, classname, BUFSIZE) &&
	(!strcmp(classname, TUTTY) ||
	 !strcmp(classname, TUTTYTEL) ||
	 !strcmp(classname, PUTTY) ||
	 !strcmp(classname, PUTTYTEL)))
	(*count)++;

    free(classname);

    return TRUE;
};

static HWND hwnd_windowlistbox = NULL;

/*
 * Window List Box: helper enumeration function.
 * Returns an array of PuTTY or PuTTYtel window handles in lParam.
 */
BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam)
{
    unsigned char *classname;
    struct windowlist *wl = (struct windowlist *) lParam;

    classname = malloc(BUFSIZE);
    memset(classname, 0, BUFSIZE);

    if (GetClassName(hwnd, classname, BUFSIZE) &&
	(!strcmp(classname, TUTTY) ||
	 !strcmp(classname, TUTTYTEL) ||
	 !strcmp(classname, PUTTY) ||
	 !strcmp(classname, PUTTYTEL))) {
	wl->handles[wl->current] = hwnd;
	wl->current++;
    };

    free(classname);

    return TRUE;
};

/*
 * Window List Box: helper listbox refreshing function.
 * Updates contents of the window list by enumerating PuTTY
 * and PuTTYtel windows then updates listbox content.
 */
static void refresh_listbox(HWND listbox, struct windowlist *wl)
{
    unsigned int i;

    if (wl->handles && wl->nhandles > 0)
	free(wl->handles);

    memset(wl, 0, sizeof(struct windowlist));

    SendMessage(listbox, (UINT) LB_RESETCONTENT, 0, 0);

    EnumWindows(CountPuTTYWindows, (LPARAM) & wl->nhandles);

    if (wl->nhandles == 0) {
	wl->handles = NULL;
    } else {
	unsigned char *buf, *title, visible;
	unsigned int len;

	wl->handles = (HWND *) malloc(wl->nhandles * sizeof(HWND));
	memset(wl->handles, 0, wl->nhandles * sizeof(HWND));

	EnumWindows(EnumPuTTYWindows, (LPARAM) wl);

	buf = (char *) malloc(BUFSIZE);
	title = (char *) malloc(BUFSIZE);

	for (i = 0; i < wl->nhandles; i++) {
	    memset(buf, 0, BUFSIZE);

	    if (len = GetWindowText(wl->handles[i], buf, BUFSIZE)) {
		visible = IsWindowVisible(wl->handles[i]) ? 'v' : 'h';
		sprintf(title, "[%c] %s", visible, buf);
	    } else
		title[0] = '\0';

	    SendMessage(listbox, (UINT) LB_ADDSTRING, 0, (LPARAM) title);
	};

	free(buf);
    };
};

/*
 * Window List Box: dialog function.
 */
static int CALLBACK WindowListBoxProc(HWND hwnd, UINT msg,
				      WPARAM wParam, LPARAM lParam)
{
    unsigned int i = 0;
    static struct windowlist wl;
    static HWND listbox;
    static HWND hidebutton, showbutton, killbutton;
    static HMENU context_menu = NULL;

    switch (msg) {
    case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
	if (!config->have_shell)
	    center_window(hwnd);
#endif				/* WINDOWS_NT351_COMPATIBLE */

	hwnd_windowlistbox = hwnd;
	SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
		    (LPARAM) config->main_icon);

	listbox = GetDlgItem(hwnd, IDC_WINDOWLISTBOX_LISTBOX);
	hidebutton = GetDlgItem(hwnd, IDC_WINDOWLISTBOX_BUTTON_HIDE);
	showbutton = GetDlgItem(hwnd, IDC_WINDOWLISTBOX_BUTTON_SHOW);
	killbutton = GetDlgItem(hwnd, IDC_WINDOWLISTBOX_BUTTON_KILL);

	memset(&wl, 0, sizeof(struct windowlist));

	refresh_listbox(listbox, &wl);

	if (!context_menu) {
	    context_menu = CreatePopupMenu();
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED,
		       IDC_WINDOWLISTBOX_BUTTON_HIDE, "&Hide");
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED,
		       IDC_WINDOWLISTBOX_BUTTON_SHOW, "&Show");
	    AppendMenu(context_menu, MF_SEPARATOR, 0, "");
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED,
		       IDC_WINDOWLISTBOX_BUTTON_KILL, "&Kill");
	};

	SetForegroundWindow(hwnd);
	SetActiveWindow(hwnd);
	SetFocus(listbox);

	SendMessage(listbox, (UINT) LB_SETCURSEL, 0, 0);
	SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, LBN_SELCHANGE),
		    (LPARAM) listbox);

	return FALSE;
    case WM_COMMAND:
	if (HIWORD(wParam) == LBN_DBLCLK && (HWND) lParam == listbox) {
	    SendMessage(hwnd, WM_COMMAND, (WPARAM) IDOK, 0);

	    return FALSE;
	} else if (HIWORD(wParam) == LBN_SELCHANGE &&
		   (HWND) lParam == listbox) {
	    unsigned int show, hide, kill;
	    int item = SendMessage(listbox, LB_GETCURSEL, 0, 0);

	    if (item == LB_ERR) {
		EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
		hide = FALSE;
		show = FALSE;
		kill = FALSE;
	    } else if (IsWindowVisible(wl.handles[item])) {
		hide = TRUE;
		show = FALSE;
		kill = TRUE;
	    } else {
		hide = FALSE;
		show = TRUE;
		kill = TRUE;
	    };

	    EnableWindow(hidebutton, hide);
	    EnableWindow(showbutton, show);
	    EnableWindow(killbutton, kill);
	    EnableMenuItem(context_menu, IDC_WINDOWLISTBOX_BUTTON_HIDE,
			   hide ? MF_ENABLED : MF_GRAYED);
	    EnableMenuItem(context_menu, IDC_WINDOWLISTBOX_BUTTON_SHOW,
			   show ? MF_ENABLED : MF_GRAYED);
	    EnableMenuItem(context_menu, IDC_WINDOWLISTBOX_BUTTON_KILL,
			   kill ? MF_ENABLED : MF_GRAYED);

	    return FALSE;
	};
	switch (wParam) {
	case IDC_WINDOWLISTBOX_STATIC:
	    SetFocus(listbox);
	    break;
	case IDOK:
	    if (wl.handles) {
		int item;

		item = SendMessage(listbox, (UINT) LB_GETCURSEL, 0, 0);

		if (item != LB_ERR) {
		    ShowWindow(wl.handles[item], SW_SHOWNORMAL);
		    SetForegroundWindow(wl.handles[item]);
		    SetFocus(wl.handles[item]);
		};
	    };
	    SendMessage(hwnd, WM_COMMAND, (WPARAM) IDCANCEL, 0);
	    break;
	case IDC_WINDOWLISTBOX_BUTTON_HIDE:
	case IDC_WINDOWLISTBOX_BUTTON_SHOW:
	    if (wl.handles) {
		int item;

		item = SendMessage(listbox, (UINT) LB_GETCURSEL, 0, 0);

		if (item != LB_ERR)
		    ShowWindow(wl.handles[item],
			       wParam ==
			       IDC_WINDOWLISTBOX_BUTTON_HIDE ? SW_HIDE :
			       SW_SHOWNOACTIVATE);

		refresh_listbox(listbox, &wl);
		SendMessage(listbox, (UINT) LB_SETCURSEL, item, 0);
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, LBN_SELCHANGE),
			    (LPARAM) listbox);
	    };
	    break;
	case IDC_WINDOWLISTBOX_BUTTON_KILL:
	    if (wl.handles) {
		int item;
		item = SendMessage(listbox, (UINT) LB_GETCURSEL, 0, 0);

		if (item != LB_ERR) {
		    SetForegroundWindow(wl.handles[item]);
		    SetActiveWindow(wl.handles[item]);
		    SendMessage(wl.handles[item], WM_CLOSE, 0, 0);
		    if (!IsWindow(wl.handles[item])) {
			int j;

			refresh_listbox(listbox, &wl);
			j = SendMessage(listbox, (UINT) LB_GETCOUNT, 0, 0);
			if (item > (j - 1))
			    item = j - 1;
			SendMessage(listbox, (UINT) LB_SETCURSEL, item, 0);
			SendMessage(hwnd, WM_COMMAND,
				    MAKEWPARAM(0, LBN_SELCHANGE),
				    (LPARAM) listbox);
		    };
		};
	    };
	    break;
	case IDCANCEL:
	    if (wl.handles) {
		free(wl.handles);
		wl.handles = NULL;
		wl.nhandles = 0;
	    };
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

	    pt.x = ((int) (short) LOWORD(lParam));
	    pt.y = ((int) (short) HIWORD(lParam));

	    item = SendMessage(listbox, LB_GETCURSEL, 0, 0);
	    top = SendMessage(listbox, LB_GETTOPINDEX, 0, 0);
	    GetClientRect(listbox, &rc);
	    bottom = top +
		(int) ((rc.bottom - rc.top) /
		       SendMessage(listbox, LB_GETITEMHEIGHT, (WPARAM) 0,
				   0));

	    if (pt.x == -1 && pt.y == -1) {
		if (item < top || item > bottom)
		    SendMessage(listbox, LB_SETTOPINDEX, (WPARAM) item, 0);
		SendMessage(listbox, LB_GETITEMRECT, (WPARAM) item,
			    (LPARAM) & rc);
		pt.x = rc.left + config->iconx;
		if ((bottom - item) < 6)
		    pt.y = rc.top;
		else
		    pt.y = rc.bottom;
	    } else {
		ScreenToClient(listbox, &pt);
		GetClientRect(listbox, (LPRECT) & rc);

		if (PtInRect((LPRECT) & rc, pt)) {
		    item =
			SendMessage(listbox, LB_ITEMFROMPOINT, 0,
				    MAKELPARAM(pt.x, pt.y));

		    if (HIWORD(item))
			return FALSE;

		    if (item < top || item > bottom)
			SendMessage(listbox, LB_SETTOPINDEX, (WPARAM) item,
				    0);

		    SendMessage(listbox, LB_SETCURSEL, item, 0);
		    SendMessage(hwnd, WM_COMMAND,
				MAKEWPARAM(0, LBN_SELCHANGE),
				(LPARAM) listbox);
		};
	    };

	    flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
	    if ((bottom - item) < 6)
		flags |= TPM_BOTTOMALIGN;
	    else
		flags |= TPM_TOPALIGN;

	    ClientToScreen(listbox, &pt);

	    if (msg =
		TrackPopupMenu(context_menu, flags, pt.x, pt.y, 0, hwnd,
			       NULL))
		PostMessage(hwnd, WM_COMMAND, msg, MAKELPARAM(item, 0));

	    return TRUE;
	}
    case WM_VKEYTOITEM:
	switch (LOWORD(wParam)) {
	case 'H':
	    SendMessage(hwnd, WM_COMMAND,
			(WPARAM) IDC_WINDOWLISTBOX_BUTTON_HIDE, 0);
	    return -2;
	case 'S':
	    SendMessage(hwnd, WM_COMMAND,
			(WPARAM) IDC_WINDOWLISTBOX_BUTTON_SHOW, 0);
	    return -2;
	case VK_DELETE:
	    SendMessage(hwnd, WM_COMMAND,
			(WPARAM) IDC_WINDOWLISTBOX_BUTTON_KILL, 0);
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
void do_windowlistbox(void)
{
    if (hwnd_windowlistbox) {
	SetForegroundWindow(hwnd_windowlistbox);
	return;
    };

    DialogBox(config->hinst, MAKEINTRESOURCE(IDD_WINDOWLISTBOX), NULL,
	      WindowListBoxProc);

    hwnd_windowlistbox = NULL;
};

#include "plaunch.h"
#include <stdio.h>
#include "entry.h"
#include "misc.h"
#include "hotkey.h"
#include "registry.h"
#include "resource.h"
#include "dlgtmpl.h"
#include "resrc1.h"
#include "session.h"

#define IDM_CTXM_COPY		0x0100
#define IDM_CTXM_CUT		0x0101
#define IDM_CTXM_PASTE		0x0102
#define IDM_CTXM_DELETE		0x0103
#define IDM_CTXM_UNDO		0x0104
#define IDM_CTXM_CRTFLD		0x0105
#define IDM_CTXM_CRTSES		0x0106
#define IDM_CTXM_RENAME		0x0107
#define	IDM_CTXM_CANCEL		0x0108

#define NEWFOLDER			"New Folder"
#define NEWSESSION			"New Session"

#define	TAB_HOTKEYS			0
#define	TAB_AUTOPROCESS			1
#define	TAB_LIMITS			2
#define	TAB_LAST			3
#define	TAB_ACTIONS			3
#define	TAB_MOREACTIONS			4
#define	TAB_CHILDDIALOGS		5

/*
const char *const LIMIT_SEARCHFOR_STRINGS[] = {
    "last", "first", "second", "third", "fourth", "fifth",
    "sixth", "seventh", "eighth", "ninth", "tenth", "eleventh",
    "twelfth", "%dth"
};

const char *const LIMIT_ACTION_STRINGS[] = {
    "do nothing with", "hide", "show", "minimize", "maximize", "center",
    "kill", "murder", "run another"
};

const char *const AUTORUN_WHEN_STRINGS[] = {
    "start", "network start", "quit"
};

const char *const AUTORUN_ACTION_STRINGS[] = {
    "do nothing with", "hide", "show", "minimize", "maximize", "center"
};
*/

char *TAB_NAMES[] =
    { "Hot Keys", "Auto Processing", "Instance Limits" };

typedef struct tag_childdialog {
    UINT type;
    HWND hwnd;
    DLGPROC dlgproc;
    DLGTEMPLATE *dlgtemplate;
} CHILDDIALOG;

typedef struct tag_dlghdr {
    HWND tab;			    // tab control 
    RECT rect;			    // display rectangle for the tab control 
    HWND treeview;		    // tree view control
    HWND kidwnd;		    // current child dialog box 
    UINT kidtype;		    // current child dialog box type
    UINT level;			    // depth
    CHILDDIALOG kids[TAB_CHILDDIALOGS];
} DLGHDR;

static int CALLBACK LaunchBoxChildDialogProc0(HWND hwnd, UINT msg, 
					      WPARAM wParam, LPARAM lParam);
static int CALLBACK LaunchBoxChildDialogProc1(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);
static int CALLBACK LaunchBoxChildDialogProc2(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);
static int CALLBACK LaunchBoxChildDialogProc3(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);
static int CALLBACK LaunchBoxChildDialogProc4(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);

static DLGPROC childdialogprocedures[TAB_CHILDDIALOGS] =
    { LaunchBoxChildDialogProc0, LaunchBoxChildDialogProc1,
      LaunchBoxChildDialogProc2, LaunchBoxChildDialogProc3,
      LaunchBoxChildDialogProc4 };

/*
 * trivial case for tree-view: we don't mind whether this is a folder
 * or a session, just plain case-sensitive comparison.
 * default settings come first, of course
 */
static int treeview_sessioncmp(const char *a, const char *b)
{
    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a, "Default Settings"))
	return -1;		/* a comes first */
    if (!strcmp(b, "Default Settings"))
	return +1;		/* b comes first */
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);	/* otherwise, compare normally */
};

/*
 * Launch Box: tree view compare function.
 */

static int CALLBACK treeview_compare(LPARAM p1, LPARAM p2, LPARAM sort)
{
    char item1[BUFSIZE], item2[BUFSIZE];

    treeview_getitemname((HWND) sort, (HTREEITEM) p1, item1, BUFSIZE);
    treeview_getitemname((HWND) sort, (HTREEITEM) p2, item2, BUFSIZE);

    return treeview_sessioncmp(item1, item2);
};

/*
 * Launch Box: a helper subroutine that finds unused name for the item
 */

static unsigned int treeview_find_unused_name(HWND treeview,
					      HTREEITEM parent, char *name,
					      char *buf)
{
    TVITEM tvi;
    HTREEITEM child;
    char oname[BUFSIZE], cname[BUFSIZE];
    unsigned int i = 0;

    if (!name || name[0] == '\0')
	return FALSE;

    if (!parent || !TreeView_GetCount(treeview)) {
	strcpy(buf, name);
	return TRUE;
    };

    strcpy(oname, name);
    child = TreeView_GetChild(treeview, parent);
    while (child) {
	memset(cname, 0, BUFSIZE);
	memset(&tvi, 0, sizeof(TVITEM));
	tvi.hItem = child;
	tvi.mask = TVIF_HANDLE | TVIF_TEXT;
	tvi.pszText = cname;
	tvi.cchTextMax = BUFSIZE;

	TreeView_GetItem(treeview, &tvi);

	if (!strcmp(cname, oname)) {
	    i++;
	    sprintf(oname, "%s (%d)", name, i);

	    child = TreeView_GetChild(treeview, parent);
	} else
	    child = TreeView_GetNextSibling(treeview, child);
    };

    strcpy(buf, oname);

    return TRUE;
};

/*
 * Helper function for emptying child dialog's fields.
 */

/*
static void empty_fields(HWND hwnd, UINT type, BOOL enable) {
    HWND control;

    switch (type) {
	case TAB_HOTKEYS:
	    {
		control = GetDlgItem(hwnd, );
		SendMessage(control, WM_SETTEXT, 0, 0);
		EnableWindow(control, enable);

		control = GetDlgItem(hwnd, );
		SendMessage(control, WM_SETTEXT, 0, 0);
		EnableWindow(control, enable);

		control = GetDlgItem(hwnd, );
		SendMessage(control, WM_SETTEXT, 0, 0);
		EnableWindow(control, enable);

		control = GetDlgItem(hwnd, );
		EnableWindow(control, FALSE);

		control = GetDlgItem(hwnd, );
		EnableWindow(control, FALSE);

		control = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_KILL);
		EnableWindow(control, FALSE);
	    };
	    break;
	case TAB_AUTOPROCESS:
	    {
		CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_AUTORUN_ATSTART,
		    BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_AUTORUN_ATNETWORKUP,
		    BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_AUTORUN_ATNETWORKDOWN,
		    BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_AUTORUN_ATSTOP,
		    BST_UNCHECKED);
		control = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB1_BUTTON_AUTORUN_ATSTART);
		EnableWindow(control, FALSE);
		control = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB1_BUTTON_AUTORUN_ATNETWORKUP);
		EnableWindow(control, FALSE);
		control = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB1_BUTTON_AUTORUN_ATNETWORKDOWN);
		EnableWindow(control, FALSE);
		control = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB1_BUTTON_AUTORUN_ATSTOP);
		EnableWindow(control, FALSE);
	    };
	    break;
	case TAB_LIMITS:
	    {
	    };
	    break;
	case TAB_ACTIONS:
	    {
	    };
	    break;
	case TAB_MOREACTIONS:
	    {
	    };
	    break;
    };
};
*/

/*
 * Launch Box: child dialog window function.
 */

static int CALLBACK LaunchBoxChildDialogProc0(HWND hwnd, UINT msg, 
					      WPARAM wParam, LPARAM lParam)
{
    static HBRUSH background;
    static DLGHDR *hdr = NULL;
    static HWND treeview = NULL;
    static HWND launch_hotkey = NULL, edit_hotkey = NULL, kill_hotkey = NULL,
	launch_button = NULL, edit_button = NULL, kill_button = NULL;
    HTREEITEM item;

    switch (msg) {
    case WM_INITDIALOG:
	{
	    HWND parent;

	    /*
	     * set up DLGHDR structure pointer...
	     */

	    parent = GetParent(hwnd);
	    hdr = (DLGHDR *)GetWindowLong(parent, GWL_USERDATA);
	    treeview = hdr->treeview;

	    /*
	     * ... adjust window position...
	     */

	    SetWindowPos(hwnd, HWND_TOP, hdr->rect.left,
			 hdr->rect.top,
			 hdr->rect.right - hdr->rect.left,
			 hdr->rect.bottom - hdr->rect.top,
			 SWP_SHOWWINDOW);

	    background = GetStockObject(HOLLOW_BRUSH);

	    /*
	     * pull edit box controls into local variables and make them
	     * custom hotkey controls.
	     */

	    launch_hotkey = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_EDITBOX_HOTKEY_LAUNCH);
	    make_hotkey(launch_hotkey, 0);

	    edit_hotkey = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_EDITBOX_HOTKEY_EDIT);
	    make_hotkey(edit_hotkey, 0);

	    kill_hotkey = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_EDITBOX_HOTKEY_KILL);
	    make_hotkey(kill_hotkey, 0);

	    /*
	     * pull button controls into local variables, for faster access
	     */

	    launch_button = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_LAUNCH);
	    edit_button = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_EDIT);
	    kill_button = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_KILL);

	    /*
	     * ... and refresh item states.
	     */

	    SendMessage(hwnd, WM_REFRESHITEMS, 0, 0);
	};
	break;
    case WM_DESTROY:
	unmake_hotkey(launch_hotkey);
	unmake_hotkey(edit_hotkey);
	unmake_hotkey(kill_hotkey);
	break;
    case WM_CTLCOLORDLG:
	return (INT_PTR) background;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORLISTBOX:
	if (COMPAT_WINDOWSXP(config->version_major, config->version_minor)) {
	    SetBkMode((HDC) wParam, TRANSPARENT);
	    return (INT_PTR) wParam;
	} else
	    return FALSE;
	break;
    case WM_EMPTYITEMS:
	{
	    BOOL enable = (BOOL)wParam;

	    set_hotkey(launch_hotkey, 0);
	    EnableWindow(launch_hotkey, enable);

	    EnableWindow(launch_button, FALSE);

	    set_hotkey(edit_hotkey, 0);
	    EnableWindow(edit_hotkey, enable);

	    EnableWindow(edit_button, FALSE);

	    set_hotkey(kill_hotkey, 0);
	    EnableWindow(kill_hotkey, enable);

	    EnableWindow(kill_button, FALSE);
	};
	break;
    case WM_REFRESHITEMS:
	{
	    BOOL isfolder, enable;
	    UINT hotkey = 0;
	    char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];

	    /*
	     * first we get the current selected item in tree view
	     */

	    item = (HTREEITEM)lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		SendMessage(hwnd, WM_EMPTYITEMS, (WPARAM)FALSE, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    /*
	     * then we get its name, full path and type (session/folder)
	     */

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path);
	    isfolder = ses_is_folder(&config->sessionroot, path);

	    /*
	     * empty all controls first, in case it's a folder or a session
	     * without any hot keys set
	     */

	    if (isfolder || !strcmp(name, DEFAULTSETTINGS))
		enable = FALSE;
	    else
		enable = TRUE;

	    SendMessage(hwnd, WM_EMPTYITEMS, (WPARAM)enable, 0);

	    /*
	     * if it is a folder, return. a folder can't have any hot keys.
	     * if it is a "Default Settings" session, it cannot have any hot keys too.
	     */

	    if (!enable)
		break;

	    /*
	     * pull all hot keys from the registry and fill them in the edit
	     * boxes
	     */

	    sprintf(buf, "%s%d", HOTKEY, 0);
	    ses_read_i(&config->sessionroot, path, buf, 0, &hotkey);
	    set_hotkey(launch_hotkey, hotkey);

	    sprintf(buf, "%s%d", HOTKEY, 1);
	    ses_read_i(&config->sessionroot, path, buf, 0, &hotkey);
	    set_hotkey(edit_hotkey, hotkey);

	    sprintf(buf, "%s%d", HOTKEY, 2);
	    ses_read_i(&config->sessionroot, path, buf, 0, &hotkey);
	    set_hotkey(kill_hotkey, hotkey);
	};
	break;
    case WM_COMMAND:
	switch (HIWORD(wParam)) {
	case HK_CHANGE:
	    {
		if ((HWND)lParam == launch_hotkey)
		    EnableWindow(launch_button, TRUE);
		else if ((HWND)lParam == edit_hotkey)
		    EnableWindow(edit_button, TRUE);
		else if ((HWND)lParam == kill_hotkey)
		    EnableWindow(kill_button, TRUE);
	    };
	    break;
/*
	case CBN_SELCHANGE:
	    {
		HTREEITEM item;
		char buf[BUFSIZE], path[BUFSIZE], *valname;
		int index;

		item = TreeView_GetSelection(treeview);

		if (!item)
		    break;

		treeview_getitempath(treeview, item, path);
		reg_make_path(NULL, path, buf);

		if (LOWORD(wParam) == IDC_LAUNCHBOX_COMBOBOX_LIMIT1)
		    valname = PLAUNCH_LIMIT_SEARCHFOR;
		else if (LOWORD(wParam) == IDC_LAUNCHBOX_COMBOBOX_LIMIT2)
		    valname = PLAUNCH_LIMIT_ACTION1;
		else if (LOWORD(wParam) == IDC_LAUNCHBOX_COMBOBOX_LIMIT3)
		    valname = PLAUNCH_LIMIT_ACTION2;
		else if (LOWORD(wParam) == IDC_LAUNCHBOX_COMBOBOX_RUN1)
		    valname = PLAUNCH_AUTORUN_WHEN;
		else if (LOWORD(wParam) == IDC_LAUNCHBOX_COMBOBOX_RUN2)
		    valname = PLAUNCH_AUTORUN_ACTION;

		index = SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);
		if (index)
		    reg_write_i(buf, valname, index);
		else
		    reg_delete_v(buf, valname);

		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);
	    };
	    break;
/*
	case EN_CHANGE:
	    if (LOWORD(wParam) == IDC_LAUNCHBOX_EDITBOX_LIMIT1) {
		HTREEITEM item;
		char buf[BUFSIZE], path[BUFSIZE], str[BUFSIZE];
		int value;

		item = TreeView_GetSelection(treeview);

		if (!item)
		    break;

		treeview_getitempath(treeview, item, path);
		reg_make_path(NULL, path, buf);

		GetWindowText(limit_edit, str, BUFSIZE);
		value = small_atoi(str);
		if (value)
		    reg_write_i(buf, PLAUNCH_LIMIT_NUMBER, value);
		else
		    reg_delete_v(buf, PLAUNCH_LIMIT_NUMBER);

		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM) item);
	    };
	    break;
	case BN_CLICKED:
	    if (LOWORD(wParam) == IDC_LAUNCHBOX_CHECKBOX_LIMIT ||
		LOWORD(wParam) == IDC_LAUNCHBOX_CHECKBOX_RUN) {
		HTREEITEM item;
		char buf[BUFSIZE], path[BUFSIZE], *valname;
		int check;

		item = TreeView_GetSelection(treeview);

		if (!item)
		    break;

		treeview_getitempath(treeview, item, path);
		reg_make_path(NULL, path, buf);
		check = IsDlgButtonChecked(hwnd, LOWORD(wParam));
		valname = LOWORD(wParam) == IDC_LAUNCHBOX_CHECKBOX_LIMIT ?
		    PLAUNCH_LIMIT_ENABLE : PLAUNCH_AUTORUN_ENABLE;
		if (check == BST_CHECKED)
		    reg_write_i(buf, valname, 1);
		else
		    reg_delete_v(buf, valname);

		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM) item);
	    };
*/
	};
	switch (wParam) {
	case IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_LAUNCH:
	case IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_EDIT:
	case IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_KILL:
	    {
		char path[BUFSIZE], valname[BUFSIZE];
		unsigned int i, found, htype, hotkey;
		HWND editbox = NULL, button = NULL;

		switch (wParam) {
		case IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_LAUNCH:
		    editbox = launch_hotkey;
		    button = launch_button;
		    htype = 0;
		    break;
		case IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_EDIT:
		    editbox = edit_hotkey;
		    button = edit_button;
		    htype = 1;
		    break;
		case IDC_LAUNCHBOX_TAB0_BUTTON_HOTKEY_KILL:
		    editbox = kill_hotkey;
		    button = kill_button;
		    htype = 2;
		    break;
		};

		item = TreeView_GetSelection(treeview);
		treeview_getitempath(treeview, item, path);

		if (!SendMessage(editbox, EM_GETMODIFY, 0, 0))
		    break;

		hotkey = get_hotkey(editbox);
		sprintf(valname, "%s%d", HOTKEY, htype);
		if (hotkey)
		    ses_write_i(&config->sessionroot, path, valname, hotkey);
		else
		    ses_delete_value(&config->sessionroot, path, valname);

		found = FALSE;
		for (i = HOTKEY_LAST; i < (int)config->nhotkeys; i++) {
		    if (config->hotkeys[i].action == 
			(htype + HOTKEY_ACTION_LAUNCH) &&
			!strcmp(config->hotkeys[i].destination, path)) {
			found = TRUE;
			config->hotkeys[i].hotkey = hotkey;
			SendMessage(config->hwnd_mainwindow,
				    WM_HOTKEYCHANGE, i, 0);
			break;
		    };
		};
		if (!found) {
		    config->hotkeys[config->nhotkeys].action = 
			(htype + HOTKEY_ACTION_LAUNCH);
		    config->hotkeys[config->nhotkeys].hotkey = hotkey;
		    config->hotkeys[config->nhotkeys].destination =
			dupstr(path);
		    config->nhotkeys++;
		    SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
				config->nhotkeys - 1, 0);
		};

		EnableWindow(button, FALSE);

		return FALSE;
	    }
	};
	break;
    };

    return FALSE;
};

static int CALLBACK LaunchBoxChildDialogProc1(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    return FALSE;
};

static int CALLBACK LaunchBoxChildDialogProc2(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    return FALSE;
};

static int CALLBACK LaunchBoxChildDialogProc3(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    return FALSE;
};

static int CALLBACK LaunchBoxChildDialogProc4(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    return FALSE;
};

/*
 * Launch Box: dialog function.
 */
static int CALLBACK LaunchBoxProc(HWND hwnd, UINT msg,
				  WPARAM wParam, LPARAM lParam)
{
    static DLGHDR *dlghdr = NULL;
    static POINT bigpt = {0}, smallpt = {0};
    static HWND treeview = NULL, tabview = NULL;
    static HMENU context_menu = NULL;
    static unsigned int cut_or_copy = 0;
    static unsigned int morestate;
    static HTREEITEM editing_now = NULL;
    static HTREEITEM copying_now = NULL;
    static HTREEITEM dragging_now = NULL;
    static HIMAGELIST draglist = NULL;
//    int i, j;

    switch (msg) {
    case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
	if (!config->have_shell)
	    center_window(hwnd);
#endif				/* WINDOWS_NT351_COMPATIBLE */

	hwnd_launchbox = hwnd;
	SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
		    (LPARAM) config->main_icon);

	/*
	 * Create the tree view.
	 */
	{
	    RECT r;
	    POINT pt;
	    WPARAM font;

	    GetWindowRect(GetDlgItem
			  (hwnd, IDC_LAUNCHBOX_GROUPBOX_TREEVIEW), &r);
	    pt.x = r.left;
	    pt.y = r.top;
	    ScreenToClient(hwnd, &pt);

	    treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
				      WS_CHILD |
#ifdef WINDOWS_NT351_COMPATIBLE
				      (config->
				       have_shell ? 0 : WS_BORDER) |
#endif	/* WINDOWS_NT351_COMPATIBLE */
				      WS_VISIBLE |
				      WS_TABSTOP | TVS_HASLINES |
				      (config->
				       options & OPTION_ENABLEDRAGDROP ? 0
				       : TVS_DISABLEDRAGDROP) |
				      TVS_HASBUTTONS | TVS_LINESATROOT |
				      TVS_SHOWSELALWAYS | TVS_EDITLABELS,
				      pt.x, pt.y, r.right - r.left,
				      r.bottom - r.top, hwnd, NULL,
				      config->hinst, NULL);
	    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	    SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	    if (config->image_list)
		TreeView_SetImageList(treeview, config->image_list,
				      TVSIL_NORMAL);
	}

	ShowWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TABVIEW),
		   SW_HIDE);

	/*
	 * Create the tab view.
	 */
	{
	    RECT r;
	    POINT pt;
	    WPARAM font;
	    TCITEM item;
	    unsigned int i;

	    GetWindowRect(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TABVIEW),
			  &r);
	    pt.x = r.left;
	    pt.y = r.top;
	    ScreenToClient(hwnd, &pt);

	    tabview = CreateWindow(WC_TABCONTROL, "",
				   WS_CHILD | WS_VISIBLE | WS_TABSTOP |
				   WS_CLIPSIBLINGS |
				   TCS_SINGLELINE,
				   pt.x, pt.y,
				   r.right - r.left, r.bottom - r.top,
				   hwnd, NULL, config->hinst, NULL);
	    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	    SendMessage(tabview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	    dlghdr = (DLGHDR *)LocalAlloc(LPTR, sizeof(DLGHDR));
	    dlghdr->tab = tabview;
	    dlghdr->treeview = treeview;
	    dlghdr->rect.left = pt.x;
	    dlghdr->rect.top = pt.y;
	    dlghdr->rect.right = pt.x + (r.right - r.left);
	    dlghdr->rect.bottom = pt.y + (r.bottom - r.top);

	    TabCtrl_AdjustRect(tabview, FALSE, &dlghdr->rect);
	    TabCtrl_GetItemRect(tabview, 0, &r);
	    dlghdr->rect.top += (r.bottom - r.top) + 5;

	    SetWindowLong(hwnd, GWL_USERDATA, (LONG)dlghdr);

	    memset(&item, 0, sizeof(TCITEM));
	    item.mask = TCIF_TEXT | TCIF_IMAGE;
	    item.iImage = -1;

	    for (i = 0; i < 1 /*TAB_CHILDDIALOGS*/; i++) {
		HRSRC hrsrc;
		HGLOBAL hglobal;

		if (i < TAB_LAST) {
		    item.pszText = TAB_NAMES[i];
		    TabCtrl_InsertItem(tabview, i, &item);
		};

		hrsrc = FindResource(NULL,
			    MAKEINTRESOURCE(IDD_LAUNCHBOX_TAB0 + i),
			    RT_DIALOG);
		hglobal = LoadResource(config->hinst, hrsrc);
		dlghdr->kids[i].dlgtemplate = (DLGTEMPLATE *)LockResource(hglobal);
		dlghdr->kids[i].dlgproc = childdialogprocedures[i];
		dlghdr->kids[i].hwnd = CreateDialogIndirect(config->hinst, 
					    dlghdr->kids[i].dlgtemplate, 
					    hwnd, dlghdr->kids[i].dlgproc);
		dlghdr->kids[i].type = i;
		ShowWindow(dlghdr->kids[i].hwnd, SW_HIDE);
	    };

	    dlghdr->kidwnd = dlghdr->kids[0].hwnd;

	    {
		NMHDR nmhdr;

		memset(&nmhdr, 0, sizeof(NMHDR));
		nmhdr.code = TCN_SELCHANGE;
		nmhdr.hwndFrom = tabview;
		nmhdr.idFrom = 0;

		SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM)&nmhdr);
	    };
	};

	/*
	 * Create a popup context menu and fill it with elements.
	 */
	if (!context_menu) {
	    context_menu = CreatePopupMenu();
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_UNDO,
		       "&Undo");
	    AppendMenu(context_menu, MF_SEPARATOR, 0, "");
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_CUT,
		       "Cu&t");
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_COPY,
		       "&Copy");
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_PASTE,
		       "&Paste");
	    AppendMenu(context_menu, MF_SEPARATOR, 0, 0);
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED,
		       IDM_CTXM_DELETE, "&Delete");
	    AppendMenu(context_menu, MF_STRING | MF_GRAYED,
		       IDM_CTXM_RENAME, "&Rename");
	    AppendMenu(context_menu, MF_SEPARATOR, 0, "");
	    AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTFLD,
		       "New &folder");
	    AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTSES,
		       "New sessi&on");
	}

	SendMessage(hwnd, WM_REFRESHTV, 0, 0);

	if (config->options & OPTION_ENABLESAVECURSOR) {
	    char curpos[BUFSIZE];

	    if (reg_read_s(PLAUNCH_REGISTRY_ROOT, 
		PLAUNCH_SAVEDCURSORPOS, NULL,
		 curpos, BUFSIZE)) {
		HTREEITEM pos;

		pos = treeview_getitemfrompath(treeview, curpos);

		if (pos && pos != TVI_ROOT) {
		    TreeView_EnsureVisible(treeview, pos);
		    TreeView_SelectItem(treeview, pos);
		};
	    };
	};

	{
	    RECT r1, r2;
	    int i;

	    GetWindowRect(hwnd, &r1);
	    bigpt.x = r1.right - r1.left;
	    bigpt.y = r1.bottom - r1.top;
	    GetWindowRect(GetDlgItem(hwnd, IDC_LAUNCHBOX_STATIC_DIVIDER),
			  &r2);
	    smallpt.x = bigpt.x;
	    smallpt.y = r2.top - r1.top;
	    morestate = 1;
	    reg_read_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_SAVEMOREBUTTON, 0,
		       &i);
	    if (!i) {
		SendMessage(hwnd, WM_COMMAND, IDC_LAUNCHBOX_BUTTON_MORE,
			    0);
		center_window(hwnd);
	    };
	}

	SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);

	SetForegroundWindow(hwnd);
	SetActiveWindow(hwnd);
	SetFocus(treeview);

	return FALSE;
    case WM_DESTROY:
	{
	    int i;

	    for (i = 0; i < TAB_CHILDDIALOGS; i++) {
		DestroyWindow(dlghdr->kids[i].hwnd);
	    };

	    LocalFree(dlghdr);
	};

	return FALSE;
    case WM_REFRESHTV:
	/*
	 * Refresh the tree view contents.
	 */
	{
	    TreeView_DeleteAllItems(treeview);
	    treeview_addtree(treeview, TVI_ROOT, "");
	    TreeView_SelectSetFirstVisible(treeview,
					   TreeView_GetRoot(treeview));
	}
	break;
    case WM_REFRESHBUTTONS:
	/*
	 * Refresh the dialog's push buttons and context menu items state.
	 */
	{
	    HTREEITEM item;
	    char name[BUFSIZE];
	    BOOL rendel;
	    UINT menuflag;

	    item = (HTREEITEM)lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		rendel = FALSE;
		menuflag = MF_GRAYED;

		EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_EDIT), FALSE);
	    } else {
		memset(name, 0, BUFSIZE);

		treeview_getitemname(treeview, item, name, BUFSIZE);

		if (!name || !strcmp(name, DEFAULTSETTINGS)) {
		    rendel = FALSE;
		    menuflag = MF_GRAYED;
		} else {
		    rendel = TRUE;
		    menuflag = MF_ENABLED;
		};
	    };

	    EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_DELETE),
			 rendel);
	    EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_RENAME),
			 rendel);
	    EnableMenuItem(context_menu, IDM_CTXM_CUT, menuflag);
	    EnableMenuItem(context_menu, IDM_CTXM_COPY, menuflag);
	    EnableMenuItem(context_menu, IDM_CTXM_DELETE, menuflag);
	    EnableMenuItem(context_menu, IDM_CTXM_RENAME, menuflag);

	    if (morestate) {
		ShowWindow(tabview, SW_SHOW);
		EnableWindow(tabview, TRUE);
		SendMessage(dlghdr->kidwnd, WM_REFRESHITEMS, (WPARAM)TRUE, 0);
	    };
	};
	break;
    case WM_CONTEXTMENU:
	{
	    RECT rc;
	    POINT pt;
	    DWORD msg;
	    HTREEITEM item;

	    pt.x = ((int)(short)LOWORD(lParam));
	    pt.y = ((int)(short)HIWORD(lParam));

	    if (pt.x == -1 && pt.y == -1) {
		item = TreeView_GetSelection(treeview);
		if (!item)
		    return FALSE;
		TreeView_EnsureVisible(treeview, item);
		TreeView_GetItemRect(treeview, item, &rc, TRUE);
		pt.x = rc.left + config->iconx;
		pt.y = rc.bottom;
	    } else {
		ScreenToClient(treeview, &pt);
		GetClientRect(treeview, (LPRECT) & rc);

		if (PtInRect((LPRECT) & rc, pt)) {
		    TVHITTESTINFO hit;

		    hit.pt.x = pt.x;
		    hit.pt.y = pt.y;
		    item = TreeView_HitTest(treeview, &hit);

		    if (!item || !(hit.flags & TVHT_ONITEM))
			return FALSE;

		    TreeView_SelectItem(treeview, item);
		};
	    };

	    ClientToScreen(treeview, &pt);

	    if (msg =
		TrackPopupMenu(context_menu,
			       TPM_LEFTALIGN | TPM_TOPALIGN |
			       TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y,
			       0, hwnd, NULL))
		PostMessage(hwnd, WM_COMMAND, msg, MAKELPARAM(item, 0));
	}
	break;
    case WM_MOUSEMOVE:
	{
	    HTREEITEM target;
	    TVHITTESTINFO hit;
	    int x, y, height;

	    if (dragging_now) {
		x = LOWORD(lParam);
		y = HIWORD(lParam);

		ImageList_DragMove(x, y);

		hit.pt.x = x;
		hit.pt.y = y;

		if ((target = TreeView_HitTest(treeview, &hit))) {
		    HTREEITEM visible;
		    RECT r;

		    GetClientRect(treeview, &r);
		    height = TreeView_GetItemHeight(treeview);
//                                      ScreenToClient(treeview, &hit.pt);

		    if ((hit.pt.y >= 0) && (hit.pt.y <= height / 2))
			visible =
			    TreeView_GetPrevVisible(treeview, target);
//                                              visible = TreeView_GetNextItem(treeview, target, TVGN_PREVIOUS);
		    else if ((hit.pt.y >= r.bottom - height / 2)
			     && (hit.pt.y <= r.bottom))
			visible =
			    TreeView_GetNextVisible(treeview, target);
//                                              visible = TreeView_GetNextItem(treeview, target, TVGN_NEXT);
		    else
			visible = target;
		    if (visible)
			TreeView_EnsureVisible(treeview, target);
		    ImageList_DragShowNolock(FALSE);
//                                      TreeView_SetInsertMark(treeview, target, TRUE);
		    TreeView_SelectDropTarget(treeview, target);
//                                      TreeView_SelectItem(treeview, target);
		    ImageList_DragShowNolock(TRUE);
		};
	    };
	}
	break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
	if (dragging_now) {
	    HTREEITEM target;
	    TVHITTESTINFO hit;

//                      TreeView_SetInsertMark(treeview, NULL, TRUE);
	    ImageList_EndDrag();
	    ReleaseCapture();
	    ShowCursor(TRUE);
	    copying_now = dragging_now;
	    dragging_now = NULL;
	    ImageList_Destroy(draglist);
	    draglist = NULL;

	    hit.pt.x = LOWORD(lParam);
	    hit.pt.y = HIWORD(lParam);

	    if ((target = TreeView_HitTest(treeview, &hit))) {
		switch (msg) {
		case WM_LBUTTONUP:
		    cut_or_copy = 1;	// cut;
		    break;
		case WM_RBUTTONUP:
		    {
			HMENU menu;
			RECT r;
			int ret, height, flags;

			menu = CreatePopupMenu();
			AppendMenu(menu, MF_STRING, IDCANCEL, "Cancel");
			AppendMenu(menu, MF_SEPARATOR, 0, 0);
			AppendMenu(menu, MF_STRING, IDM_CTXM_COPY,
				   "Copy here");
			AppendMenu(menu, MF_STRING, IDM_CTXM_CUT,
				   "Move here");

			GetClientRect(treeview, &r);
			height = TreeView_GetItemHeight(treeview);

			flags =
			    TPM_RIGHTBUTTON | TPM_RETURNCMD |
			    TPM_LEFTALIGN;
			if ((hit.pt.y >= 0) && (hit.pt.y <= height))
			    flags |= TPM_TOPALIGN;
			else if ((hit.pt.y >= (r.bottom - height))
				 && (hit.pt.y <= r.bottom))
			    flags |= TPM_BOTTOMALIGN;
			ClientToScreen(treeview, &hit.pt);

			if (ret = TrackPopupMenu(menu, flags,
						 hit.pt.x, hit.pt.y, 0,
						 hwnd, NULL)) {
			    switch (ret) {
			    case IDCANCEL:
				copying_now = NULL;
				return FALSE;
			    case IDM_CTXM_COPY:
				cut_or_copy = 2;	// copy
				break;
			    case IDM_CTXM_CUT:
				cut_or_copy = 1;	// cut
				break;
			    };
			} else {
			    copying_now = NULL;
			    return FALSE;
			};
		    };
		    break;
		};
		TreeView_SelectItem(treeview, target);
		SendMessage(hwnd, WM_COMMAND, (WPARAM) IDM_CTXM_PASTE, 0);
		return FALSE;
	    };
	};
	break;
    case WM_COMMAND:
	switch (wParam) {
	case IDC_LAUNCHBOX_STATIC_TREEVIEW:
	    SetFocus(treeview);
	    break;
	case IDC_LAUNCHBOX_BUTTON_MORE:
	    {
		HWND divider;

		divider = GetDlgItem(hwnd, IDC_LAUNCHBOX_STATIC_DIVIDER);

		switch (morestate) {
		case 0:
		    {
			SetWindowPos(hwnd, 0, 0, 0, bigpt.x, bigpt.y,
				     SWP_NOMOVE | SWP_NOZORDER);
			ShowWindow(divider, SW_SHOW);
			SetWindowText(GetDlgItem
				      (hwnd, IDC_LAUNCHBOX_BUTTON_MORE),
				      "<< &Less");
		    };
		    break;
		case 1:
		    {
			SetWindowPos(hwnd, 0, 0, 0, smallpt.x, smallpt.y,
				     SWP_NOMOVE | SWP_NOZORDER);
			ShowWindow(divider, SW_HIDE);
			SetWindowText(GetDlgItem
				      (hwnd, IDC_LAUNCHBOX_BUTTON_MORE),
				      "&More >>");
		    };
		    break;
		};

		morestate = !morestate;
		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);
	    };
	    break;
	case IDOK:
	    {
		if (editing_now) {
		    TreeView_EndEditLabelNow(treeview, FALSE);
		    return FALSE;
		};

		if (config->options & OPTION_ENABLESAVECURSOR) {
		    HTREEITEM item;
		    char path[BUFSIZE];

		    item = TreeView_GetSelection(treeview);
		    treeview_getitempath(treeview, item, path);
		    reg_write_s(PLAUNCH_REGISTRY_ROOT,
				PLAUNCH_SAVEDCURSORPOS, path);
		};

	    };
	case IDC_LAUNCHBOX_BUTTON_EDIT:
	    {
		HTREEITEM item;
		char path[BUFSIZE];

		item = TreeView_GetSelection(treeview);

		if (!treeview_getitempath(treeview, item, path)) {
		    EndDialog(hwnd, 0);
		    return FALSE;
		};

		if (ses_is_folder(&config->sessionroot, path)) {
		    TreeView_Expand(treeview, item, TVE_TOGGLE);
		    return FALSE;
		};

		SendMessage(config->hwnd_mainwindow, WM_LAUNCHPUTTY,
			    (WPARAM) (wParam == IDOK ?
				      HOTKEY_ACTION_LAUNCH :
				      HOTKEY_ACTION_EDIT), (LPARAM) path);
		EndDialog(hwnd, 0);

		return FALSE;
	    };
	case IDCANCEL:
	    {
		if (editing_now) {
		    TreeView_EndEditLabelNow(treeview, TRUE);
		    return FALSE;
		};

		EndDialog(hwnd, 0);

		return FALSE;
	    };
	case IDC_LAUNCHBOX_BUTTON_RENAME:
	case IDM_CTXM_RENAME:
	    {
		HTREEITEM item;

		EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
		item = TreeView_GetSelection(treeview);
		SetFocus(treeview);
		TreeView_EditLabel(treeview, item);

		return TRUE;
	    };
	case IDC_LAUNCHBOX_BUTTON_DELETE:
	case IDM_CTXM_DELETE:
	    {
		HTREEITEM item;
		char buf[BUFSIZE], name[BUFSIZE], path[BUFSIZE], *s;

		item = TreeView_GetSelection(treeview);
		treeview_getitemname(treeview, item, name, BUFSIZE);
		treeview_getitempath(treeview, item, path);

		if (ses_is_folder(&config->sessionroot, path))
		    s = "folder and all its contents?";
		else
		    s = "session?";

		sprintf(buf,
			"Are you sure you want to delete the \"%s\" %s",
			name, s);

		if (MessageBox(hwnd, buf, "Confirmation required",
			       MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION)
		    == IDYES) {
			if (ses_delete_tree(&config->sessionroot, path))
			TreeView_DeleteItem(treeview, item);
		};

		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);
	    };
	    break;
	case IDM_CTXM_CUT:
	case IDM_CTXM_COPY:
	    {
		TVITEM item;

		copying_now = TreeView_GetSelection(treeview);

		cut_or_copy = (wParam == IDM_CTXM_CUT ? 1 :	// cut
			       2);	// copy

		if (cut_or_copy == 1) {
		    memset(&item, 0, sizeof(TVITEM));
		    item.mask = TVIF_HANDLE | TVIF_STATE;
		    item.state = item.stateMask = TVIS_CUT;
		    item.hItem = copying_now;
		    TreeView_SetItem(treeview, &item);
		};

		EnableMenuItem(context_menu, IDM_CTXM_PASTE, MF_ENABLED);
		ModifyMenu(context_menu, IDM_CTXM_UNDO,
			   MF_STRING | MF_BYCOMMAND, IDM_CTXM_CANCEL,
			   (cut_or_copy == 1 ? "Cancel Cut" : "Cancel Copy"));
		
//		SendMessage(hwnd, WM_REFRESHBUTTONS, 0,
//			    (LPARAM) item.hItem);
	    };
	    break;
	case IDM_CTXM_CANCEL:
	    {
		TVITEM item;

		if (cut_or_copy == 1) {
		    memset(&item, 0, sizeof(TVITEM));
		    item.mask = TVIF_HANDLE | TVIF_STATE;
		    item.state = 0;
		    item.stateMask = TVIS_CUT;
		    item.hItem = copying_now;
		    TreeView_SetItem(treeview, &item);
		};

		copying_now = NULL;
		cut_or_copy = 0;

		EnableMenuItem(context_menu, IDM_CTXM_PASTE, MF_GRAYED);
		ModifyMenu(context_menu, IDM_CTXM_CANCEL,
			   MF_STRING | MF_BYCOMMAND | MF_GRAYED, IDM_CTXM_UNDO,
			   "Undo");

		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM)item.hItem);
	    };
	    break;
	case IDC_LAUNCHBOX_BUTTON_NEW_FOLDER:
	case IDM_CTXM_CRTFLD:
	case IDC_LAUNCHBOX_BUTTON_NEW_SESSION:
	case IDM_CTXM_CRTSES:
	case IDM_CTXM_PASTE:
	    {
		TVSORTCB tscb;
		HTREEITEM tv_parent, tv_curitem, tv_newitem, tv_copyitem;
		char copy_name[BUFSIZE], copy_path[BUFSIZE],
		    cur_path[BUFSIZE], parent_path[BUFSIZE],
		    to_name[BUFSIZE], from_path[BUFSIZE], to_path[BUFSIZE];
		int i, isfolder, success = 0;

		tv_copyitem = copying_now;
		tv_curitem = TreeView_GetSelection(treeview);

		if (tv_curitem) {
		    tv_parent = TreeView_GetParent(treeview, tv_curitem);
		    treeview_getitempath(treeview, tv_curitem, cur_path);
		} else {
		    tv_parent = NULL;
		    cur_path[0] = '\0';
		};

		if (cur_path[0] && ses_is_folder(&config->sessionroot, cur_path)) {
		    tv_parent = tv_curitem;
		    strcpy(parent_path, cur_path);
		} else if (tv_parent) {
		    tv_parent = tv_parent;
		    treeview_getitempath(treeview, tv_parent, parent_path);
		} else {
		    tv_parent = TVI_ROOT;
		    parent_path[0] = '\0';
		};

		if (cut_or_copy) {
		    treeview_getitemname(treeview, tv_copyitem, copy_name,
					 BUFSIZE);
		    treeview_getitempath(treeview, tv_copyitem, copy_path);
		    isfolder = ses_is_folder(&config->sessionroot, copy_path);
		    ses_make_path(NULL, copy_path, from_path, BUFSIZE);
		} else {
		    isfolder = (wParam == IDC_LAUNCHBOX_BUTTON_NEW_FOLDER
				|| wParam == IDM_CTXM_CRTFLD) ? 1 : 0;
		    strcpy(copy_name, isfolder ? NEWFOLDER : NEWSESSION);
		    copy_path[0] = '\0';
		};

		treeview_find_unused_name(treeview, tv_parent, copy_name,
					  to_name);
		ses_make_path(parent_path, to_name, to_path, BUFSIZE);

		switch (cut_or_copy) {
		case 0:	// insert new session/folder
		    success = ses_write_i(&config->sessionroot, to_path, 
			ISFOLDER, isfolder);
		    break;
		case 1:	// cut
		    success = ses_move_tree(&config->sessionroot, from_path, 
			to_path);
		    break;
		case 2:	// copy
		    success = ses_copy_tree(&config->sessionroot, from_path, 
			to_path);
		    break;
		};

		if (success) {
		    session_callback_t scb;
		    session_t ses;

		    memset(&ses, 0, sizeof(ses));
		    ses.root = config->sessionroot;
		    ses.isfolder = isfolder;
		    ses.name = to_name;
		    ses.path = to_path;

		    memset(&scb, 0, sizeof(scb));
		    scb.session = &ses;
		    tv_newitem = treeview_additem(treeview, tv_parent, &scb);
		    if (isfolder)
			treeview_addtree(treeview, tv_newitem, to_path);
		    else if (cut_or_copy == 2) {
			// by default, clear all hotkeys if copying
			for (i = 0; i < 3; i++) {
			    sprintf(copy_name, "%s%d", HOTKEY, i);
			    ses_delete_value(&config->sessionroot, to_path, 
				copy_name);
			    ses_delete_value(&config->sessionroot, to_path, 
				copy_name);
			};
		    };

		    if (cut_or_copy == 1)
			TreeView_DeleteItem(treeview, tv_copyitem);

		    SetFocus(treeview);

		    tscb.hParent = tv_parent;
		    tscb.lpfnCompare = treeview_compare;
		    tscb.lParam = (LPARAM) treeview;
		    TreeView_SortChildrenCB(treeview, &tscb, FALSE);

		    TreeView_EnsureVisible(treeview, tv_newitem);
		    TreeView_SelectItem(treeview, tv_newitem);
		};

		if (cut_or_copy) {
		    copying_now = NULL;
		    cut_or_copy = 0;
		    ModifyMenu(context_menu, IDM_CTXM_CANCEL,
			       MF_STRING | MF_BYCOMMAND, IDM_CTXM_UNDO,
			       (cut_or_copy ==
				1 ? "Undo Cut" : "Undo Copy"));
		    SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);
		};
	    };
	};
	break;
    case WM_NOTIFY:
	if (wParam == 0) {
	    switch (((LPNMHDR)lParam)->code) {
	    case NM_DBLCLK:
		{
		    TVHITTESTINFO hit;
		    TVITEM item;

		    GetCursorPos(&hit.pt);
		    ScreenToClient(treeview, &hit.pt);

		    item.hItem = TreeView_HitTest(treeview, &hit);

		    if (hit.flags & TVHT_ONITEM) {
			item.mask = TVIF_HANDLE | TVIF_CHILDREN;
			TreeView_GetItem(treeview, &item);

			if (item.cChildren == 0)
			    SendMessage(hwnd, WM_COMMAND, (WPARAM) IDOK,
					0);
		    };
		};
		break;
	    case TVN_KEYDOWN:
		{
		    LPNMTVKEYDOWN tvkd = (LPNMTVKEYDOWN)lParam;

		    switch (tvkd->wVKey)
		    {
		    case 'X':
		    case 'C':
		    case 'V':
		    case VK_INSERT:
		    case VK_DELETE:
			{
			    UINT modifiers = 0, msg = 0;

			    if (GetAsyncKeyState(VK_SHIFT))
				modifiers |= MOD_SHIFT;
			    if (GetAsyncKeyState(VK_CONTROL))
				modifiers |= MOD_CONTROL;
			    if (GetAsyncKeyState(VK_MENU))
				modifiers |= MOD_ALT;
			    if (GetAsyncKeyState(VK_LWIN) || 
				GetAsyncKeyState(VK_RWIN))
				modifiers |= MOD_WIN;

			    if (modifiers == MOD_CONTROL) {
				switch (tvkd->wVKey) {
				    case 'X':
					msg = IDM_CTXM_CUT;
					break;
				    case 'C':
				    case VK_INSERT:
					msg = IDM_CTXM_COPY;
					break;
				    case 'V':
					msg = IDM_CTXM_PASTE;
					break;
				    case VK_DELETE:
					msg = IDM_CTXM_DELETE;
					break;
				};
			    } else if (modifiers == MOD_SHIFT) {
				switch (tvkd->wVKey) {
				case VK_INSERT:
				    msg = IDM_CTXM_PASTE;
				    break;
				case VK_DELETE:
				    msg = IDM_CTXM_CUT;
				    break;
				};
			    } else if (modifiers == 0 &&
				tvkd->wVKey == VK_DELETE)
				msg = IDM_CTXM_DELETE;

			    if (msg)
				SendMessage(hwnd, WM_COMMAND, msg, 0);

			    return FALSE;
			};
		    };
		};
		break;
	    case TVN_ITEMEXPANDED:
		{
		    LPNMTREEVIEW tv = (LPNMTREEVIEW) lParam;

		    if (tv->itemNew.cChildren) {
			unsigned int isexpanded, wasexpanded;
			char path[BUFSIZE];

			wasexpanded = (tv->itemOld.state & 
			    TVIS_EXPANDED) ? TRUE : FALSE;
			isexpanded = (tv->itemNew.state & 
			    TVIS_EXPANDED) ? TRUE : FALSE;

			tv->itemNew.iImage = isexpanded ?
			    config->img_open : config->img_closed;
			tv->itemNew.iSelectedImage = isexpanded ?
			    config->img_open : config->img_closed;
			TreeView_SetItem(treeview, &tv->itemNew);

			treeview_getitempath(treeview, tv->itemNew.hItem,
					     path);

			ses_write_i(&config->sessionroot, path, 
			    ISEXPANDED, isexpanded);
		    };
		};
		break;
	    case TVN_SELCHANGED:
		{
		    LPNMTREEVIEW tv = (LPNMTREEVIEW) lParam;

		    SendMessage(hwnd, WM_REFRESHBUTTONS, 0,
				(LPARAM)tv->itemNew.hItem);
		};
		break;
	    case TVN_BEGINLABELEDIT:
		{
		    LPNMTVDISPINFO di = (LPNMTVDISPINFO) lParam;
		    char name[BUFSIZE];

		    treeview_getitemname(treeview, di->item.hItem, name,
					 BUFSIZE);
		    if (!strcmp(name, DEFAULTSETTINGS))
			TreeView_EndEditLabelNow(treeview, TRUE);
		    else
			editing_now = di->item.hItem;
		};
		break;
	    case TVN_ENDLABELEDIT:
		{
		    LPNMTVDISPINFO di = (LPNMTVDISPINFO) lParam;
		    editing_now = NULL;

		    if (di->item.pszText) {
			HTREEITEM item, parent;
			char path[BUFSIZE], parent_path[BUFSIZE],
			    from[BUFSIZE], to[BUFSIZE];

			item = di->item.hItem;
			parent = TreeView_GetParent(treeview, item);
			if (parent)
			    treeview_getitempath(treeview, parent,
						 parent_path);
			else
			    parent_path[0] = '\0';

			treeview_getitempath(treeview, item, path);

			strcpy(from, path);
			ses_make_path(parent_path, di->item.pszText, to, BUFSIZE);

			if (ses_move_tree(&config->sessionroot, path, to)) {
			    TVSORTCB tscb;

			    di->item.mask = TVIF_HANDLE | TVIF_TEXT;
			    TreeView_SetItem(treeview, &di->item);

			    tscb.hParent = parent;
			    tscb.lpfnCompare = treeview_compare;
			    tscb.lParam = (LPARAM) treeview;
			    TreeView_SortChildrenCB(treeview, &tscb,
						    FALSE);

			    TreeView_EnsureVisible(treeview, item);
			    TreeView_SelectItem(treeview, item);
			} else
			    TreeView_EditLabel(treeview, di->item.hItem);
		    };
		};
		break;
	    case TVN_BEGINDRAG:
	    case TVN_BEGINRDRAG:
		{
		    RECT r;
		    POINT pt;
		    DWORD indent;
		    LPNMTREEVIEW nmtv = (LPNMTREEVIEW) lParam;

		    draglist =
			TreeView_CreateDragImage(treeview,
						 nmtv->itemNew.hItem);
		    TreeView_GetItemRect(treeview, nmtv->itemNew.hItem, &r,
					 TRUE);
		    indent = TreeView_GetIndent(treeview);

		    ImageList_BeginDrag(draglist, 0, 0, 0);
		    GetCursorPos(&pt);
		    ScreenToClient(hwnd, &pt);
		    ImageList_DragEnter(treeview, pt.x, pt.y);

		    ShowCursor(FALSE);
		    SetCapture(hwnd);

		    dragging_now = nmtv->itemNew.hItem;
		};
		break;
	    case TCN_SELCHANGE:
		{
		    int selected;

		    selected = TabCtrl_GetCurSel(dlghdr->tab);
		    ShowWindow(dlghdr->kidwnd, SW_HIDE);

		    dlghdr->kidwnd = dlghdr->kids[selected].hwnd;
		    dlghdr->kidtype = dlghdr->kids[selected].type;
		    dlghdr->level = 0;

		    ShowWindow(dlghdr->kidwnd, SW_SHOW);
		};
	    };
	};
    };
    return FALSE;
};

/*
 * Launch Box: setup function.
 */
int do_launchbox(void)
{
    int ret;

    if (hwnd_launchbox) {
	SetForegroundWindow(hwnd_launchbox);
	return 0;
    };

    ret =
	DialogBox(config->hinst, MAKEINTRESOURCE(IDD_LAUNCHBOX), NULL,
		  LaunchBoxProc);

    hwnd_launchbox = NULL;

    return (ret == IDOK);
};

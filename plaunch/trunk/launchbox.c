/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004-2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * LaunchBox Dialog implementation file.
 */

#include "plaunch.h"
#include <stdio.h>
#include "entry.h"
#include "misc.h"
#include "hotkey.h"
#include "registry.h"
#include "resource.h"
#include "dlgtmpl.h"
#include "session.h"

#define IDM_CTXM_COPY			0x0400
#define IDM_CTXM_CUT			0x0401
#define IDM_CTXM_PASTE			0x0402
#define IDM_CTXM_DELETE			0x0403
#define IDM_CTXM_UNDO			0x0404
#define IDM_CTXM_CRTFLD			0x0405
#define IDM_CTXM_CRTSES			0x0406
#define IDM_CTXM_RENAME			0x0407
#define	IDM_CTXM_CANCEL			0x0408

#define NEWFOLDER			"New Folder"
#define NEWSESSION			"New Session"

#define	TAB_HOTKEYS			0
#define	TAB_AUTOPROCESS			1
#define	TAB_LIMITS			2
#define	TAB_LAST			3
#define	TAB_ACTIONS			3
#define	TAB_MOREACTIONS			4
#define	TAB_CHILDDIALOGS		5

#define SEARCHFOR_ITEMS			10

static char *SEARCHFOR_STRINGS[] = {
    "none", "last", "first", "second", "third", 
    "fourth", "fifth", "sixth", "seventh", "n-th"
};

#define ACTION_ITEMS			9

static char *ACTION_STRINGS[] = {
    "do nothing with", "hide", "show", "minimize", "maximize", "center",
    "kill", "murder", "run another"
};

#define	ACTION_ENABLE			0
#define	ACTION_FIND			1
#define	ACTION_ACTION			2

char *ATSTART_STRINGS[3] = {
    PLAUNCH_ACTION_ENABLE_ATSTART,
    PLAUNCH_ACTION_FIND_ATSTART,
    PLAUNCH_ACTION_SEQ_ATSTART
};

char *ATNETWORKUP_STRINGS[3] = {
    PLAUNCH_ACTION_ENABLE_ATNETWORKUP,
    PLAUNCH_ACTION_FIND_ATNETWORKUP,
    PLAUNCH_ACTION_SEQ_ATNETWORKUP
};

char *ATNETWORKDOWN_STRINGS[3] = {
    PLAUNCH_ACTION_ENABLE_ATNETWORKDOWN,
    PLAUNCH_ACTION_FIND_ATNETWORKDOWN,
    PLAUNCH_ACTION_SEQ_ATNETWORKDOWN
};

char *ATSTOP_STRINGS[3] = {
    PLAUNCH_ACTION_ENABLE_ATSTOP,
    PLAUNCH_ACTION_FIND_ATSTOP,
    PLAUNCH_ACTION_SEQ_ATSTOP
};

char *LIMIT_LOWER_STRINGS[3] = {
    NULL,
    PLAUNCH_LIMIT_FIND_LOWER,
    PLAUNCH_LIMIT_SEQ_LOWER
};

char *LIMIT_UPPER_STRINGS[3] = {
    NULL,
    PLAUNCH_LIMIT_FIND_UPPER,
    PLAUNCH_LIMIT_SEQ_UPPER
};

static char *TAB_NAMES[] =
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
    UINT toptype;		    // top level child dialog box type
    UINT subtype;		    // sub level child dialog box type
    UINT subsubtype;		    // sub sub level child dialog box type
    void *kiddata;		    // current child data
    void *topdata;		    // top level child data
    void *subdata;		    // sub level child data
    void *subsubdata;		    // sub sub level child data
    UINT level;			    // depth
    CHILDDIALOG kids[TAB_CHILDDIALOGS];
} DLGHDR;

/*
 * HotKeys tab
 */

static int CALLBACK LaunchBoxChildDialogProc0(HWND hwnd, UINT msg, 
					      WPARAM wParam, LPARAM lParam);

/*
 * Auto processing tab
 */

static int CALLBACK LaunchBoxChildDialogProc1(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);

/*
 * Instance limits tab
 */

static int CALLBACK LaunchBoxChildDialogProc2(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);

/*
 * Generic actions tab 1st level
 */

static int CALLBACK LaunchBoxChildDialogProc3(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);

/*
 * Generic actions tab 2nd level
 */

static int CALLBACK LaunchBoxChildDialogProc4(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam);

static DLGPROC childdialogprocedures[TAB_CHILDDIALOGS] =
    { LaunchBoxChildDialogProc0, LaunchBoxChildDialogProc1,
      LaunchBoxChildDialogProc2, LaunchBoxChildDialogProc3,
      LaunchBoxChildDialogProc4 };

/*
 * Launch Box: tree view compare function.
 * We want to sort 'em case sensitive, folders come first.
 * Default Settings is special case and always goes firstmost.
 */

static int CALLBACK treeview_compare(LPARAM p1, LPARAM p2, LPARAM sort)
{
    char item1_n[BUFSIZE], item1_p[BUFSIZE],
	 item2_n[BUFSIZE], item2_p[BUFSIZE];
    int item1_f = 0, item2_f = 0;

    treeview_getitemname((HWND) sort, (HTREEITEM) p1, item1_n, BUFSIZE);
    treeview_getitemname((HWND) sort, (HTREEITEM) p2, item2_n, BUFSIZE);

    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(item1_n, "Default Settings"))
	return -1;		/* item1 comes first */
    if (!strcmp(item2_n, "Default Settings"))
	return +1;		/* item2 comes first */

    treeview_getitempath((HWND) sort, (HTREEITEM) p1, item1_p, BUFSIZE);
    treeview_getitempath((HWND) sort, (HTREEITEM) p2, item2_p, BUFSIZE);

    item1_f = ses_is_folder(&config->sessionroot, item1_p);
    item2_f = ses_is_folder(&config->sessionroot, item2_p);

    if (item1_f && !item2_f)
	return -1;		/* item1 comes first */
    if (!item1_f && item2_f)
	return 1;		/* item2 comes first */

    return strcmp(item1_n, item2_n);
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
 * Launch Box: child dialog window function.
 * Hot Keys panel.
 */

static int CALLBACK LaunchBoxChildDialogProc0(HWND hwnd, UINT msg, 
					      WPARAM wParam, LPARAM lParam)
{
    static HBRUSH background;
    static DLGHDR *hdr = NULL;
    static HWND treeview = NULL;
    static HWND launch_hotkey = NULL, 
		launch_button = NULL, 
		edit_hotkey = NULL, 
		edit_button = NULL, 
		kill_hotkey = NULL,
		kill_button = NULL;
    HTREEITEM item;

    switch (msg) {
    case WM_INITDIALOG:
	{
	    HWND parent;

	    /*
	     * get DLGHDR structure pointer...
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

	    launch_hotkey = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_EDITBOX_LAUNCH);
	    make_hotkey(launch_hotkey, 0);

	    edit_hotkey = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_EDITBOX_EDIT);
	    make_hotkey(edit_hotkey, 0);

	    kill_hotkey = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_EDITBOX_KILL);
	    make_hotkey(kill_hotkey, 0);

	    /*
	     * pull button controls into local variables, for faster access
	     */

	    launch_button = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_BUTTON_LAUNCH);
	    edit_button = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_BUTTON_EDIT);
	    kill_button = GetDlgItem(hwnd, IDC_LAUNCHBOX_TAB0_BUTTON_KILL);

	    /*
	     * ... and refresh item states.
	     */

//	    SendMessage(hwnd, WM_REFRESHITEMS, 0, 0);
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
	    set_hotkey(launch_hotkey, 0);
	    EnableWindow(launch_button, FALSE);

	    set_hotkey(edit_hotkey, 0);
	    EnableWindow(edit_button, FALSE);

	    set_hotkey(kill_hotkey, 0);
	    EnableWindow(kill_button, FALSE);
	};
	break;
    case WM_ENABLEITEMS:
	{
	    BOOL enable = (BOOL) wParam;

	    EnableWindow(launch_hotkey, enable);
	    EnableWindow(edit_hotkey, enable);
	    EnableWindow(kill_hotkey, enable);
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
		SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    /*
	     * then we get its name, full path and type (session/folder)
	     */

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path, BUFSIZE);
	    isfolder = ses_is_folder(&config->sessionroot, path);

	    /*
	     * empty all controls first, in case it's a folder or a session
	     * without any hot keys set
	     */

	    if (isfolder || !strcmp(name, DEFAULTSETTINGS))
		enable = FALSE;
	    else
		enable = TRUE;

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
	    SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) enable, 0);

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

	    if (config->sessionroot.readonly)
		SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
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
	};
	switch (wParam) {
	case IDOK:
	case IDCANCEL:
	    SendMessage(GetParent(hwnd), WM_COMMAND, wParam, lParam);
	    break;
	case IDC_LAUNCHBOX_TAB0_BUTTON_LAUNCH:
	case IDC_LAUNCHBOX_TAB0_BUTTON_EDIT:
	case IDC_LAUNCHBOX_TAB0_BUTTON_KILL:
	    {
		char path[BUFSIZE], valname[BUFSIZE];
		unsigned int i, found, htype, hotkey;
		HWND editbox = NULL, button = NULL;

		switch (wParam) {
		case IDC_LAUNCHBOX_TAB0_BUTTON_LAUNCH:
		    editbox = launch_hotkey;
		    button = launch_button;
		    htype = 0;
		    break;
		case IDC_LAUNCHBOX_TAB0_BUTTON_EDIT:
		    editbox = edit_hotkey;
		    button = edit_button;
		    htype = 1;
		    break;
		case IDC_LAUNCHBOX_TAB0_BUTTON_KILL:
		    editbox = kill_hotkey;
		    button = kill_button;
		    htype = 2;
		    break;
		};

		item = TreeView_GetSelection(treeview);
		treeview_getitempath(treeview, item, path, BUFSIZE);

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

/*
 * Launch Box: child dialog window function.
 * Auto Processing panel.
 */

static int CALLBACK LaunchBoxChildDialogProc1(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    static HBRUSH background;
    static DLGHDR *hdr = NULL;
    static HWND treeview = NULL;
    static HWND cb_atstart = NULL,
		btn_atstart = NULL,
		cb_atnetworkup = NULL,
		btn_atnetworkup = NULL,
		cb_atnetworkdown = NULL,
		btn_atnetworkdown = NULL,
		cb_atstop = NULL,
		btn_atstop = NULL;

    switch (msg) {
    case WM_INITDIALOG:
	{
	    HWND parent;

	    /*
	     * get DLGHDR structure pointer...
	     */

	    parent = GetParent(hwnd);
	    hdr = (DLGHDR *) GetWindowLong(parent, GWL_USERDATA);
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
	     * pull controls into local variables for faster access.
	     */

	    cb_atstart = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTART);
	    btn_atstart = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_BUTTON_ATSTART);

	    cb_atnetworkup = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKUP);
	    btn_atnetworkup = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKUP);

	    cb_atnetworkdown = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKDOWN);
	    btn_atnetworkdown = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKDOWN);

	    cb_atstop = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTOP);
	    btn_atstop = GetDlgItem(hwnd, 
		IDC_LAUNCHBOX_TAB1_BUTTON_ATSTOP);

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
	    SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
	};
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
	    CheckDlgButton(hwnd, 
		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTART, 
		BST_UNCHECKED);
	    EnableWindow(btn_atstart, FALSE);
	    CheckDlgButton(hwnd, 
    		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKUP, 
		BST_UNCHECKED);
	    EnableWindow(btn_atnetworkup, FALSE);
	    CheckDlgButton(hwnd, 
		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKDOWN, 
		BST_UNCHECKED);
	    EnableWindow(btn_atnetworkdown, FALSE);
	    CheckDlgButton(hwnd, 
		IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTOP, 
		BST_UNCHECKED);
	    EnableWindow(btn_atstop, FALSE);
	};
	break;
    case WM_ENABLEITEMS:
	{
	    BOOL enable = (BOOL) wParam;

	    EnableWindow(cb_atstart, enable);
	    EnableWindow(cb_atnetworkup, enable);
	    EnableWindow(cb_atnetworkdown, enable);
	    EnableWindow(cb_atstop, enable);
	};
	break;
    case WM_REFRESHITEMS:
	{
	    HTREEITEM item;
	    BOOL isfolder, enable;
	    char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];

	    /*
	     * first we get the current selected item in tree view
	     */

	    item = (HTREEITEM)lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    /*
	     * then we get its name, full path and type (session/folder)
	     */

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path, BUFSIZE);
	    isfolder = ses_is_folder(&config->sessionroot, path);

	    /*
	     * empty all controls first, in case it's a folder or a session
	     * without any auto actions set
	     */

	    if (isfolder || !strcmp(name, DEFAULTSETTINGS))
		enable = FALSE;
	    else
		enable = TRUE;

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
	    SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) enable, 0);

	    /*
	     * if it is a folder, return. a folder can't have any auto actions.
	     * if it is a "Default Settings" session, it cannot have any auto actions too.
	     */

	    if (!enable)
		break;

	    /*
	     * pull all auto action settings from the registry and fill them in the
	     * check boxes
	     */
	    ses_read_i(&config->sessionroot, path,
		PLAUNCH_ACTION_ENABLE_ATSTART, 0, &enable);
	    CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTART,
		enable ? BST_CHECKED : BST_UNCHECKED);
	    EnableWindow(btn_atstart, enable);

	    ses_read_i(&config->sessionroot, path,
		PLAUNCH_ACTION_ENABLE_ATNETWORKUP, 0, &enable);
	    CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKUP,
		enable ? BST_CHECKED : BST_UNCHECKED);
	    EnableWindow(btn_atnetworkup, enable);

	    ses_read_i(&config->sessionroot, path,
		PLAUNCH_ACTION_ENABLE_ATNETWORKDOWN, 0, &enable);
	    CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKDOWN,
		enable ? BST_CHECKED : BST_UNCHECKED);
	    EnableWindow(btn_atnetworkdown, enable);

	    ses_read_i(&config->sessionroot, path,
		PLAUNCH_ACTION_ENABLE_ATSTOP, 0, &enable);
	    CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTOP,
		enable ? BST_CHECKED : BST_UNCHECKED);
	    EnableWindow(btn_atstop, enable);

	    if (config->sessionroot.readonly)
		SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
	};
	break;
    case WM_COMMAND:
	switch (HIWORD(wParam)) {
	case BN_CLICKED:
	    switch (LOWORD(wParam)) {
	    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTART:
	    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKUP:
	    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKDOWN:
	    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTOP:
		{
		    HTREEITEM item = NULL;
		    HWND button;
		    char name[BUFSIZE], path[BUFSIZE], *valname = NULL;
		    int value;

		    if (!item && TreeView_GetCount(treeview))
			item = TreeView_GetSelection(treeview);

		    if (!item) {
			SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
			SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
			break;
		    };

		    memset(name, 0, BUFSIZE);
		    memset(path, 0, BUFSIZE);

		    treeview_getitemname(treeview, item, name, BUFSIZE);
		    treeview_getitempath(treeview, item, path, BUFSIZE);

		    switch (LOWORD(wParam)) {
		    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTART:
			valname = PLAUNCH_ACTION_ENABLE_ATSTART;
			button = btn_atstart;
			break;
		    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKUP:
			valname = PLAUNCH_ACTION_ENABLE_ATNETWORKUP;
			button = btn_atnetworkup;
			break;
		    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKDOWN:
			valname = PLAUNCH_ACTION_ENABLE_ATNETWORKDOWN;
			button = btn_atnetworkdown;
			break;
		    case IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTOP:
			valname = PLAUNCH_ACTION_ENABLE_ATSTOP;
			button = btn_atstop;
			break;
		    };

		    if (!valname)
			break;

		    value = (IsDlgButtonChecked(hwnd, LOWORD(wParam)) == BST_CHECKED);

		    if (value)
			ses_write_i(&config->sessionroot, path, valname, value);
		    else
			ses_delete_value(&config->sessionroot, path, valname);

		    EnableWindow(button, value);
		};
	    };
	    break;
	};
	switch (wParam) {
	case IDOK:
	case IDCANCEL:
	    SendMessage(GetParent(hwnd), WM_COMMAND, wParam, lParam);
	    break;
	case IDC_LAUNCHBOX_TAB1_BUTTON_ATSTART:
	case IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKUP:
	case IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKDOWN:
	case IDC_LAUNCHBOX_TAB1_BUTTON_ATSTOP:
	    ShowWindow(hdr->kidwnd, SW_HIDE);
	    hdr->kidwnd = hdr->kids[TAB_ACTIONS].hwnd;
	    hdr->toptype = hdr->kidtype;
	    hdr->kidtype = hdr->kids[TAB_ACTIONS].type;
	    hdr->subtype = 0;
	    hdr->subsubtype = 0;
	    hdr->subdata = NULL;
	    hdr->subsubdata = NULL;
	    switch (wParam) {
	    case IDC_LAUNCHBOX_TAB1_BUTTON_ATSTART:
		hdr->topdata = (void *) ATSTART_STRINGS;
		break;
	    case IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKUP:
		hdr->topdata = (void *) ATNETWORKUP_STRINGS;
		break;
	    case IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKDOWN:
		hdr->topdata = (void *) ATNETWORKDOWN_STRINGS;
		break;
	    case IDC_LAUNCHBOX_TAB1_BUTTON_ATSTOP:
		hdr->topdata = (void *) ATSTOP_STRINGS;
		break;
	    };
	    hdr->kiddata = hdr->topdata;
	    hdr->level = 1;

	    SendMessage(hdr->kidwnd, WM_REFRESHITEMS, 0, 0);
	    ShowWindow(hdr->kidwnd, SW_SHOW);
	};
	break;
    };

    return FALSE;
};

/*
 * Launch Box: child dialog window function.
 * Instance limits panel.
 */

static int CALLBACK LaunchBoxChildDialogProc2(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    static HBRUSH background;
    static DLGHDR *hdr = NULL;
    static HWND treeview = NULL;
    static HWND cb_enable = NULL,
		cb_lower = NULL,
		cb_upper = NULL,
		eb_lower = NULL,
		eb_upper = NULL,
		spin_lower = NULL,
		spin_upper = NULL,
		btn_lower = NULL,
		btn_upper = NULL;
    static int int_event = FALSE;

    switch (msg) {
    case WM_INITDIALOG:
	{
	    HWND parent;

	    /*
	     * get DLGHDR structure pointer...
	     */

	    parent = GetParent(hwnd);
	    hdr = (DLGHDR *) GetWindowLong(parent, GWL_USERDATA);
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

	    int_event = TRUE;
	    cb_enable = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_CHECKBOX_ENABLE);
	    cb_lower = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_CHECKBOX_LOWER);
	    cb_upper = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_CHECKBOX_UPPER);
	    eb_lower = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_EDITBOX_LOWER);
	    eb_upper = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_EDITBOX_UPPER);
	    spin_lower = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_SPIN_LOWER);
	    SendMessage(spin_lower, UDM_SETRANGE, 0, MAKELONG(255, 1));
	    spin_upper = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_SPIN_UPPER);
	    SendMessage(spin_upper, UDM_SETRANGE, 0, MAKELONG(255, 1));
	    btn_lower = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_BUTTON_LOWER);
	    btn_upper = GetDlgItem(hwnd,
		IDC_LAUNCHBOX_TAB2_BUTTON_UPPER);
	    int_event = FALSE;

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
	};
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
	    int_event = TRUE;
	    CheckDlgButton(hwnd, 
		IDC_LAUNCHBOX_TAB2_CHECKBOX_ENABLE, BST_UNCHECKED);
	    EnableWindow(cb_enable, TRUE);
	    CheckDlgButton(hwnd,
		IDC_LAUNCHBOX_TAB2_CHECKBOX_LOWER, BST_UNCHECKED);
	    EnableWindow(cb_lower, FALSE);
	    CheckDlgButton(hwnd,
		IDC_LAUNCHBOX_TAB2_CHECKBOX_UPPER, BST_UNCHECKED);
	    EnableWindow(cb_upper, FALSE);
	    SetWindowText(eb_lower, "1");
	    EnableWindow(eb_lower, FALSE);
	    SetWindowText(eb_upper, "1");
	    EnableWindow(eb_upper, FALSE);
	    SendMessage(spin_lower, UDM_SETPOS, 0, MAKELONG((short) 1, 0));
	    EnableWindow(spin_lower, FALSE);
	    SendMessage(spin_upper, UDM_SETPOS, 0, MAKELONG((short) 1, 0));
	    EnableWindow(spin_upper, FALSE);
	    EnableWindow(btn_lower, FALSE);
	    EnableWindow(btn_upper, FALSE);
	    int_event = FALSE;
	};
	break;
   case WM_ENABLEITEMS:
       {
	   HWND controls[9] = {
	       cb_enable,
	       cb_lower, eb_lower, spin_lower, btn_lower,
	       cb_upper, eb_upper, spin_upper, btn_upper
	   };
	   int i;

	   for (i = 0; i < 9; i++) {
	       EnableWindow(controls[i], (BOOL) wParam);
	   };
       };
       break;
   case WM_REFRESHITEMS:
	{
	    HTREEITEM item;
	    BOOL isfolder, enable, top_enable;
	    int value;
	    char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];

	    /*
	     * first we get the current selected item in tree view
	     */

	    item = (HTREEITEM)lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    /*
	     * then we get its name, full path and type (session/folder)
	     */

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path, BUFSIZE);
	    isfolder = ses_is_folder(&config->sessionroot, path);

	    /*
	     * empty all controls first, in case it's a folder or a session
	     * without any instance limits set
	     */

	    if (isfolder || !strcmp(name, DEFAULTSETTINGS))
		enable = FALSE;
	    else
		enable = TRUE;

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
	    SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) enable, 0);

	    /*
	     * if it is a folder, return. a folder can't have any instance limits.
	     * if it is a "Default Settings" session, it cannot have any limits too.
	     */

	    if (!enable)
		break;

	    /*
	     * pull all instance limit settings from the registry and fill them in the
	     * check boxes
	     */
	    int_event = TRUE;

	    ses_read_i(&config->sessionroot, path, PLAUNCH_LIMIT_ENABLE, 0, &top_enable);
	    CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB2_CHECKBOX_ENABLE,
		top_enable ? BST_CHECKED : BST_UNCHECKED);
	    EnableWindow(cb_enable, TRUE);

	    ses_read_i(&config->sessionroot, path,
		PLAUNCH_LIMIT_LOWER_ENABLE, 0, &enable);
	    CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB2_CHECKBOX_LOWER,
		enable ? BST_CHECKED : BST_UNCHECKED);
	    EnableWindow(cb_lower, top_enable);

	    ses_read_i(&config->sessionroot, path, PLAUNCH_LIMIT_LOWER_THRESHOLD, 0, &value);
	    SendMessage(spin_lower, UDM_SETPOS, 0, MAKELONG((short) value, 0));
	    EnableWindow(eb_lower, enable);
	    EnableWindow(spin_lower, enable);
	    EnableWindow(btn_lower, enable);

	    ses_read_i(&config->sessionroot, path, PLAUNCH_LIMIT_UPPER_ENABLE, 0, &enable);
	    CheckDlgButton(hwnd, IDC_LAUNCHBOX_TAB2_CHECKBOX_UPPER,
		enable ? BST_CHECKED : BST_UNCHECKED);
	    EnableWindow(cb_upper, top_enable);

	    ses_read_i(&config->sessionroot, path, PLAUNCH_LIMIT_UPPER_THRESHOLD, 0, &value);
	    SendMessage(spin_upper, UDM_SETPOS, 0, MAKELONG((short) value, 0));
	    EnableWindow(eb_upper, enable);
	    EnableWindow(spin_upper, enable);
	    EnableWindow(btn_upper, enable);

	    int_event = FALSE;

	    if (config->sessionroot.readonly)
		SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
	};
	break;
    case WM_COMMAND:
	switch (HIWORD(wParam)) {
	case EN_CHANGE:
	    {
		HTREEITEM item = NULL;
		char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
		char *valname = NULL;
		int value;

		if (int_event)
		    break;

		if (((HWND) lParam) == eb_lower) {
		    valname = PLAUNCH_LIMIT_LOWER_THRESHOLD;
		} else if (((HWND) lParam) == eb_upper) {
		    valname = PLAUNCH_LIMIT_UPPER_THRESHOLD;
		} else
		    break;

		if (!item && TreeView_GetCount(treeview))
		    item = TreeView_GetSelection(treeview);

		if (!item) {
		    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		    break;
		};

		memset(name, 0, BUFSIZE);
		memset(path, 0, BUFSIZE);
		memset(buf, 0, BUFSIZE);

		treeview_getitemname(treeview, item, name, BUFSIZE);
		treeview_getitempath(treeview, item, path, BUFSIZE);

		GetWindowText((HWND) lParam, buf, BUFSIZE);

		if (buf[0]) {
		    value = atoi(buf);
		    if ((value < 0 && (value = 0)) || 
			(value > 255 && (value = 255))) {
			sprintf(buf, "%d", value);
			SetWindowText((HWND) lParam, buf);
			return FALSE;
		    };
		    if (value)
			ses_write_i(&config->sessionroot, path, valname, value);
		    else
			ses_delete_value(&config->sessionroot, path, valname);
		};
	    };
	    break;
	case BN_CLICKED:
	    switch (LOWORD(wParam)) {
	    case IDC_LAUNCHBOX_TAB2_CHECKBOX_ENABLE:
	    case IDC_LAUNCHBOX_TAB2_CHECKBOX_LOWER:
	    case IDC_LAUNCHBOX_TAB2_CHECKBOX_UPPER:
		{
		    HTREEITEM item = NULL;
		    HWND controls[8] = {
			NULL, NULL, NULL, NULL,
			NULL, NULL, NULL, NULL
		    };
		    char name[BUFSIZE], path[BUFSIZE], *valname = NULL;
		    int i, value;

		    if (int_event)
			break;

		    if (!item && TreeView_GetCount(treeview))
			item = TreeView_GetSelection(treeview);

		    if (!item) {
			SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
			SendMessage(hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
			break;
		    };

		    memset(name, 0, BUFSIZE);
		    memset(path, 0, BUFSIZE);

		    treeview_getitemname(treeview, item, name, BUFSIZE);
		    treeview_getitempath(treeview, item, path, BUFSIZE);

		    value = (IsDlgButtonChecked(hwnd, LOWORD(wParam)) == BST_CHECKED);

		    switch (LOWORD(wParam)) {
		    case IDC_LAUNCHBOX_TAB2_CHECKBOX_ENABLE:
			valname = PLAUNCH_LIMIT_ENABLE;
			controls[0] = cb_lower; controls[4] = cb_upper;
			if (value == BST_UNCHECKED) {
			    controls[1] = eb_lower; controls[5] = eb_upper;
			    controls[2] = spin_lower; controls[6] = spin_upper;
			    controls[3] = btn_lower; controls[7] = btn_upper;
			};
			break;
		    case IDC_LAUNCHBOX_TAB2_CHECKBOX_LOWER:
			valname = PLAUNCH_LIMIT_LOWER_ENABLE;
			controls[0] = eb_lower;
			controls[1] = spin_lower;
			controls[2] = btn_lower;
			SendMessage(hwnd, WM_COMMAND, 
				    MAKELONG(IDC_LAUNCHBOX_TAB2_EDITBOX_LOWER, EN_CHANGE),
				    (LPARAM) eb_lower);
			break;
		    case IDC_LAUNCHBOX_TAB2_CHECKBOX_UPPER:
			valname = PLAUNCH_LIMIT_UPPER_ENABLE;
			controls[0] = eb_upper;
			controls[1] = spin_upper;
			controls[2] = btn_upper;
			SendMessage(hwnd, WM_COMMAND, 
				    MAKELONG(IDC_LAUNCHBOX_TAB2_EDITBOX_UPPER, EN_CHANGE),
				    (LPARAM) eb_upper);
			break;
		    };

		    if (!valname)
			break;

		    if (value)
			ses_write_i(&config->sessionroot, path, valname, value);
		    else
			ses_delete_value(&config->sessionroot, path, valname);

		    for (i = 0; i < 8; i++) {
			if (controls[i])
			    EnableWindow(controls[i], (BOOL) value);
		    };
		};
	    };
	    break;
	};
	switch (wParam) {
	case IDOK:
	case IDCANCEL:
	    SendMessage(GetParent(hwnd), WM_COMMAND, wParam, lParam);
	    break;
	case IDC_LAUNCHBOX_TAB2_BUTTON_LOWER:
	case IDC_LAUNCHBOX_TAB2_BUTTON_UPPER:
	    ShowWindow(hdr->kidwnd, SW_HIDE);
	    hdr->kidwnd = hdr->kids[TAB_ACTIONS].hwnd;
	    hdr->toptype = hdr->kidtype;
	    hdr->kidtype = hdr->kids[TAB_ACTIONS].type;
	    hdr->subtype = 0;
	    hdr->subsubtype = 0;
	    hdr->subdata = NULL;
	    hdr->subsubdata = NULL;
	    switch (wParam) {
	    case IDC_LAUNCHBOX_TAB2_BUTTON_LOWER:
		hdr->topdata = (void *) LIMIT_LOWER_STRINGS;
		break;
	    case IDC_LAUNCHBOX_TAB2_BUTTON_UPPER:
		hdr->topdata = (void *) LIMIT_UPPER_STRINGS;
		break;
	    };
	    hdr->kiddata = hdr->topdata;
	    hdr->level = 1;

	    SendMessage(hdr->kidwnd, WM_REFRESHITEMS, 0, 0);
	    ShowWindow(hdr->kidwnd, SW_SHOW);
	};
	break;
    case WM_NOTIFY:
	if (int_event ||
	    (wParam != IDC_LAUNCHBOX_TAB2_SPIN_LOWER &&
	    wParam != IDC_LAUNCHBOX_TAB2_SPIN_UPPER))
	    break;
	switch (((LPNMHDR) lParam)->code) {
	case UDN_DELTAPOS:
	    {
		int lower = 0, upper = 0;
		LPNMHDR hdr = ((LPNMHDR) lParam);
		LPNMUPDOWN nmud = ((LPNMUPDOWN) lParam);

		lower = SendMessage(spin_lower, UDM_GETPOS, 0, 0);
		upper = SendMessage(spin_upper, UDM_GETPOS, 0, 0);

		if (hdr->hwndFrom == spin_lower &&
		    (lower + nmud->iDelta) > upper) {
			upper = lower + nmud->iDelta;
			SendMessage(spin_upper, UDM_SETPOS, 0, MAKELONG((short) upper, 0));
		 } else if (hdr->hwndFrom == spin_upper &&
		    (upper + nmud->iDelta < lower)) {
			SendMessage(spin_upper, UDM_SETPOS, 0, MAKELONG((short) lower + 1, 0));
			return TRUE;
		 };
	    };
	};
	break;
    };

    return FALSE;
};

/*
 * Launch Box: child dialog window function.
 * Generic actions level 1 panel.
 */

static int CALLBACK LaunchBoxChildDialogProc3(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    static HBRUSH background;
    static DLGHDR *hdr = NULL;
    static HWND treeview = NULL;
    static HWND cb_find = NULL,
		eb_find = NULL,
		spin_find = NULL,
		btn_back = NULL,
		btn_moreact = NULL,
		btn_next = NULL,
		cb_actions[6] = 
		    { NULL, NULL, NULL,
		      NULL, NULL, NULL };

    switch (msg) {
    case WM_INITDIALOG:
	{
	    HWND parent;
	    int i, j;

	    /*
	     * get DLGHDR structure pointer...
	     */

	    parent = GetParent(hwnd);
	    hdr = (DLGHDR *) GetWindowLong(parent, GWL_USERDATA);
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

	    cb_find = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_FIND);
	    SendMessage(cb_find, CB_RESETCONTENT, 0, 0);
	    for (i = 0; i < SEARCHFOR_ITEMS; i++)
		SendMessage(cb_find, CB_ADDSTRING, 0, (LPARAM) SEARCHFOR_STRINGS[i]);

	    eb_find = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC1_EDITBOX_FIND);
//	    SetWindowText(eb_find, "0");

	    spin_find = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC1_SPIN_FIND);
	    SendMessage(spin_find, UDM_SETRANGE, 0, MAKELONG(255, 8));

	    btn_moreact = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC1_BUTTON_MOREACTIONS);

	    btn_back = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC1_BUTTON_BACK);

	    btn_next = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC1_BUTTON_NEXT);

	    for (i = 0;	i < 6; i++) {
		cb_actions[i] = 
		    GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION1 + i);
		SendMessage(cb_actions[i], CB_RESETCONTENT, 0, 0);
		for (j = 0; j < ACTION_ITEMS; j++)
		    SendMessage(cb_actions[i], CB_ADDSTRING, 0, (LPARAM) ACTION_STRINGS[j]);
		EnableWindow(cb_actions[i], FALSE);
	    };
	};
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
	    int i;

	    EnableWindow(cb_find, TRUE);

	    SetWindowText(eb_find, "");
	    EnableWindow(eb_find, FALSE);

	    EnableWindow(spin_find, FALSE);

	    EnableWindow(btn_moreact, FALSE);

	    EnableWindow(btn_back, TRUE);

	    EnableWindow(btn_next, FALSE);

	    for (i = 0;	i < 6; i++) {
		SendMessage(cb_actions[i], CB_SETCURSEL, 0, 0);
		EnableWindow(cb_actions[i], FALSE);
	    };
	};
	break;
    case WM_RESETITEMS:
	{
	    HTREEITEM item = NULL;
	    char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
	    char **data = (char **) hdr->kiddata;
	    int i, start;

	    item = (HTREEITEM) lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path, BUFSIZE);

	    start = (int) wParam;

	    for (i = start; i < 255; i++) {
		sprintf(buf, data[ACTION_ACTION], i);
		ses_delete_value(&config->sessionroot, path, buf);
	    };

	    for (i = start; i < 6; i++) {
		SendMessage(cb_actions[i], CB_SETCURSEL, 0, 0);
	    };

	    for (i = start + 1; i < 6; i++) {
		EnableWindow(cb_actions[i], FALSE);
	    };

	    EnableWindow(btn_moreact, FALSE);
	};
	break;
    case WM_REFRESHITEMS:
	{
	    HTREEITEM item;
	    int value, i;
	    char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
	    char **data = (char **) hdr->kiddata;

	    /*
	     * first we get the current selected item in tree view
	     */

	    item = (HTREEITEM) lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    /*
	     * then we get its name and full path. we don't check for
	     * session or folder because we can't possibly reach here for
	     * a folder.
	     */

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path, BUFSIZE);

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);

	    /*
	     * pull all settings from the registry and fill them in the
	     * combo boxes
	     */

	    for (i = 0; i < 6; i++) {
		sprintf(buf, data[ACTION_ACTION], i);
		ses_read_i(&config->sessionroot, path, buf, 0, &value);
		SendMessage(cb_actions[i], CB_SETCURSEL, (WPARAM) value, 0);
		if (value) {
		    EnableWindow(cb_actions[i], TRUE);
		    if (i < 5)
			EnableWindow(cb_actions[i + 1], TRUE);
		    else
			EnableWindow(btn_moreact, TRUE);
		};
	    };

	    ses_read_i(&config->sessionroot, path, data[ACTION_FIND], -1, &value);
	    if (value < 8)
		SendMessage(cb_find, CB_SETCURSEL, (WPARAM) (value + 1), 0);
	    else {
		SendMessage(cb_find, CB_SETCURSEL, (WPARAM) SEARCHFOR_ITEMS - 1, 0);
		SendMessage(spin_find, UDM_SETPOS, 0, MAKELONG((short) value, 0));
		EnableWindow(eb_find, TRUE);
		EnableWindow(spin_find, TRUE);
	    };
	    if (value >= 0)
		EnableWindow(cb_actions[0], TRUE);
	};
	break;
    case WM_COMMAND:
	switch (HIWORD(wParam)) {
	case EN_CHANGE:
	    {
		HTREEITEM item = NULL;
		char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
		char **data;
		int pos;

		if (((HWND )lParam) != eb_find)
		    break;

		if (hdr != NULL)
		    data  = (char **) hdr->kiddata;

		if (!item && TreeView_GetCount(treeview))
		    item = TreeView_GetSelection(treeview);

		if (!item) {
		    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		    break;
		};

		memset(name, 0, BUFSIZE);
		memset(path, 0, BUFSIZE);
		memset(buf, 0, BUFSIZE);

		treeview_getitemname(treeview, item, name, BUFSIZE);
		treeview_getitempath(treeview, item, path, BUFSIZE);

		GetWindowText(eb_find, buf, BUFSIZE);

		if (buf[0]) {
		    pos = atoi(buf);
		    if ((pos < 8 && (pos = 8)) || 
			(pos > 255 && (pos = 255))) {
			sprintf(buf, "%d", pos);
			SetWindowText(eb_find, buf);
			return FALSE;
		    };
		    ses_write_i(&config->sessionroot, path, data[ACTION_FIND], pos);
		};
	    };
	    break;
	case CBN_SELCHANGE:
	    {
		HTREEITEM item = NULL;
		char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
		char **data = (char **) hdr->kiddata;
		int i;

		if (!item && TreeView_GetCount(treeview))
		    item = TreeView_GetSelection(treeview);

		if (!item) {
		    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		    break;
		};

		memset(name, 0, BUFSIZE);
		memset(path, 0, BUFSIZE);
		memset(buf, 0, BUFSIZE);

		treeview_getitemname(treeview, item, name, BUFSIZE);
		treeview_getitempath(treeview, item, path, BUFSIZE);

		switch (LOWORD(wParam)) {
		case IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_FIND:
		    {
			i = SendMessage(cb_find, CB_GETCURSEL, 0, 0);

			if (!i) {
			    ses_delete_value(&config->sessionroot, path, data[ACTION_FIND]);
			    SendMessage(hwnd, WM_RESETITEMS, 0, (LPARAM) item);
			    EnableWindow(cb_actions[0], FALSE);
			} else if (i < 9) {
			    i--;
			    ses_write_i(&config->sessionroot, path, data[ACTION_FIND], i);
			    EnableWindow(cb_actions[0], TRUE);
			} else {
			    ses_write_i(&config->sessionroot, path, data[ACTION_FIND], 8);
			    SendMessage(spin_find, UDM_SETPOS, 0, MAKELONG(8, 0));
			    SetWindowText(spin_find, "8");
			    EnableWindow(eb_find, TRUE);
			    EnableWindow(spin_find, TRUE);
			    EnableWindow(cb_actions[0], TRUE);
			};
		    };
		    break;
		case IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION1:
		case IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION2:
		case IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION3:
		case IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION4:
		case IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION5:
		case IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION6:
		    {
			int act = 
			    LOWORD(wParam) - IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION1;

			i = SendMessage(cb_actions[act], CB_GETCURSEL, 0, 0);
			sprintf(buf, data[ACTION_ACTION], act);

			if (!i)
			    SendMessage(hwnd, WM_RESETITEMS, (WPARAM) act, (LPARAM) item);
			else {
			    ses_write_i(&config->sessionroot, path, buf, i);
			    if (act < 5)
				EnableWindow(cb_actions[act + 1], TRUE);
			    else
				EnableWindow(btn_moreact, TRUE);
			};
		    };
		    break;
		};
	    };
	    break;
	};
	switch (wParam) {
	case IDC_LAUNCHBOX_TABGENERIC1_BUTTON_BACK:
	    {
		ShowWindow(hdr->kidwnd, SW_HIDE);
		hdr->kidwnd = hdr->kids[hdr->toptype].hwnd;
		hdr->kidtype = hdr->kids[hdr->toptype].type;
		hdr->kiddata = NULL;
		hdr->level--;

		SendMessage(hdr->kidwnd, WM_REFRESHITEMS, 0, 0);
		ShowWindow(hdr->kidwnd, SW_SHOW);
	    };
	    break;
	case IDC_LAUNCHBOX_TABGENERIC1_BUTTON_MOREACTIONS:
	    {
		char **data = (char **) hdr->kiddata;

		ShowWindow(hdr->kidwnd, SW_HIDE);
		hdr->subtype = hdr->kidtype;
		hdr->subdata = hdr->kiddata;
		hdr->kidwnd = hdr->kids[TAB_MOREACTIONS].hwnd;
		hdr->kidtype = hdr->kids[TAB_MOREACTIONS].type;
//		hdr->subtype = hdr->subtype;
		hdr->subsubtype = 0;
//		hdr->subdata = (void *) data[ACTION_ACTION];
		hdr->kiddata = (void *) data[ACTION_ACTION];
		hdr->subsubdata = NULL;
		hdr->level++;

		SendMessage(hdr->kidwnd, WM_REFRESHITEMS, 0, 0);
		ShowWindow(hdr->kidwnd, SW_SHOW);
	    };
	    break;
	case IDC_LAUNCHBOX_TABGENERIC1_BUTTON_NEXT:
	    {
		/* nothing here yet */
	    };
	    break;
	};
	break;
    case WM_NOTIFY:
	switch (((LPNMHDR) lParam)->code) {
	case UDN_DELTAPOS:
	    if (((LPNMUPDOWN) lParam)->iPos == 8 &&
		((LPNMUPDOWN) lParam)->iDelta < 0)
		return TRUE;
	    else
		return FALSE;
	};
	break;
    };

    return FALSE;
};

/*
 * Launch Box: child dialog window function.
 * Generic actions level 2 panel.
 */

static int CALLBACK LaunchBoxChildDialogProc4(HWND hwnd, UINT msg,
					      WPARAM wParam, LPARAM lParam) 
{
    static HBRUSH background;
    static DLGHDR *hdr = NULL;
    static HWND treeview = NULL;
    static HWND btn_back = NULL,
		btn_next = NULL,
		cb_actions[10] = {
		    NULL, NULL, NULL, NULL, NULL,
		    NULL, NULL, NULL, NULL, NULL };
    static int base_action = 0;

    switch (msg) {
    case WM_INITDIALOG:
	{
	    HWND parent;
	    int i, j;

	    /*
	     * get DLGHDR structure pointer...
	     */

	    parent = GetParent(hwnd);
	    hdr = (DLGHDR *) GetWindowLong(parent, GWL_USERDATA);
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

	    btn_back = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC2_BUTTON_BACK);

	    btn_next = GetDlgItem(hwnd, IDC_LAUNCHBOX_TABGENERIC2_BUTTON_NEXT);

	    for (i = 0; i < 10; i++) {
		cb_actions[i] = GetDlgItem(hwnd,
		    IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION1 + i);
		SendMessage(cb_actions[i], CB_RESETCONTENT, 0, 0);
		for (j = 0; j < ACTION_ITEMS; j++)
		    SendMessage(cb_actions[i], CB_ADDSTRING, 0, (LPARAM) ACTION_STRINGS[j]);
		SendMessage(cb_actions[i], CB_SETCURSEL, 0, 0);
	    };

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
	};
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
	    int i;

	    for (i = 0;	i < 10; i++)
		EnableWindow(cb_actions[i], FALSE);

	    EnableWindow(btn_next, FALSE);
	};
	break;
    case WM_RESETITEMS:
	{
	    HTREEITEM item = NULL;
	    char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
	    char *data = (char *) hdr->kiddata;
	    int i, start;

	    item = (HTREEITEM) lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path, BUFSIZE);

	    start = (int) wParam;

	    for (i = base_action + start; i < 255; i++) {
		sprintf(buf, data, i);
		ses_delete_value(&config->sessionroot, path, buf);
	    };

	    for (i = start; i < 10; i++) {
		SendMessage(cb_actions[i], CB_SETCURSEL, 0, 0);
	    };

	    for (i = start + 1; i < 10; i++) {
		EnableWindow(cb_actions[i], FALSE);
	    };

	    EnableWindow(btn_next, FALSE);
	};
	break;
    case WM_REFRESHITEMS:
	{
	    HTREEITEM item;
	    int value, i;
	    char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
	    char *data = (char *) hdr->kiddata;

	    /*
	     * first we get the current selected item in tree view
	     */

	    item = (HTREEITEM) lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		break;
	    };

	    memset(name, 0, BUFSIZE);
	    memset(path, 0, BUFSIZE);
	    memset(buf, 0, BUFSIZE);

	    /*
	     * then we get its name and full path. we don't check for
	     * session or folder because we can't possibly reach here for
	     * a folder.
	     */

	    treeview_getitemname(treeview, item, name, BUFSIZE);
	    treeview_getitempath(treeview, item, path, BUFSIZE);

	    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);

	    base_action = (hdr->level - 2) * 10 + 6;

	    /*
	     * pull all settings from the registry and fill them in the
	     * combo boxes
	     */

	    for (i = 0; i < 10; i++) {
		sprintf(buf, data, base_action + i);
		ses_read_i(&config->sessionroot, path, buf, 0, &value);
		SendMessage(cb_actions[i], CB_SETCURSEL, (WPARAM) value, 0);
		if (value) {
		    EnableWindow(cb_actions[i], TRUE);
		    if (i < 9)
			EnableWindow(cb_actions[i + 1], TRUE);
		    else
			EnableWindow(btn_next, TRUE);
		};
	    };

	    EnableWindow(cb_actions[0], TRUE);
	};
	break;
    case WM_COMMAND:
	switch (HIWORD(wParam)) {
	case CBN_SELCHANGE:
	    {
		HTREEITEM item = NULL;
		char name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
		char *data = (char *) hdr->kiddata;
		int i;

		if (!item && TreeView_GetCount(treeview))
		    item = TreeView_GetSelection(treeview);

		if (!item) {
		    SendMessage(hwnd, WM_EMPTYITEMS, 0, 0);
		    break;
		};

		memset(name, 0, BUFSIZE);
		memset(path, 0, BUFSIZE);
		memset(buf, 0, BUFSIZE);

		treeview_getitemname(treeview, item, name, BUFSIZE);
		treeview_getitempath(treeview, item, path, BUFSIZE);

		switch (LOWORD(wParam)) {
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION1:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION2:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION3:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION4:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION5:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION6:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION7:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION8:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION9:
		case IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION10:
		    {
			int act = 
			    LOWORD(wParam) - IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION1;

			i = SendMessage(cb_actions[act], CB_GETCURSEL, 0, 0);
			sprintf(buf, data, base_action + act);

			if (!i)
			    SendMessage(hwnd, WM_RESETITEMS, (WPARAM) act, (LPARAM) item);
			else {
			    ses_write_i(&config->sessionroot, path, buf, i);
			    if (act < 9)
				EnableWindow(cb_actions[act + 1], TRUE);
			    else
				EnableWindow(btn_next, TRUE);
			};
		    };
		    break;
		};
	    };
	    break;
	};
	switch (wParam) {
	case IDC_LAUNCHBOX_TABGENERIC2_BUTTON_BACK:
	    {
		if (hdr->level > 2) {
		    hdr->level--;
		} else {
		    ShowWindow(hdr->kidwnd, SW_HIDE);
		    hdr->kidwnd = hdr->kids[hdr->subtype].hwnd;
		    hdr->kidtype = hdr->kids[hdr->subtype].type;
		    hdr->subsubtype = 0;
		    hdr->kiddata = hdr->subdata;
		    hdr->subsubdata = NULL;
		    hdr->level--;
		    base_action = 0;
		};

		ShowWindow(hdr->kidwnd, SW_HIDE);
		SendMessage(hdr->kidwnd, WM_REFRESHITEMS, 0, 0);
		ShowWindow(hdr->kidwnd, SW_SHOW);
	    };
	    break;
/*
	case IDC_LAUNCHBOX_TABGENERIC2_BUTTON_MOREACTIONS:
	    {
		char **data = (char **) hdr->kiddata;

		ShowWindow(hdr->kidwnd, SW_HIDE);
//		hdr->kidwnd = hdr->kids[TAB_MOREACTIONS].hwnd;
//		hdr->subsubtype = hdr->kids[TAB_MOREACTIONS].type;
//		hdr->kidtype = hdr->kids[TAB_MOREACTIONS].type;
//		hdr->kiddata = (void *) data[ACTION_ACTION];
		hdr->level++;

		SendMessage(hdr->kidwnd, WM_REFRESHITEMS, 0, 0);
		ShowWindow(hdr->kidwnd, SW_SHOW);
	    };
	    break;
*/
	case IDC_LAUNCHBOX_TABGENERIC2_BUTTON_NEXT:
	    {
		hdr->level++;

		ShowWindow(hdr->kidwnd, SW_HIDE);
		SendMessage(hdr->kidwnd, WM_REFRESHITEMS, 0, 0);
		ShowWindow(hdr->kidwnd, SW_SHOW);
	    };
	    break;
	};
	break;
    };

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
    static unsigned int cancel_context_menu = FALSE;
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
	 * Hide and disable two groupboxes used for tree view and tab view
	 * placement.
	 */

	EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TREEVIEW),
		    FALSE);
	ShowWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TREEVIEW),
		   SW_HIDE);
	EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TABVIEW),
		    FALSE);
	ShowWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TABVIEW),
		   SW_HIDE);

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
				      (((config->options & OPTION_ENABLEDRAGDROP) &&
				       !config->sessionroot.readonly) ? 
					    0
					    : TVS_DISABLEDRAGDROP) |
				      TVS_HASBUTTONS | TVS_LINESATROOT |
				      TVS_SHOWSELALWAYS | 
				      (config->sessionroot.readonly ? 
					    0 :
					    TVS_EDITLABELS),
				      pt.x, pt.y, r.right - r.left,
				      r.bottom - r.top, hwnd, NULL,
				      config->hinst, NULL);
	    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
	    SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

	    if (config->image_list)
		TreeView_SetImageList(treeview, config->image_list,
				      TVSIL_NORMAL);
	}

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
	    memset(dlghdr, 0, sizeof(DLGHDR));
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

	    for (i = 0; i < TAB_CHILDDIALOGS; i++) {
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
		SendMessage(dlghdr->kids[i].hwnd, WM_ENABLEITEMS, (WPARAM) FALSE, 0);
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
	    void *handle;

	    if ((handle = reg_open_key_r(PLAUNCH_REGISTRY_ROOT)) &&
		reg_read_s(handle, PLAUNCH_SAVEDCURSORPOS, 
		NULL, curpos, BUFSIZE)) {
		HTREEITEM pos;

		reg_close_key(handle);
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
	    void *handle;

	    GetWindowRect(hwnd, &r1);
	    bigpt.x = r1.right - r1.left;
	    bigpt.y = r1.bottom - r1.top;
	    GetWindowRect(GetDlgItem(hwnd, IDC_LAUNCHBOX_STATIC_DIVIDER),
			  &r2);
	    smallpt.x = bigpt.x;
	    smallpt.y = r2.top - r1.top;
	    morestate = 1;
	    handle = reg_open_key_r(PLAUNCH_REGISTRY_ROOT);
	    reg_read_i(handle, PLAUNCH_SAVEMOREBUTTON, 0, &i);
	    reg_close_key(handle);
	    if (!i)
		SendMessage(hwnd, WM_COMMAND, IDC_LAUNCHBOX_BUTTON_MORE, 0);
	};

	center_window(hwnd);

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
	    char name[BUFSIZE], path[BUFSIZE];
	    BOOL rendel, edit, isfolder;
	    UINT menuflag;

	    item = (HTREEITEM)lParam;

	    if (!item && TreeView_GetCount(treeview))
		item = TreeView_GetSelection(treeview);

	    if (!item) {
		rendel = FALSE;
		edit = FALSE;
		menuflag = MF_GRAYED;

		EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
	    } else if (config->sessionroot.readonly) {
		EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_NEW_FOLDER), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_NEW_SESSION), FALSE);
		EnableMenuItem(context_menu, IDM_CTXM_CRTFLD, MF_GRAYED);
		EnableMenuItem(context_menu, IDM_CTXM_CRTSES, MF_GRAYED);

		rendel = FALSE;
		edit = FALSE;
		menuflag = MF_GRAYED;
	    } else {
		memset(name, 0, BUFSIZE);

		treeview_getitemname(treeview, item, name, BUFSIZE);
		treeview_getitempath(treeview, item, path, BUFSIZE);
		isfolder = ses_is_folder(&config->sessionroot, path);

		if (!name) {
		    rendel = FALSE;
		    edit = FALSE;
		    menuflag = MF_GRAYED;
		} else if (!strcmp(name, DEFAULTSETTINGS)) {
		    rendel = FALSE;
		    edit = TRUE;
		    menuflag = MF_GRAYED;
		} else {
		    rendel = TRUE;
		    edit = isfolder ? FALSE : TRUE;
		    menuflag = MF_ENABLED;
		};
	    };

	    EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
	    EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_EDIT), edit);
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
		SendMessage(dlghdr->kidwnd, WM_REFRESHITEMS, (WPARAM) TRUE, 0);
	    };
	};
	break;
    case WM_CONTEXTMENU:
	{
	    RECT rc;
	    POINT pt;
	    DWORD msg;
	    HTREEITEM item;

	    if (cancel_context_menu) {
		cancel_context_menu = FALSE;
		break;
	    };

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

		ImageList_DragMove(x - 32, y - 25);
		ImageList_DragShowNolock(FALSE);

		hit.pt.x = x - 20;
		hit.pt.y = y - 20;

		if ((target = TreeView_HitTest(treeview, &hit))) {
		    HTREEITEM visible;
		    RECT r;

//		    TreeView_SelectDropTarget(treeview, target);
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
			TreeView_EnsureVisible(treeview, visible);
//		    ImageList_DragShowNolock(FALSE);
//                                      TreeView_SetInsertMark(treeview, target, TRUE);
		    TreeView_SelectDropTarget(treeview, target);
//                                      TreeView_SelectItem(treeview, target);
//		    ImageList_DragShowNolock(TRUE);
		};
		ImageList_DragShowNolock(TRUE);
	    };
	}
	break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
	if (dragging_now) {
	    HTREEITEM target;

//                      TreeView_SetInsertMark(treeview, NULL, TRUE);
	    ImageList_DragLeave(treeview);
	    ImageList_EndDrag();
	    copying_now = dragging_now;
	    dragging_now = NULL;
	    ImageList_Destroy(draglist);
	    draglist = NULL;
	    ReleaseCapture();
	    ShowCursor(TRUE);

//	    hit.pt.x = LOWORD(lParam);
//	    hit.pt.y = HIWORD(lParam);

//	    if ((target = TreeView_HitTest(treeview, &hit))) {
	    if (target = TreeView_GetDropHilight(treeview)) {
		TreeView_SelectItem(treeview, target);
		TreeView_SelectDropTarget(treeview, NULL);
		switch (msg) {
		case WM_LBUTTONUP:
		    cut_or_copy = 1;	// cut;
		    break;
		case WM_RBUTTONUP:
		    {
//			TVHITTESTINFO hit;
			HMENU menu;
			RECT r;
			POINT pt;
			int ret, height, flags;

			cancel_context_menu = TRUE;

			menu = CreatePopupMenu();
			AppendMenu(menu, MF_STRING, IDCANCEL, "Cancel");
			AppendMenu(menu, MF_SEPARATOR, 0, 0);
			AppendMenu(menu, MF_STRING, IDM_CTXM_COPY,
				   "Copy here");
			AppendMenu(menu, MF_STRING, IDM_CTXM_CUT,
				   "Move here");

			GetClientRect(treeview, &r);
			height = TreeView_GetItemHeight(treeview);

			pt.x = LOWORD(lParam) - 20;
			pt.y = HIWORD(lParam) - 20;

			flags = TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_LEFTALIGN;
			if ((pt.y >= 0) && (pt.y <= height))
			    flags |= TPM_TOPALIGN;
			else if ((pt.y >= (r.bottom - height)) && (pt.y <= r.bottom))
			    flags |= TPM_BOTTOMALIGN;
			ClientToScreen(treeview, &pt);

			if (ret = TrackPopupMenu(menu, flags, pt.x, pt.y, 0,
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
		SendMessage(hwnd, WM_COMMAND, (WPARAM) IDM_CTXM_PASTE, 0);
		return FALSE;
	    };
	    return FALSE;
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
		
		{
		    void *handle;

		    handle = reg_open_key_w(PLAUNCH_REGISTRY_ROOT);
		    reg_write_i(handle, PLAUNCH_SAVEMOREBUTTON, morestate);
		    reg_close_key(handle);
		};
		
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
		    void *handle;

		    item = TreeView_GetSelection(treeview);
		    treeview_getitempath(treeview, item, path, BUFSIZE);
		    handle = reg_open_key_w(PLAUNCH_REGISTRY_ROOT);
		    reg_write_s(handle,	PLAUNCH_SAVEDCURSORPOS, path);
		    reg_close_key(handle);
		};

	    };
	case IDC_LAUNCHBOX_BUTTON_EDIT:
	    {
		HTREEITEM item;
		char path[BUFSIZE];

		item = TreeView_GetSelection(treeview);

		if (!treeview_getitempath(treeview, item, path, BUFSIZE)) {
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
		treeview_getitempath(treeview, item, path, BUFSIZE);

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
		    treeview_getitempath(treeview, tv_curitem, cur_path, BUFSIZE);
		} else {
		    tv_parent = NULL;
		    cur_path[0] = '\0';
		};

		if (cur_path[0] && ses_is_folder(&config->sessionroot, cur_path)) {
		    tv_parent = tv_curitem;
		    strcpy(parent_path, cur_path);
		} else if (tv_parent) {
		    tv_parent = tv_parent;
		    treeview_getitempath(treeview, tv_parent, parent_path, BUFSIZE);
		} else {
		    tv_parent = TVI_ROOT;
		    parent_path[0] = '\0';
		};

		if (cut_or_copy) {
		    treeview_getitemname(treeview, tv_copyitem, copy_name,
					 BUFSIZE);
		    treeview_getitempath(treeview, tv_copyitem, copy_path, BUFSIZE);
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
					     path, BUFSIZE);

			if (!config->sessionroot.readonly)
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
						 parent_path, BUFSIZE);
			else
			    parent_path[0] = '\0';

			treeview_getitempath(treeview, item, path, BUFSIZE);

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
//		    RECT r;
//		    POINT pt;
//		    DWORD indent;
		    LPNMTREEVIEW nmtv = (LPNMTREEVIEW) lParam;

		    draglist = TreeView_CreateDragImage(treeview, nmtv->itemNew.hItem);
//		    TreeView_GetItemRect(treeview, nmtv->itemNew.hItem, &r, TRUE);
//		    indent = TreeView_GetIndent(treeview);

		    ImageList_BeginDrag(draglist, 0, 0, 0);
//		    GetCursorPos(&pt);
//		    ScreenToClient(hwnd, &pt);
//		    ImageList_DragEnter(treeview, pt.x, pt.y);
		    ImageList_DragEnter(treeview, nmtv->ptDrag.x, nmtv->ptDrag.y);

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
		    dlghdr->kiddata = NULL;
		    dlghdr->level = 0;

		    SendMessage(dlghdr->kidwnd, WM_REFRESHITEMS, 0, 0);
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

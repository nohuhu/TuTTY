/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004-2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Miscellaneous functions implementation file.
 */

#include <stdio.h>
#include "entry.h"
#include "plaunch.h"
#include "misc.h"
#include "registry.h"
#include "hotkey.h"
#include "winmenu.h"

typedef BOOL(WINAPI * SHGIL_PROC) (HIMAGELIST * phLarge,
				   HIMAGELIST * phSmall);
typedef BOOL(WINAPI * FII_PROC) (BOOL fFullInit);

int GetSystemImageLists(HMODULE * hShell32, HIMAGELIST * phLarge,
			HIMAGELIST * phSmall)
{
    SHGIL_PROC Shell_GetImageLists;
    FII_PROC FileIconInit;

    if (phLarge == 0 || phSmall == 0)
	return FALSE;

    if (*hShell32 == NULL)
	*hShell32 = LoadLibrary("shell32.dll");

    if (*hShell32 == NULL)
	return FALSE;

    Shell_GetImageLists =
	(SHGIL_PROC) GetProcAddress(*hShell32, (LPCSTR) 71);
    FileIconInit = (FII_PROC) GetProcAddress(*hShell32, (LPCSTR) 660);

    // FreeIconList@8 = ord 227

    if (Shell_GetImageLists == 0) {
	FreeLibrary(*hShell32);
	*hShell32 = NULL;
	return FALSE;
    };

    // Initialize imagelist for this process - function not present on win95/98
    if (FileIconInit != 0)
	FileIconInit(TRUE);

    // Get handles to the large+small system image lists!
    Shell_GetImageLists(phLarge, phSmall);

    return TRUE;
}

void FreeSystemImageLists(HMODULE hShell32)
{
    FreeLibrary(hShell32);
    hShell32 = 0;
};

#define	ISFOLDER	"IsFolder"
#define	ISEXPANDED	"IsExpanded"

/*
 * Returns a freshly allocated copy of the string
 * passed to it. Caller is responsible of freeing it.
 */
char *dupstr(const char *s)
{
    char *p;
    int len;

    p = NULL;

    if (s) {
	len = strlen(s);
	p = malloc(len + 1);
	strcpy(p, s);
    }
    return p;
}

static char *firstname(char *path)
{
    char *p;

    p = path;

    while (*p != '\0' && *p != '\\')
	p++;

    return p;
};

HTREEITEM treeview_additem(HWND treeview, HTREEITEM parent,
			   void *scbt)
{
    HTREEITEM hti;
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    session_callback_t *scb = (session_callback_t *)scbt;

    if (!scb)
	return NULL;

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.pszText = scb->session->name;
    tvi.cchTextMax = strlen(scb->session->name);
    tvi.cChildren = scb->session->isfolder;

    if (tvi.cChildren) {
	tvi.iImage = config->img_closed;
	tvi.iSelectedImage = config->img_closed;
    } else {
	char sesicon[256];
	HICON icon;
	int iindex;

	icon = NULL;
	iindex = 0;

	if (ses_read_s(&scb->session->root, scb->session->path, SESSIONICON, 
	    "", sesicon, 255) && sesicon[0]) {
	    char iname[256], *comma;

	    iname[0] = '\0';

	    comma = strrchr(sesicon, ',');
	    if (comma) {
		comma[0] = '\0';
		*comma++;
		iindex = atoi(comma);
		icon = ExtractIcon(config->hinst, sesicon, iindex);

		if (icon) {
		    iindex = ImageList_AddIcon(config->image_list, icon);
		    DestroyIcon(icon);
		};
	    };
	};
	if (iindex) {
	    tvi.iImage = iindex;
	    tvi.iSelectedImage = iindex;
	} else {
	    tvi.iImage = config->img_session;
	    tvi.iSelectedImage = config->img_session;
	};
    };

    tvins.item = tvi;
    tvins.hParent = parent;
    tvins.hInsertAfter = TVI_LAST;

    hti = TreeView_InsertItem(treeview, &tvins);

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = hti;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;
    tvi.lParam = (LPARAM) hti;
    TreeView_SetItem(treeview, &tvi);

    return hti;
};

static void treeview_callback(session_callback_t *scb)
{
    HWND treeview;
    HTREEITEM item, newitem;
    int isexpanded;

    if (!scb)
	return;

    treeview = (HWND) scb->public2;

    item = scb->protected1 ? 
	(HTREEITEM) scb->protected1 : 
	(HTREEITEM) scb->public1;

    switch (scb->mode) {
    case SES_MODE_PREPROCESS:
	newitem = treeview_additem(treeview, item, scb);

	if (scb->session->isfolder)
	    scb->protected1 = (void *) newitem;

	break;
    case SES_MODE_POSTPROCESS:
	{
	    if (!scb->session->isfolder)
		return;

	    ses_read_i(&scb->session->root, scb->session->path, 
		ISEXPANDED, 0, &isexpanded);

	    if (scb->session->isfolder && isexpanded)
		TreeView_Expand(treeview, item, TVE_EXPAND);
	};
    };
    return;
};

HTREEITEM treeview_addtree(HWND hwndTV, HTREEITEM _parent, char *root)
{
    session_walk_t sw;

    memset(&sw, 0, sizeof(sw));
    sw.root = config->sessionroot;
    sw.root_path = root;
    sw.depth = SES_MAX_DEPTH;
    sw.callback = treeview_callback;
    sw.public1 = _parent;
    sw.public2 = hwndTV;

    ses_walk_over_tree(&sw);

    return _parent;
};

unsigned int treeview_getitemname(HWND treeview, HTREEITEM item, char *buf,
				  unsigned int bufsize)
{
    TVITEM tvi;

    if (!item)
	return FALSE;

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = item;
    tvi.pszText = buf;
    tvi.cchTextMax = bufsize;
    TreeView_GetItem(treeview, &tvi);

    return TRUE;
};

unsigned int treeview_getitempath(HWND treeview, HTREEITEM item, char *buf,
				  unsigned int bufsize)
{
    char buf2[BUFSIZE], buf3[BUFSIZE];
    HTREEITEM parent, curitem;

    if (!item)
	return FALSE;

    treeview_getitemname(treeview, item, buf3, BUFSIZE);
    strncpy(buf, buf3, bufsize);
    curitem = item;

    while (parent = TreeView_GetParent(treeview, curitem)) {
	treeview_getitemname(treeview, parent, buf3, BUFSIZE);
	strncpy(buf2, buf, BUFSIZE);
	_snprintf(buf, bufsize, "%s\\%s", buf3, buf2);
	curitem = parent;
    };

    return TRUE;
};

static HTREEITEM _treeview_getitemfrompath(HWND treeview, HTREEITEM parent,
					   char *path)
{
    char *fname, name[BUFSIZE], iname[BUFSIZE];
    HTREEITEM child;

    if (!path || path[0] == '\0')
	return parent;

    memset(name, 0, BUFSIZE);
    fname = firstname(path);
    memmove(name, path, fname - path);
    child = TreeView_GetChild(treeview, parent);

    if (!child)
	return parent;

    do {
	treeview_getitemname(treeview, child, iname, BUFSIZE);
	if (!strcmp(name, iname)) {
	    if (!fname[0])
		return child;
	    else
		return _treeview_getitemfrompath(treeview, child,
						 fname + 1);
	};
    } while (child = TreeView_GetNextSibling(treeview, child));

    return NULL;
};

HTREEITEM treeview_getitemfrompath(HWND treeview, char *path)
{
    return _treeview_getitemfrompath(treeview, TVI_ROOT, path);
};

extern BOOL CALLBACK CountPuTTYWindows(HWND hwnd, LPARAM lParam);
extern BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam);
extern unsigned int nhandles;

HMENU menu_addrunning(HMENU menu)
{
    unsigned int i, id;
    struct windowlist wl;
//    char buf[BUFSIZE], buf2[BUFSIZE], h;

    if (!menu)
	return menu;

    memset(&wl, 0, sizeof(struct windowlist));

    EnumWindows(CountPuTTYWindows, (LPARAM)&wl.nhandles);

    if (!wl.nhandles) {
	AppendMenu(menu, MF_STRING | MF_GRAYED, IDM_EMPTY,
		   "(No running instances)");
	return menu;
    };

    wl.handles = (HWND *) malloc(wl.nhandles * sizeof(HWND));

    EnumWindows(EnumPuTTYWindows, (LPARAM) & wl);

    for (i = 0; i < wl.nhandles; i++) {
	id = IDM_RUNNING_BASE + i;

	if (id > IDM_RUNNING_MAX)
	    break;

	if (wl.handles[i]) {
/*
	    if (IsWindowVisible(wl.handles[i]))
		h = 'v';
	    else
		h = 'h';
	    GetWindowText(wl.handles[i], buf, BUFSIZE);
	    sprintf(buf2, "[%c] %s", h, buf);
	    AppendMenu(menu, MF_STRING,
		       IDM_RUNNING_BASE + (UINT) wl.handles[i], buf2);
*/
	    AppendMenu(menu, MF_OWNERDRAW, id, (LPCTSTR) wl.handles[i]);
	};
    };

    free(wl.handles);

    return menu;
};

static void menu_free(HMENU menu)
{
    int i;
    MENUITEMINFO mii;
    HMENU submenu;

    for (i = GetMenuItemCount(menu); i > 0; i--) {
	submenu = GetSubMenu(menu, 0);
	memset(&mii, 0, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_DATA | MIIM_ID;
	if (GetMenuItemInfo(menu, 0, TRUE, &mii) &&
	    !(mii.wID >= IDM_RUNNING_BASE && mii.wID <= IDM_RUNNING_MAX) &&
	    mii.dwItemData != 0)
	    free((char *) mii.dwItemData);
	DeleteMenu(menu, 0, MF_BYPOSITION);
	if (submenu) {
	    menu_free(submenu);
	    DestroyMenu(submenu);
	};
    };
};

HMENU menu_refresh(HMENU menu, char *root)
{
    HMENU m;

    m = CreatePopupMenu();
//      AppendMenu(m, MF_ENABLED, IDM_BACKREST, "&Backup/Restore...");
    AppendMenu(m, MF_ENABLED, IDM_LAUNCHBOX, "&Session Manager...");
    AppendMenu(m, MF_ENABLED, IDM_WINDOWLIST, "&Running Sessions...");
    AppendMenu(m, MF_ENABLED, IDM_OPTIONSBOX, "&Options...");
    AppendMenu(m, MF_ENABLED, IDM_ABOUT, "&About...");

    menu_free(menu);
    if (config->options & OPTION_MENUSESSIONS) {
	HMENU sessions;

	sessions = CreatePopupMenu();
	sessions = menu_addsession(sessions, root);
	AppendMenu(menu, MF_POPUP, (UINT) sessions, "&Saved sessions");
    } else
	menu = menu_addsession(menu, root);
    AppendMenu(menu, MF_SEPARATOR, 0, 0);
    if (config->options & OPTION_MENURUNNING) {
	HMENU running;

	running = CreatePopupMenu();
	running = menu_addrunning(running);
	AppendMenu(menu, MF_POPUP, (UINT) running, "&Running sessions");
    } else
	menu = menu_addrunning(menu);
    AppendMenu(menu, MF_SEPARATOR, 0, 0);
    AppendMenu(menu, MF_POPUP, (UINT) m, "&More");
#ifdef WINDOWS_NT351_COMPATIBLE
    if (config->have_shell)
#endif				/* WINDOWS_NT351_COMPATIBLE */
	AppendMenu(menu, MF_ENABLED, IDM_CLOSE, "E&xit");

    return menu;
};

/*
 * This function tries to find a full path to putty.exe by
 * checking, in the order to follow:
 * 1. The directory from which plaunch was started.
 * 2. The current directory.
 * 3. The system directory (usually C:\WINNT\system32 for NT/2k
 *    and C:\WINDOWS\system32 for 95/98/Me/XP).
 * 4. The 16-bit system directory, which is C:\WIN(NT|DOWS)\system.
 * 5. The Windows directory, C:\WIN(NT|DOWS).
 * 6. The directories listed in the PATH environment variable.
 * 7. Well-known directories like C:\Program Files\PuTTY or
 *    C:\Program Files (x86)\TuTTY or a combination.
 * If search was unsuccessful, it tries the same steps for 
 * puttytel.exe. If this search is also unsuccessful, it simply 
 * returns NULL and relies on the user to set the correct path 
 * in the options dialog.
 * UPDATE: now this function first tries to find tutty.exe or
 * tuttytel.exe, then putty.exe or puttytel.exe.
 */
unsigned int get_putty_path(char *buf, unsigned int bufsize)
{
    char *exe[4] = {"tutty.exe", "tuttytel.exe", "putty.exe", "puttytel.exe"};
    int i;

    for (i = 0; i < 4; i++) {
	if (SearchPath(NULL, exe[i], NULL, bufsize, buf, NULL) ||
	    SearchPath("C:\\Program Files (x86)\\PuTTY;"
	    	       "C:\\Program Files (x86)\\TuTTY;"
		       "C:\\Program Files\\PuTTY;"
		       "C:\\Program Files\\TuTTY", 
		       exe[i], NULL, bufsize, buf, NULL))
	    return TRUE;
    };

    return FALSE;
};

static void hotkeys_callback(session_callback_t *scb)
{
    char buf[BUFSIZE];
    int i, hotkey;
    Config *cfg = (Config *) scb->public1;

    if (!scb)
	return;

    if (scb->session->isfolder || 
	scb->mode == SES_MODE_POSTPROCESS)
	return;

    for (i = 0; i < 3; i++) {
	sprintf(buf, "%s%d", HOTKEY, i);
	if (ses_read_i(&scb->session->root, scb->session->path, 
	    buf, 0, &hotkey) && hotkey) {
	    cfg->hotkeys[cfg->nhotkeys].action = i + HOTKEY_ACTION_LAUNCH;
	    cfg->hotkeys[cfg->nhotkeys].hotkey = hotkey;
	    cfg->hotkeys[cfg->nhotkeys].destination = 
		dupstr(scb->session->path);
	    cfg->nhotkeys++;
	};
    };

    return;
};

static int extract_hotkeys(Config *cfg, char *root)
{
    session_walk_t sw;

    memset(&sw, 0, sizeof(sw));
    sw.root = cfg->sessionroot;
    sw.root_path = root;
    sw.depth = SES_MAX_DEPTH;
    sw.callback = hotkeys_callback;
    sw.public1 = cfg;
    
    ses_walk_over_tree(&sw);

    return TRUE;
};

static void launching_callback(session_callback_t *scb)
{
    int when;
    char **when_strings[AUTORUN_WHEN_MAX] = {
	ATSTART_STRINGS,
	ATNETWORKUP_STRINGS,
	ATNETWORKDOWN_STRINGS,
	ATSTOP_STRINGS,
    };

    if (!scb)
	return;

    if (scb->session->isfolder)
	return;

    when = (int) scb->public1;

    if (scb->mode == SES_MODE_POSTPROCESS)
	return;

    work_over_actions(config, scb->session->path, (when_strings[when]));

    return;
};

int launch_autoruns(char *root, int when)
{
    session_walk_t sw;

    memset(&sw, 0, sizeof(sw));
    sw.root = config->sessionroot;
    sw.root_path = root;
    sw.depth = SES_MAX_DEPTH;
    sw.callback = launching_callback;
    sw.public1 = (void *) when;

    ses_walk_over_tree(&sw);

    return TRUE;
};

/*
static void sync_callback(session_callback_t *scb)
{
    session_root_t *to = (session_root_t *) scb->public1;
    int i, max, type, ivalue;
    char name[BUFSIZE], svalue[BUFSIZE];
    void *sfrom, *sto, *en;

    if ((scb->mode == SES_MODE_PREPROCESS &&
	!scb->session->isfolder) ||
	(scb->mode == SES_MODE_POSTPROCESS &&
	scb->session->isfolder))
	return;

    sfrom = ses_open_session_r(&scb->root, scb->session->path);
    sto = ses_open_session_w(to, scb->session->path);

    if (!sfrom || !sto)
	return;

    en = ses_enum_values_start(&scb->root, sfrom);

    if (!en)
	return;

    max = ses_enum_values_count(&scb->root, en);

    for (i = 0; i < max; i++) {
	if (ses_enum_values_next(&scb->root, en, name, BUFSIZE)) {
	    type = ses_enum_values_type(&scb->root, en);
	    switch (type) {
	    case SES_VALUE_INTEGER:
		{
		    ivalue = 0;
		    ses_read_handle_i(&scb->root, sfrom, name, 0, &ivalue);
		    ses_write_handle_i(to, sto, name, ivalue);
		};
		break;
	    case SES_VALUE_STRING:
		{
		    memset(svalue, 0, BUFSIZE);
		    ses_read_handle_s(&scb->root, sfrom, name, "", svalue, BUFSIZE);
		    ses_write_handle_s(to, sto, name, svalue);
		};
		break;
	    };
	};
    };

    ses_close_session(&scb->root, sfrom);
    ses_close_session(to, sto);
};

int sync_session_roots(session_root_t *from, session_root_t *to)
{
    session_walk_t sw;

    memset(&sw, 0, sizeof(sw));
    sw.root = *from;
    sw.root_path = "";
    sw.depth = SES_MAX_DEPTH;
    sw.callback = sync_callback;
    sw.public1 = (void *) to;

    ses_walk_over_tree(&sw);

    return TRUE;
};
*/

unsigned int read_config(Config *cfg)
{
    char buf[BUFSIZE];
    int hk;
    void *handle;

    if (!cfg)
	return FALSE;

    handle = reg_open_key_r(PLAUNCH_REGISTRY_ROOT);

    if (reg_read_s(handle, PLAUNCH_PUTTY_PATH, NULL, buf, BUFSIZE))
	strcpy(config->putty_path, buf);
    else
	get_putty_path(config->putty_path, BUFSIZE);

    reg_read_i(handle, PLAUNCH_HOTKEY_LB, 0, &hk);

    if (hk == 0)
	hk = HOTKEY_DEFAULT_LAUNCHBOX;

    config->hotkeys[HOTKEY_LAUNCHBOX].action = HOTKEY_ACTION_MESSAGE;
    config->hotkeys[HOTKEY_LAUNCHBOX].hotkey = hk;
    config->hotkeys[HOTKEY_LAUNCHBOX].destination = (char *) WM_LAUNCHBOX;

    reg_read_i(handle, PLAUNCH_HOTKEY_WL, 0, &hk);

    if (hk == 0)
	hk = HOTKEY_DEFAULT_WINDOWLIST;

    config->hotkeys[HOTKEY_WINDOWLIST].action = HOTKEY_ACTION_MESSAGE;
    config->hotkeys[HOTKEY_WINDOWLIST].hotkey = hk;
    config->hotkeys[HOTKEY_WINDOWLIST].destination = (char *) WM_WINDOWLIST;

    reg_read_i(handle, PLAUNCH_HOTKEY_HIDEWINDOW, 0, &hk);

    if (hk == 0)
	hk = HOTKEY_DEFAULT_HIDEWINDOW;

    config->hotkeys[HOTKEY_HIDEWINDOW].action = HOTKEY_ACTION_MESSAGE;
    config->hotkeys[HOTKEY_HIDEWINDOW].hotkey = hk;
    config->hotkeys[HOTKEY_HIDEWINDOW].destination = (char *) WM_HIDEWINDOW;

    reg_read_i(handle, PLAUNCH_HOTKEY_CYCLEWINDOW, 0, &hk);

    if (hk == 0)
	hk = HOTKEY_DEFAULT_CYCLEWINDOW;

    config->hotkeys[HOTKEY_CYCLEWINDOW].action = HOTKEY_ACTION_MESSAGE;
    config->hotkeys[HOTKEY_CYCLEWINDOW].hotkey = hk;
    config->hotkeys[HOTKEY_CYCLEWINDOW].destination = (char *) WM_CYCLEWINDOW;

    config->nhotkeys = 4;

    reg_read_i(handle, PLAUNCH_ENABLEDRAGDROP, TRUE, &hk);
    if (hk)
	config->options |= OPTION_ENABLEDRAGDROP;

    reg_read_i(handle, PLAUNCH_ENABLESAVECURSOR, TRUE, &hk);
    if (hk)
	config->options |= OPTION_ENABLESAVECURSOR;

    reg_read_i(handle, PLAUNCH_SHOWONQUIT, TRUE, &hk);
    if (hk)
	config->options |= OPTION_SHOWONQUIT;

    reg_read_i(handle, PLAUNCH_MENUSESSIONS, FALSE, &hk);
    if (hk)
	config->options |= OPTION_MENUSESSIONS;

    reg_read_i(handle, PLAUNCH_MENURUNNING, FALSE, &hk);
    if (hk)
	config->options |= OPTION_MENURUNNING;

    reg_close_key(handle);

    extract_hotkeys(cfg, "");

    return TRUE;
};

unsigned int save_config(Config *cfg, int what)
{
    char buf[BUFSIZE];
    void *handle;

    if (!cfg || !what)
	return FALSE;

    handle = reg_open_key_w(PLAUNCH_REGISTRY_ROOT);

    if (what & CFG_SAVE_PUTTY_PATH) {
	get_putty_path(buf, BUFSIZE);
	if (!strcmp(buf, config->putty_path))
	    reg_delete_v(handle, PLAUNCH_PUTTY_PATH);
	else
	    reg_write_s(handle, PLAUNCH_PUTTY_PATH,
			config->putty_path);
    };

    if (what & CFG_SAVE_HOTKEY_LB) {
	if (config->hotkeys[HOTKEY_LAUNCHBOX].hotkey == HOTKEY_DEFAULT_LAUNCHBOX)
	    reg_delete_v(handle, PLAUNCH_HOTKEY_LB);
	else
	    reg_write_i(handle, PLAUNCH_HOTKEY_LB,
			config->hotkeys[HOTKEY_LAUNCHBOX].hotkey);
    };

    if (what & CFG_SAVE_HOTKEY_WL) {
	if (config->hotkeys[HOTKEY_WINDOWLIST].hotkey == HOTKEY_DEFAULT_WINDOWLIST)
	    reg_delete_v(handle, PLAUNCH_HOTKEY_WL);
	else
	    reg_write_i(handle, PLAUNCH_HOTKEY_WL,
			config->hotkeys[HOTKEY_WINDOWLIST].hotkey);
    };

    if (what & CFG_SAVE_HOTKEY_HIDEWINDOW) {
	if (config->hotkeys[HOTKEY_HIDEWINDOW].hotkey == HOTKEY_DEFAULT_HIDEWINDOW)
	    reg_delete_v(handle, PLAUNCH_HOTKEY_HIDEWINDOW);
	else
	    reg_write_i(handle, PLAUNCH_HOTKEY_HIDEWINDOW,
			config->hotkeys[HOTKEY_HIDEWINDOW].hotkey);
    };

    if (what & CFG_SAVE_HOTKEY_CYCLEWINDOW) {
	if (config->hotkeys[HOTKEY_CYCLEWINDOW].hotkey == HOTKEY_DEFAULT_CYCLEWINDOW)
	    reg_delete_v(handle, PLAUNCH_HOTKEY_CYCLEWINDOW);
	else
	    reg_write_i(handle, PLAUNCH_HOTKEY_CYCLEWINDOW,
			config->hotkeys[HOTKEY_CYCLEWINDOW].hotkey);
    };

    if (what & CFG_SAVE_DRAGDROP) {
	if (config->options & OPTION_ENABLEDRAGDROP)
	    reg_delete_v(handle, PLAUNCH_ENABLEDRAGDROP);
	else
	    reg_write_i(handle, PLAUNCH_ENABLEDRAGDROP, 0);
    };

    if (what & CFG_SAVE_SAVECURSOR) {
	if (config->options & OPTION_ENABLESAVECURSOR)
	    reg_delete_v(handle, PLAUNCH_ENABLESAVECURSOR);
	else
	    reg_write_i(handle, PLAUNCH_ENABLESAVECURSOR, 0);
    };

    if (what & CFG_SAVE_SHOWONQUIT) {
	if (config->options & OPTION_SHOWONQUIT)
	    reg_delete_v(handle, PLAUNCH_SHOWONQUIT);
	else
	    reg_write_i(handle, PLAUNCH_SHOWONQUIT, 0);
    };

    if (what & CFG_SAVE_MENUSESSIONS) {
	if (config->options & OPTION_MENUSESSIONS)
	    reg_write_i(handle, PLAUNCH_MENUSESSIONS, 1);
	else
	    reg_delete_v(handle, PLAUNCH_MENUSESSIONS);
    };

    if (what & CFG_SAVE_MENURUNNING) {
	if (config->options & OPTION_MENURUNNING)
	    reg_write_i(handle, PLAUNCH_MENURUNNING, 1);
	else
	    reg_delete_v(handle, PLAUNCH_MENURUNNING);
    };

    reg_close_key(handle);

    return TRUE;
};

void center_window(HWND window)
{
    HWND parent;
    RECT r, rp, rw;

    parent = GetParent(window);

    if (parent == NULL)
	parent = GetDesktopWindow();

    GetWindowRect(parent, &rp);
    GetWindowRect(window, &rw);
    CopyRect(&r, &rp);

    OffsetRect(&rw, -rw.left, -rw.top);
    OffsetRect(&r, -r.left, -r.top);
    OffsetRect(&r, -rw.right, -rw.bottom);

    SetWindowPos(window, HWND_TOP,
		 rp.left + (r.right / 2),
		 rp.top + (r.bottom / 2), 0, 0, SWP_NOSIZE);
};

void item_insert(void **array, int *anum, void *item)
{
    DWORD *ar = (DWORD *) * array, *tmp;

    if (ar == NULL) {
	ar = (DWORD *) malloc(sizeof(DWORD));
	ar[0] = (DWORD) item;
	*array = ar;
	*anum = 1;
	return;
    };

    tmp = (DWORD *) malloc((*anum + 1) * sizeof(DWORD));
    memmove(tmp, ar, *anum * sizeof(DWORD));
    tmp[*anum] = (DWORD) item;
    (*anum)++;
    free(ar);
    *array = tmp;

    return;
};

int item_find(void *array, int anum, void *item)
{
    DWORD *ar = (DWORD *) array;
    int i;

    if (!array || !item)
	return -1;

    for (i = 0; i < anum; i++) {
	if (ar[i] == (DWORD) item)
	    return i;
    };

    return -1;
};

void item_remove(void **array, int *anum, void *item)
{
    int i, j, pos;
    DWORD *ar = (DWORD *) * array, *tmp, *tmp2, *tmp3;

    j = *anum;

    if (!*array || !j || !item)
	return;

    pos = -1;
    for (i = 0; i < j; i++) {
	if (ar[i] == (DWORD) item) {
	    pos = i;
	    break;
	};
    };

    if (pos < 0)
	return;

    j--;
    tmp = (DWORD *) malloc(j * sizeof(DWORD));

    if (pos == j) {
	if (j)
	    memmove(tmp, ar, j * sizeof(DWORD));
	else
	    tmp = NULL;
    } else if (pos == 0) {
	tmp2 = ar + 1;
	memmove(tmp, tmp2, j * sizeof(DWORD));
    } else {
	memmove(tmp, ar, pos * sizeof(DWORD));
	tmp2 = tmp + pos;
	tmp3 = ar + pos + 1;
	memmove(tmp2, tmp3, (j - pos) * sizeof(DWORD));
    };

    free(ar);
    *array = tmp;
    *anum = j;

    return;
};

static BOOL CALLBACK FindPuttyWindowCallback(HWND hwnd, LPARAM lParam)
{
    char classname[BUFSIZE];

    if (GetClassName(hwnd, classname, BUFSIZE) &&
	(!strcmp(classname, PUTTY) ||
	 !strcmp(classname, TUTTY) ||
	 !strcmp(classname, PUTTYTEL) ||
	 !strcmp(classname, TUTTYTEL))) {
	*((HWND *) lParam) = hwnd;
	return TRUE;
    };

    return FALSE;
};

unsigned int nprocesses = 0;
HANDLE *process_handles = NULL;
struct process_record **process_records = NULL;

#define	LAUNCH_TIMEOUT	10000

HWND launch_putty(int action, char *path)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD wait;
    HWND pwin;
    struct process_record *pr;
    char buf[BUFSIZE];

    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    memset(&pi, 0, sizeof(PROCESS_INFORMATION));

    sprintf(buf, "%s -%s \"%s\"", config->putty_path,
	    action ? "edit" : "load", path);

    if (!CreateProcess(config->putty_path, buf, NULL, NULL,
		       FALSE, 0, NULL, NULL, &si, &pi))
	return NULL;

    wait = WaitForInputIdle(pi.hProcess, LAUNCH_TIMEOUT);

    if (wait != 0) {
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return NULL;
    };

    CloseHandle(pi.hThread);

    pwin = NULL;
    EnumThreadWindows(pi.dwThreadId, FindPuttyWindowCallback, (LPARAM) &pwin);

    if (!pwin) {
	CloseHandle(pi.hProcess);
	return NULL;
    };

    pr = (struct process_record *) malloc(sizeof(struct process_record));
    pr->pid = pi.dwProcessId;
    pr->tid = pi.dwThreadId;
    pr->hprocess = pi.hProcess;
    pr->window = pwin;
    pr->path = dupstr(path);

    item_insert((void *) &process_handles, &nprocesses, pi.hProcess);
    nprocesses -= 1;
    item_insert((void *) &process_records, &nprocesses, pr);

    return pwin;
};

void free_process_records(void)
{
    unsigned int i;

    if (!process_handles)
	free(process_handles);

    if (!process_records) {
	for (i = 0; i < nprocesses; i++) {
	    if (process_records[i]->path)
		free(process_records[i]->path);
	    free(process_records[i]);
	};
	free(process_records);
    };
};

int enum_process_records(void **array, int nrecords, char *path)
{
    struct process_record **records = (struct process_record **) array;
    int i, count = 0;

    for (i = 0; i < nrecords; i++) {
	if (records[i]->path && !strcmp(records[i]->path, path))
	    count++;
    };

    return count;
};

void *get_nth_process_record(void **array, int nrecords, char *path, int n)
{
    struct process_record **records = (struct process_record **) array;
    int i, count = 0;

    if (n == 0) {
	for (i = nrecords - 1; i >= 0; i--) {
	    if (records[i]->path && !strcmp(records[i]->path, path))
		return records[i];
	};
    } else {
	for (i = 0; i < nrecords; i++) {
	    if (records[i]->path && !strcmp(records[i]->path, path)) {
		count++;

		if (count == n)
		    return records[i];
	    };
	};
    };

    return NULL;
};

void *get_process_record_by_window(void **array, int nrecords, HWND window)
{
    struct process_record **records = (struct process_record **) array;
    struct process_record *pr;
    int i;

    for (i = 0; i < nrecords; i++) {
	pr = records[i];
	if (pr && (pr->window == window))
	    return pr;
    };

    return NULL;
};

int work_over_actions(struct _config *cfg, char *path, char *strings[3])
{
    int i = 0, enable = TRUE, searchfor = 0, action = 0;
    struct process_record *pr = NULL;
    char buf[BUFSIZE];

    if (strings[0])
	ses_read_i(&cfg->sessionroot, path, strings[0], 0, &enable);

    if (!enable)
	return 0;

    if (!ses_read_i(&cfg->sessionroot, path, strings[1], 0, &searchfor))
	return 0;

    pr = get_nth_process_record(process_records, nprocesses, path, searchfor);

    i = 0; action = 0;
    sprintf(buf, strings[2], i);

    while (ses_read_i(&cfg->sessionroot, path, buf, 0, &action)) {
	switch (action) {
	case SESSION_ACTION_HIDE:
	    if (pr && pr->window)
		ShowWindow(pr->window, SW_HIDE);
	    break;
	case SESSION_ACTION_SHOW:
	    if (pr && pr->window) {
		ShowWindow(pr->window, /*SW_SHOWNORMAL*/ SW_SHOW);
		SetForegroundWindow(pr->window);
	    };
	    break;
	case SESSION_ACTION_MINIMIZE:
	    if (pr && pr->window)
		ShowWindow(pr->window, SW_MINIMIZE);
	    break;
	case SESSION_ACTION_MAXIMIZE:
	    if (pr && pr->window)
		ShowWindow(pr->window, SW_MAXIMIZE);
	    break;
	case SESSION_ACTION_CENTER:
	    if (pr && pr->window)
		center_window(pr->window);
	    break;
	case SESSION_ACTION_KILL:
	    if (pr && pr->window)
		SendMessage(pr->window, WM_CLOSE, 0, 0);
	    break;
	case SESSION_ACTION_MURDER:
	    if (pr)
		TerminateProcess(pr->hprocess, 0);
	    break;
	case SESSION_ACTION_RUN:
	    launch_putty(0, path);
	    break;
	};

	i++;
	sprintf(buf, strings[2], i);
    };

    return i;
};

extern BOOL CALLBACK CountPuTTYWindows(HWND hwnd, LPARAM lParam);
extern BOOL CALLBACK EnumPuTTYWindows(HWND hwnd, LPARAM lParam);

int find_existing_processes(void)
{
    struct windowlist *wl = NULL;
    struct process_record *pr = NULL;
    unsigned int i, count;

    wl = (struct windowlist *) malloc(sizeof(struct windowlist));
    memset(wl, 0, sizeof(struct windowlist));

    EnumWindows(CountPuTTYWindows, (LPARAM) &wl->nhandles);
    count = wl->nhandles;

    if (wl->nhandles == 0)
	wl->handles = NULL;
    else {
	wl->handles = (HWND *) malloc(wl->nhandles * sizeof(HWND));
	memset(wl->handles, 0, wl->nhandles * sizeof(HWND));

	EnumWindows(EnumPuTTYWindows, (LPARAM) wl);

	for (i = 0; i < wl->nhandles; i++) {
	    DWORD pid = 0, tid = 0;
	    HANDLE hprocess;

	    tid = GetWindowThreadProcessId(wl->handles[i], &pid);
	    hprocess = OpenProcess(PROCESS_QUERY_INFORMATION | 
				   PROCESS_TERMINATE | 
				   PROCESS_VM_READ |
				   SYNCHRONIZE, FALSE, pid);

	    if (!pid || !tid || !hprocess)
		continue;

	    pr = (struct process_record *) malloc(sizeof(struct process_record));
	    pr->pid = pid;
	    pr->tid = tid;
	    pr->hprocess = hprocess;
	    pr->window = wl->handles[i];
	    pr->path = NULL;

	    item_insert((void *) &process_handles, &nprocesses, hprocess);
	    nprocesses -= 1;
	    item_insert((void *) &process_records, &nprocesses, pr);
	};

	free(wl->handles);
    };

    free(wl);

    return count;
};

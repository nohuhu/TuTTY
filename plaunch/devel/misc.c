#include "plaunch.h"
#include "misc.h"
#include "registry.h"

typedef BOOL (WINAPI * SHGIL_PROC)	(HIMAGELIST *phLarge, HIMAGELIST *phSmall);
typedef BOOL (WINAPI * FII_PROC)	(BOOL fFullInit);

int GetSystemImageLists(HMODULE hShell32, HIMAGELIST *phLarge, HIMAGELIST *phSmall) {
    SHGIL_PROC	Shell_GetImageLists;
    FII_PROC	FileIconInit;

    if (phLarge == 0 || phSmall == 0)
		return FALSE;

    if (hShell32 == 0)
		hShell32 = LoadLibrary("shell32.dll");

    if (hShell32 == 0)
		return FALSE;

    Shell_GetImageLists = (SHGIL_PROC)  GetProcAddress(hShell32, (LPCSTR)71);
    FileIconInit	    = (FII_PROC)    GetProcAddress(hShell32, (LPCSTR)660);

    // FreeIconList@8 = ord 227

    if (Shell_GetImageLists == 0) {
		FreeLibrary(hShell32);
		return FALSE;
    };

    // Initialize imagelist for this process - function not present on win95/98
    if (FileIconInit != 0)
		FileIconInit(TRUE);

    // Get handles to the large+small system image lists!
    Shell_GetImageLists(phLarge, phSmall);

    return TRUE;
}

void FreeSystemImageLists(HMODULE hShell32) {
    FreeLibrary(hShell32);
    hShell32 = 0;
};

#define	ISFOLDER	"IsFolder"
#define	ISEXPANDED	"IsExpanded"

char *lastname(char *in) {
    char *p;

    if (in == "")
		return in;
    
    p = &in[strlen(in)];

    while (p >= in && *p != '\\')
		*p--;

    return p + 1;
};

HTREEITEM treeview_additem(HWND treeview, HTREEITEM parent, struct _config *cfg,
						   char *name, int isfolder) {
	HTREEITEM hti;
	TVITEM tvi;
	TVINSERTSTRUCT tvins;

	tvi.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.pszText = name; 
	tvi.cchTextMax = strlen(name); 
	tvi.cChildren = isfolder;

	if (tvi.cChildren) {
		tvi.iImage = cfg->img_closed;
		tvi.iSelectedImage = cfg->img_closed;
	} else {
		tvi.iImage = cfg->img_session;
		tvi.iSelectedImage = cfg->img_session;
	};

	tvins.item = tvi; 
	tvins.hParent = parent; 
	tvins.hInsertAfter = TVI_FIRST;

	hti = TreeView_InsertItem(treeview, &tvins);

	memset(&tvi, 0, sizeof(TVITEM));
	tvi.hItem = hti;
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.lParam = (LPARAM)hti;
	TreeView_SetItem(treeview, &tvi);

	return hti;
};

int treeview_callback(char *name, char *path, 
					  int isfolder, int mode, 
					  void *priv1, void *priv2, void *priv3) {
	HWND treeview;
	HTREEITEM hti;
	int isexpanded;
	char *buf;

	switch (mode) {
	case REG_MODE_PREPROCESS:
		return (int)treeview_additem((HWND)priv2, (HTREEITEM)priv1,
				(struct _config *)priv3, name, isfolder);
	case REG_MODE_POSTPROCESS:
		{
			int ret;

			if (!isfolder)
				return 0;

			hti = (HTREEITEM)priv1;
			treeview = (HWND)priv2;

			buf = (char *)malloc(BUFSIZE);
			sprintf(buf, "%s\\%s", REGROOT, path);
			isexpanded = reg_read_i(buf, ISEXPANDED, 0);
			free(buf);

			if (isfolder && isexpanded)
				ret = TreeView_Expand(treeview, hti, TVE_EXPAND);

			return (int)hti;
		};
	};
	return 0;
};

HTREEITEM treeview_addtree(HWND hwndTV, HTREEITEM _parent, char *root) {
	reg_walk_over_tree(root, treeview_callback, _parent, hwndTV, config);

	return _parent;
};

char *treeview_getitemname(HWND treeview, HTREEITEM item) {
	TVITEM tvi;
	char *buf, *name;

	if (!item)
		return NULL;

	buf = (char *)malloc(BUFSIZE);
	memset(buf, 0, BUFSIZE);

	memset(&tvi, 0, sizeof(TVITEM));
	tvi.mask = TVIF_HANDLE | TVIF_TEXT;
	tvi.hItem = item;
	tvi.pszText = buf;
	tvi.cchTextMax = BUFSIZE;
	TreeView_GetItem(treeview, &tvi);

	name = dupstr(buf);
	free(buf);

	return name;
};

char *treeview_getitempath(HWND treeview, HTREEITEM item) {
	char *buf, *path, *iname, *pname;
	HTREEITEM parent, curitem;

	if (!item)
		return NULL;

	buf = (char *)malloc(BUFSIZE);
	memset(buf, 0, BUFSIZE);
	path = (char *)malloc(BUFSIZE);
	memset(path, 0, BUFSIZE);
	iname = treeview_getitemname(treeview, item);
	strcpy(path, iname);
	free(iname);
	curitem = item;

	while (parent = TreeView_GetParent(treeview, curitem)) {
		pname = treeview_getitemname(treeview, parent);
		strcpy(buf, pname);
		strcat(buf, "\\");
		strcat(buf, path);
		strcpy(path, buf);
		free(pname);
		curitem = parent;
	};

	free(buf);

	return path;
};

char *first_name(char *path) {
	char *p;

	p = path;

	while (*p != '\0' && *p != '\\')
		p++;

	return p;
};

static HTREEITEM _treeview_getitemfrompath(HWND treeview, HTREEITEM parent, char *path) {
	char *fname, *name, *iname;
	HTREEITEM child;

	if (!path)
		return parent;

	name = (char *)malloc(BUFSIZE);
	memset(name, 0, BUFSIZE);
	fname = first_name(path);
	memmove(name, path, fname - path);
	child = TreeView_GetChild(treeview, parent);

	if (!child)
		return parent;

	do {
		iname = treeview_getitemname(treeview, child);
		if (!strcmp(name, iname)) {
			free(iname);
			if (fname[0] == '\0')
				return child;
			else
				return _treeview_getitemfrompath(treeview, child, fname + 1);
		};
		free(iname);
	} while (child = TreeView_GetNextSibling(treeview, child));

	free(name);

	return NULL;
};

HTREEITEM treeview_getitemfrompath(HWND treeview, char *path) {
	return _treeview_getitemfrompath(treeview, TVI_ROOT, path);
};

int menu_callback(char *name, char *path, 
				  int isfolder, int mode, 
				  void *priv1, void *priv2, void *priv3) {
	HMENU menu;
	HMENU add_menu;
	static int id = 0;
	int ret;
	DWORD err;

	if (mode == REG_MODE_POSTPROCESS)
		return 0;

	menu = (HMENU)priv1;

	if (isfolder) {
		add_menu = CreatePopupMenu();
		ret = InsertMenu(menu, 0, MF_OWNERDRAW | MF_POPUP | MF_BYPOSITION,
				(UINT)add_menu, dupstr(path));
		if (!ret)
			err = GetLastError();
		return (int)add_menu;
	} else {
		ret = InsertMenu(menu, 0, MF_OWNERDRAW | MF_ENABLED | MF_BYPOSITION,
				IDM_SESSION_BASE + id, dupstr(path));
		if (!ret)
			err = GetLastError();
		id++;
		return 0;
	};
};

HMENU menu_addsession(HMENU menu, char *root) {
	reg_walk_over_tree(root, menu_callback, menu, NULL, NULL);

	return menu;
};

void menu_free(HMENU menu) {
	int i;
	MENUITEMINFO mii;
	HMENU submenu;

	for (i = GetMenuItemCount(menu); i > 0; i--) {
		submenu = GetSubMenu(menu, 0);
		memset(&mii, 0, sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA;
		if (GetMenuItemInfo(menu, 0, TRUE, &mii) &&
			mii.dwItemData != 0)
			free((char *)mii.dwItemData);
		DeleteMenu(menu, 0, MF_BYPOSITION);
		if (submenu) {
			menu_free(submenu);
			DestroyMenu(submenu);
		};
	};
};

HMENU menu_refresh(HMENU menu, char *root) {
	HMENU m;

	m = CreatePopupMenu();
	AppendMenu(m, MF_ENABLED, IDM_BACKREST, "&Backup/Restore...");
	AppendMenu(m, MF_ENABLED, IDM_OPTIONSBOX, "&Options...");
	AppendMenu(m, MF_ENABLED, IDM_ABOUT, "&About...");

	menu_free(menu);
	menu = menu_addsession(menu, root);
	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	AppendMenu(menu, MF_ENABLED, IDM_LAUNCHBOX, "&Session manager...");
	AppendMenu(menu, MF_ENABLED, IDM_WINDOWLIST, "&Window list...");
	AppendMenu(menu, MF_POPUP, (UINT)m, "&More");
#ifdef WINDOWS_NT351_COMPATIBLE
	if (config->have_shell)
#endif /* WINDOWS_NT351_COMPATIBLE */
		AppendMenu(menu, MF_ENABLED, IDM_CLOSE, "E&xit");

	return menu;
};

/*
 * Returns a freshly allocated copy of the string
 * passed to it. Caller is responsible of freeing it.
 */
char *dupstr(const char *s) {
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
 * If search was unsuccessful, it tries the same steps for 
 * puttytel.exe. If this search is also unsuccessful, it simply 
 * returns NULL and relies on the user to set the correct path 
 * in the options dialog.
 */
char *get_putty_path(void) {
	char *path, *file;

    path = malloc(BUFSIZE);

	if (SearchPath(NULL, "putty.exe", NULL, BUFSIZE, path, &file))
		return path;
	else if (SearchPath(NULL, "puttytel.exe", NULL, BUFSIZE, path, &file))
		return path;
	else {
		free(path);
		return NULL;
	};
};

/* 
 * Set up a system tray icon.
 */
int AddTrayIcon(HWND hwnd) {
    BOOL res;
    NOTIFYICONDATA tnid;
    HICON hicon;

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hwnd;
    tnid.uID = 1;	       /* unique within this systray use */
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = WM_SYSTRAY;
    tnid.hIcon = hicon = LoadIcon(config->hinst, MAKEINTRESOURCE(IDI_MAINICON));
#ifdef _DEBUG
	strcpy(tnid.szTip, 
		"(DEBUG) PuTTY Launcher version " APPVERSION " built " __DATE__ " " __TIME__ );
#else
    strcpy(tnid.szTip, "PuTTY Launcher");
#endif /* _DEBUG */

    res = Shell_NotifyIcon(NIM_ADD, &tnid);

    if (hicon) 
		DestroyIcon(hicon);
    
    return res;
}

int hotkeys_callback(char *name, char *path, int isfolder, int mode,
					 void *priv1, void *priv2, void *priv3) {
	struct _config *cfg = (struct _config *)priv2;
	char *buf1, *buf2;
	int i, hotkey;

	if (mode == REG_MODE_POSTPROCESS)
		return 0;

	buf1 = (char *)malloc(BUFSIZE);
	buf2 = (char *)malloc(BUFSIZE);

	sprintf(buf1, "%s\\%s", REGROOT, path);

	for (i = 0; i < 2; i++) {
		sprintf(buf2, "%s%d", HOTKEY, i);
		hotkey = reg_read_i(buf1, buf2, 0);
		if (hotkey) {
			cfg->hotkeys[cfg->nhotkeys].action = HOTKEY_ACTION_LAUNCH + i;
			cfg->hotkeys[cfg->nhotkeys].hotkey = hotkey;
			cfg->hotkeys[cfg->nhotkeys].destination = dupstr(path);
			cfg->nhotkeys++;
		};
	};

	free(buf2);
	free(buf1);

	return 0;
};

static int extract_hotkeys(struct _config *cfg, char *root) {
	reg_walk_over_tree(root, hotkeys_callback, NULL, config, NULL);

	return TRUE;
};

int is_folder(char *name) {
	char *buf;
	int ret;

	if (!name)
		return 0;

	buf = (char *)malloc(BUFSIZE);
	sprintf(buf, "%s\\%s", REGROOT, name);

	ret = reg_read_i(buf, ISFOLDER, 0);

	free(buf);

	return ret;
};

int read_config(struct _config *cfg) {
	char *buf;
	int hk;

	if (!cfg)
		return FALSE;

	buf = reg_read_s(PLAUNCH_REGISTRY_ROOT, PLAUNCH_PUTTY_PATH, NULL);

	if (config->putty_path)
		free(config->putty_path);

	if (buf)
		config->putty_path = dupstr(buf);
	else
		config->putty_path = get_putty_path();

	hk = reg_read_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_HOTKEY_LB, 0);

	if (hk == 0)
		hk = MAKELPARAM(MOD_WIN, 'P');

	config->hotkeys[config->nhotkeys].action = HOTKEY_ACTION_MESSAGE;
	config->hotkeys[config->nhotkeys].hotkey = hk;
	config->hotkeys[config->nhotkeys].destination = (char *)WM_LAUNCHBOX;
	config->nhotkeys++;

	hk = reg_read_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_HOTKEY_WL, 0);

	if (hk == 0)
		hk = MAKELPARAM(MOD_WIN, 'W');

	config->hotkeys[config->nhotkeys].action = HOTKEY_ACTION_MESSAGE;
	config->hotkeys[config->nhotkeys].hotkey = hk;
	config->hotkeys[config->nhotkeys].destination = (char *)WM_WINDOWLIST;
	config->nhotkeys++;

	if (reg_read_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_ENABLEDRAGDROP, TRUE))
		config->options |= OPTION_ENABLEDRAGDROP;
	else
		config->options &= !OPTION_ENABLEDRAGDROP;

	if (reg_read_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_ENABLESAVECURSOR, TRUE))
		config->options |= OPTION_ENABLESAVECURSOR;
	else
		config->options &= !OPTION_ENABLESAVECURSOR;

	extract_hotkeys(cfg, "");

	return TRUE;
};

int save_config(struct _config *cfg, int what) {
	char *buf;

	if (!cfg || !what)
		return FALSE;

	if (what & CFG_SAVE_PUTTY_PATH) {
		buf = get_putty_path();
		if (!strcmp(buf, config->putty_path))
			reg_delete_v(PLAUNCH_REGISTRY_ROOT, PLAUNCH_PUTTY_PATH);
		else
			reg_write_s(PLAUNCH_REGISTRY_ROOT, PLAUNCH_PUTTY_PATH, config->putty_path);
		free(buf);
	};

	if (what & CFG_SAVE_HOTKEY_LB) {
		if (config->hotkeys[0].hotkey == MAKELPARAM(MOD_WIN, 'P'))
			reg_delete_v(PLAUNCH_REGISTRY_ROOT, PLAUNCH_HOTKEY_LB);
		else
			reg_write_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_HOTKEY_LB, config->hotkeys[0].hotkey);
	};

	if (what & CFG_SAVE_HOTKEY_WL) {
		if (config->hotkeys[1].hotkey == MAKELPARAM(MOD_WIN, 'W'))
			reg_delete_v(PLAUNCH_REGISTRY_ROOT, PLAUNCH_HOTKEY_WL);
		else
			reg_write_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_HOTKEY_WL, config->hotkeys[1].hotkey);
	};

	if (what & CFG_SAVE_DRAGDROP) {
		if (config->options & OPTION_ENABLEDRAGDROP)
			reg_delete_v(PLAUNCH_REGISTRY_ROOT, PLAUNCH_ENABLEDRAGDROP);
		else
			reg_write_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_ENABLEDRAGDROP, 0);
	};

	if (what & CFG_SAVE_SAVECURSOR) {
		if (config->options & OPTION_ENABLESAVECURSOR)
			reg_delete_v(PLAUNCH_REGISTRY_ROOT, PLAUNCH_ENABLESAVECURSOR);
		else
			reg_write_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_ENABLESAVECURSOR, 0);
	};

	return TRUE;
};

#ifdef WINDOWS_NT351_COMPATIBLE
void center_window(HWND window) {
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
		rp.top + (r.bottom / 2),
		0, 0, SWP_NOSIZE);
};
#endif /* WINDOWS_NT351_COMPATIBLE */

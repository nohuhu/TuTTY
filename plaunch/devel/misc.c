#include "plaunch.h"
#include "misc.h"
#include "session.h"

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

HTREEITEM treeview_addsession(HWND hwndTV, HTREEITEM _parent, struct session *s) {
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    HTREEITEM parent = TVI_ROOT;
    HTREEITEM prev = TVI_FIRST;
    HTREEITEM hti, ret = NULL;
    int i;

    if (s == NULL)
		return NULL;

    /*
     * Insert this session itself, if it is not a root.
     */
    if (s->parent != NULL) {
		parent = _parent;
		tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | 
				   TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.pszText = s->name; 
		tvi.cchTextMax = strlen(s->name); 
		tvi.lParam = (LPARAM)s; 
		tvi.cChildren = s->type;

		if (tvi.cChildren) {
			tvi.iImage = config->img_closed;
			tvi.iSelectedImage = config->img_closed;
		} else {
			tvi.iImage = config->img_session;
			tvi.iSelectedImage = config->img_session;
		};

		tvins.item = tvi; 
		tvins.hInsertAfter = prev; 
		tvins.hParent = parent; 

		hti = TreeView_InsertItem(hwndTV, &tvins);
		parent = hti;

		if (!ret)
			ret = hti;
	};

    for (i = 0; i < s->nchildren; i++) {
		if (s->children[i]->type) {
			treeview_addsession(hwndTV, parent, s->children[i]);

			if (s->children[i]->isexpanded)
				TreeView_Expand(hwndTV, hti, TVE_EXPAND);
		} else {
			tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | 
					TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvi.pszText = s->children[i]->name; 
			tvi.cchTextMax = strlen(s->children[i]->name); 
			tvi.lParam = (LPARAM)s->children[i]; 
			tvi.cChildren = s->children[i]->type;

			if (tvi.cChildren) {
				tvi.iImage = config->img_closed;
				tvi.iSelectedImage = config->img_closed;
			} else {
				tvi.iImage = config->img_session;
				tvi.iSelectedImage = config->img_session;
			};

			tvins.item = tvi; 
			tvins.hInsertAfter = prev; 
			tvins.hParent = parent; 

			hti = TreeView_InsertItem(hwndTV, &tvins);

		};

		if (!ret)
			ret = hti;
	};

    return ret;
}

HMENU menu_addsession(HMENU menu, struct session *s) {
    HMENU newmenu;
    int i;

    if (!s)
		return menu;

    if (s->name[0] != '\0')
		newmenu = CreateMenu();
    else
		newmenu = menu;

    for (i = 0; i < s->nchildren; i++) {
		if (s->children[i]->type) {
			HMENU add_menu;

			add_menu = menu_addsession(newmenu, s->children[i]);
			InsertMenu(newmenu, 0, MF_OWNERDRAW | MF_POPUP | MF_BYPOSITION,
				(UINT)add_menu, (LPCSTR)s->children[i]);
		} else {
			InsertMenu(newmenu, 0, MF_OWNERDRAW | MF_ENABLED | MF_BYPOSITION,
				IDM_SESSION_BASE + s->children[i]->id,
				(LPCSTR)s->children[i]);
		};
    };

    return newmenu;
};

HMENU menu_refresh(HMENU menu, struct session *root) {
	int i;
	HMENU m;

	m = CreatePopupMenu();
	AppendMenu(m, MF_ENABLED, IDM_BACKREST, "&Backup/Restore...");
	AppendMenu(m, MF_ENABLED, IDM_OPTIONSBOX, "&Options...");
	AppendMenu(m, MF_ENABLED, IDM_ABOUT, "&About...");

	for (i = GetMenuItemCount(menu); i > 0; i--)
		DeleteMenu(menu, 0, MF_BYPOSITION);
	menu = menu_addsession(menu, root);
	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	AppendMenu(menu, MF_ENABLED, IDM_LAUNCHBOX, "&Session manager...");
	AppendMenu(menu, MF_ENABLED, IDM_WINDOWLIST, "&Window list...");
	AppendMenu(menu, MF_POPUP, (UINT)m, "&Miscellaneous");
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

#define REGISTRY_ROOT	"Software\\SimonTatham\\PuTTY\\PLaunch"
#define REG_PUTTY_PATH	"PuTTYPath"
#define REG_HOTKEY_LB	"HotKeyLaunchBox"
#define REG_HOTKEY_WL	"HotKeyWindowList"

static int extract_hotkeys(struct _config *cfg, struct session *root) {
	int i;

	if (!root)
		return FALSE;

	for (i = 0; i < root->nchildren; i++) {
		if (!root->children[i])
			continue;
		if (root->children[i]->type)
			return extract_hotkeys(cfg, root->children[i]);
		else if (root->children[i]->nhotkeys) {
			int j;

			for (j = 0; j < root->children[i]->nhotkeys; j++) {
				if (root->children[i]->hotkeys[j]) {
					cfg->hotkeys[cfg->nhotkeys].action = HOTKEY_ACTION_LAUNCH + j;
					cfg->hotkeys[cfg->nhotkeys].hotkey = root->children[i]->hotkeys[j];
					cfg->hotkeys[cfg->nhotkeys].destination = root->children[i]->id;
					cfg->nhotkeys++;
				};
			};
		};
	};

	return TRUE;
};

int read_config(struct _config *cfg) {
	HKEY key;
	DWORD type, size, value;
	char *buf;
	struct session *root;

	if (!cfg)
		return FALSE;

	RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_ROOT, 0, KEY_READ, &key);

	buf = (char *)malloc(BUFSIZE);
	size = BUFSIZE;
	if (RegQueryValueEx(key, 
						REG_PUTTY_PATH, 
						NULL, 
						&type, 
						(LPBYTE)buf, 
						&size) == ERROR_SUCCESS) {
		if (config->putty_path)
			free(config->putty_path);
		config->putty_path = (char *)malloc(size + 1);
		strcpy(config->putty_path, buf);
	} else {
		if (config->putty_path)
			free(config->putty_path);
		config->putty_path = get_putty_path();
	};
	free(buf);

	size = sizeof(DWORD);

	if (RegQueryValueEx(key,
						REG_HOTKEY_LB,
						NULL,
						&type,
						(LPBYTE)&value,
						&size) == ERROR_SUCCESS) {
		config->hotkeys[config->nhotkeys].action = HOTKEY_ACTION_MESSAGE;
		config->hotkeys[config->nhotkeys].hotkey = value;
		config->hotkeys[config->nhotkeys].destination = WM_LAUNCHBOX;
		config->nhotkeys++;
	} else {
		config->hotkeys[config->nhotkeys].action = HOTKEY_ACTION_MESSAGE;
		config->hotkeys[config->nhotkeys].hotkey = MAKELPARAM(MOD_WIN, 'P');
		config->hotkeys[config->nhotkeys].destination = WM_LAUNCHBOX;
		config->nhotkeys++;
	};

	if (RegQueryValueEx(key,
						REG_HOTKEY_WL,
						NULL,
						&type,
						(LPBYTE)&value,
						&size) == ERROR_SUCCESS) {
		config->hotkeys[config->nhotkeys].action = HOTKEY_ACTION_MESSAGE;
		config->hotkeys[config->nhotkeys].hotkey = value;
		config->hotkeys[config->nhotkeys].destination = WM_WINDOWLIST;
		config->nhotkeys++;
	} else {
		config->hotkeys[config->nhotkeys].action = HOTKEY_ACTION_MESSAGE;
		config->hotkeys[config->nhotkeys].hotkey = MAKELPARAM(MOD_WIN, 'W');
		config->hotkeys[config->nhotkeys].destination = WM_WINDOWLIST;
		config->nhotkeys++;
	};

	RegCloseKey(key);

	root = session_get_root();
	extract_hotkeys(cfg, root);
	session_free(root);

	return TRUE;
};

int save_config(struct _config *cfg, int what) {
	HKEY key;
	DWORD value;
	char *buf;

	if (!cfg || !what)
		return FALSE;

	RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_ROOT, 0, NULL, 
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &value);

	if (what & CFG_SAVE_PUTTY_PATH) {
		buf = get_putty_path();
		if (!strcmp(buf, config->putty_path))
			RegDeleteValue(key, REG_PUTTY_PATH);
		else
			RegSetValueEx(key, REG_PUTTY_PATH, 0, REG_SZ, 
						config->putty_path, strlen(config->putty_path));
		free(buf);
	};

	if (what & CFG_SAVE_HOTKEY_LB) {
		if (config->hotkeys[0].hotkey == MAKELPARAM(MOD_WIN, 'P'))
			RegDeleteValue(key, REG_HOTKEY_LB);
		else
			RegSetValueEx(key, REG_HOTKEY_LB, 0, REG_DWORD, 
						(char *)&config->hotkeys[0].hotkey, sizeof(DWORD));
	};

	if (what & CFG_SAVE_HOTKEY_WL) {
		if (config->hotkeys[1].hotkey == MAKELPARAM(MOD_WIN, 'W'))
			RegDeleteValue(key, REG_HOTKEY_WL);
		else
			RegSetValueEx(key, REG_HOTKEY_WL, 0, REG_DWORD,
						(char *)&config->hotkeys[1].hotkey, sizeof(DWORD));
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
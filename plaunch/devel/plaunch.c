/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Implementation file.
 */

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <commctrl.h>

#include "plaunch.h"
#include "entry.h"
#include "session.h"
#include "dlgtmpl.h"

BOOL GetSystemImageLists(HMODULE hShell32, HIMAGELIST *phLarge, HIMAGELIST *phSmall) {
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

static HTREEITEM treeview_addsession(HWND hwndTV, HTREEITEM _parent, struct session *s) {
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    HTREEITEM parent = TVI_ROOT;
    HTREEITEM prev = TVI_FIRST;
    HTREEITEM hti;
    int i;

    if (s == NULL)
		return NULL;

    /*
     * Insert this session itself, if it is not a root.
     */
    if (s->parent != NULL)
		parent = _parent;

    for (i = 0; i < s->nchildren; i++) {
		tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | 
				   TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.pszText = s->children[i]->name; 
		tvi.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]); 
		tvi.lParam = (LPARAM)s->children[i]; 
		tvi.cChildren = s->children[i]->type;

		if (tvi.cChildren) {
			tvi.iImage = img_closed;
			tvi.iSelectedImage = img_closed;
		} else {
			tvi.iImage = img_session;
			tvi.iSelectedImage = img_session;
		};

		tvins.item = tvi; 
		tvins.hInsertAfter = prev; 
		tvins.hParent = parent; 

		hti = TreeView_InsertItem(hwndTV, &tvins);

		if (s->children[i]->type)
			treeview_addsession(hwndTV, hti, s->children[i]);

		if (s->children[i]->isexpanded)
			TreeView_Expand(hwndTV, hti, TVE_EXPAND);
	};

    return prev;
}

static HMENU menu_addsession(HMENU menu, struct session *s) {
    HMENU newmenu;
    int i;

    if (!s)
		return menu;

    if (s->name[0] != '\0')
		newmenu = CreateMenu();
    else
		newmenu = menu;

    for (i = 0; i < s->nchildren; i++) {
		LPMENUITEMINFO mii;

		if (s->children[i]->type) {
			HMENU add_menu;
			add_menu = menu_addsession(newmenu, s->children[i]);
			mii = malloc(sizeof(MENUITEMINFO));
			mii->cbSize = sizeof(MENUITEMINFO);
			mii->fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
			mii->fType = MFT_OWNERDRAW;
			mii->fState = MFS_ENABLED;
			mii->hSubMenu = add_menu;
			mii->wID = IDM_SESSION_BASE + s->children[i]->id;
			mii->dwItemData = (DWORD)s->children[i];
			InsertMenuItem(newmenu, 0, TRUE, mii);
			free(mii);
		} else {
			mii = malloc(sizeof(MENUITEMINFO));
			mii->cbSize = sizeof(MENUITEMINFO);
			mii->fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
			mii->fType = MFT_OWNERDRAW;
			mii->fState = MFS_ENABLED;
			mii->wID = IDM_SESSION_BASE + s->children[i]->id;
			mii->dwItemData = (DWORD)s->children[i];
			InsertMenuItem(newmenu, 0, TRUE, mii);
			free(mii);
		};
    };

    return newmenu;
};

/*
 * License Box: dialog function.
 */
static int CALLBACK LicenseProc(HWND hwnd, UINT msg,
								WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwnd, TRUE);
			return FALSE;
		}
		return FALSE;
    }
    return FALSE;
};

/*
 * License Box: setup function.
 */
void do_licensebox(void) {
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE * 2);
	ptr = dialogtemplate_create((void *)tmpl, WS_POPUP | WS_VISIBLE | WS_CAPTION |
								WS_SYSMENU | DS_MODALFRAME | DS_CENTER,
								0, 0, 240, 250, "PuTTY Launcher License", 2, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
								90, 225, 50, 14, "OK", IDOK);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE | SS_CENTER,
								7, 7, 226, 212,
								"Copyright © 2004 dwalin <dwalin@dwalin.ru>\n"
								"\n"
								"Portions copyright Simon Tatham, 1997-2004\n"
								"\n"
								"Permission is hereby granted, free of charge, to any person\n"
								"obtaining a copy of this software and associated documentation\n"
								"files (the ""Software""), to deal in the Software without restriction,\n"
								"including without limitation the rights to use, copy, modify, merge,\n"
								"publish, distribute, sublicense, and/or sell copies of the Software,\n"
								"and to permit persons to whom the Software is furnished to do so,\n"
								"subject to the following conditions:\n"
								"\n"
								"The above copyright notice and this permission notice shall be\n"
								"included in all copies or substantial portions of the Software.\n"
								"\n"
								"THE SOFTWARE IS PROVIDED ""AS IS"", WITHOUT\n"
								"WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,\n"
								"INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
								"MERCHANTABILITY, FITNESS FOR A PARTICULAR\n"
								"PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
								"COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES\n"
								"OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\n"
								"TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
								"CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
								"OTHER DEALINGS IN THE SOFTWARE.", 100);
	DialogBoxIndirect(hinst, (LPDLGTEMPLATE)tmpl, NULL, LicenseProc);

	GlobalFree(tmpl);
};

/*
 * About Box: dialog function.
 */
static int CALLBACK AboutProc(HWND hwnd, UINT msg,
							  WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			DestroyWindow(hwnd);
			return FALSE;
		case ID_LICENSE:
			EnableWindow(hwnd, FALSE);
			do_licensebox();
			EnableWindow(hwnd, TRUE);
			SetActiveWindow(hwnd);
			return FALSE;
		}
		return FALSE;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return FALSE;
    }

    return FALSE;
};

/*
 * About Box: setup function.
 */
void do_aboutbox(void) {
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE);
	ptr = dialogtemplate_create((void *)tmpl, WS_POPUP | WS_CAPTION | WS_SYSMENU |
								WS_VISIBLE | DS_MODALFRAME | DS_CENTER,
								0, 0, 140, 80, "About PuTTY Launcher", 3, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								82, 62, 48, 14, "&Close", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								6, 62, 70, 14, "View &License...", ID_LICENSE);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE | SS_CENTER,
								10, 7, 120, 50,
								"PuTTY launcher/session manager\n"
								"\n"
								"Version " APPVERSION ", " __DATE__ " " __TIME__ "\n"
								"\n"
								"© 2004 dwalin <dwalin@dwalin.ru>. All rights reserved.",
								100);
	DialogBoxIndirect(hinst, (LPDLGTEMPLATE)tmpl, NULL, AboutProc);

	GlobalFree(tmpl);
};

//void do_optionsbox(void);

static HWND hwnd_launchbox = NULL;

/*
 * Launch Box: dialog function.
 */
static int CALLBACK LaunchBoxProc(HWND hwnd, UINT msg,
								  WPARAM wParam, LPARAM lParam) {
	static HWND treeview;
	static HMENU context_menu = NULL;
	static int tvctrl;
	static struct session *root;
	static HTREEITEM editing_now = NULL;

	switch (msg) {
	case WM_INITDIALOG:
		hwnd_launchbox = hwnd;
		SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
					(LPARAM) LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAINICON)));

		/*
		 * Create the tree view.
		 */
		{
			RECT r;
			WPARAM font;
			HWND tvstatic;

			GetClientRect(hwnd, &r);
			r.left += 7;
			r.right -= 95;
			r.top += 7;
			r.bottom = r.top + 15;
			tvstatic = CreateWindowEx(0, "STATIC", "&Sessions:",
										WS_CHILD | WS_VISIBLE,
										r.left, r.top,
										r.right - r.left, r.bottom - r.top,
										hwnd, NULL, hinst,
										NULL);
			font = SendMessage(hwnd, WM_GETFONT, 0, 0);
			SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

			GetClientRect(hwnd, &r);
			r.left += 7;
			r.right -= 95;
			r.top = 27;
			r.bottom -= 7;
			treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
										WS_CHILD | WS_VISIBLE |
										WS_TABSTOP | TVS_HASLINES |
										TVS_DISABLEDRAGDROP | TVS_HASBUTTONS |
										TVS_LINESATROOT | TVS_SHOWSELALWAYS | 
										TVS_EDITLABELS,
										r.left, r.top,
										r.right - r.left, r.bottom - r.top,
										hwnd, NULL, hinst, NULL);
			font = SendMessage(hwnd, WM_GETFONT, 0, 0);
			SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
			tvctrl = GetDlgCtrlID(treeview);

			if (image_list)
				TreeView_SetImageList(treeview, image_list, TVSIL_NORMAL);
		}

		/*
		 * Set up the tree view contents.
		 */
		{
			HTREEITEM hfirst = NULL;

			root = get_root();
			hfirst = treeview_addsession(treeview, TVI_ROOT, root);

			TreeView_SelectItem(treeview, TreeView_GetRoot(treeview));
		}

		/*
		 * Create a popup context menu and fill it with elements.
		 */
		{
			context_menu = CreatePopupMenu();
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_UNDO, "&Undo");
			AppendMenu(context_menu, MF_SEPARATOR, 0, "");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_CUT, "Cu&t");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_COPY, "&Copy");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_PASTE, "&Paste");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_DELETE, "&Delete");
			AppendMenu(context_menu, MF_SEPARATOR, 0, "");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTFLD, "Create &folder");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTSES, "Create &session");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_RENAME, "&Rename");
		}

		SetWindowLong(hwnd, GWL_USERDATA, 1);
		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(treeview);

		return FALSE;
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
				TreeView_EnsureVisible(treeview, item);
				TreeView_GetItemRect(treeview, item, &rc, TRUE);
				pt.x = rc.left + iconx;
				pt.y = rc.bottom;
			} else {
				ScreenToClient(treeview, &pt);
				GetClientRect(treeview, (LPRECT)&rc);

				if (PtInRect((LPRECT)&rc, pt)) {
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

			if (msg = TrackPopupMenuEx(context_menu, TPM_LEFTALIGN | TPM_TOPALIGN |
				TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL))
				PostMessage(hwnd, WM_COMMAND, msg, MAKELPARAM(item, 0));

			return TRUE;
		}
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			{
				if (editing_now) {
					TreeView_EndEditLabelNow(treeview, FALSE);
					return TRUE;
				};
			};
		case ID_EDIT:
			{
				TVITEM item;
				struct session *s;
				char *p;

				if (!TreeView_GetCount(treeview)) {
					EndDialog(hwnd, 0);
					return FALSE;
				};

				item.hItem = TreeView_GetSelection(treeview);
				TreeView_GetItem(treeview, &item);
				s = (struct session *)item.lParam;

				if (!s) {
					EndDialog(hwnd, 0);
					return FALSE;
				};

				if (s->type) {
					TreeView_Expand(treeview, item.hItem, TVE_TOGGLE);
					return FALSE;
				};

				p = get_full_path(s);

				if (p) {
					char *buf;

					buf = malloc(strlen(s->name) + 9);

					if (wParam == IDOK)
						sprintf(buf, "-load \"%s\"", s->name);
					else 
						sprintf(buf, "-edit \"%s\"", s->name);

					ShellExecute(hwnd, "open", putty_path,
								buf, _T(""), SW_SHOWNORMAL);
					free(buf);
					free(p);
				} else {
					ShellExecute(hwnd, "open", putty_path,
								_T(""), _T(""), SW_SHOWNORMAL);
				};

				if (root)
					root = free_session(root);

				EndDialog(hwnd, 0);

				return FALSE;
			};
		case IDCANCEL:
			{
				if (editing_now) {
					TreeView_EndEditLabelNow(treeview, TRUE);
					return TRUE;
				};
			};
			{
				if (root)
					root = free_session(root);

				EndDialog(hwnd, 0);

				return FALSE;
			};
		case ID_OPTIONS:
//			do_optionsbox();

			return FALSE;
		case ID_RENAME:
		case IDM_CTXM_RENAME:
			{
				HTREEITEM item;

				item = TreeView_GetSelection(treeview);
				SetFocus(treeview);
				TreeView_EditLabel(treeview, item);

				return TRUE;
			};
		case ID_NEWFOLDER:
		case IDM_CTXM_CRTFLD:
		case ID_NEWSESSION:
		case IDM_CTXM_CRTSES:
			{
				HTREEITEM item, parent, newitem;
				TVITEM tvi;
				TVINSERTSTRUCT tvins;
				struct session *olds, *news;

				item = tvi.hItem = TreeView_GetSelection(treeview);
				TreeView_GetItem(treeview, &tvi);
				olds = (struct session *)tvi.lParam;

				parent = TreeView_GetParent(treeview, item);

				if (olds->type) {
					news = new_session((wParam == ID_NEWFOLDER ||
						wParam == IDM_CTXM_CRTFLD) ? STYPE_FOLDER : STYPE_SESSION,
						"", olds);
					parent = item;
				} else if (parent) {
					tvi.hItem = parent;
					TreeView_GetItem(treeview, &tvi);
					olds = (struct session *)tvi.lParam;

					news = new_session((wParam == ID_NEWFOLDER ||
						wParam == IDM_CTXM_CRTFLD) ? STYPE_FOLDER : STYPE_SESSION,
						"", olds);
				} else {
					olds = root;
					news = new_session((wParam == ID_NEWFOLDER ||
						wParam == IDM_CTXM_CRTFLD) ? STYPE_FOLDER : STYPE_SESSION,
						"", olds);
					parent = NULL;
				};

				olds->children = insert_session(olds->children, &olds->nchildren, news);

				tvins.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN |
					TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tvins.item.pszText = news->name;
				tvins.item.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]);
				tvins.item.lParam = (LPARAM)news;
				tvins.item.cChildren = news->nchildren;
				tvins.item.iImage = tvins.item.iSelectedImage = 
					(wParam == ID_NEWFOLDER || wParam == IDM_CTXM_CRTFLD) ?
					img_closed : img_session;
				tvins.hInsertAfter = item;
				tvins.hParent = parent;
				newitem = TreeView_InsertItem(treeview, &tvins);

				SetFocus(treeview);
				TreeView_EnsureVisible(treeview, newitem);
				TreeView_EditLabel(treeview, newitem);

				return TRUE;
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
							SendMessage(hwnd, WM_COMMAND, (WPARAM)IDOK, 0);

						return FALSE;
					} else {
						return FALSE;
					};
				};
				break;
			case TVN_ITEMEXPANDED:
				{
					LPNMTREEVIEW tv = (LPNMTREEVIEW)lParam;

					if (tv->itemNew.cChildren) {
						int isexpanded, wasexpanded;
						struct session *s;

						wasexpanded = (tv->itemOld.state & TVIS_EXPANDED) ? TRUE : FALSE;
						isexpanded = (tv->itemNew.state & TVIS_EXPANDED) ? TRUE : FALSE;

						s = (struct session *)tv->itemNew.lParam;

						tv->itemNew.iImage = isexpanded ? img_open : img_closed;
						tv->itemNew.iSelectedImage = isexpanded ? img_open : img_closed;
						TreeView_SetItem(treeview, &(tv->itemNew));
						TreeView_SelectItem(treeview, &(tv->itemNew));

						if (s->isexpanded != isexpanded) {
							s->isexpanded = isexpanded;
							set_expanded(s, isexpanded);
						};

						return FALSE;
					};
				};
				break;
			case TVN_SELCHANGED:
				{
					LPNMTREEVIEW tv = (LPNMTREEVIEW)lParam;
					struct session *s = 
						(struct session *)tv->itemNew.lParam;
					int boolean;
					int flag;

					if (s && s->name && !strcmp(s->name, DEFAULTSETTINGS)) {
						boolean = FALSE;
						flag = MF_GRAYED;
					} else {
						boolean = TRUE;
						flag = MF_ENABLED;
					};

					EnableWindow(GetDlgItem(hwnd, ID_DELETE), boolean);
					EnableWindow(GetDlgItem(hwnd, ID_RENAME), boolean);
					EnableMenuItem(context_menu, IDM_CTXM_DELETE, flag);
					EnableMenuItem(context_menu, IDM_CTXM_RENAME, flag);

					return TRUE;
				};
			case TVN_BEGINLABELEDIT:
				{
					LPNMTVDISPINFO di = (LPNMTVDISPINFO)lParam;
					struct session *s =
						(struct session *)di->item.lParam;

					if (s->name && !strcmp(s->name, DEFAULTSETTINGS)) {
						TreeView_EndEditLabelNow(treeview, TRUE);
						return TRUE;
					} else {
						editing_now = di->item.hItem;

						return FALSE;
					};
				};
			case TVN_ENDLABELEDIT:
				{
					LPNMTVDISPINFO di = (LPNMTVDISPINFO)lParam;

					editing_now = NULL;

					if (di->item.pszText) {
						struct session *s = 
							(struct session *)di->item.lParam;

						if (rename_session(s, di->item.pszText)) {
							TreeView_SetItem(treeview, &di->item);

							return TRUE;
						} else {
							TreeView_EditLabel(treeview, di->item.hItem);
							return FALSE;
						};
						
					} else {
						struct session *s =
							(struct session *)di->item.lParam;

						if (!s->name || s->name[0] == '\0')
							TreeView_EditLabel(treeview, di->item.hItem);

						return FALSE;
					};
				};
			default:
				return FALSE;
			};
		};
    default:
		return FALSE;
    }
    return FALSE;
}

/*
 * Launch Box: setup function.
 */
int do_launchbox(void) {
    int ret;
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	if (hwnd_launchbox) {
		SetForegroundWindow(hwnd_launchbox);
		return 0;
	};

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE * 2);
	ptr = dialogtemplate_create((void *)tmpl, WS_POPUP | WS_VISIBLE | WS_CAPTION |
								WS_SYSMENU | DS_CENTER | DS_3DLOOK | DS_MODALFRAME,
								0, 0, 242, 215, "PuTTY session manager", 8, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								185, 21, 50, 14, "Open", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								185, 38, 50, 14, "Cancel", IDCANCEL);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								185, 77, 50, 14, "New &folder", ID_NEWFOLDER);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								185, 93, 50, 15, "New &session", ID_NEWSESSION);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								185, 109, 50, 14, "&Edit", ID_EDIT);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								185, 125, 50, 14, "&Rename", ID_RENAME);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								185, 141, 50, 14, "&Delete", ID_DELETE);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								185, 176, 50, 14, "&Options...", ID_OPTIONS);

	ret = DialogBoxIndirect(hinst, (LPDLGTEMPLATE)tmpl, NULL, LaunchBoxProc);

	GlobalFree(tmpl);
	hwnd_launchbox = NULL;

    return (ret == IDOK);
}

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

    switch (msg) {
    case WM_INITDIALOG:
		hwnd_windowlistbox = hwnd;
		SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
					(LPARAM) LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAINICON)));

		listbox = GetDlgItem(hwnd, ID_WINDOWLISTLISTBOX);

		handles = NULL;
		windowlist = NULL;

		refresh_listbox(listbox, &nwindows, &handles, &windowlist);

		SendMessage(listbox, (UINT)LB_SETCURSEL, 0, 0);

		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(listbox);
		return FALSE;
	case WM_COMMAND:
		if (HIWORD(wParam) == LBN_DBLCLK &&
			(HWND)lParam == listbox) {
			SendMessage(hwnd, WM_COMMAND, (WPARAM)IDOK, 0);
			return FALSE;
		};
		switch (wParam) {
		case ID_WINDOWLISTSTATIC:
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
		case ID_WINDOWLISTHIDE:
		case ID_WINDOWLISTSHOW:
			if (handles) {
				i = SendMessage(listbox, (UINT)LB_GETCURSEL, 0, 0);

				if (i != LB_ERR)
					ShowWindow(handles[i], wParam == ID_WINDOWLISTHIDE ?
												SW_HIDE : SW_SHOWNOACTIVATE);

				update_listbox(listbox, nwindows, handles, windowlist);
				SendMessage(listbox, (UINT)LB_SETCURSEL, i, 0);
			};
			break;
		case ID_WINDOWLISTCLOSE:
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
	case WM_VKEYTOITEM:
		switch (LOWORD(wParam)) {
		case 'H':
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_WINDOWLISTHIDE, 0);
			return -2;
		case 'S':
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_WINDOWLISTSHOW, 0);
			return -2;
		case VK_DELETE:
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_WINDOWLISTCLOSE, 0);
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
								ID_WINDOWLISTSTATIC);
	ptr = dialogtemplate_addlistbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
								LBS_HASSTRINGS | LBS_NOTIFY | LBS_DISABLENOSCROLL |
								LBS_USETABSTOPS | LBS_WANTKEYBOARDINPUT,
								7, 15, 169, 132, NULL, ID_WINDOWLISTLISTBOX);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								183, 21, 50, 14, "Switch to", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 38, 50, 14, "Cancel", IDCANCEL);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 77, 50, 14, "&Hide", ID_WINDOWLISTHIDE);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 93, 50, 14, "&Show", ID_WINDOWLISTSHOW);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								183, 109, 50, 14, "&Close", ID_WINDOWLISTCLOSE);

	SetForegroundWindow(hwnd);
	DialogBoxIndirect(hinst, (LPDLGTEMPLATE)tmpl, NULL, WindowListBoxProc);

	GlobalFree(tmpl);
	hwnd_windowlistbox = NULL;
};

/*
 * Options Box: dialog function.
 */
/*
static int CALLBACK OptionsBoxProc(HWND hwnd, UINT msg,
								   WPARAM wParam, LPARAM lParam) {
	static HWND ppedit, ppbutton, lbedit, wledit;
	static DWORD lb_key, wl_key;

    switch (msg) {
    case WM_INITDIALOG:
		ppedit = GetDlgItem(hwnd, ID_OPTIONS_PUTTYPATH_EDIT);
		ppbutton = GetDlgItem(hwnd, ID_OPTIONS_PUTTYPATH_BUTTON);
		lbedit = GetDlgItem(hwnd, ID_OPTIONS_LB_HOTKEY_EDIT);
		wledit = GetDlgItem(hwnd, ID_OPTIONS_WL_HOTKEY_EDIT);
		lb_key = 0;
		wl_key = 0;

		SendMessage(ppedit, (UINT)EM_SETLIMITTEXT, (WPARAM)BUFSIZE, 0);

		if (putty_path)
			SetWindowText(ppedit, putty_path);

		SetFocus(ppedit);

		return FALSE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, 0);

			return FALSE;
		case ID_OPTIONS_PUTTYPATH_STATIC:
			SetFocus(ppedit);

			return FALSE;
		case ID_OPTIONS_LB_HOTKEY_STATIC:
			SetFocus(lbedit);

			return FALSE;
		case ID_OPTIONS_WL_HOTKEY_STATIC:
			SetFocus(wledit);

			return FALSE;
		};
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (GetFocus() == lbedit) {
			lb_key = wParam;

			return FALSE;
		} else if (GetFocus() == wledit) {
			wl_key = wParam;

			return FALSE;
		} else
			return FALSE;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		break;
	default:
		return FALSE;
	};
	return FALSE;
};

/*
 * Options Box: setup function.
 */
/*
void do_optionsbox(void) {
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE * 2);
	ptr = dialogtemplate_create(tmpl, WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU |
								DS_CENTER | DS_MODALFRAME,
								0, 0, 189, 122, "Putty Launcher options", 10, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
								7, 7, 175, 108, NULL, ID_OPTIONS_GROUPBOX);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								16, 17, 129, 9, "&Path to PuTTY or PuTTYtel executable:",
								ID_OPTIONS_PUTTYPATH_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
								ES_AUTOHSCROLL,
								16, 31, 99, 14, NULL, ID_OPTIONS_PUTTYPATH_EDIT);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								124, 31, 50, 14, "&Browse...", 
								ID_OPTIONS_PUTTYPATH_BUTTON);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								16, 54, 75, 9, "&Launch Box hot key:",
								ID_OPTIONS_LB_HOTKEY_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
								16, 68, 75, 14, NULL, ID_OPTIONS_LB_HOTKEY_EDIT);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								98, 54, 75, 9, "&Window List hot key:",
								ID_OPTIONS_WL_HOTKEY_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
								98, 68, 75, 14, NULL, ID_OPTIONS_WL_HOTKEY_EDIT);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								39, 91, 50, 14, "OK", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								99, 91, 50, 14, "Cancel", IDCANCEL);

	DialogBoxIndirect(hinst, (LPDLGTEMPLATE)tmpl, NULL, OptionsBoxProc);

	GlobalFree(tmpl);
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
static BOOL AddTrayIcon(HWND hwnd) {
    BOOL res;
    NOTIFYICONDATA tnid;
    HICON hicon;

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hwnd;
    tnid.uID = 1;	       /* unique within this systray use */
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = WM_SYSTRAY;
    tnid.hIcon = hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAINICON));
#ifdef _DEBUG
	strcpy(tnid.szTip, "(DEBUG) PuTTY Launcher");
#else
    strcpy(tnid.szTip, "PuTTY Launcher");
#endif /* _DEBUG */

    res = Shell_NotifyIcon(NIM_ADD, &tnid);

    if (hicon) 
		DestroyIcon(hicon);
    
    return res;
}

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
		ImageList_GetIconSize(image_list, &iconx, &icony);
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
			struct session *root;
			int i;

			menuinprogress = 1;
			root = get_sessions("");

			for (i = GetMenuItemCount(systray_menu); i > 0; i--)
				DeleteMenu(systray_menu, 0, MF_BYPOSITION);
			systray_menu = menu_addsession(systray_menu, root);
			AppendMenu(systray_menu, MF_SEPARATOR, 0, 0);
			AppendMenu(systray_menu, MF_ENABLED, IDM_LAUNCHBOX, "&Session manager...");
			AppendMenu(systray_menu, MF_ENABLED, IDM_WINDOWLIST, "&Window list...");
			AppendMenu(systray_menu, MF_ENABLED, IDM_ABOUT, "&About...");
			AppendMenu(systray_menu, MF_ENABLED, IDM_CLOSE, "E&xit");
			DrawMenuBar(hwnd);

			SetForegroundWindow(hwnd);
			ret = TrackPopupMenu(systray_menu,
								 TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
								 TPM_RIGHTBUTTON | TPM_RETURNCMD,
								 wParam, lParam, 0, hwnd, NULL);
			PostMessage(hwnd, WM_NULL, 0, 0);

			if (ret > IDM_SESSION_BASE && ret < IDM_SESSION_MAX) {
				struct session *s;

				s = find_session_by_id(root, ret - IDM_SESSION_BASE);

				if (s) {
					char *p;

					p = get_full_path(s);

					if (p) {
						char *buf;

						buf = malloc(strlen(p) + 9);
						sprintf(buf, "-load \"%s\"", p);

						ShellExecute(hwnd, "open", putty_path,
									 buf, _T(""), SW_SHOWNORMAL);

						free(buf);
						free(p);
					} else {
						ShellExecute(hwnd, "open", putty_path,
									 _T(""), _T(""), SW_SHOWNORMAL);
					};
				};
			} else
				SendMessage(hwnd, WM_COMMAND, (WPARAM)ret, 0);

			if (root)
				free_session(root);

			menuinprogress = 0;
		}
		break;
	case WM_HOTKEY:
		if (wParam == HOTKEY_LAUNCHBOX) {
			do_launchbox();
			return TRUE;
		} else if (wParam == HOTKEY_WINDOWLIST) {
			do_windowlistbox();
			return TRUE;
		};
		break;
	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT mis;
			HDC hdc;
			SIZE size;
			struct session *s;

			mis = (LPMEASUREITEMSTRUCT)lParam;
			s = (struct session *)mis->itemData;

			if (!s || !s->name)
				return FALSE;

			hdc = GetDC(hwnd);
			GetTextExtentPoint32(hdc, s->name, strlen(s->name), &size);
			mis->itemWidth = size.cx + iconx + 7 + GetSystemMetrics(SM_CXMENUCHECK);
			if (icony > size.cy)
				mis->itemHeight = icony + 2;
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
			struct session *s;
			int selected = 0, x, y;

			dis = (LPDRAWITEMSTRUCT)lParam;
			s = (struct session *)dis->itemData;

			if (!s || !s->name)
				return FALSE;

			if (dis->itemState & ODS_SELECTED) {
				SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
				selected = TRUE;
			};

			GetTextExtentPoint32(dis->hDC, s->name, strlen(s->name), &size);

			x = dis->rcItem.left + iconx + 7;
			y = dis->rcItem.top + 
				(int)((dis->rcItem.bottom - dis->rcItem.top - size.cy) / 2);

			ExtTextOut(dis->hDC, x, y, ETO_OPAQUE, &dis->rcItem, s->name, strlen(s->name), NULL);
			
			x = dis->rcItem.left + 2;
			y = dis->rcItem.top + 1;

			if (s->type) {
				if (selected)
					icon = ImageList_ExtractIcon(0, image_list, img_open);
				else
					icon = ImageList_ExtractIcon(0, image_list, img_closed);
			} else
				icon = ImageList_ExtractIcon(0, image_list, img_session);
			DrawIconEx(dis->hDC, x, y, icon, iconx, icony, 0, NULL, DI_NORMAL);
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

	    plaunch = FindWindow(APPNAME, APPNAME);
        if (plaunch) {
            PostMessage(plaunch, WM_COMMAND, IDM_LAUNCHBOX, 0);
            return 0;
        };
    };
#endif /* _DEBUG */

    hinst = inst;
    InitCommonControls();
    putty_path = get_putty_path();

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

		if (GetSystemImageLists(shell32, &large_list, &small_list)) {
			ImageList_GetIconSize(small_list, &cx, &cy);
			image_list = ImageList_Create(cx, cy, ILC_COLOR8 | ILC_MASK, 3, 3);

			icon = ImageList_ExtractIcon(0, small_list, IMG_FOLDER_CLOSED);
			img_closed = ImageList_AddIcon(image_list, icon);
			DestroyIcon(icon);

			icon = ImageList_ExtractIcon(0, small_list, IMG_FOLDER_OPEN);
			img_open = ImageList_AddIcon(image_list, icon);
			DestroyIcon(icon);

			icon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAINICON));
			img_session = ImageList_AddIcon(image_list, icon);
			DeleteObject(icon);

			FreeSystemImageLists(shell32);
		};
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

    hwnd = NULL;

    hwnd = CreateWindow(APPNAME, APPNAME,
						WS_OVERLAPPEDWINDOW | WS_VSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT,
						100, 100, NULL, NULL, inst, NULL);
	SetForegroundWindow(hwnd);

    /* Set up a system tray icon */
    AddTrayIcon(hwnd);

    /* Create popup menu */
    systray_menu = CreatePopupMenu();
/*    AppendMenu(systray_menu, MF_SEPARATOR, 0, 0);
	AppendMenu(systray_menu, MF_ENABLED, IDM_LAUNCHBOX, "&Session manager...");
	AppendMenu(systray_menu, MF_ENABLED, IDM_WINDOWLIST, "&Window list...");
    AppendMenu(systray_menu, MF_ENABLED, IDM_ABOUT, "&About");
    AppendMenu(systray_menu, MF_ENABLED, IDM_CLOSE, "E&xit");
*/
    ShowWindow(hwnd, SW_HIDE);

    if (!RegisterHotKey(hwnd, HOTKEY_LAUNCHBOX, MOD_WIN, PL_HOTKEY_LAUNCHBOX)) {
		char *err;
		err = malloc(BUFSIZE);
		sprintf(err, "Cannot register '%c' hotkey!", PL_HOTKEY_LAUNCHBOX);
		MessageBox(hwnd, err, "Error", MB_OK | MB_ICONERROR);
		free(err);
	};

    if (!RegisterHotKey(hwnd, HOTKEY_WINDOWLIST, MOD_WIN, PL_HOTKEY_WINDOWLIST)) {
		char *err;
		err = malloc(BUFSIZE);
		sprintf(err, "Cannot register '%c' hotkey!", PL_HOTKEY_WINDOWLIST);
		MessageBox(hwnd, err, "Error", MB_OK | MB_ICONERROR);
		free(err);
	};

    while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
    };

	UnregisterHotKey(hwnd, HOTKEY_WINDOWLIST);
    UnregisterHotKey(hwnd, HOTKEY_LAUNCHBOX);

    /* Clean up the system tray icon */
    {
		NOTIFYICONDATA tnid;

		tnid.cbSize = sizeof(NOTIFYICONDATA);
		tnid.hWnd = hwnd;
		tnid.uID = 1;

		Shell_NotifyIcon(NIM_DELETE, &tnid);
    }

	DestroyMenu(systray_menu);

	if (putty_path)
		free(putty_path);

	if (image_list)
		ImageList_Destroy(image_list);

    return 0;
};

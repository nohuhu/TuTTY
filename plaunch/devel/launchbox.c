#include <stdio.h>
#include "entry.h"
#include "plaunch.h"
#include "misc.h"
#include "hotkey.h"
#include "registry.h"
#include "resource.h"
#include "dlgtmpl.h"
#include "resrc1.h"

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
#define	TAB_AUTOPROCESS		1
#define	TAB_LAST			2
#define	TAB_ACTIONS			2

char * TAB_NAMES[] =
	{ "Hot Keys", "Limits && Auto Processing", "%s Actions"};

typedef struct tag_dlghdr { 
    HWND hwndTab;       // tab control 
    HWND hwndDisplay;   // current child dialog box 
    RECT rcDisplay;     // display rectangle for the tab control 
    DLGTEMPLATE *apRes[TAB_LAST]; 
} DLGHDR; 

/*
 * Launch Box: tree view compare function.
 */

static int CALLBACK treeview_compare(LPARAM p1, LPARAM p2, LPARAM sort) {
	char item1[BUFSIZE], item2[BUFSIZE];

	treeview_getitemname((HWND)sort, (HTREEITEM)p1, item1, BUFSIZE);
	treeview_getitemname((HWND)sort, (HTREEITEM)p2, item2, BUFSIZE);

	return sessioncmp(item1, item2);
};

/*
 * Launch Box: a helper subroutine that finds unused name for the item
 */

static unsigned int treeview_find_unused_name(HWND treeview, HTREEITEM parent, 
											  char *name, char *buf) {
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
 */

static int CALLBACK LaunchBoxChildProc(HWND hwnd, UINT msg,
									   WPARAM wParam, LPARAM lParam) {
static HBRUSH background, staticback;
	switch (msg) {
	case WM_INITDIALOG:
		{
			HWND parent;
			DLGHDR *hdr;

			parent = GetParent(hwnd);
			hdr = (DLGHDR *)GetWindowLong(parent, GWL_USERDATA);

			SetWindowPos(hwnd, HWND_TOP, hdr->rcDisplay.left, hdr->rcDisplay.top,
						hdr->rcDisplay.right - hdr->rcDisplay.left, 
						hdr->rcDisplay.bottom - hdr->rcDisplay.top, 
						SWP_SHOWWINDOW);

			background = GetStockObject(HOLLOW_BRUSH);
//			staticback = GetStockObject(config->version_major >= 5 ? WHITE_BRUSH : LTGRAY_BRUSH);
		};
		break;
	case WM_CTLCOLORDLG:
		return (INT_PTR)background;
//	case WM_CTLCOLORSTATIC:
//	case WM_CTLCOLOREDIT:
//	case WM_CTLCOLORBTN:
//	case WM_CTLCOLORLISTBOX:
//		SetBkMode((HDC)wParam, TRANSPARENT);
//		return (INT_PTR)wParam;
//		break;
	};

	return FALSE;
};

/*
 * Launch Box: dialog function.
 */
static int CALLBACK LaunchBoxProc(HWND hwnd, UINT msg,
								  WPARAM wParam, LPARAM lParam) {
	static DLGHDR *dlghdr = NULL;
	static POINT bigpt = {0}, smallpt = {0};
	static HWND treeview = NULL, tabview = NULL, hotkey_combo = NULL, 
		hotkey_edit = NULL,	limit_checkbox = NULL, limit_edit = NULL, 
		limit_searchfor = NULL,	limit_action1 = NULL, limit_action2 = NULL, 
		autorun_checkbox = NULL, autorun_when = NULL, autorun_action = NULL;
	static HMENU context_menu = NULL;
	static unsigned int cut_or_copy = 0;
	static unsigned int morestate;
	static HTREEITEM editing_now = NULL;
	static HTREEITEM copying_now = NULL;
	static HTREEITEM dragging_now = NULL;
	static HIMAGELIST draglist = NULL;
	int i, j;

	switch (msg) {
	case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
		if (!config->have_shell)
			center_window(hwnd);
#endif /* WINDOWS_NT351_COMPATIBLE */

		hwnd_launchbox = hwnd;
		SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)config->main_icon);

		hotkey_combo = GetDlgItem(hwnd, IDC_LAUNCHBOX_COMBOBOX_HOTKEY);

		SendMessage(hotkey_combo, CB_RESETCONTENT, 0, 0);
		for (i = 2; i <= HOTKEY_MAX_ACTION; i++) {
			j = SendMessage(hotkey_combo, CB_ADDSTRING, 0, (LPARAM)HOTKEY_STRINGS[i]);
			SendMessage(hotkey_combo, CB_SETITEMDATA, (WPARAM)j, (LPARAM)i);
		};
		SendMessage(hotkey_combo, CB_SETCURSEL, 0, 0);

		hotkey_edit = GetDlgItem(hwnd, IDC_LAUNCHBOX_EDITBOX_HOTKEY);
		make_hotkey(hotkey_edit, 0);

		limit_checkbox = GetDlgItem(hwnd, IDC_LAUNCHBOX_CHECKBOX_LIMIT);
		limit_edit = GetDlgItem(hwnd, IDC_LAUNCHBOX_EDITBOX_LIMIT1);
		SendDlgItemMessage(hwnd, IDC_LAUNCHBOX_SPIN_LIMIT, UDM_SETRANGE, 0, 
							(LPARAM)MAKELONG((short)UD_MAXVAL, (short)0));
		limit_searchfor = GetDlgItem(hwnd, IDC_LAUNCHBOX_COMBOBOX_LIMIT1);

		limit_action1 = GetDlgItem(hwnd, IDC_LAUNCHBOX_COMBOBOX_LIMIT2);
		SendMessage(limit_action1, CB_RESETCONTENT, 0, 0);

		limit_action2 = GetDlgItem(hwnd, IDC_LAUNCHBOX_COMBOBOX_LIMIT3);
		SendMessage(limit_action2, CB_RESETCONTENT, 0, 0);

		for (i = 0; i < LIMIT_ACTION_MAX; i++) {
			SendMessage(limit_action1, CB_ADDSTRING, 0, 
						(LPARAM)LIMIT_ACTION_STRINGS[i]);
			SendMessage(limit_action2, CB_ADDSTRING, 0,
						(LPARAM)LIMIT_ACTION_STRINGS[i]);
		};

		autorun_checkbox = GetDlgItem(hwnd, IDC_LAUNCHBOX_CHECKBOX_RUN);

		autorun_when = GetDlgItem(hwnd, IDC_LAUNCHBOX_COMBOBOX_RUN1);
		SendMessage(autorun_when, CB_RESETCONTENT, 0, 0);
		for (i = 0; i < AUTORUN_WHEN_MAX; i++)
			SendMessage(autorun_when, CB_ADDSTRING, 0,
						(LPARAM)AUTORUN_WHEN_STRINGS[i]);

		autorun_action = GetDlgItem(hwnd, IDC_LAUNCHBOX_COMBOBOX_RUN2);
		SendMessage(autorun_action, CB_RESETCONTENT, 0, 0);
		for (i = 0; i < AUTORUN_ACTION_MAX; i++)
			SendMessage(autorun_action, CB_ADDSTRING, 0,
						(LPARAM)AUTORUN_ACTION_STRINGS[i]);

		/*
		 * Create the tree view.
		 */
		{
			RECT r;
			POINT pt;
			WPARAM font;

			GetWindowRect(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TREEVIEW), &r);
			pt.x = r.left;
			pt.y = r.top;
			ScreenToClient(hwnd, &pt);

			treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
										WS_CHILD | 
#ifdef WINDOWS_NT351_COMPATIBLE
										(config->have_shell ? 0 : WS_BORDER) |
#endif WINDOWS_NT351_COMPATIBLE
										WS_VISIBLE |
										WS_TABSTOP | TVS_HASLINES |
										(config->options & OPTION_ENABLEDRAGDROP ? 
										0 :	TVS_DISABLEDRAGDROP) | 
										TVS_HASBUTTONS |
										TVS_LINESATROOT | TVS_SHOWSELALWAYS | 
										TVS_EDITLABELS,
										pt.x, pt.y,
										r.right - r.left, r.bottom - r.top,
										hwnd, NULL, config->hinst, NULL);
			font = SendMessage(hwnd, WM_GETFONT, 0, 0);
			SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

			if (config->image_list)
				TreeView_SetImageList(treeview, config->image_list, TVSIL_NORMAL);
		}

		ShowWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TABVIEW), SW_HIDE);

		/*
		 * Create the tab view.
		 */
		{
			RECT r;
			POINT pt;
			WPARAM font;
			TCITEM item;
			unsigned int i;

			GetWindowRect(GetDlgItem(hwnd, IDC_LAUNCHBOX_GROUPBOX_TABVIEW), &r);
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
			dlghdr->hwndTab = tabview;
			dlghdr->rcDisplay.left = pt.x;
			dlghdr->rcDisplay.top = pt.y;
			dlghdr->rcDisplay.right = pt.x + (r.right - r.left);
			dlghdr->rcDisplay.bottom = pt.y + (r.bottom - r.top);

			TabCtrl_AdjustRect(tabview, FALSE, &dlghdr->rcDisplay);

			memset(&item, 0, sizeof(TCITEM));
			item.mask = TCIF_TEXT | TCIF_IMAGE;
			item.iImage = -1;
			for (i = 0; i < 2 /*TAB_LAST*/; i++) {
				HRSRC hrsrc;
				HGLOBAL hglobal;

				item.pszText = TAB_NAMES[i];
				TabCtrl_InsertItem(tabview, i, &item);
				hrsrc = FindResource(NULL, MAKEINTRESOURCE(IDD_LAUNCHBOX_TAB0 + i), RT_DIALOG);
				hglobal = LoadResource(config->hinst, hrsrc);
				dlghdr->apRes[i] = (DLGTEMPLATE *)LockResource(hglobal);
			};

			TabCtrl_GetItemRect(tabview, 0, &r);
			dlghdr->rcDisplay.top += (r.bottom - r.top) + 5;

			SetWindowLong(hwnd, GWL_USERDATA, (LONG)dlghdr);

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
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_UNDO, "&Undo");
			AppendMenu(context_menu, MF_SEPARATOR, 0, "");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_CUT, "Cu&t");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_COPY, "&Copy");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_PASTE, "&Paste");
			AppendMenu(context_menu, MF_SEPARATOR, 0, 0);
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_DELETE, "&Delete");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_RENAME, "&Rename");
			AppendMenu(context_menu, MF_SEPARATOR, 0, "");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTFLD, "New &folder");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTSES, "New sessi&on");
		}

		SendMessage(hwnd, WM_REFRESHTV, 0, 0);

		if (config->options & OPTION_ENABLESAVECURSOR) {
			char curpos[BUFSIZE];

			if (reg_read_s(PLAUNCH_REGISTRY_ROOT, PLAUNCH_SAVEDCURSORPOS, NULL, curpos, BUFSIZE)) {
				HTREEITEM pos;

				pos = treeview_getitemfrompath(treeview, curpos);

				if (pos) {
					TreeView_EnsureVisible(treeview, pos);
					TreeView_SelectItem(treeview, pos);
				};
			};
		};

		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);

		{
			RECT r1, r2;
			int i;

			GetWindowRect(hwnd, &r1);
			bigpt.x = r1.right - r1.left;
			bigpt.y = r1.bottom - r1.top;
			GetWindowRect(GetDlgItem(hwnd, IDC_LAUNCHBOX_STATIC_DIVIDER), &r2);
			smallpt.x = bigpt.x;
			smallpt.y = r2.top - r1.top;
			morestate = 1;
			reg_read_i(PLAUNCH_REGISTRY_ROOT, PLAUNCH_SAVEMOREBUTTON, 0, &i);
			if (!i) {
				SendMessage(hwnd, WM_COMMAND, IDC_LAUNCHBOX_BUTTON_MORE, 0);
				center_window(hwnd);
			};
		}

		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(treeview);

		return FALSE;
	case WM_DESTROY:
		unmake_hotkey(hotkey_edit);

		return FALSE;
	case WM_REFRESHTV:
		/*
		 * Refresh the tree view contents.
		 */
		{
			TreeView_DeleteAllItems(treeview);
			treeview_addtree(treeview, TVI_ROOT, "");
			TreeView_SelectSetFirstVisible(treeview, TreeView_GetRoot(treeview));
		}
		break;
	case WM_REFRESHBUTTONS:
		/*
		 * Refresh the dialog's push buttons and context menu items state.
		 */
		{
			HTREEITEM item;
			char buf2[10], name[BUFSIZE], path[BUFSIZE], buf[BUFSIZE];
			unsigned int isfolder, flag, flag2, boolean, htype, hotkey;

			item = (HTREEITEM)lParam;

			if (!item && TreeView_GetCount(treeview))
				item = TreeView_GetSelection(treeview);

			if (!item)
				break;

			memset(name, 0, BUFSIZE);
			memset(path, 0, BUFSIZE);

			treeview_getitemname(treeview, item, name, BUFSIZE);
			treeview_getitempath(treeview, item, path);
			isfolder = is_folder(path);
			reg_make_path(NULL, path, buf);
			htype = SendMessage(hotkey_combo, CB_GETCURSEL, 0, 0);
			htype = SendMessage(hotkey_combo, CB_GETITEMDATA, (WPARAM)htype, 0);
			sprintf(buf2, "%s%d", HOTKEY, htype);
			reg_read_i(buf, buf2, 0, &hotkey);

			if (!name || !strcmp(name, DEFAULTSETTINGS)) {
				boolean = FALSE;
				flag = MF_GRAYED;
			} else {
				boolean = TRUE;
				flag = MF_ENABLED;
			};

			set_hotkey(hotkey_edit, hotkey);
			EnableWindow(hotkey_combo, boolean && !isfolder);
			EnableWindow(hotkey_edit, boolean && !isfolder);
			EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_HOTKEY), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_EDIT), !isfolder);
			EnableWindow(GetDlgItem(hwnd, IDOK), !isfolder);

			EnableWindow(limit_checkbox, boolean);
			EnableWindow(autorun_checkbox, boolean);

			if (!cut_or_copy) {
				flag = MF_ENABLED;
				flag2 = MF_GRAYED;
				boolean = FALSE;
				EnableMenuItem(context_menu, IDM_CTXM_COPY, flag);
				EnableMenuItem(context_menu, IDM_CTXM_CUT, flag);
				ModifyMenu(context_menu, IDM_CTXM_CANCEL, MF_STRING | MF_BYCOMMAND,
							IDM_CTXM_UNDO, "Undo");
			} else {
				flag = MF_GRAYED;
				flag2 = MF_ENABLED;
				boolean = FALSE;
				ModifyMenu(context_menu, IDM_CTXM_UNDO, MF_STRING | MF_BYCOMMAND,
							IDM_CTXM_CANCEL, 
							(cut_or_copy == 1 ?
							"Cancel Cut" :
							"Cancel Copy"));
			};

			EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_DELETE), boolean);
			EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_RENAME), boolean);
			EnableMenuItem(context_menu, IDM_CTXM_CUT, flag);
			EnableMenuItem(context_menu, IDM_CTXM_COPY, flag);
			EnableMenuItem(context_menu, IDM_CTXM_PASTE, flag2);
			EnableMenuItem(context_menu, IDM_CTXM_DELETE, flag2);
			EnableMenuItem(context_menu, IDM_CTXM_RENAME, flag2);

			if (morestate) {
				int i, tmp, lnumber, limit = 0, autorun = 0;

				ShowWindow(tabview, SW_SHOW);
				EnableWindow(tabview, TRUE);

				reg_read_i(buf, PLAUNCH_LIMIT_ENABLE, 0, &limit);

				CheckDlgButton(hwnd, IDC_LAUNCHBOX_CHECKBOX_LIMIT, 
					limit ? BST_CHECKED : BST_UNCHECKED);

				reg_read_i(buf, PLAUNCH_LIMIT_NUMBER, 0, &tmp);
				GetWindowText(limit_edit, buf2, BUFSIZE);
				lnumber = small_atoi(buf2);
				if (tmp != lnumber) {
					sprintf(buf2, "%d", tmp);
					SetWindowText(limit_edit, buf2);
				};
				EnableWindow(limit_edit, limit);
				lnumber = tmp;

				SendMessage(limit_searchfor, CB_RESETCONTENT, 0, 0);
				for (i = 0; i < tmp + 1; i++) {
					if (i < LIMIT_SEARCHFOR_UMPTEENTH) {
						SendMessage(limit_searchfor, CB_ADDSTRING, 0,
									(LPARAM)LIMIT_SEARCHFOR_STRINGS[i]);
					} else {
						sprintf(buf2, 
							LIMIT_SEARCHFOR_STRINGS[LIMIT_SEARCHFOR_UMPTEENTH],	i);
						SendMessage(limit_searchfor, CB_ADDSTRING, 0, (LPARAM)buf2);
					};
				};
				reg_read_i(buf, PLAUNCH_LIMIT_SEARCHFOR, 0, &tmp);
				SendMessage(limit_searchfor, CB_SETCURSEL, tmp, 0);
				EnableWindow(limit_searchfor, limit && lnumber);

				reg_read_i(buf, PLAUNCH_LIMIT_ACTION1, 0, &tmp);
				SendMessage(limit_action1, CB_SETCURSEL, (WPARAM)tmp, 0);
				EnableWindow(limit_action1, limit && lnumber);
				boolean = (tmp > 0);

				reg_read_i(buf, PLAUNCH_LIMIT_ACTION2, 0, &tmp);
				SendMessage(limit_action2, CB_SETCURSEL, (WPARAM)tmp, 0);
				EnableWindow(limit_action2, limit && lnumber && boolean);

				reg_read_i(buf, PLAUNCH_AUTORUN_ENABLE, 0, &autorun);

				CheckDlgButton(hwnd, IDC_LAUNCHBOX_CHECKBOX_RUN, 
					autorun ? BST_CHECKED : BST_UNCHECKED);

				reg_read_i(buf, PLAUNCH_AUTORUN_WHEN, 0, &tmp);
				SendMessage(autorun_when, CB_SETCURSEL, (WPARAM)tmp, 0);
				EnableWindow(autorun_when, autorun);

				reg_read_i(buf, PLAUNCH_AUTORUN_ACTION, 0, &tmp);
				SendMessage(autorun_action, CB_SETCURSEL, (WPARAM)tmp, 0);
				EnableWindow(autorun_action, autorun);
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

			if (msg = TrackPopupMenu(context_menu, TPM_LEFTALIGN | TPM_TOPALIGN |
				TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL))
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
//					ScreenToClient(treeview, &hit.pt);

					if ((hit.pt.y >= 0) && (hit.pt.y <= height / 2))
						visible = TreeView_GetPrevVisible(treeview, target);
//						visible = TreeView_GetNextItem(treeview, target, TVGN_PREVIOUS);
					else if ((hit.pt.y >= r.bottom - height / 2) && (hit.pt.y <= r.bottom))
						visible = TreeView_GetNextVisible(treeview, target);
//						visible = TreeView_GetNextItem(treeview, target, TVGN_NEXT);
					else
						visible = target;
					if (visible)
						TreeView_EnsureVisible(treeview, target);
					ImageList_DragShowNolock(FALSE);
//					TreeView_SetInsertMark(treeview, target, TRUE);
					TreeView_SelectDropTarget(treeview, target);
//					TreeView_SelectItem(treeview, target);
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

//			TreeView_SetInsertMark(treeview, NULL, TRUE);
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
					cut_or_copy = 1; // cut;
					break;
				case WM_RBUTTONUP:
					{
						HMENU menu;
						RECT r;
						int ret, height, flags;

						menu = CreatePopupMenu();
						AppendMenu(menu, MF_STRING, IDCANCEL, "Cancel");
						AppendMenu(menu, MF_SEPARATOR, 0, 0);
						AppendMenu(menu, MF_STRING, IDM_CTXM_COPY, "Copy here");
						AppendMenu(menu, MF_STRING, IDM_CTXM_CUT, "Move here");

						GetClientRect(treeview, &r);
						height = TreeView_GetItemHeight(treeview);

						flags = TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_LEFTALIGN;
						if ((hit.pt.y >= 0) && (hit.pt.y <= height))
							flags |= TPM_TOPALIGN;
						else if ((hit.pt.y >= (r.bottom - height)) && (hit.pt.y <= r.bottom))
							flags |= TPM_BOTTOMALIGN;
						ClientToScreen(treeview, &hit.pt);

						if (ret = TrackPopupMenu(menu, flags, 
							hit.pt.x, hit.pt.y, 0, hwnd, NULL)) {
							switch (ret) {
							case IDCANCEL:
								copying_now = NULL;
								return FALSE;
							case IDM_CTXM_COPY:
								cut_or_copy = 2; // copy
								break;
							case IDM_CTXM_CUT:
								cut_or_copy = 1; // cut
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
				SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_CTXM_PASTE, 0);
				return FALSE;
			};
		};
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case HK_CHANGE:
			{
				if ((HWND)lParam == hotkey_edit) {
					EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_HOTKEY), TRUE);
					return FALSE;
				};
			};
			break;
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

				index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (index)
					reg_write_i(buf, valname, index);
				else
					reg_delete_v(buf, valname);

				SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);
			};
			break;
		case EN_CHANGE:
			if (LOWORD(wParam) == IDC_LAUNCHBOX_EDITBOX_LIMIT1){
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

				SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM)item);
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

				SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM)item);
			};
		};
		switch (wParam) {
		case IDC_LAUNCHBOX_STATIC_TREEVIEW:
			SetFocus(treeview);
			break;
		case IDC_LAUNCHBOX_STATIC_HOTKEY:
			SetFocus(hotkey_edit);
			break;
		case IDC_LAUNCHBOX_BUTTON_MORE:
			{
				HWND divider;

				divider = GetDlgItem(hwnd, IDC_LAUNCHBOX_STATIC_DIVIDER);

				switch (morestate) {
					case 0: 
					{
						SetWindowPos(hwnd, 0, 0, 0, bigpt.x, bigpt.y, SWP_NOMOVE | SWP_NOZORDER);
						ShowWindow(divider, SW_SHOW);
						SetWindowText(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_MORE),
							"<< &Less");
					};
					break;
					case 1:
					{
						SetWindowPos(hwnd, 0, 0, 0,	smallpt.x, smallpt.y, 
							SWP_NOMOVE | SWP_NOZORDER);
						ShowWindow(divider, SW_HIDE);
						SetWindowText(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_MORE),
							"&More >>");
					};
					break;
				};

				morestate = !morestate;
				SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);
			};
			break;
		case IDC_LAUNCHBOX_BUTTON_HOTKEY:
			{
				HTREEITEM item;
				char path[BUFSIZE], buf[BUFSIZE], valname[BUFSIZE];
				unsigned int i, found, htype, hotkey;

				item = TreeView_GetSelection(treeview);
				treeview_getitempath(treeview, item, path);
				reg_make_path(NULL, path, buf);

				if (!SendMessage(hotkey_edit, EM_GETMODIFY, 0, 0))
					break;

				i = SendMessage(hotkey_combo, CB_GETCURSEL, 0, 0);
				htype = SendMessage(hotkey_combo, CB_GETITEMDATA, (WPARAM)i, 0);
				hotkey = get_hotkey(hotkey_edit);
				sprintf(valname, "%s%d", HOTKEY, htype);
				if (hotkey)
					reg_write_i(buf, valname, hotkey);
				else
					reg_delete_v(buf, valname);
				found = FALSE;
				for (i = 0; i < (int)config->nhotkeys; i++) {
					if (config->hotkeys[i].action == htype &&
						!strcmp(config->hotkeys[i].destination, path)) {
						found = TRUE;
						config->hotkeys[i].hotkey = hotkey;
						SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE, i, 0);
						break;
					};
				};
				if (!found) {
					config->hotkeys[config->nhotkeys].action = htype;
					config->hotkeys[config->nhotkeys].hotkey = hotkey;
					config->hotkeys[config->nhotkeys].destination = dupstr(path);
					config->nhotkeys++;
					SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
								config->nhotkeys - 1, 0);
				};

				EnableWindow(GetDlgItem(hwnd, IDC_LAUNCHBOX_BUTTON_HOTKEY), FALSE);

				return FALSE;
			}
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
					reg_write_s(PLAUNCH_REGISTRY_ROOT, PLAUNCH_SAVEDCURSORPOS, path);
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

				if (is_folder(path)) {
					TreeView_Expand(treeview, item, TVE_TOGGLE);
					return FALSE;
				};

				SendMessage(config->hwnd_mainwindow, WM_LAUNCHPUTTY,
							(WPARAM)(wParam == IDOK ?
									HOTKEY_ACTION_LAUNCH :
									HOTKEY_ACTION_EDIT),
							(LPARAM)path);
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

				if (is_folder(path))
					s = "folder and all its contents?";
				else
					s = "session?";

				sprintf(buf, "Are you sure you want to delete the \"%s\" %s", name, s);

				if (MessageBox(hwnd, buf, "Confirmation required", 
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDYES) {
					reg_make_path(NULL, path, buf);
					if (reg_delete_tree(buf))
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

				cut_or_copy = (wParam == IDM_CTXM_CUT ?
								1 : // cut
								2); // copy

				if (cut_or_copy == 1) {
					memset(&item, 0, sizeof(TVITEM));
					item.mask = TVIF_HANDLE | TVIF_STATE;
					item.state = item.stateMask = TVIS_CUT;
					item.hItem = copying_now;
					TreeView_SetItem(treeview, &item);
				};
				SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM)item.hItem);
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

				SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM)item.hItem);

				copying_now = NULL;
				cut_or_copy = 0;
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
				char copy_name[BUFSIZE], copy_path[BUFSIZE], cur_path[BUFSIZE], 
					parent_path[BUFSIZE], to_name[BUFSIZE], from_path[BUFSIZE], 
					to_path[BUFSIZE];
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

				if (cur_path[0] && is_folder(cur_path)) {
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
					treeview_getitemname(treeview, tv_copyitem, copy_name, BUFSIZE);
					treeview_getitempath(treeview, tv_copyitem, copy_path);
					isfolder = is_folder(copy_path);
					reg_make_path(NULL, copy_path, from_path);
				} else {
					isfolder = (wParam == IDC_LAUNCHBOX_BUTTON_NEW_FOLDER ||
								wParam == IDM_CTXM_CRTFLD) ? 1 : 0;
					strcpy(copy_name, isfolder ? NEWFOLDER : NEWSESSION);
					copy_path[0] = '\0';
				};

				treeview_find_unused_name(treeview, tv_parent, copy_name, to_name);
				reg_make_path(parent_path, to_name, to_path);

				switch (cut_or_copy) {
				case 0: // insert new session/folder
					success = reg_write_i(to_path, ISFOLDER, isfolder);
					break;
				case 1: // cut
					success = reg_move_tree(from_path, to_path);
					break;
				case 2: // copy
					success = reg_copy_tree(from_path, to_path);
					break;
				};

				if (success) {
					tv_newitem = treeview_additem(treeview, tv_parent, config,
						to_name, isfolder);
					if (isfolder)
						treeview_addtree(treeview, tv_newitem, to_path);
					else {
						// by default, clear all hotkeys
						for (i = 0; i < 6; i++) {
							sprintf(copy_name, "%s%d", HOTKEY, i);
							reg_delete_v(to_path, copy_name);
							reg_delete_v(to_path, copy_name);
						};
					};

					if (cut_or_copy == 1)
						TreeView_DeleteItem(treeview, tv_copyitem);

					SetFocus(treeview);

					tscb.hParent = tv_parent;
					tscb.lpfnCompare = treeview_compare;
					tscb.lParam = (LPARAM)treeview;
					TreeView_SortChildrenCB(treeview, &tscb, FALSE);

					TreeView_EnsureVisible(treeview, tv_newitem);
					TreeView_SelectItem(treeview, tv_newitem);
				};

				if (cut_or_copy) {
					copying_now = NULL;
					cut_or_copy = 0;
					ModifyMenu(context_menu, IDM_CTXM_CANCEL, MF_STRING | MF_BYCOMMAND,
								IDM_CTXM_UNDO, (cut_or_copy == 1 ?
												"Undo Cut" :
												"Undo Copy"));
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
							SendMessage(hwnd, WM_COMMAND, (WPARAM)IDOK, 0);
					};
				};
				break;
			case TVN_ITEMEXPANDED:
				{
					LPNMTREEVIEW tv = (LPNMTREEVIEW)lParam;

					if (tv->itemNew.cChildren) {
						unsigned int isexpanded, wasexpanded;
						char buf[BUFSIZE], path[BUFSIZE];

						wasexpanded = (tv->itemOld.state & TVIS_EXPANDED) ? TRUE : FALSE;
						isexpanded = (tv->itemNew.state & TVIS_EXPANDED) ? TRUE : FALSE;

						tv->itemNew.iImage = isexpanded ? 
								config->img_open : 
								config->img_closed;
						tv->itemNew.iSelectedImage = isexpanded ? 
								config->img_open : 
								config->img_closed;
						TreeView_SetItem(treeview, &tv->itemNew);

						treeview_getitempath(treeview, tv->itemNew.hItem, path);

						reg_make_path(NULL, path, buf);
						reg_write_i(buf, ISEXPANDED, isexpanded);
					};
				};
				break;
			case TVN_SELCHANGED:
				{
					LPNMTREEVIEW tv = (LPNMTREEVIEW)lParam;

					SendMessage(hwnd, WM_REFRESHBUTTONS, 0, (LPARAM)tv->itemNew.hItem);
				};
				break;
			case TVN_BEGINLABELEDIT:
				{
					LPNMTVDISPINFO di = (LPNMTVDISPINFO)lParam;
					char name[BUFSIZE];

					treeview_getitemname(treeview, di->item.hItem, name, BUFSIZE);
					if (!strcmp(name, DEFAULTSETTINGS))
						TreeView_EndEditLabelNow(treeview, TRUE);
					else
						editing_now = di->item.hItem;
				};
				break;
			case TVN_ENDLABELEDIT:
				{
					LPNMTVDISPINFO di = (LPNMTVDISPINFO)lParam;
					editing_now = NULL;

					if (di->item.pszText) {
						HTREEITEM item, parent;
						char path[BUFSIZE], parent_path[BUFSIZE], from[BUFSIZE], to[BUFSIZE];

						item = di->item.hItem;
						parent = TreeView_GetParent(treeview, item);
						if (parent)
							treeview_getitempath(treeview, parent, parent_path);
						else
							parent_path[0] = '\0';

						treeview_getitempath(treeview, item, path);

						reg_make_path(NULL, path, from);
						reg_make_path(parent_path, di->item.pszText, to);

						if (reg_move_tree(from, to)) {
							TVSORTCB tscb;

							di->item.mask = TVIF_HANDLE | TVIF_TEXT;
							TreeView_SetItem(treeview, &di->item);

							tscb.hParent = parent;
							tscb.lpfnCompare = treeview_compare;
							tscb.lParam = (LPARAM)treeview;
							TreeView_SortChildrenCB(treeview, &tscb, FALSE);

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
					LPNMTREEVIEW nmtv = (LPNMTREEVIEW)lParam;

					draglist = TreeView_CreateDragImage(treeview, nmtv->itemNew.hItem);
					TreeView_GetItemRect(treeview, nmtv->itemNew.hItem, &r, TRUE);
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

					selected = TabCtrl_GetCurSel(dlghdr->hwndTab);

					if (dlghdr->hwndDisplay != NULL)
						DestroyWindow(dlghdr->hwndDisplay);

					dlghdr->hwndDisplay = CreateDialogIndirect(config->hinst,
						dlghdr->apRes[selected], hwnd, LaunchBoxChildProc);
				};
			};
		};
	};
    return FALSE;
};

/*
 * Launch Box: setup function.
 */
int do_launchbox(void) {
    int ret;

	if (hwnd_launchbox) {
		SetForegroundWindow(hwnd_launchbox);
		return 0;
	};

	ret = DialogBox(config->hinst, MAKEINTRESOURCE(IDD_LAUNCHBOX), NULL, LaunchBoxProc);

	hwnd_launchbox = NULL;

    return (ret == IDOK);
};


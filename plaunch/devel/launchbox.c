#include "plaunch.h"
#include "dlgtmpl.h"
#include "misc.h"
#include "hotkey.h"
#include "registry.h"

#define ID_LAUNCHBOX_TREEVIEW_STATIC		100
#define ID_LAUNCHBOX_TREEVIEW_GROUPBOX		101
#define ID_LAUNCHBOX_NEW_FOLDER				102
#define ID_LAUNCHBOX_NEW_SESSION			103
#define ID_LAUNCHBOX_EDIT					104
#define ID_LAUNCHBOX_RENAME					105
#define ID_LAUNCHBOX_DELETE					106
#define ID_LAUNCHBOX_LAUNCH_HOTKEY_STATIC	107
#define ID_LAUNCHBOX_LAUNCH_HOTKEY_EDITBOX	108
#define	ID_LAUNCHBOX_EDIT_HOTKEY_STATIC		109
#define ID_LAUNCHBOX_EDIT_HOTKEY_EDITBOX	110
#define ID_LAUNCHBOX_HOTKEY_BUTTON_SET		111

#define IDM_CTXM_COPY						0x0100
#define IDM_CTXM_CUT						0x0101
#define IDM_CTXM_PASTE						0x0102
#define IDM_CTXM_DELETE						0x0103
#define IDM_CTXM_UNDO						0x0104
#define IDM_CTXM_CRTFLD						0x0105
#define IDM_CTXM_CRTSES						0x0106
#define IDM_CTXM_RENAME						0x0107

#define NEWFOLDER	"New Folder"
#define NEWSESSION	"New Session"

/*
 * Launch Box: tree view compare function.
 */

int CALLBACK treeview_compare(LPARAM p1, LPARAM p2, LPARAM sort) {
	int ret;
	char *item1, *item2;

	item1 = treeview_getitemname((HWND)sort, (HTREEITEM)p1);
	item2 = treeview_getitemname((HWND)sort, (HTREEITEM)p2);

	ret = sessioncmp(item1, item2);

	free(item1);
	free(item2);

	return ret;
};

/*
 * Launch Box: a helper subroutine that finds unused name for the item
 */

char *treeview_find_unused_name(HWND treeview, HTREEITEM parent, char *name) {
	TVITEM tvi;
	HTREEITEM child;
	char *oname, *cname, *ret;
	int i = 0;

	if (!name)
		return NULL;

	if (!parent || !TreeView_GetCount(treeview))
		return dupstr(name);

	oname = (char *)malloc(BUFSIZE);
	strcpy(oname, name);
	cname = (char *)malloc(BUFSIZE);
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

	free(cname);
	ret = dupstr(oname);
	free(oname);
	return ret;
};

/*
 * Launch Box: dialog function.
 */
static int CALLBACK LaunchBoxProc(HWND hwnd, UINT msg,
								  WPARAM wParam, LPARAM lParam) {
	static HWND treeview = NULL, launchhotkey = NULL, edithotkey = NULL;
	static HMENU context_menu = NULL;
	static int cut_or_copy = 0;
	static HTREEITEM editing_now = NULL;
	static HTREEITEM copying_now = NULL;
	static HIMAGELIST draglist = NULL;
	static int dragging_now = FALSE;

	switch (msg) {
	case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
		if (!config->have_shell)
			center_window(hwnd);
#endif /* WINDOWS_NT351_COMPATIBLE */

		hwnd_launchbox = hwnd;
		SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG,
					(LPARAM)LoadIcon(config->hinst, MAKEINTRESOURCE(IDI_MAINICON)));

		launchhotkey = GetDlgItem(hwnd, ID_LAUNCHBOX_LAUNCH_HOTKEY_EDITBOX);
		make_hotkey(launchhotkey, 0);
		edithotkey = GetDlgItem(hwnd, ID_LAUNCHBOX_EDIT_HOTKEY_EDITBOX);
		make_hotkey(edithotkey, 0);

		/*
		 * Create the tree view.
		 */
		{
			RECT r;
			POINT pt;
			WPARAM font;

			GetWindowRect(GetDlgItem(hwnd, ID_LAUNCHBOX_TREEVIEW_GROUPBOX), &r);
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
			char *curpos;

			curpos = reg_read_s(PLAUNCH_REGISTRY_ROOT, PLAUNCH_SAVEDCURSORPOS, NULL);

			if (curpos) {
				HTREEITEM pos;

				pos = treeview_getitemfrompath(treeview, curpos);

				if (pos) {
					TreeView_EnsureVisible(treeview, pos);
					TreeView_SelectItem(treeview, pos);
				};
			};

			free(curpos);
		};

		SendMessage(hwnd, WM_REFRESHBUTTONS, 0, 0);

		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(treeview);

		return FALSE;
	case WM_DESTROY:
		unmake_hotkey(launchhotkey);
		unmake_hotkey(edithotkey);

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
		 * Refresh the dialog's push button and context menu items state.
		 */
		{
			HTREEITEM item;
			char *name, *path, *buf;
			unsigned int isfolder, flag, boolean, ehotkey, lhotkey;

			item = (HTREEITEM)lParam;

			if (!item && TreeView_GetCount(treeview))
				item = TreeView_GetSelection(treeview);

			name = treeview_getitemname(treeview, item);
			path = treeview_getitempath(treeview, item);
			isfolder = is_folder(path);

			if (!name || !strcmp(name, DEFAULTSETTINGS)) {
				boolean = FALSE;
				flag = MF_GRAYED;
				lhotkey = 0;
				ehotkey = 0;
				isfolder = 1;
			} else {
				boolean = TRUE;
				flag = MF_ENABLED;
				buf = reg_make_path(NULL, path);
				lhotkey = reg_read_i(buf, HOTKEY "0", 0);
				ehotkey = reg_read_i(buf, HOTKEY "1", 0);
				free(buf);
			};

			free(path);
			free(name);

			set_hotkey(launchhotkey, lhotkey);
			set_hotkey(edithotkey, ehotkey);
			EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_DELETE), boolean);
			EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_RENAME), boolean);
			EnableWindow(launchhotkey, boolean && !isfolder);
			EnableWindow(edithotkey, boolean && !isfolder);
			EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_HOTKEY_BUTTON_SET), FALSE);
			EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_EDIT), !isfolder);
			EnableWindow(GetDlgItem(hwnd, IDOK), !isfolder);

			if (!cut_or_copy) {
				EnableMenuItem(context_menu, IDM_CTXM_COPY, flag);
				EnableMenuItem(context_menu, IDM_CTXM_CUT, flag);
				EnableMenuItem(context_menu, IDM_CTXM_DELETE, flag);
				EnableMenuItem(context_menu, IDM_CTXM_RENAME, flag);
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
//						visible = TreeView_GetPrevVisible(treeview, target);
						visible = TreeView_GetNextItem(treeview, target, TVGN_PREVIOUS);
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
					ImageList_DragShowNolock(TRUE);
				};
			};
		}
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		if (dragging_now) {
//			TreeView_SetInsertMark(treeview, NULL, TRUE);
			ImageList_EndDrag();
			ReleaseCapture();
			ShowCursor(TRUE);
			dragging_now = FALSE;
			draglist = NULL;
		};
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case HK_CHANGE:
			{
				if ((HWND)lParam == launchhotkey || 
					(HWND)lParam == edithotkey) {
					EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_HOTKEY_BUTTON_SET), TRUE);
					return FALSE;
				};
			};
		};
		switch (wParam) {
		case ID_LAUNCHBOX_TREEVIEW_STATIC:
			SetFocus(treeview);
			break;
		case ID_LAUNCHBOX_LAUNCH_HOTKEY_STATIC:
			SetFocus(launchhotkey);
			break;
		case ID_LAUNCHBOX_EDIT_HOTKEY_STATIC:
			SetFocus(edithotkey);
			break;
		case ID_LAUNCHBOX_HOTKEY_BUTTON_SET:
			{
				HTREEITEM item;
				char *path, *buf, *valname;
				int i, j, found, hotkey;

				item = TreeView_GetSelection(treeview);
				path = treeview_getitempath(treeview, item);
				buf = reg_make_path(NULL, path);
				valname = (char *)malloc(BUFSIZE);

				for (i = 0; i < 2; i++) {
					if (SendMessage(i == 0 ? 
							launchhotkey :
						edithotkey, EM_GETMODIFY, 0, 0)) {
						hotkey = get_hotkey(i == 0 ? launchhotkey :	edithotkey);
						sprintf(valname, "%s%d", HOTKEY, i);
						if (hotkey)
							reg_write_i(buf, valname, hotkey);
						else
							reg_delete_v(buf, valname);
						found = FALSE;
						for (j = 0; j < config->nhotkeys; j++) {
							if (config->hotkeys[j].action == (i == 0 ?
																HOTKEY_ACTION_LAUNCH :
																HOTKEY_ACTION_EDIT) &&
								!strcmp(config->hotkeys[j].destination, path)) {
								found = TRUE;
								config->hotkeys[j].hotkey = hotkey;
								SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE, j, 0);
								break;
							};
						};
						if (!found) {
							config->hotkeys[config->nhotkeys].action = (i == 0 ?
																		HOTKEY_ACTION_LAUNCH :
																		HOTKEY_ACTION_EDIT);
							config->hotkeys[config->nhotkeys].hotkey = hotkey;
							config->hotkeys[config->nhotkeys].destination = dupstr(path);
							config->nhotkeys++;
							SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
										config->nhotkeys - 1, 0);
						};
					};
				};

				free(valname);
				free(buf);
				free(path);

				EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_HOTKEY_BUTTON_SET), FALSE);

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
					char *path;

					item = TreeView_GetSelection(treeview);
					path = treeview_getitempath(treeview, item);
					reg_write_s(PLAUNCH_REGISTRY_ROOT, PLAUNCH_SAVEDCURSORPOS, path);
					free(path);
				};

			};
		case ID_LAUNCHBOX_EDIT:
			{
				HTREEITEM item;
				char *path, *buf;

				item = TreeView_GetSelection(treeview);
				path = treeview_getitempath(treeview, item);

				if (!path) {
					EndDialog(hwnd, 0);
					return FALSE;
				};

				if (is_folder(path)) {
					TreeView_Expand(treeview, item, TVE_TOGGLE);
					return FALSE;
				};

				buf = malloc(strlen(path) + 9);

				sprintf(buf, (wParam == IDOK) ? 
								"-load \"%s\"" :
								"-edit \"%s\"", path);

				ShellExecute(hwnd, "open", config->putty_path,
							buf, _T(""), SW_SHOWNORMAL);

				free(buf);
				free(path);

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
		case ID_LAUNCHBOX_RENAME:
		case IDM_CTXM_RENAME:
			{
				HTREEITEM item;

				EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
				item = TreeView_GetSelection(treeview);
				SetFocus(treeview);
				TreeView_EditLabel(treeview, item);

				return TRUE;
			};
		case ID_LAUNCHBOX_DELETE:
		case IDM_CTXM_DELETE:
			{
				HTREEITEM item;
				char *buf, *name, *path;

				item = TreeView_GetSelection(treeview);
				name = treeview_getitemname(treeview, item);
				path = treeview_getitempath(treeview, item);

				buf = (char *)malloc(BUFSIZE);

				sprintf(buf, "Are you sure you want to delete the \"%s\"", name);

				if (is_folder(path))
					strcat(buf, " folder and all its contents?");
				else
					strcat(buf, " session?");

				if (MessageBox(hwnd, buf, "Confirmation required", 
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDYES) {
					buf = reg_make_path(NULL, path);
					if (reg_delete_tree(buf))
						TreeView_DeleteItem(treeview, item);
				};

				free(buf);
				free(path);
				free(name);

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

				EnableMenuItem(context_menu, IDM_CTXM_CUT, MF_GRAYED);
				EnableMenuItem(context_menu, IDM_CTXM_COPY, MF_GRAYED);
				EnableMenuItem(context_menu, IDM_CTXM_PASTE, MF_ENABLED);

				memset(&item, 0, sizeof(TVITEM));
				item.mask = TVIF_HANDLE | TVIF_STATE;
				item.state = item.stateMask = TVIS_CUT;
				item.hItem = copying_now;
				TreeView_SetItem(treeview, &item);
			};
			break;
		case ID_LAUNCHBOX_NEW_FOLDER:
		case IDM_CTXM_CRTFLD:
		case ID_LAUNCHBOX_NEW_SESSION:
		case IDM_CTXM_CRTSES:
		case IDM_CTXM_PASTE:
			{
				TVSORTCB tscb;
				HTREEITEM tv_parent, tv_curitem, tv_newitem, tv_copyitem;
				char *copy_name, *copy_path, *cur_path, *parent_path, *to_name, 
					*from_path, *to_path;
				int isfolder, success = 0;

				tv_copyitem = copying_now;
				tv_curitem = TreeView_GetSelection(treeview);

				if (tv_curitem) {
					tv_parent = TreeView_GetParent(treeview, tv_curitem);
					cur_path = treeview_getitempath(treeview, tv_curitem);
				} else {
					tv_parent = NULL;
					cur_path = NULL;
				};

				if (cur_path && is_folder(cur_path)) {
					tv_parent = tv_curitem;
					parent_path = dupstr(cur_path);
				} else if (tv_parent) {
					tv_parent = tv_parent;
					parent_path = treeview_getitempath(treeview, tv_parent);
				} else {
					tv_parent = TVI_ROOT;
					parent_path = NULL;
				};

				if (cut_or_copy) {
					copy_name = treeview_getitemname(treeview, tv_copyitem);
					copy_path = treeview_getitempath(treeview, tv_copyitem);
					isfolder = is_folder(copy_path);
					from_path = reg_make_path(NULL, copy_path);
				} else {
					isfolder = (wParam == ID_LAUNCHBOX_NEW_FOLDER ||
								wParam == IDM_CTXM_CRTFLD) ? 1 : 0;
					copy_name = dupstr(isfolder ? NEWFOLDER : NEWSESSION);
					copy_path = NULL;
					from_path = NULL;
				};

				to_name = treeview_find_unused_name(treeview, tv_parent, copy_name);
				to_path = reg_make_path(parent_path, to_name);

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

				free(from_path);
				free(parent_path);
				free(cur_path);
				free(copy_name);
				free(copy_path);

				if (success) {
					tv_newitem = treeview_additem(treeview, tv_parent, config,
						to_name, isfolder);
					if (isfolder)
						treeview_addtree(treeview, tv_newitem, to_path);
					else {
						// by default, clear all hotkeys
						reg_delete_v(to_path, HOTKEY "0");
						reg_delete_v(to_path, HOTKEY "1");
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

				free(to_name);
				free(to_path);

				if (cut_or_copy) {
					copying_now = NULL;
					cut_or_copy = 0;
					EnableMenuItem(context_menu, IDM_CTXM_CUT, MF_ENABLED);
					EnableMenuItem(context_menu, IDM_CTXM_COPY, MF_ENABLED);
					EnableMenuItem(context_menu, IDM_CTXM_PASTE, MF_GRAYED);
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
						int isexpanded, wasexpanded;
						char *path, *buf;

						wasexpanded = (tv->itemOld.state & TVIS_EXPANDED) ? TRUE : FALSE;
						isexpanded = (tv->itemNew.state & TVIS_EXPANDED) ? TRUE : FALSE;

						tv->itemNew.iImage = isexpanded ? 
								config->img_open : 
								config->img_closed;
						tv->itemNew.iSelectedImage = isexpanded ? 
								config->img_open : 
								config->img_closed;
						TreeView_SetItem(treeview, &tv->itemNew);

						path = treeview_getitempath(treeview, tv->itemNew.hItem);

						buf = reg_make_path(NULL, path);
						reg_write_i(buf, ISEXPANDED, isexpanded);
						free(buf);
						free(path);
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
					char *name;

					name = treeview_getitemname(treeview, di->item.hItem);
					if (!strcmp(name, DEFAULTSETTINGS)) {
						TreeView_EndEditLabelNow(treeview, TRUE);
						free(name);
					} else {
						editing_now = di->item.hItem;
						free(name);
					};
				};
				break;
			case TVN_ENDLABELEDIT:
				{
					LPNMTVDISPINFO di = (LPNMTVDISPINFO)lParam;
					editing_now = NULL;

					if (di->item.pszText) {
						HTREEITEM item, parent;
						char *path, *parent_path, *from, *to;

						item = di->item.hItem;
						parent = TreeView_GetParent(treeview, item);
						if (parent)
							parent_path = treeview_getitempath(treeview, parent);
						else
							parent_path = NULL;

						path = treeview_getitempath(treeview, item);

						from = reg_make_path(NULL, path);
						to = reg_make_path(parent_path, di->item.pszText);

						if (reg_move_tree(from, to)) {
							TVSORTCB tscb;

							di->item.mask = TVIF_HANDLE | TVIF_TEXT;
							TreeView_SetItem(treeview, &di->item);

							tscb.hParent = TreeView_GetParent(treeview, item);
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

					dragging_now = TRUE;
				};
				break;
			};
		};
	};
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
								0, 0, 242, 202, "PuTTY session manager", 14, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								4, 4, 75, 9, "&Sessions:",
								ID_LAUNCHBOX_TREEVIEW_STATIC);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | BS_GROUPBOX,
								4, 17, 180, 150, NULL,
								ID_LAUNCHBOX_TREEVIEW_GROUPBOX);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								188, 17, 50, 14, "Open", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								188, 34, 50, 14, "Cancel", IDCANCEL);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								188, 68, 50, 14, "New &folder", ID_LAUNCHBOX_NEW_FOLDER);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								188, 84, 50, 15, "New sessi&on", ID_LAUNCHBOX_NEW_SESSION);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								188, 100, 50, 14, "&Edit", ID_LAUNCHBOX_EDIT);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								188, 116, 50, 14, "&Rename", ID_LAUNCHBOX_RENAME);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								188, 132, 50, 14, "&Delete", ID_LAUNCHBOX_DELETE);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								4, 171, 88, 9, "&Launch hotkey:", 
								ID_LAUNCHBOX_LAUNCH_HOTKEY_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
								4, 184, 88, 14, NULL, 
								ID_LAUNCHBOX_LAUNCH_HOTKEY_EDITBOX);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								96, 171, 88, 9, "Ed&it hotkey:",
								ID_LAUNCHBOX_EDIT_HOTKEY_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
								96, 184, 88, 14, NULL,
								ID_LAUNCHBOX_EDIT_HOTKEY_EDITBOX);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								188, 184, 50, 14, "Se&t",
								ID_LAUNCHBOX_HOTKEY_BUTTON_SET);

	ret = DialogBoxIndirect(config->hinst, (LPDLGTEMPLATE)tmpl, NULL, LaunchBoxProc);

	GlobalFree(tmpl);
	hwnd_launchbox = NULL;

    return (ret == IDOK);
}


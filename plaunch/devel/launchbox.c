#include "plaunch.h"
#include "session.h"
#include "dlgtmpl.h"
#include "misc.h"
#include "hotkey.h"

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
	struct session *s1 =
		(struct session *)p1;
	struct session *s2 = 
		(struct session *)p2;

	return session_compare(s1, s2);
};

/*
 * Launch Box: a helper subroutine that finds unused name for the item
 */

char *treeview_find_unused_name(HWND treeview, HTREEITEM parent, char *name) {
	TVITEM tvi;
	HTREEITEM child;
	char *oname, *cname;
	int i = 0;
	
	oname = (char *)malloc(BUFSIZE);
	strcpy(oname, name);
	cname = (char *)malloc(BUFSIZE);
	child = TreeView_GetChild(treeview, parent);
	while (child) {
		memset(&tvi, 0, sizeof(TVITEM));
		tvi.hItem = child;
		tvi.mask = TVIF_TEXT;
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
	return oname;
};

/*
 * Launch Box: dialog function.
 */
static int CALLBACK LaunchBoxProc(HWND hwnd, UINT msg,
								  WPARAM wParam, LPARAM lParam) {
	static HWND treeview = NULL, launchhotkey = NULL, edithotkey = NULL;
	static HMENU context_menu = NULL;
	static int tvctrl, cut_or_copy = 0;
	static struct session *root;
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
										WS_CHILD | WS_VISIBLE |
										WS_TABSTOP | TVS_HASLINES |
										TVS_DISABLEDRAGDROP | TVS_HASBUTTONS |
										TVS_LINESATROOT | TVS_SHOWSELALWAYS | 
										TVS_EDITLABELS,
										pt.x, pt.y,
										r.right - r.left, r.bottom - r.top,
										hwnd, NULL, config->hinst, NULL);
			font = SendMessage(hwnd, WM_GETFONT, 0, 0);
			SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
			tvctrl = GetDlgCtrlID(treeview);

			if (config->image_list)
				TreeView_SetImageList(treeview, config->image_list, TVSIL_NORMAL);
		}

		/*
		 * Set up the tree view contents.
		 */
		{
			HTREEITEM hfirst = NULL;

			root = session_get_root();
			hfirst = treeview_addsession(treeview, TVI_ROOT, root);

			TreeView_SelectItem(treeview, TreeView_GetFirstVisible(treeview));
		}

		/*
		 * Create a popup context menu and fill it with elements.
		 */
		if (!context_menu) {
			context_menu = CreatePopupMenu();
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_UNDO, "&Undo");
			AppendMenu(context_menu, MF_SEPARATOR, 0, "");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_CUT, "Cu&t");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_COPY, "&Copy");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_PASTE, "&Paste");
			AppendMenu(context_menu, MF_SEPARATOR, 0, 0);
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_DELETE, "&Delete");
			AppendMenu(context_menu, MF_STRING | MF_GRAYED, IDM_CTXM_RENAME, "&Rename");
			AppendMenu(context_menu, MF_SEPARATOR, 0, "");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTFLD, "New &folder");
			AppendMenu(context_menu, MF_STRING, IDM_CTXM_CRTSES, "New sessi&on");
		}

		SetWindowLong(hwnd, GWL_USERDATA, 1);
		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(treeview);

		return FALSE;
	case WM_DESTROY:
		unmake_hotkey(launchhotkey);
		unmake_hotkey(edithotkey);

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

			return TRUE;
		}
	case WM_REFRESHTV:
		/*
		 * Refresh the tree view contents.
		 */
		{
			HTREEITEM hfirst = NULL;

			TreeView_DeleteAllItems(treeview);
			session_free(root);

			root = session_get_root();
			hfirst = treeview_addsession(treeview, TVI_ROOT, root);

			TreeView_SelectItem(treeview, TreeView_GetFirstVisible(treeview));

			break;
		}
	case WM_MOUSEMOVE:
		{
			HTREEITEM target;
			TVHITTESTINFO hit;
			int x, y;

			if (dragging_now) {
				x = LOWORD(lParam);
				y = HIWORD(lParam);

				ImageList_DragMove(x, y);

				hit.pt.x = x;
				hit.pt.y = y;

				if ((target = TreeView_HitTest(treeview, &hit))) {
					TreeView_EnsureVisible(treeview, target);
					TreeView_SelectDropTarget(treeview, target);
//					TreeView_SetInsertMark(treeview, target, TRUE);
					UpdateWindow(treeview);
				};
			};
		}
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		{
			if (dragging_now) {
				ImageList_EndDrag();
				ReleaseCapture();
				ShowCursor(TRUE);
				dragging_now = FALSE;
				draglist = NULL;
			};
		}
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
		case ID_LAUNCHBOX_LAUNCH_HOTKEY_STATIC:
			SetFocus(launchhotkey);
			break;
		case ID_LAUNCHBOX_EDIT_HOTKEY_STATIC:
			SetFocus(edithotkey);
			break;
		case ID_LAUNCHBOX_HOTKEY_BUTTON_SET:
			{
				TVITEM item;
				struct session *s;
				int i, j, found, hotkey;

				item.hItem = TreeView_GetSelection(treeview);
				item.mask = TVIF_PARAM;
				TreeView_GetItem(treeview, &item);
				s = (struct session *)item.lParam;

				if (!s)
					return FALSE;

				for (i = 0; i < 2; i++) {
					if (SendMessage(i == 0 ? 
							launchhotkey :
						edithotkey, EM_GETMODIFY, 0, 0)) {
						hotkey = get_hotkey(i == 0 ? launchhotkey :	edithotkey);
						s->hotkeys[i] = hotkey;
						s->nhotkeys = 2;
						session_set_hotkey(s, i, hotkey);
						found = FALSE;
						for (j = 0; j < config->nhotkeys; j++) {
							if (config->hotkeys[j].action == (i == 0 ?
																HOTKEY_ACTION_LAUNCH :
																HOTKEY_ACTION_EDIT) &&
								config->hotkeys[j].destination == s->id) {
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
							config->hotkeys[config->nhotkeys].destination = s->id;
							config->nhotkeys++;
							SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
										config->nhotkeys - 1, 0);
						};
					};
				};
				EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_HOTKEY_BUTTON_SET), FALSE);

				return FALSE;
			}
		case IDOK:
			{
				if (editing_now) {
					TreeView_EndEditLabelNow(treeview, FALSE);
					return TRUE;
				};
			};
		case ID_LAUNCHBOX_EDIT:
			{
				TVITEM item;
				struct session *s;
				char *p;

				if (!TreeView_GetCount(treeview)) {
					EndDialog(hwnd, 0);
					return FALSE;
				};

				item.hItem = TreeView_GetSelection(treeview);
				item.mask = TVIF_PARAM;
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

				p = session_get_full_path(s);

				if (p) {
					char *buf;

					buf = malloc(strlen(s->name) + 9);

					if (wParam == IDOK)
						sprintf(buf, "-load \"%s\"", s->name);
					else 
						sprintf(buf, "-edit \"%s\"", s->name);

					ShellExecute(hwnd, "open", config->putty_path,
								buf, _T(""), SW_SHOWNORMAL);
					free(buf);
					free(p);
				} else {
					ShellExecute(hwnd, "open", config->putty_path,
								_T(""), _T(""), SW_SHOWNORMAL);
				};

				if (root)
					root = session_free(root);

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
					root = session_free(root);

				EndDialog(hwnd, 0);

				return FALSE;
			};
		case ID_LAUNCHBOX_RENAME:
		case IDM_CTXM_RENAME:
			{
				HTREEITEM item;

				item = TreeView_GetSelection(treeview);
				SetFocus(treeview);
				TreeView_EditLabel(treeview, item);

				return TRUE;
			};
		case ID_LAUNCHBOX_DELETE:
		case IDM_CTXM_DELETE:
			{
				TVITEM item;
				struct session *s;
				char *buf;

				item.hItem = TreeView_GetSelection(treeview);
				item.mask = TVIF_PARAM;
				TreeView_GetItem(treeview, &item);
				s = (struct session *)item.lParam;

				buf = (char *)malloc(BUFSIZE);

				sprintf(buf, "Are you sure you want to delete the \"%s\"", s->name);

				if (s && s->type)
					strcat(buf, " folder and all its contents?");
				else
					strcat(buf, " session?");

				if (MessageBox(hwnd, buf, 
					"Confirmation required", 
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES ||
					!session_delete(s)) {
					free(buf);
					return FALSE;
				};

				free(buf);

				SendMessage(hwnd, WM_REFRESHTV, 0, 0);

				return TRUE;
			}
		case ID_LAUNCHBOX_NEW_FOLDER:
		case IDM_CTXM_CRTFLD:
		case ID_LAUNCHBOX_NEW_SESSION:
		case IDM_CTXM_CRTSES:
			{
				HTREEITEM tv_curitem, tv_parent, tv_newitem;
				TVITEM tvi;
				TVINSERTSTRUCT tvins;
				TVSORTCB tscb;
				struct session *ses_curitem, *ses_parent, *ses_newitem;
				int type;

				type = (wParam == ID_LAUNCHBOX_NEW_FOLDER ||
						wParam == IDM_CTXM_CRTFLD) ?
						STYPE_FOLDER :
						STYPE_SESSION;

				tv_curitem = tvi.hItem = TreeView_GetSelection(treeview);
				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(treeview, &tvi);
				ses_curitem = (struct session *)tvi.lParam;

				if (!ses_curitem)
					return FALSE;

				tv_parent = TreeView_GetParent(treeview, tv_curitem);

				if (ses_curitem->type) {
					ses_parent = ses_curitem;
					tv_parent = tv_curitem;
				} else if (tv_parent) {
					ses_parent = ses_curitem->parent;
					tv_parent = tv_parent;
				} else {
					ses_parent = root;
					tv_parent = NULL;
				};

				ses_newitem = session_create(type, type ? NEWFOLDER : NEWSESSION, ses_parent);

				if (!ses_newitem)
					return FALSE;

				ses_parent->children = 
					session_insert(ses_parent->children, &ses_parent->nchildren, ses_newitem);

				tvins.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE |
					(type ? TVIF_CHILDREN : 0);
				tvins.item.pszText = ses_newitem->name;
				tvins.item.cchTextMax = strlen(ses_newitem->name);
				tvins.item.lParam = (LPARAM)ses_newitem;
				if (type)
					tvins.item.cChildren = type;
				tvins.item.iImage = tvins.item.iSelectedImage = 
					type ? config->img_closed : config->img_session;
				tvins.hInsertAfter = tv_parent;
				tvins.hParent = tv_parent;
				tv_newitem = TreeView_InsertItem(treeview, &tvins);

				SetFocus(treeview);

				tscb.hParent = tv_parent;
				tscb.lpfnCompare = treeview_compare;
				tscb.lParam = 0;
				TreeView_SortChildrenCB(treeview, &tscb, FALSE);

				TreeView_EnsureVisible(treeview, tv_newitem);
				TreeView_SelectItem(treeview, tv_newitem);
				TreeView_EditLabel(treeview, tv_newitem);

				return TRUE;
			};
		case IDM_CTXM_CUT:
			copying_now = TreeView_GetSelection(treeview);
			TreeView_SelectDropTarget(treeview, copying_now);
			cut_or_copy = 1; // cut

			EnableMenuItem(context_menu, IDM_CTXM_CUT, MF_GRAYED);
			EnableMenuItem(context_menu, IDM_CTXM_COPY, MF_GRAYED);
			EnableMenuItem(context_menu, IDM_CTXM_PASTE, MF_ENABLED);
			DrawMenuBar(hwnd);

			return TRUE;
		case IDM_CTXM_COPY:
			copying_now = TreeView_GetSelection(treeview);
			cut_or_copy = 2; // copy

			EnableMenuItem(context_menu, IDM_CTXM_CUT, MF_GRAYED);
			EnableMenuItem(context_menu, IDM_CTXM_COPY, MF_GRAYED);
			EnableMenuItem(context_menu, IDM_CTXM_PASTE, MF_ENABLED);
			DrawMenuBar(hwnd);

			return TRUE;
		case IDM_CTXM_PASTE:
			{
				TVITEM tvi;
				TVSORTCB tscb;
				HTREEITEM tv_parent, tv_curitem, tv_newitem, tv_copyitem;
				struct session *ses_parent, *ses_curitem, *ses_newitem, *ses_copyitem;
				char *path, *to, *to_path;
				int success = 0;

				tv_copyitem = copying_now;
				memset(&tvi, 0, sizeof(TVITEM));
				tvi.hItem = tv_copyitem;
				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(treeview, &tvi);
				ses_copyitem = (struct session *)tvi.lParam;

				memset(&tvi, 0, sizeof(TVITEM));
				tv_curitem = TreeView_GetSelection(treeview);
				tvi.hItem = tv_curitem;
				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(treeview, &tvi);
				ses_curitem = (struct session *)tvi.lParam;

				tv_parent = TreeView_GetParent(treeview, tv_curitem);

				if (ses_curitem->type) {
					ses_parent = ses_curitem;
					tv_parent = tv_curitem;
				} else if (tv_parent) {
					ses_parent = ses_curitem->parent;
					tv_parent = tv_parent;
				} else {
					ses_parent = root;
					tv_parent = NULL;
				};

				path = session_get_full_path(ses_parent);
				to = treeview_find_unused_name(treeview, tv_parent,
					ses_copyitem->name);
				to_path = (char *)malloc(BUFSIZE);
				if (path) {
					strcpy(to_path, path);
					strcat(to_path, "\\");
				} else
					strcpy(to_path, "");
				strcat(to_path, to);

				switch (cut_or_copy) {
				case 1: // cut
					success = session_rename(ses_copyitem, to_path);
					break;
				case 2: // copy
					success = session_copy(ses_copyitem, to_path);
					break;
				};

				if (success) {
					ses_newitem = session_duplicate(ses_copyitem);
					if (ses_newitem->name)
						free(ses_newitem->name);
					ses_newitem->name = (char *)malloc(strlen(to) + 1);
					strcpy(ses_newitem->name, to);
					ses_newitem->parent = ses_parent;
					ses_parent->children = 
						session_insert(ses_parent->children, 
							&ses_parent->nchildren, 
							ses_newitem);
					if (cut_or_copy == 1) {
						ses_copyitem->parent->children =
							session_remove(ses_copyitem->parent->children,
							&ses_copyitem->parent->nchildren,
							ses_copyitem);
						session_free(ses_copyitem);
					};

					tv_newitem = treeview_addsession(treeview, tv_parent, ses_newitem);

					if (cut_or_copy == 1)
						TreeView_DeleteItem(treeview, copying_now);

					SetFocus(treeview);

					tscb.hParent = tv_parent;
					tscb.lpfnCompare = treeview_compare;
					tscb.lParam = 0;
					TreeView_SortChildrenCB(treeview, &tscb, FALSE);

					TreeView_EnsureVisible(treeview, tv_newitem);
					TreeView_SelectItem(treeview, tv_newitem);
				};

				if (to)
					free(to);
				if (path)
					free(path);
				if (to_path)
					free(to_path);

				copying_now = NULL;
				cut_or_copy = 0;
				EnableMenuItem(context_menu, IDM_CTXM_CUT, MF_ENABLED);
				EnableMenuItem(context_menu, IDM_CTXM_COPY, MF_ENABLED);
				EnableMenuItem(context_menu, IDM_CTXM_PASTE, MF_GRAYED);

				return TRUE;
			}
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

						tv->itemNew.iImage = isexpanded ? 
								config->img_open : 
								config->img_closed;
						tv->itemNew.iSelectedImage = isexpanded ? 
								config->img_open : 
								config->img_closed;
						TreeView_SetItem(treeview, &tv->itemNew);

						if (s->isexpanded != isexpanded) {
							s->isexpanded = isexpanded;
							session_set_expanded(s, isexpanded);
						};

						return FALSE;
					};
				};
				break;
			case TVN_SELCHANGED:
				{
					LPNMTREEVIEW tv = (LPNMTREEVIEW)lParam;
					int boolean, flag, lhotkey, ehotkey;
					struct session *s = 
						(struct session *)tv->itemNew.lParam;

					if (s && s->name && !strcmp(s->name, DEFAULTSETTINGS)) {
						boolean = FALSE;
						flag = MF_GRAYED;
						lhotkey = 0;
						ehotkey = 0;
					} else {
						boolean = TRUE;
						flag = MF_ENABLED;
						lhotkey = s->hotkeys[0];
						ehotkey = s->hotkeys[1];
					};

					set_hotkey(launchhotkey, lhotkey);
					set_hotkey(edithotkey, ehotkey);
					EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_DELETE), boolean);
					EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_RENAME), boolean);
					EnableWindow(launchhotkey, boolean);
					EnableWindow(edithotkey, boolean);
					EnableWindow(GetDlgItem(hwnd, ID_LAUNCHBOX_HOTKEY_BUTTON_SET), FALSE);

					if (!cut_or_copy) {
						EnableMenuItem(context_menu, IDM_CTXM_CUT, flag);
						EnableMenuItem(context_menu, IDM_CTXM_DELETE, flag);
						EnableMenuItem(context_menu, IDM_CTXM_RENAME, flag);
					};

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

						if (session_rename(s, di->item.pszText)) {
							TVSORTCB tscb;

							TreeView_SetItem(treeview, &di->item);

							tscb.hParent = TreeView_GetParent(treeview, &di->item);
							tscb.lpfnCompare = treeview_compare;
							tscb.lParam = 0;
							TreeView_SortChildrenCB(treeview, &tscb, FALSE);

							TreeView_EnsureVisible(treeview, &di->item);
							TreeView_SelectItem(treeview, &di->item);

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
			case TVN_BEGINDRAG:
			case TVN_BEGINRDRAG:
				{
					RECT r;
					DWORD indent;
					LPNMTREEVIEW nmtv = (LPNMTREEVIEW)lParam;

					draglist = TreeView_CreateDragImage(treeview, nmtv->itemNew.hItem);
					TreeView_GetItemRect(treeview, nmtv->itemNew.hItem, &r, TRUE);
					indent = TreeView_GetIndent(treeview);

					ImageList_BeginDrag(draglist, 0, 0, 0);
					ImageList_DragEnter(treeview, 50, 50);

					ShowCursor(FALSE);
					SetCapture(hwnd);

					dragging_now = TRUE;
				};
				break;
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


#include "hotkey.h"

#define CONTROL_KEY "Ctrl"
#define ALT_KEY "Alt"
#define SHIFT_KEY "Shift"
#define WIN_KEY "Win"
#define PLUS_SIGN " + "

#define BUFSIZE 100

static WNDPROC oldwindowproc;

char *key_name(UINT modifiers, UINT vkey) {
	char *buf, *buf2, key;

	buf = (char *)malloc(BUFSIZE);
	memset(buf, 0, BUFSIZE);

	if (modifiers & MOD_CONTROL) {
		strcat(buf, CONTROL_KEY);
		strcat(buf, PLUS_SIGN);
	};
	if (modifiers & MOD_ALT) {
		strcat(buf, ALT_KEY);
		strcat(buf, PLUS_SIGN);
	};
	if (modifiers & MOD_SHIFT) {
		strcat(buf, SHIFT_KEY);
		strcat(buf, PLUS_SIGN);
	};
	if (modifiers & MOD_WIN) {
		strcat(buf, WIN_KEY);
		strcat(buf, PLUS_SIGN);
	};

	if (vkey >= 0x30 && vkey <= 0x5a) {
		key = (char)MapVirtualKey(vkey, 2);
		buf2 = (char *)malloc(BUFSIZE);
		sprintf(buf2, "%c", key);
		strcat(buf, buf2);
		free(buf2);
	} else if (vkey >= VK_F1 && vkey <= VK_F24) {
		buf2 = (char *)malloc(BUFSIZE);
		sprintf(buf2, "F%d", (int)(vkey - VK_F1 + 1));
		strcat(buf, buf2);
		free(buf2);
	};

	return buf;
};

static int CALLBACK HotKeyControlProc(HWND hwnd, UINT msg,
									  WPARAM wParam, LPARAM lParam) {
	static LONG oldhotkey = 0;
	static int firstmessage = FALSE;

	switch (msg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSDEADCHAR:
	case WM_SYSCHAR:
		return FALSE;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		{
			UINT modifiers, vkey;
			char *buf;

			firstmessage = TRUE;

//			if (wParam < 0x30 || (wParam > 0x5a && wParam < 0x70))
//				return TRUE;

			modifiers = 0;
			vkey = wParam;

			if (GetAsyncKeyState(VK_SHIFT))
				modifiers |= MOD_SHIFT;
			if (GetAsyncKeyState(VK_CONTROL))
				modifiers |= MOD_CONTROL;
			if (GetAsyncKeyState(VK_MENU))
				modifiers |= MOD_ALT;
			if (GetAsyncKeyState(VK_LWIN) || GetAsyncKeyState(VK_RWIN))
				modifiers |= MOD_WIN;

			if (!modifiers ||
				(((modifiers == MOD_CONTROL) ||
				(modifiers == MOD_ALT) ||
				(modifiers == MOD_SHIFT)) &&
				(vkey >= 0x30 && vkey <= 0x5a)))
				return TRUE;

			oldhotkey = GetWindowLong(hwnd, GWL_USERDATA);

			buf = key_name(modifiers, vkey);
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buf);
			free(buf);

			return FALSE;
		};
	case WM_KEYUP:
	case WM_SYSKEYUP:
		{
			LONG hotkey;
			UINT modifiers, vkey;
			char *buf;
			int modf = 0;

			if (!firstmessage)
				return FALSE;
			else
				firstmessage = FALSE;

			modifiers = 0;
			vkey = wParam;

			switch (vkey) {
			case VK_BACK:
			case VK_CLEAR:
				hotkey = 0;
				SetWindowLong(hwnd, GWL_USERDATA, hotkey);
				SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)"None");
				SendMessage(hwnd, EM_SETMODIFY, (WPARAM)TRUE, 0);
				SendMessage(GetParent(hwnd),
							WM_COMMAND,
							MAKEWPARAM(GetDlgCtrlID(hwnd), HK_CHANGE),
							(LPARAM)hwnd);

				return FALSE;
			case VK_TAB:
				SetFocus(GetNextDlgTabItem(GetParent(hwnd),
											hwnd,
											GetAsyncKeyState(VK_SHIFT)));

				return FALSE;
			case VK_RETURN:
				PostMessage(GetParent(hwnd), WM_COMMAND, (WPARAM)IDOK, 0);

				return FALSE;
			case VK_ESCAPE:
				PostMessage(GetParent(hwnd), WM_COMMAND, (WPARAM)IDCANCEL, 0);

				return FALSE;
			};

//			if (wParam < 0x30 || (wParam > 0x5a && wParam < 0x70))
//				return TRUE;

			if (GetAsyncKeyState(VK_SHIFT)) {
				modifiers |= MOD_SHIFT;
				modf++;
			};
			if (GetAsyncKeyState(VK_CONTROL)) {
				modifiers |= MOD_CONTROL;
				modf++;
			};
			if (GetAsyncKeyState(VK_MENU)) {
				modifiers |= MOD_ALT;
				modf++;
			};
			if (GetAsyncKeyState(VK_LWIN) || GetAsyncKeyState(VK_RWIN)) {
				modifiers |= MOD_WIN;
				modf++;
			};

			if (!modifiers ||
				((modf < 2) && !(modifiers & MOD_WIN)) ||
				(vkey <= 0x30 || vkey >= 0x5a)) {
				SetWindowLong(hwnd, GWL_USERDATA, oldhotkey);
				SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)"");
				SendMessage(hwnd, EM_SETMODIFY, (WPARAM)FALSE, 0);
				return TRUE;
			};

			if (RegisterHotKey(hwnd, 0, modifiers, vkey)) {
				hotkey = MAKELPARAM(modifiers, vkey);
				SetWindowLong(hwnd, GWL_USERDATA, hotkey);
				buf = key_name(modifiers, vkey);
				SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buf);
				free(buf);
				UnregisterHotKey(hwnd, 0);
				SendMessage(hwnd, EM_SETMODIFY, (WPARAM)TRUE, 0);
				SendMessage(GetParent(hwnd), 
							WM_COMMAND, 
							MAKEWPARAM(GetDlgCtrlID(hwnd), HK_CHANGE), 
							(LPARAM)hwnd);
			} else {
				SetWindowLong(hwnd, GWL_USERDATA, oldhotkey);
				SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)"Invalid hotkey");
				SendMessage(hwnd, EM_SETMODIFY, (WPARAM)TRUE, 0);
			};

			return FALSE;
		};
	default:
		return CallWindowProc(oldwindowproc, hwnd, msg, wParam, lParam);
	};
};

int make_hotkey(HWND editbox, LONG defhotkey) {
	char *buf;

	oldwindowproc = 
			(WNDPROC)SetWindowLong(editbox, GWL_WNDPROC, (LONG)HotKeyControlProc);
	SetWindowLong(editbox, GWL_USERDATA, defhotkey);
	if (defhotkey) {
		buf = key_name(LOWORD(defhotkey), HIWORD(defhotkey));
		SendMessage(editbox, WM_SETTEXT, 0, (LPARAM)buf);
		free(buf);
	} else
		SendMessage(editbox, WM_SETTEXT, 0, (LPARAM)"None");

	return TRUE;
};

int unmake_hotkey(HWND editbox) {
	SetWindowLong(editbox, GWL_WNDPROC, (LONG)oldwindowproc);
	
	return TRUE;
};

LONG get_hotkey(HWND editbox) {
	return GetWindowLong(editbox, GWL_USERDATA);
};

LONG set_hotkey(HWND editbox, LONG newhotkey) {
	char *buf;
	LONG oldhotkey;

	oldhotkey = SetWindowLong(editbox, GWL_USERDATA, newhotkey);
	if (newhotkey) {
		buf = key_name(LOWORD(newhotkey), HIWORD(newhotkey));
		SendMessage(editbox, WM_SETTEXT, 0, (LPARAM)buf);
		free(buf);
	} else
		SendMessage(editbox, WM_SETTEXT, 0, (LPARAM)"None");

	return oldhotkey;
};

/*
 * PLaunch: a convenient PuTTY launching and session-management utility.
 * Distributed under MIT license, same as PuTTY itself.
 * (c) 2004-2006 dwalin <dwalin@dwalin.ru>
 * Portions (c) Simon Tatham.
 *
 * Advanced Hotkey Control implementation file.
 */

#include "hotkey.h"

#if !defined(_DEBUG) && defined(MINIRTL)
#include "entry.h"
#else
#include <stdio.h>
#endif				/* _NDEBUG && MINIRTL */

#define HOTKEY_NONE		"None"
#define HOTKEY_INVALID		"Invalid hotkey"
#define HOTKEY_CONTROL_KEY	"Ctrl"
#define HOTKEY_ALT_KEY		"Alt"
#define HOTKEY_SHIFT_KEY	"Shift"
#define HOTKEY_WIN_KEY		"Win"
#define HOTKEY_PLUS_SIGN	" + "

#define BUFSIZE 100

static WNDPROC oldwindowproc;

void key_name(UINT modifiers, UINT vkey, char *buffer,
	      unsigned int bufsize)
{
    char buf2[10];

    memset(buffer, 0, bufsize);

    if (modifiers & MOD_CONTROL) {
	strcat(buffer, HOTKEY_CONTROL_KEY);
	strcat(buffer, HOTKEY_PLUS_SIGN);
    };
    if (modifiers & MOD_ALT) {
	strcat(buffer, HOTKEY_ALT_KEY);
	strcat(buffer, HOTKEY_PLUS_SIGN);
    };
    if (modifiers & MOD_SHIFT) {
	strcat(buffer, HOTKEY_SHIFT_KEY);
	strcat(buffer, HOTKEY_PLUS_SIGN);
    };
    if (modifiers & MOD_WIN) {
	strcat(buffer, HOTKEY_WIN_KEY);
	strcat(buffer, HOTKEY_PLUS_SIGN);
    };

    if (vkey >= 0x30 && vkey <= 0x5a) {
	buf2[0] = (char) vkey;
	buf2[1] = '\0';
	strcat(buffer, buf2);
    } else if (vkey >= VK_F1 && vkey <= VK_F24) {
	unsigned int i, j;

	i = vkey - VK_F1 + 1;
	j = 0;

	buf2[j++] = 'F';
	if (i > 9) {
	    buf2[j++] = (char) ((i / 10) + 0x30);
	    buf2[j++] = (char) ((i - ((i / 10) * 10)) + 0x30);
	} else
	    buf2[j++] = (char) (i + 0x30);
	buf2[j++] = '\0';
	strcat(buffer, buf2);
    };
};

static int CALLBACK HotKeyControlProc(HWND hwnd, UINT msg,
				      WPARAM wParam, LPARAM lParam)
{
    LONG hotkey;
    static LONG oldhotkey = 0;
    static unsigned int firstmessage = FALSE;

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
	    char buf[BUFSIZE];

	    firstmessage = TRUE;

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

	    key_name(modifiers, vkey, buf, BUFSIZE);
	    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) buf);

	    return FALSE;
	};
    case WM_KEYUP:
    case WM_SYSKEYUP:
	{
	    UINT modifiers, vkey;
	    char buf[BUFSIZE];
	    unsigned int modf = 0;

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
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) HOTKEY_NONE);
		SendMessage(hwnd, EM_SETMODIFY, (WPARAM) TRUE, 0);
		SendMessage(GetParent(hwnd), WM_COMMAND,
			    MAKEWPARAM(GetDlgCtrlID(hwnd), HK_CHANGE),
			    (LPARAM) hwnd);

		return FALSE;
	    case VK_TAB:
		hotkey = GetWindowLong(hwnd, GWL_USERDATA);
		if (hotkey) {
		    modifiers = LOWORD(hotkey);
		    vkey = HIWORD(hotkey);
		    key_name(modifiers, vkey, buf, BUFSIZE);
		    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) buf);
		} else
		    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) HOTKEY_NONE);
		SendMessage(hwnd, EM_SETMODIFY, (WPARAM) TRUE, 0);
		SetFocus(GetNextDlgTabItem(GetParent(hwnd),
					   hwnd,
					   GetAsyncKeyState(VK_SHIFT)));

		return FALSE;
	    case VK_RETURN:
		PostMessage(GetParent(hwnd), WM_COMMAND, (WPARAM) IDOK, 0);

		return FALSE;
	    case VK_ESCAPE:
		hotkey = GetWindowLong(hwnd, GWL_USERDATA);
		if (hotkey) {
		    modifiers = LOWORD(hotkey);
		    vkey = HIWORD(hotkey);
		    key_name(modifiers, vkey, buf, BUFSIZE);
		    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) buf);
		} else
		    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) HOTKEY_NONE);
		SendMessage(hwnd, EM_SETMODIFY, (WPARAM) TRUE, 0);
		PostMessage(GetParent(hwnd), WM_COMMAND, (WPARAM) IDCANCEL,
			    0);

		return FALSE;
	    };

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
		!((vkey >= 0x30 && vkey <= 0x5a) ||
		  (vkey >= VK_F1 && vkey <= VK_F24))) {
		SetWindowLong(hwnd, GWL_USERDATA, oldhotkey);
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) HOTKEY_NONE);
		SendMessage(hwnd, EM_SETMODIFY, (WPARAM) FALSE, 0);
		return TRUE;
	    };

	    if (RegisterHotKey(hwnd, 0, modifiers, vkey)) {
		hotkey = MAKELPARAM(modifiers, vkey);
		SetWindowLong(hwnd, GWL_USERDATA, hotkey);
		key_name(modifiers, vkey, buf, BUFSIZE);
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) buf);
		UnregisterHotKey(hwnd, 0);
		SendMessage(hwnd, EM_SETMODIFY, (WPARAM) TRUE, 0);
		SendMessage(GetParent(hwnd),
			    WM_COMMAND,
			    MAKEWPARAM(GetDlgCtrlID(hwnd), HK_CHANGE),
			    (LPARAM) hwnd);
	    } else {
		SetWindowLong(hwnd, GWL_USERDATA, oldhotkey);
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) HOTKEY_INVALID);
		SendMessage(hwnd, EM_SETMODIFY, (WPARAM) TRUE, 0);
	    };

	    return FALSE;
	};
    };
    return CallWindowProc(oldwindowproc, hwnd, msg, wParam, lParam);
};

unsigned int make_hotkey(HWND editbox, LONG defhotkey)
{
    char buf[BUFSIZE];

    oldwindowproc =
	(WNDPROC) SetWindowLong(editbox, GWL_WNDPROC,
				(LONG) HotKeyControlProc);
    SetWindowLong(editbox, GWL_USERDATA, defhotkey);
    if (defhotkey) {
	key_name(LOWORD(defhotkey), HIWORD(defhotkey), buf, BUFSIZE);
	SendMessage(editbox, WM_SETTEXT, 0, (LPARAM) buf);
    } else
	SendMessage(editbox, WM_SETTEXT, 0, (LPARAM) HOTKEY_NONE);

    return TRUE;
};

unsigned int unmake_hotkey(HWND editbox)
{
    SetWindowLong(editbox, GWL_WNDPROC, (LONG) oldwindowproc);

    return TRUE;
};

LONG get_hotkey(HWND editbox)
{
    return GetWindowLong(editbox, GWL_USERDATA);
};

LONG set_hotkey(HWND editbox, LONG newhotkey)
{
    char buf[BUFSIZE];
    LONG oldhotkey;

    oldhotkey = SetWindowLong(editbox, GWL_USERDATA, newhotkey);
    if (newhotkey) {
	key_name(LOWORD(newhotkey), HIWORD(newhotkey), buf, BUFSIZE);
	SendMessage(editbox, WM_SETTEXT, 0, (LPARAM) buf);
    } else
	SendMessage(editbox, WM_SETTEXT, 0, (LPARAM) HOTKEY_NONE);

    return oldhotkey;
};

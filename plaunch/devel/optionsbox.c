#include <windows.h>
#include "plaunch.h"
#include "dlgtmpl.h"
#include "hotkey.h"
#include "misc.h"
#include "registry.h"

#define ID_OPTIONS_GROUPBOX			120
#define ID_OPTIONS_PUTTYPATH_STATIC	121
#define	ID_OPTIONS_PUTTYPATH_EDIT	122
#define ID_OPTIONS_PUTTYPATH_BUTTON	123
#define	ID_OPTIONS_WL_HOTKEY_STATIC	124
#define ID_OPTIONS_WL_HOTKEY_EDIT	125
#define ID_OPTIONS_LB_HOTKEY_STATIC	126
#define	ID_OPTIONS_LB_HOTKEY_EDIT	127
#define ID_OPTIONS_BUTTON_STARTUP	128
#define ID_OPTIONS_BUTTON_DRAGDROP	129

#define PLAUNCH_AUTO_STARTUP	"Software\\Microsoft\\Windows\\CurrentVersion\\Run"

/*
 * Options Box: dialog function.
 */
static int CALLBACK OptionsBoxProc(HWND hwnd, UINT msg,
								   WPARAM wParam, LPARAM lParam) {
	static HWND ppedit, ppbutton, lbedit, wledit;
	static DWORD lb_key, wl_key;
	int check;
	char *buf;

    switch (msg) {
    case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
		if (!config->have_shell)
			center_window(hwnd);
#endif /* WINDOWS_NT351_COMPATIBLE */

		ppedit = GetDlgItem(hwnd, ID_OPTIONS_PUTTYPATH_EDIT);
		ppbutton = GetDlgItem(hwnd, ID_OPTIONS_PUTTYPATH_BUTTON);
		lbedit = GetDlgItem(hwnd, ID_OPTIONS_LB_HOTKEY_EDIT);
		make_hotkey(lbedit, config->hotkeys[0].hotkey);
		wledit = GetDlgItem(hwnd, ID_OPTIONS_WL_HOTKEY_EDIT);
		make_hotkey(wledit, config->hotkeys[1].hotkey);

		buf = reg_read_s(PLAUNCH_AUTO_STARTUP, APPNAME, NULL);

		if (buf) {
			free(buf);
			check = BST_CHECKED;
		} else
			check = BST_UNCHECKED;

		CheckDlgButton(hwnd, ID_OPTIONS_BUTTON_STARTUP, check);

		check = config->dragdrop ? BST_CHECKED : BST_UNCHECKED;

		CheckDlgButton(hwnd, ID_OPTIONS_BUTTON_DRAGDROP, check);

		SendMessage(ppedit, (UINT)EM_SETLIMITTEXT, (WPARAM)BUFSIZE, 0);

		if (config->putty_path)
			SetWindowText(ppedit, config->putty_path);

		SetFocus(ppedit);

		return FALSE;
	case WM_DESTROY:
		unmake_hotkey(lbedit);
		unmake_hotkey(wledit);
		return FALSE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			{
				LONG hotkey;
				int what = 0;

				hotkey = get_hotkey(lbedit);

				if (hotkey) {
					config->hotkeys[0].hotkey = hotkey;
					what |= CFG_SAVE_HOTKEY_LB;
					SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE, 0, 0);
				};

				hotkey = get_hotkey(wledit);

				if (hotkey) {
					config->hotkeys[1].hotkey = hotkey;
					what |= CFG_SAVE_HOTKEY_WL;
					SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE, 1, 0);
				};

				if (SendMessage(ppedit, EM_GETMODIFY, 0, 0)) {
					char *buf;

					buf = (char *)malloc(BUFSIZE);
					memset(buf, 0, BUFSIZE);
					GetWindowText(ppedit, buf, BUFSIZE);

					if (config->putty_path)
						free(config->putty_path);

					if (buf[0] == '\0') {
						config->putty_path = get_putty_path();
					} else {
						config->putty_path = (char *)malloc(strlen(buf) + 1);
						strcpy(config->putty_path, buf);
						free(buf);
					};

					what |= CFG_SAVE_PUTTY_PATH;
				};

				check = IsDlgButtonChecked(hwnd, ID_OPTIONS_BUTTON_STARTUP);

				if (check == BST_CHECKED) {
					buf = (char *)malloc(BUFSIZE);
					GetModuleFileName(NULL, buf, BUFSIZE);
					reg_write_s(PLAUNCH_AUTO_STARTUP, APPNAME, buf);
					free(buf);
				} else if (check == BST_UNCHECKED)
					reg_delete_v(PLAUNCH_AUTO_STARTUP, APPNAME);

				check = IsDlgButtonChecked(hwnd, ID_OPTIONS_BUTTON_DRAGDROP);

				config->dragdrop = check > 0 ? TRUE : FALSE;
				what |= CFG_SAVE_DRAGDROP;

				save_config(config, what);

				EndDialog(hwnd, 0);

				return FALSE;
			}
		case IDCANCEL:
			EndDialog(hwnd, 0);

			return FALSE;
		case ID_OPTIONS_PUTTYPATH_BUTTON:
			{
				OPENFILENAME ofn;
				char *buf;

				buf = (char *)malloc(BUFSIZE);
				GetWindowText(ppedit, buf, BUFSIZE);

				memset(&ofn, 0, sizeof(OPENFILENAME));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hwnd;
				ofn.lpstrFilter = "PuTTY executable\0putty.exe;puttytel.exe\0All Files\0*.*\0\0";
				ofn.lpstrFile = buf;
				ofn.nMaxFile = BUFSIZE;
				ofn.lpstrTitle = "Locate PuTTY executable";
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

				if (GetOpenFileName(&ofn)) {
					if (config->putty_path)
						free(config->putty_path);
					config->putty_path = (char *)malloc(strlen(buf) + 1);
					strcpy(config->putty_path, buf);
					SendMessage(ppedit, WM_SETTEXT, 0, (LPARAM)buf);
					SendMessage(ppedit, EM_SETMODIFY, (WPARAM)TRUE, 0);
				};

				free(buf);

				return FALSE;
			}
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
	default:
		return FALSE;
	};
	return FALSE;
};

/*
 * Options Box: setup function.
 */
void do_optionsbox(void) {
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE * 2);
	ptr = dialogtemplate_create(tmpl, WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU |
								DS_CENTER | DS_MODALFRAME,
								0, 0, 229, 150, "PuTTY Launcher Options", 12, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
								7, 7, 215, 136, NULL, ID_OPTIONS_GROUPBOX);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								16, 17, 169, 9, "&Path to PuTTY or PuTTYtel executable:",
								ID_OPTIONS_PUTTYPATH_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
								ES_AUTOHSCROLL,
								16, 31, 139, 14, NULL, ID_OPTIONS_PUTTYPATH_EDIT);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								164, 31, 50, 14, "&Browse...", 
								ID_OPTIONS_PUTTYPATH_BUTTON);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								16, 54, 75, 9, "&Launch Box hot key:",
								ID_OPTIONS_LB_HOTKEY_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
								16, 68, 95, 14, NULL, ID_OPTIONS_LB_HOTKEY_EDIT);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE,
								118, 54, 75, 9, "&Window List hot key:",
								ID_OPTIONS_WL_HOTKEY_STATIC);
	ptr = dialogtemplate_addeditbox(ptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
								118, 68, 95, 14, NULL, ID_OPTIONS_WL_HOTKEY_EDIT);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
								16, 91, 200, 9, "&Run PuTTY Launcher automatically at startup?",
								ID_OPTIONS_BUTTON_STARTUP);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
								16, 105, 200, 9, "&Enable tree-view items drag&&drop in Launch Box?",
								ID_OPTIONS_BUTTON_DRAGDROP);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								61, 121, 50, 14, "OK", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								118, 121, 50, 14, "Cancel", IDCANCEL);

	DialogBoxIndirect(config->hinst, (LPDLGTEMPLATE)tmpl, NULL, OptionsBoxProc);

	GlobalFree(tmpl);
};

#include <windows.h>
#include <stdio.h>
#include "entry.h"
#include "plaunch.h"
#include "hotkey.h"
#include "misc.h"
#include "registry.h"
#include "resource.h"
#include "dlgtmpl.h"

#define PLAUNCH_AUTO_STARTUP	"Software\\Microsoft\\Windows\\CurrentVersion\\Run"

/*
 * Options Box: dialog function.
 */
static int CALLBACK OptionsBoxProc(HWND hwnd, UINT msg,
				   WPARAM wParam, LPARAM lParam)
{
    static HWND ppedit, ppbutton, lbedit, wledit, hdedit, cycledit;
    static DWORD lb_key, wl_key;
    int check;
    char buf[256];

    switch (msg) {
    case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
	if (!config->have_shell)
	    center_window(hwnd);
#endif				/* WINDOWS_NT351_COMPATIBLE */
	SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
		    (LPARAM) config->main_icon);

	ppedit = GetDlgItem(hwnd, IDC_OPTIONSBOX_EDITBOX_PUTTYPATH);
	ppbutton = GetDlgItem(hwnd, IDC_OPTIONSBOX_BUTTON_PUTTYPATH);
	lbedit = GetDlgItem(hwnd, IDC_OPTIONSBOX_EDITBOX_HOTKEY_LBOX);
	make_hotkey(lbedit, config->hotkeys[HOTKEY_LAUNCHBOX].hotkey);
	wledit = GetDlgItem(hwnd, IDC_OPTIONSBOX_EDITBOX_HOTKEY_WLIST);
	make_hotkey(wledit, config->hotkeys[HOTKEY_WINDOWLIST].hotkey);
	hdedit =
	    GetDlgItem(hwnd, IDC_OPTIONSBOX_EDITBOX_HOTKEY_HIDEWINDOW);
	make_hotkey(hdedit, config->hotkeys[HOTKEY_HIDEWINDOW].hotkey);
	cycledit = GetDlgItem(hwnd, IDC_OPTIONSBOX_EDITBOX_HOTKEY_CYCLEWINDOW);
	make_hotkey(cycledit, config->hotkeys[HOTKEY_CYCLEWINDOW].hotkey);

	if (reg_read_s(PLAUNCH_AUTO_STARTUP, APPNAME, NULL, buf, BUFSIZE))
	    check = BST_CHECKED;
	else
	    check = BST_UNCHECKED;

	CheckDlgButton(hwnd, IDC_OPTIONSBOX_CHECKBOX_STARTUP, check);

	check =
	    config->
	    options & OPTION_ENABLEDRAGDROP ? BST_CHECKED : BST_UNCHECKED;

	CheckDlgButton(hwnd, IDC_OPTIONSBOX_CHECKBOX_DRAGDROP, check);

	check =
	    config->
	    options & OPTION_ENABLESAVECURSOR ? BST_CHECKED :
	    BST_UNCHECKED;

	CheckDlgButton(hwnd, IDC_OPTIONSBOX_CHECKBOX_SAVECUR, check);

	check =
	    config->
	    options & OPTION_SHOWONQUIT ? BST_CHECKED : BST_UNCHECKED;

	CheckDlgButton(hwnd, IDC_OPTIONSBOX_CHECKBOX_SHOWONQUIT, check);

	check =
	    config->
	    options & OPTION_MENUSESSIONS ? BST_CHECKED : BST_UNCHECKED;

	CheckDlgButton(hwnd, IDC_OPTIONSBOX_CHECKBOX_MENUSESSIONS, check);

	check =
	    config->
	    options & OPTION_MENURUNNING ? BST_CHECKED : BST_UNCHECKED;

	CheckDlgButton(hwnd, IDC_OPTIONSBOX_CHECKBOX_MENURUNNING, check);

	SendMessage(ppedit, (UINT) EM_SETLIMITTEXT, (WPARAM) BUFSIZE, 0);

	if (config->putty_path)
	    SetWindowText(ppedit, config->putty_path);

	SetFocus(ppedit);

	return FALSE;
    case WM_DESTROY:
	unmake_hotkey(lbedit);
	unmake_hotkey(wledit);
	unmake_hotkey(hdedit);
	unmake_hotkey(cycledit);
	return FALSE;
    case WM_COMMAND:
	switch (wParam) {
	case IDOK:
	    {
		LONG hotkey;
		unsigned int what = 0;

		hotkey = get_hotkey(lbedit);

		if (hotkey) {
		    config->hotkeys[HOTKEY_LAUNCHBOX].hotkey = hotkey;
		    what |= CFG_SAVE_HOTKEY_LB;
		    SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
				HOTKEY_LAUNCHBOX, 0);
		};

		hotkey = get_hotkey(wledit);

		if (hotkey) {
		    config->hotkeys[HOTKEY_WINDOWLIST].hotkey = hotkey;
		    what |= CFG_SAVE_HOTKEY_WL;
		    SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
				HOTKEY_WINDOWLIST, 0);
		};

		hotkey = get_hotkey(hdedit);

		if (hotkey) {
		    config->hotkeys[HOTKEY_HIDEWINDOW].hotkey = hotkey;
		    what |= CFG_SAVE_HOTKEY_HIDEWINDOW;
		    SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
				HOTKEY_HIDEWINDOW, 0);
		};

		hotkey = get_hotkey(cycledit);
		if (hotkey) {
		    config->hotkeys[HOTKEY_CYCLEWINDOW].hotkey = hotkey;
		    what |= CFG_SAVE_HOTKEY_CYCLEWINDOW;
		    SendMessage(config->hwnd_mainwindow, WM_HOTKEYCHANGE,
				HOTKEY_CYCLEWINDOW, 0);
		};

		if (SendMessage(ppedit, EM_GETMODIFY, 0, 0)) {
		    GetWindowText(ppedit, buf, 256);

		    if (buf[0] == '\0')
			get_putty_path(config->putty_path, BUFSIZE);
		    else
			strcpy(config->putty_path, buf);

		    what |= CFG_SAVE_PUTTY_PATH;
		};

		check =
		    IsDlgButtonChecked(hwnd,
				       IDC_OPTIONSBOX_CHECKBOX_STARTUP);

#ifndef _DEBUG
		if (check == BST_CHECKED) {
		    GetModuleFileName(NULL, buf, 256);
		    reg_write_s(PLAUNCH_AUTO_STARTUP, APPNAME, buf);
		} else if (check == BST_UNCHECKED)
		    reg_delete_v(PLAUNCH_AUTO_STARTUP, APPNAME);
#endif				/* _DEBUG */

		check =
		    IsDlgButtonChecked(hwnd,
				       IDC_OPTIONSBOX_CHECKBOX_DRAGDROP);

		config->options =
		    check > 0 ?
		    config->options | OPTION_ENABLEDRAGDROP :
		    config->options & ~OPTION_ENABLEDRAGDROP;
		what |= CFG_SAVE_DRAGDROP;

		check =
		    IsDlgButtonChecked(hwnd,
				       IDC_OPTIONSBOX_CHECKBOX_SAVECUR);

		config->options =
		    check > 0 ?
		    config->options | OPTION_ENABLESAVECURSOR :
		    config->options & ~OPTION_ENABLESAVECURSOR;
		what |= CFG_SAVE_SAVECURSOR;

		check =
		    IsDlgButtonChecked(hwnd,
				       IDC_OPTIONSBOX_CHECKBOX_SHOWONQUIT);

		config->options =
		    check > 0 ?
		    config->options | OPTION_SHOWONQUIT :
		    config->options & ~OPTION_SHOWONQUIT;
		what |= CFG_SAVE_SHOWONQUIT;

		check =
		    IsDlgButtonChecked(hwnd,
				       IDC_OPTIONSBOX_CHECKBOX_MENUSESSIONS);

		config->options =
		    check > 0 ?
		    config->options | OPTION_MENUSESSIONS :
		    config->options & ~OPTION_MENUSESSIONS;
		what |= CFG_SAVE_MENUSESSIONS;

		check =
		    IsDlgButtonChecked(hwnd,
				       IDC_OPTIONSBOX_CHECKBOX_MENURUNNING);

		config->options =
		    check > 0 ?
		    config->options | OPTION_MENURUNNING :
		    config->options & ~OPTION_MENURUNNING;
		what |= CFG_SAVE_MENURUNNING;

		save_config(config, what);

		EndDialog(hwnd, 0);

		return FALSE;
	    }
	case IDCANCEL:
	    EndDialog(hwnd, 0);

	    return FALSE;
	case IDC_OPTIONSBOX_BUTTON_PUTTYPATH:
	    {
		OPENFILENAME ofn;

		GetWindowText(ppedit, buf, 256);

		memset(&ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter =
		    "PuTTY executable\0tutty.exe;tuttytel.exe;putty.exe;puttytel.exe\0All Files\0*.*\0\0";
		ofn.lpstrFile = buf;
		ofn.nMaxFile = BUFSIZE;
		ofn.lpstrTitle = "Locate PuTTY executable";
		ofn.Flags =
		    OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
		    OFN_PATHMUSTEXIST;

		if (GetOpenFileName(&ofn)) {
		    strcpy(config->putty_path, buf);
		    SendMessage(ppedit, WM_SETTEXT, 0, (LPARAM) buf);
		    SendMessage(ppedit, EM_SETMODIFY, (WPARAM) TRUE, 0);
		};

		return FALSE;
	    }
	case IDC_OPTIONSBOX_STATIC_PUTTYPATH:
	    SetFocus(ppedit);

	    return FALSE;
	case IDC_OPTIONSBOX_STATIC_HOTKEY_LBOX:
	    SetFocus(lbedit);

	    return FALSE;
	case IDC_OPTIONSBOX_STATIC_HOTKEY_WLIST:
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
void do_optionsbox(void)
{
    DialogBox(config->hinst, MAKEINTRESOURCE(IDD_OPTIONSBOX), NULL,
	      OptionsBoxProc);
};

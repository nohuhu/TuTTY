#include "plaunch.h"
#include "resource.h"
#include "dlgtmpl.h"

#define HOMEPAGE	"http://putty.dwalin.ru/plaunch"

/*
 * About Box: dialog function.
 */
static int CALLBACK AboutProc(HWND hwnd, UINT msg,
							  WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:

#ifdef WINDOWS_NT351_COMPATIBLE
		if (!config->have_shell)
			center_window(hwnd);
#endif /* WINDOWS_NT351_COMPATIBLE */
		SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM)config->main_icon);
#ifdef RELEASE
		SetWindowText(GetDlgItem(hwnd, IDC_ABOUTBOX_STATIC_VERSION),
			"Version " APPVERSION ", release.");
#else
		SetWindowText(GetDlgItem(hwnd, IDC_ABOUTBOX_STATIC_VERSION),
			"Verion " APPVERSION ", built on " __DATE__ " " __TIME__);
#endif /* RELEASE */
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwnd, 0);
			break;
		case IDC_ABOUTBOX_BUTTON_LICENSE:
			do_licensebox();
			break;
		case IDC_ABOUTBOX_BUTTON_HOMEPAGE:
			ShellExecute(NULL, "open", HOMEPAGE, NULL, NULL, SW_SHOWDEFAULT);
			break;
		};
    };
    return FALSE;
};

/*
 * About Box: setup function.
 */
void do_aboutbox(void) {
	DialogBox(config->hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, AboutProc);
};

#include "plaunch.h"
#include "resource.h"
#include "dlgtmpl.h"

/*
 * License Box: dialog function.
 */
static int CALLBACK LicenseProc(HWND hwnd, UINT msg,
								WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
#ifdef WINDOWS_NT351_COMPATIBLE
		if (!config->have_shell)
			center_window(hwnd);
#endif /* WINDOWS_NT351_COMPATIBLE */
		SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM)config->main_icon);
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
	DialogBox(config->hinst, MAKEINTRESOURCE(IDD_LICENSEBOX), NULL, LicenseProc);
};

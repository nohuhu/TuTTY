#include "plaunch.h"
#include "dlgtmpl.h"
#include "misc.h"

#define HOMEPAGE	"http://putty.dwalin.ru/plaunch"

#define ID_LICENSE	100
#define	ID_HOMEPAGE	101

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
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwnd, 0);
			break;
		case ID_LICENSE:
			EnableWindow(hwnd, FALSE);
			do_licensebox();
			EnableWindow(hwnd, TRUE);
			SetActiveWindow(hwnd);
			break;
		case ID_HOMEPAGE:
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
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE);
	ptr = dialogtemplate_create((void *)tmpl, WS_POPUP | WS_CAPTION | WS_SYSMENU |
								WS_VISIBLE | DS_MODALFRAME | DS_CENTER,
								0, 0, 234, 80, "About", 4, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
								157, 62, 70, 14, "&Close", IDOK);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								82, 62, 70, 14, "Visit &home page...", ID_HOMEPAGE);
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
								7, 62, 70, 14, "View &License...", ID_LICENSE);
#ifdef RELEASE
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE | SS_CENTER,
								7, 7, 227, 50,
								"PuTTY Launcher && Session Manager\n"
								"\n"
								"Version " APPVERSION "\n"
								"\n"
								"© 2004, 2005 dwalin <dwalin@dwalin.ru>.\n"
								"All rights reserved.",
								100);
#else
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE | SS_CENTER,
								7, 7, 227, 50,
								"PuTTY Launcher && Session Manager\n"
								"\n"
								"Version " APPVERSION ", " __DATE__ " " __TIME__ "\n"
								"\n"
								"© 2004, 2005 dwalin <dwalin@dwalin.ru>. "
								"All rights reserved.",
								100);
#endif /* RELEASE */
	DialogBoxIndirect(config->hinst, (LPDLGTEMPLATE)tmpl, NULL, AboutProc);

	GlobalFree(tmpl);
};

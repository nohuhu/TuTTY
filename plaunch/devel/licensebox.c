#include "plaunch.h"
#include "dlgtmpl.h"
#include "misc.h"

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
	LPDLGTEMPLATE tmpl = NULL;
	void *ptr;

	tmpl = (LPDLGTEMPLATE)GlobalAlloc(GMEM_ZEROINIT, BUFSIZE * 2);
	ptr = dialogtemplate_create((void *)tmpl, WS_POPUP | WS_VISIBLE | WS_CAPTION |
								WS_SYSMENU | DS_MODALFRAME | DS_CENTER,
								0, 0, 240, 250, "PuTTY Launcher License", 2, NULL,
								8, "MS Sans Serif");
	ptr = dialogtemplate_addbutton(ptr, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
								90, 225, 50, 14, "OK", IDOK);
	ptr = dialogtemplate_addstatic(ptr, WS_CHILD | WS_VISIBLE | SS_CENTER,
								7, 7, 226, 212,
								"Copyright © 2004, 2005 dwalin <dwalin@dwalin.ru>\n"
								"\n"
								"Portions copyright Simon Tatham, 1997-2004\n"
								"\n"
								"Permission is hereby granted, free of charge, to any person\n"
								"obtaining a copy of this software and associated documentation\n"
								"files (the ""Software""), to deal in the Software without restriction,\n"
								"including without limitation the rights to use, copy, modify, merge,\n"
								"publish, distribute, sublicense, and/or sell copies of the Software,\n"
								"and to permit persons to whom the Software is furnished to do so,\n"
								"subject to the following conditions:\n"
								"\n"
								"The above copyright notice and this permission notice shall be\n"
								"included in all copies or substantial portions of the Software.\n"
								"\n"
								"THE SOFTWARE IS PROVIDED ""AS IS"", WITHOUT\n"
								"WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,\n"
								"INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
								"MERCHANTABILITY, FITNESS FOR A PARTICULAR\n"
								"PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
								"COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES\n"
								"OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\n"
								"TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
								"CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
								"OTHER DEALINGS IN THE SOFTWARE.", 100);
	DialogBoxIndirect(config->hinst, (LPDLGTEMPLATE)tmpl, NULL, LicenseProc);

	GlobalFree(tmpl);
};

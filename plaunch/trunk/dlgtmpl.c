/*
 * Run-time dialog box template manipulation routines.
 *
 * (c) 2004-2005 dwalin <dwalin@dwalin.ru>
 */

#include "dlgtmpl.h"

void *dialogtemplate_create_ex(void *templt, DWORD style, DWORD exstyle,
			       int x, int y, int cx, int cy,
			       char *caption, int controls,
			       char *wclass, HMENU menu,
			       int fontsize, char *font)
{
    LPDLGTEMPLATE tmpl;
    LPWORD lpw;
    LPWSTR str;
    int nchar, len;

    if (!templt)
	return NULL;

    tmpl = (LPDLGTEMPLATE) GlobalLock(templt);

    tmpl->style = style;
    tmpl->dwExtendedStyle = 0;
    tmpl->cdit = controls;
    tmpl->x = x;
    tmpl->y = y;
    tmpl->cx = cx;
    tmpl->cy = cy;

    lpw = (LPWORD) (tmpl + 1);
    *lpw++ = (WORD) menu;
    *lpw++ = 0;			// predefined dialog box class

    str = (LPWSTR) lpw;
    len = MultiByteToWideChar(CP_ACP, 0, caption, -1, NULL, 0);
    nchar = MultiByteToWideChar(CP_ACP, 0, caption, -1, str, len);
    lpw += nchar;

    if (fontsize != 0 && font) {
	tmpl->style |= DS_SETFONT;
	*lpw++ = fontsize;
	str = (LPWSTR) lpw;
	len = MultiByteToWideChar(CP_ACP, 0, font, -1, NULL, 0);
	nchar = MultiByteToWideChar(CP_ACP, 0, font, -1, str, len);
	lpw += nchar;
    };

    GlobalUnlock(templt);

    return (void *) lpw;
};

void *dialogtemplate_addcontrol(void *tmpl, DWORD style, DWORD exstyle,
				int x, int y, int cx, int cy,
				char *caption, DWORD id, char *wclass)
{
    LPDLGITEMTEMPLATE item;
    LPWORD lpw;
    LPWSTR str;
    int nchar, len;

    lpw = (LPWORD) tmpl;
    (ULONG) lpw += 3;
    (ULONG) lpw >>= 2;
    (ULONG) lpw <<= 2;
    item = (LPDLGITEMTEMPLATE) lpw;
    item->x = x;
    item->y = y;
    item->cx = cx;
    item->cy = cy;
    item->style = style;
    item->dwExtendedStyle = 0;
    item->id = (WORD) id;

    lpw = (LPWORD) (item + 1);

    if (wclass == NULL)
	*lpw++ = 0;
    else if (HIWORD(wclass) == 0xffff) {
	*lpw++ = 0xffff;
	*lpw++ = (WORD) (LOWORD(wclass));
    } else {
	str = (LPWSTR) lpw;
	len = MultiByteToWideChar(CP_ACP, 0, wclass, -1, NULL, 0) + 1;
	nchar = MultiByteToWideChar(CP_ACP, 0, caption, -1, str, len);
	lpw += nchar;
    };

    if (caption) {
	str = (LPWSTR) lpw;
	len = MultiByteToWideChar(CP_ACP, 0, caption, -1, NULL, 0) + 1;
	nchar = MultiByteToWideChar(CP_ACP, 0, caption, -1, str, len);
	lpw += nchar;
    } else
	*lpw++ = 0;

    *lpw++ = 0;

    return (void *) lpw;
};

void __inline *dialogtemplate_addbutton(void *tmpl, DWORD style,
					DWORD exstyle, int x, int y,
					int cx, int cy, char *caption,
					DWORD id)
{
    return dialogtemplate_addcontrol(tmpl, style, exstyle, x, y, cx, cy,
				     caption, id, (char *) 0xffff0080);
};

void __inline *dialogtemplate_addeditbox(void *tmpl, DWORD style,
					 DWORD exstyle, int x, int y,
					 int cx, int cy, char *caption,
					 DWORD id)
{
    return dialogtemplate_addcontrol(tmpl, style, exstyle, x, y, cx, cy,
				     caption, id, (char *) 0xffff0081);
};

void __inline *dialogtemplate_addstatic(void *tmpl, DWORD style,
					DWORD exstyle, int x, int y,
					int cx, int cy, char *caption,
					DWORD id)
{
    return dialogtemplate_addcontrol(tmpl, style, exstyle, x, y, cx, cy,
				     caption, id, (char *) 0xffff0082);
};

void __inline *dialogtemplate_addlistbox(void *tmpl, DWORD style,
					 DWORD exstyle, int x, int y,
					 int cx, int cy, char *caption,
					 DWORD id)
{
    return dialogtemplate_addcontrol(tmpl, style, exstyle, x, y, cx, cy,
				     caption, id, (char *) 0xffff0083);
};

void __inline *dialogtemplate_addscrollbar(void *tmpl, DWORD style,
					   DWORD exstyle, int x, int y,
					   int cx, int cy, char *caption,
					   DWORD id)
{
    return dialogtemplate_addcontrol(tmpl, style, exstyle, x, y, cx, cy,
				     caption, id, (char *) 0xffff0084);
};

void __inline *dialogtemplate_addcombobox(void *tmpl, DWORD style,
					  DWORD exstyle, int x, int y,
					  int cx, int cy, char *caption,
					  DWORD id)
{
    return dialogtemplate_addcontrol(tmpl, style, exstyle, x, y, cx, cy,
				     caption, id, (char *) 0xffff0085);
};

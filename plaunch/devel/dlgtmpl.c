/*
 * Run-time dialog box template manipulation routines.
 *
 * (c) 2004 dwalin <dwalin@dwalin.ru>
 */

#include "dlgtmpl.h"

/*
static LPWORD lpwAlign(LPWORD lpIn) {
    ULONG ul;

    ul = (ULONG) lpIn;
    ul +=3;
    ul >>=2;
    ul <<=2;

    return (LPWORD) ul;
}
*/

void *dialogtemplate_create(void *templt, DWORD style,
							int x, int y, int cx, int cy,
							char *caption, int controls, 
							HMENU menu,
							int fontsize, char *font) {
	LPDLGTEMPLATE tmpl;
	LPWORD lpw;
	LPWSTR str;
	int nchar, len;

	if (!templt)
		return NULL;

	tmpl = (LPDLGTEMPLATE)GlobalLock(templt);

	tmpl->style = style;
	tmpl->cdit = controls;
	tmpl->x = x;
	tmpl->y = y;
	tmpl->cx = cx;
	tmpl->cy = cy;

	lpw = (LPWORD)(tmpl + 1);
	*lpw++ = (WORD)menu;
	*lpw++ = 0; // predefined dialog box class

	str = (LPWSTR)lpw;
	len = MultiByteToWideChar(CP_ACP, 0, caption, -1, NULL, 0);
	nchar = MultiByteToWideChar(CP_ACP, 0, caption, -1,	str, len);
	lpw += nchar;

	if (fontsize != 0 && font) {
		tmpl->style |= DS_SETFONT;
		*lpw++ = fontsize;
		str = (LPWSTR)lpw;
		len = MultiByteToWideChar(CP_ACP, 0, font, -1, NULL, 0);
		nchar = MultiByteToWideChar(CP_ACP, 0, font, -1, str, len);
		lpw += nchar;
	};

	GlobalUnlock(templt);

	return (void *)lpw;
};

void *dialogtemplate_addcontrol(void *tmpl, DWORD style,
								int x, int y, int cx, int cy,
								char *caption, DWORD id, WORD wclass) {
	LPDLGITEMTEMPLATE item;
	LPWORD lpw;
	LPWSTR str;
	int nchar, len;

	lpw = (LPWORD)tmpl;
    (ULONG)lpw +=3;
    (ULONG)lpw >>=2;
    (ULONG)lpw <<=2;
	item = (LPDLGITEMTEMPLATE)lpw;
	item->x = x;
	item->y = y;
	item->cx = cx;
	item->cy = cy;
	item->style = style;
	item->id = (WORD)id;
	
	lpw = (LPWORD)(item + 1);
	*lpw++ = 0xFFFF;
	*lpw++ = wclass;

	if (caption) {
		str = (LPWSTR)lpw;
		len = MultiByteToWideChar(CP_ACP, 0, caption, -1, NULL, 0) + 1;
		nchar = MultiByteToWideChar(CP_ACP, 0, caption, -1, str, len);
		lpw += nchar;
	} else
		*lpw++ = 0;

	*lpw++ = 0;

	return (void *)lpw;
};

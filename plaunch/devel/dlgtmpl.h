/*
 * Run-time dialog box template manipulation routines.
 *
 * (c) 2004 dwalin <dwalin@dwalin.ru>
 */

#ifndef DLGTMPL_H
#define DLGTMPL_H

#include <windows.h>

void *dialogtemplate_create(void *templt, DWORD style,
							int x, int y, int cx, int cy,
							char *caption, int controls, 
							HMENU menu,
							int fontsize, char *font);

void *dialogtemplate_addcontrol(void *tmpl, DWORD style,
								int x, int y, int cx, int cy,
								char *caption, DWORD id, WORD wclass);

#define dialogtemplate_addbutton(tmpl, style, x, y, cx, cy, caption, id) \
			dialogtemplate_addcontrol(tmpl, style, x, y, cx, cy, caption, id, 0x0080)

#define dialogtemplate_addeditbox(tmpl, style, x, y, cx, cy, caption, id) \
			dialogtemplate_addcontrol(tmpl, style, x, y, cx, cy, caption, id, 0x0081)

#define dialogtemplate_addstatic(tmpl, style, x, y, cx, cy, caption, id) \
			dialogtemplate_addcontrol(tmpl, style, x, y, cx, cy, caption, id, 0x0082)

#define dialogtemplate_addlistbox(tmpl, style, x, y, cx, cy, caption, id) \
			dialogtemplate_addcontrol(tmpl, style, x, y, cx, cy, caption, id, 0x0083)

#define dialogtemplate_addscrollbar(tmpl, style, x, y, cx, cy, caption, id) \
			dialogtemplate_addcontrol(tmpl, style, x, y, cx, cy, caption, id, 0x0084)

#define dialogtemplate_addcombobox(tmpl, style, x, y, cx, cy, caption, id) \
			dialogtemplate_addcontrol(tmpl, style, x, y, cx, cy, caption, id, 0x0085)


#endif /* DLGTMPL_H */
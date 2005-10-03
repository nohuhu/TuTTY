/*
 * Run-time dialog box template manipulation routines.
 *
 * (c) 2004 dwalin <dwalin@dwalin.ru>
 */

#ifndef DLGTMPL_H
#define DLGTMPL_H

#include <windows.h>

void *dialogtemplate_create_ex(void *templt, DWORD style, DWORD exstyle,
			       int x, int y, int cx, int cy,
			       char *caption, int controls,
			       char *wclass, HMENU menu, int fontsize,
			       char *font);

//#define dialogtemplate_create(tmpl, style, x, y, cx, cy, caption, controls) \
//                      dialogtemplate_create_ex(tmpl, style, 0, x, y, cx, cy, caption,
//                                                                       controls, 0, 0, 0, 0)

void *dialogtemplate_addcontrol(void *tmpl, DWORD style, DWORD exstyle,
				int x, int y, int cx, int cy,
				char *caption, DWORD id, char *wclass);

void *dialogtemplate_addbutton(void *tmpl, DWORD style, DWORD exstyle,
			       int x, int y, int cx, int cy,
			       char *caption, DWORD id);

void *dialogtemplate_addeditbox(void *tmpl, DWORD style, DWORD exstyle,
				int x, int y, int cx, int cy,
				char *caption, DWORD id);

void *dialogtemplate_addstatic(void *tmpl, DWORD style, DWORD exstyle,
			       int x, int y, int cx, int cy,
			       char *caption, DWORD id);

void *dialogtemplate_addlistbox(void *tmpl, DWORD style, DWORD exstyle,
				int x, int y, int cx, int cy,
				char *caption, DWORD id);

void *dialogtemplate_addscrollbar(void *tmpl, DWORD style, DWORD exstyle,
				  int x, int y, int cx, int cy,
				  char *caption, DWORD id);

void *dialogtemplate_addcombobox(void *tmpl, DWORD style, DWORD exstyle,
				 int x, int y, int cx, int cy,
				 char *caption, DWORD id);

#endif				/* DLGTMPL_H */

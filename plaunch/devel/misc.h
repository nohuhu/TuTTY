#ifndef MISC_H
#define MISC_H

#include <windows.h>
#include <commctrl.h>

#define CFG_SAVE_PUTTY_PATH		0x0001
#define CFG_SAVE_HOTKEY_LB		0x0002
#define CFG_SAVE_HOTKEY_WL		0x0004
#define CFG_SAVE_DRAGDROP		0x0008
#define CFG_SAVE_SAVECURSOR		0x0010	

#define PLAUNCH_REGISTRY_ROOT		"Software\\SimonTatham\\PuTTY\\PLaunch"
#define PLAUNCH_PUTTY_PATH			"PuTTYPath"
#define PLAUNCH_HOTKEY_LB			"HotKeyLaunchBox"
#define PLAUNCH_HOTKEY_WL			"HotKeyWindowList"
#define PLAUNCH_ENABLEDRAGDROP		"EnableDragDrop"
#define PLAUNCH_ENABLESAVECURSOR	"EnableSaveCursor"
#define PLAUNCH_SAVEDCURSORPOS		"SavedCursorPosition"
#define PLAUNCH_SAVEMOREBUTTON		"SavedMoreListBoxOptions"

int GetSystemImageLists(HMODULE hShell32, HIMAGELIST *phLarge, HIMAGELIST *phSmall);
void FreeSystemImageLists(HMODULE hShell32);

HTREEITEM treeview_additem(HWND treeview, HTREEITEM parent, struct _config *cfg,
						   char *name, int isfolder);
HTREEITEM treeview_addtree(HWND hwndTV, HTREEITEM _parent, char *root);
HMENU menu_addsession(HMENU menu, char *root);
HMENU menu_refresh(HMENU menu, char *root);

unsigned int is_folder(char *name);

unsigned int treeview_getitemname(HWND treeview, HTREEITEM item, char *buf,
								  unsigned int bufsize);
unsigned int treeview_getitempath(HWND treeview, HTREEITEM item, char *buf);
HTREEITEM treeview_getitemfrompath(HWND treeview, char *path);

void center_window(HWND window);

char *dupstr(const char *s);
unsigned int get_putty_path(char *buf, unsigned int bufsize);

unsigned int read_config(struct _config *cfg);
unsigned int save_config(struct _config *cfg, int what);

#endif /* MISC_H */

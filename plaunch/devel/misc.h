#ifndef MISC_H
#define MISC_H

#include <windows.h>
#include <commctrl.h>

#define CFG_SAVE_PUTTY_PATH		0x0001
#define CFG_SAVE_HOTKEY_LB		0x0002
#define CFG_SAVE_HOTKEY_WL		0x0004

int GetSystemImageLists(HMODULE hShell32, HIMAGELIST *phLarge, HIMAGELIST *phSmall);
void FreeSystemImageLists(HMODULE hShell32);

HTREEITEM treeview_addsession(HWND hwndTV, HTREEITEM _parent, struct session *s);
HMENU menu_addsession(HMENU menu, struct session *s);
HMENU menu_refresh(HMENU menu, struct session *root);

void center_window(HWND window);

char *dupstr(const char *s);
char *get_putty_path(void);

int AddTrayIcon(HWND hwnd);

int read_config(struct _config *cfg);
int save_config(struct _config *cfg, int what);

#endif /* MISC_H */
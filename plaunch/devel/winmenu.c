#include "winmenu.h"
#ifdef PLAUNCH
/*
 * Building with PLaunch.
 */
#include "plaunch.h"
#endif
#include "misc.h"

#ifndef PLAUNCH
#define IDM_SAVED_MIN 0x1000
#define IDM_SAVED_MAX 0x5000
#define MENU_SAVED_STEP 16
/* Maximum number of sessions on saved-session submenu */
#define MENU_SAVED_MAX ((IDM_SAVED_MAX-IDM_SAVED_MIN) / MENU_SAVED_STEP)
#define	IDM_EMPTY		0x0200
#define BUFSIZE		2048
#endif

static int menu_callback(char *name, char *path,
			 int isfolder, int mode,
			 void *priv1, void *priv2, void *priv3)
{
    HMENU menu;
    HMENU add_menu;
    static int id = 0;
    int ret;

    if (mode == REG_MODE_POSTPROCESS)
	return 0;

    menu = (HMENU) priv1;

    if (isfolder) {
	add_menu = CreatePopupMenu();
	ret = InsertMenu(menu, 0, MF_OWNERDRAW | MF_POPUP | MF_BYPOSITION,
			 (UINT) add_menu, dupstr(path));
	return (int) add_menu;
    } else {
	ret =
	    InsertMenu(menu, 0, MF_OWNERDRAW | MF_ENABLED | MF_BYPOSITION,
#ifdef PLAUNCH
		       IDM_SESSION_BASE + id,
#else
		       IDM_SAVED_MIN + id,
#endif
		       dupstr(path));
#ifdef PLAUNCH
	id++;
#else
	id += MENU_SAVED_STEP;
#endif
	return 0;
    };
};

HMENU menu_addsession(HMENU menu, char *root)
{
    reg_walk_over_tree(root, menu_callback, menu, NULL, NULL);

    if (GetMenuItemCount(menu) == 0)
	AppendMenu(menu, MF_STRING | MF_GRAYED, IDM_EMPTY,
		   "(No saved sessions)");

    return menu;
};

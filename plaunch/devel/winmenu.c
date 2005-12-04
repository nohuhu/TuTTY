#ifdef PLAUNCH
/*
 * Building with PLaunch.
 */
#include "plaunch.h"
#include "misc.h"
#else
#include "putty.h"
#endif
#include "winmenu.h"

#ifndef PLAUNCH
#define IDM_SAVED_MIN 0x1000
#define IDM_SAVED_MAX 0x5000
#define MENU_SAVED_STEP 16
/* Maximum number of sessions on saved-session submenu */
#define MENU_SAVED_MAX ((IDM_SAVED_MAX-IDM_SAVED_MIN) / MENU_SAVED_STEP)
#define	IDM_EMPTY		0x0200
#define BUFSIZE		2048
extern Config cfg;
#endif

static void menu_callback(session_callback_t *scb)
{
    HMENU menu;
    HMENU add_menu;
    static int id = 0;

    if (!scb)
	return;

    if (scb->mode == SES_MODE_POSTPROCESS)
	return;

    menu = scb->protected1 ?
	(HMENU) scb->protected1 :
	(HMENU) scb->public1;

    if (scb->session->isfolder) {
	add_menu = CreatePopupMenu();
	AppendMenu(menu, MF_OWNERDRAW | MF_POPUP,
		   (UINT)add_menu, dupstr(scb->session->path));
	scb->protected1 = (void *) add_menu;
	return;
    } else {
	AppendMenu(menu, MF_OWNERDRAW | MF_ENABLED,
#ifdef PLAUNCH
		    IDM_SESSION_BASE + id,
#else
		    IDM_SAVED_MIN + id,
#endif
		    dupstr(scb->session->path));
#ifdef PLAUNCH
	id++;
#else
	id += MENU_SAVED_STEP;
#endif
	return;
    };
};

HMENU menu_addsession(HMENU menu, char *root)
{
    session_walk_t sw;

    memset(&sw, 0, sizeof(sw));
#ifdef PLAUNCH
    sw.root = config->sessionroot;
#else
    sw.root = cfg.sessionroot;
#endif /* PLAUNCH */
    sw.root_path = root;
    sw.depth = SES_MAX_DEPTH;
    sw.callback = menu_callback;
    sw.public1 = menu;

    ses_walk_over_tree(&sw);

    if (GetMenuItemCount(menu) == 0)
	AppendMenu(menu, MF_STRING | MF_GRAYED, IDM_EMPTY,
		   "(No saved sessions)");

    return menu;
};

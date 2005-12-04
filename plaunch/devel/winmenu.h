#ifndef WINMENU_H
#define WINMENU_H
#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#ifndef SESSIONICON
#define SESSIONICON "WindowIcon"
#endif /* SESSIONICON */

HMENU menu_addsession(HMENU menu, char *root);
HMENU menu_refresh(HMENU menu, char *root);

#endif				/* WINMENU_H */

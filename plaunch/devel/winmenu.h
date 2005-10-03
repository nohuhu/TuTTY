#ifndef WINMENU_H
#define WINMENU_H
#ifndef _INC_WINDOWS
#include <windows.h>
#endif
#include "registry.h"

HMENU menu_addsession(HMENU menu, char *root);
HMENU menu_refresh(HMENU menu, char *root);

#endif				/* WINMENU_H */

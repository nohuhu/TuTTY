/*
 * Some definitions for Minimal RTL mode.
 */

#ifndef ENTRY_H
#define ENTRY_H

#if !defined(_DEBUG) && defined(MINIRTL)

#define WIN32_LEAN_AND_MEAN
#define NO_PARSE_CMD_LINE

#include <shlwapi.h>

#define	sprintf	wsprintf
#define	strlen	lstrlen
#define	strcpy	lstrcpy
#define	strcmp	lstrcmp
#define stricmp	lstrcmpi
#define	strcat	lstrcat
#define strrchr(x, y)	StrRChr(x, NULL, y)

#endif /* _DEBUG && MINIRTL */

#endif /* ENTRY_H */
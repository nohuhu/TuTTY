/*
 * Some definitions for Minimal RTL mode.
 */

#ifndef ENTRY_H
#define ENTRY_H

#include <windows.h>

#if !defined(_DEBUG) && defined(MINIRTL)

#define WIN32_LEAN_AND_MEAN

void * __cdecl malloc(size_t);
void __cdecl free(void *);
void * __cdecl memmove(void *, const void *, size_t);
void * __cdecl memset(void *, int, size_t);

#define	sprintf	wsprintf
#define	strlen	lstrlen
#define	strcpy	lstrcpy
#define	strcmp	lstrcmp
#define stricmp	lstrcmpi
#define	strcat	lstrcat

#endif /* _DEBUG && MINIRTL */

#endif /* ENTRY_H */
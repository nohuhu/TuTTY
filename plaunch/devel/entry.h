/*
 * Some definitions for Minimal RTL mode.
 */

#ifndef ENTRY_H
#define ENTRY_H

#include <windows.h>

#define COMPAT_WINDOWSNT351(major, minor) \
	(major > 3 || (major == 3 && minor == 51))

#define COMPAT_WINDOWSNT40(major, minor, platform) \
	(platform == VER_PLATFORM_WIN32_NT && \
		(major > 4 || (major == 4 && minor >= 0)))

#define COMPAT_WINDOWS95(major, minor) \
	(major > 4 || (major == 4 && minor >= 0))

#define COMPAT_WINDOWS98(major, minor) \
	(major > 4 || (major == 4 && minor >= 10))

#define COMPAT_WINDOWSME(major, minor) \
	(major > 4 || (major == 4 && minor >= 90))

#define COMPAT_WINDOWS2000(major, minor) \
	(major > 5 || (major == 5 && minor >= 0))

#define COMPAT_WINDOWSXP(major, minor) \
	(major > 5 || (major == 5 && minor >= 1))

#define COMPAT_WINDOWS2003(major, minor) \
	(major > 5 || (major == 5 && minor >= 2))

#if !defined(_DEBUG) && defined(MINIRTL)

#define WIN32_LEAN_AND_MEAN

void *__cdecl malloc(size_t);
void __cdecl free(void *);
void *__cdecl memmove(void *, const void *, size_t);
void *__cdecl memset(void *, int, size_t);

char *minirtl_strrchr(const char *, int);
char *minirtl_strstr(const char *str1, const char *str2);

int minirtl_atoi(char *);
char *minirtl_getenv(const char *option);

int minirtl_snprintf(char *, size_t, const char *, ...);

#define	sprintf	wsprintf
#define	strlen	lstrlen
#define	strcpy	lstrcpy
#define strncpy lstrcpyn
#define	strcmp	lstrcmp
#define stricmp	lstrcmpi
#define	strcat	lstrcat
#define strrchr	minirtl_strrchr
#define _snprintf minirtl_snprintf
#define atoi    minirtl_atoi
//#define _vsnprintf StringCbVPrintf
#define strstr	minirtl_strstr
#define	getenv	minirtl_getenv

//#include <strsafe.h>

#endif				/* _DEBUG && MINIRTL */

#endif				/* ENTRY_H */

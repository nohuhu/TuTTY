//
//	MODULE:		entry.c
//
//	PURPOSE:	Define a minimal run-time for MSVC6.0+
//
//	HISTORY:	v1.0 09/04/2002 J Brown		- Initial Version
//				v1.1 09/26/2004 dwalin		- fixed for more compatibility,
//											  added static variable for heap handle
//

#include <windows.h>

#if !defined(_DEBUG) && defined(MINIRTL)

// Remove run-time library
#pragma comment(linker, "/NODEFAULTLIB:LIBC.LIB")

#pragma optimize("gsy", on)

#pragma comment(linker,"/merge:.rdata=.data")
#pragma comment(linker,"/merge:.text=.data")
#pragma comment(linker,"/merge:.reloc=.data")
#pragma comment(linker,"/FILEALIGN:0x200")

//
//	Define wrappers around c-heap-management
//

HANDLE __ApplicationHeap;

#ifdef __cplusplus

void * __cdecl operator new(unsigned int s)
{
    return HeapAlloc( __ApplicationHeap, 0, s );
}

void __cdecl operator delete( void * p )
{
    HeapFree( __ApplicationHeap, 0, p );
}

extern "C" void * __cdecl	malloc(size_t size);
extern "C" void   __cdecl	free(void * p);
extern "C" int    __cdecl	WinMainCRTStartup();

#endif	// __cplusplus

void * __cdecl malloc(size_t size)
{
    return HeapAlloc( __ApplicationHeap, HEAP_ZERO_MEMORY, size );
}

void __cdecl free(void * p)
{
    HeapFree( __ApplicationHeap, 0, p );
}

void * __cdecl memmove(void *dst, const void *src, size_t count) {
	void * ret = dst;

	if (dst <= src || (char *)dst >= ((char *)src + count)) {
		while (count--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst + 1;
			src = (char *)src + 1;
		}
	} else {
		dst = (char *)dst + count - 1;
		src = (char *)src + count - 1;

		while (count--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst - 1;
			src = (char *)src - 1;
		}
	}

	return(ret);
}

void * __cdecl memset(void *dst, int val, size_t count) {
    void *start = dst;

    while (count--) {
		*(char *)dst = (char)val;
		dst = (char *)dst + 1;
    }

    return(start);
}

int  __cdecl WinMainCRTStartup()
{
	UINT  ret;
	char *ptr = 0;

	HINSTANCE hInst = GetModuleHandle(0);
	__ApplicationHeap = GetProcessHeap();

#ifndef NO_PARSE_CMD_LINE
	
	ptr = GetCommandLineA();

	// Skip any leading whitespace
	while(*ptr == ' ') 
		ptr++;

	// Is executable path enclosed in quotes?
	if(*ptr == '\"')
	{
		ptr++;

		while(*ptr && *ptr != '\"')
			ptr++;

		ptr++;
	}
	// No? Find next whitespace - that is where args start
	else
	{
		while(*ptr && *ptr != ' ')
			ptr++;
	}

	// find start of any arguments
	while(*ptr && *ptr == ' ')
		ptr++;

#endif

	ret = WinMain(hInst, 0, ptr, SW_SHOWNORMAL);
	ExitProcess(ret);
	return ret;
}

#endif	// _DEBUG && MINIRTL

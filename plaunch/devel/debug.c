#include <windows.h>
#include <stdio.h>
#include "debug.h"

void _debug_err(int err, char *str, int line, char *file) {
#ifdef _DEBUG
	char *buf;

	buf = (char *)malloc(2048);
	sprintf(buf, "Error %d, \"%s\" occured on line %d of file %s", err, str, line, file);
	MessageBox(NULL, buf, "Debug: an error", MB_OK | MB_ICONERROR);
	free(buf);
#endif
};
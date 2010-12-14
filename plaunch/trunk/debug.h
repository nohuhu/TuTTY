#ifndef DEBUG_H
#define DEBUG_H

#define debug_err(x, y)	_debug_err(x, y, __LINE__, __FILE__)

void _debug_err(int err, char *str, int line, char *file);

#endif				/* DEBUG_H */

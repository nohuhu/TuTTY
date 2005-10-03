#ifndef SESSION_H
#define SESSION_H

#include "putty.h"

char *ses_lastname(char *in);
char *ses_pathname(char *in);

unsigned int ses_is_folder(char *name, Config * cfg);

unsigned int ses_make_folder(char *name, Config * cfg);
unsigned int ses_delete(char *name, Config * cfg);

#endif				/* SESSION_H */

#include "session.h"
#include "misc.h"
#include "registry.h"

char *ses_lastname(char *in)
{
    char *p;

    if (!in || in[0] == '\0')
	return in;

    p = &in[strlen(in)];

    while (p >= in && *p != '\\')
	*p--;

    return p + 1;
};

char *ses_pathname(char *in)
{
    char *p, *ret, *tmp;

    if (!in || in[0] == '\0')
	return in;

    tmp = dupstr(in);
    p = ses_lastname(tmp);

    if (p == tmp) {
	ret = malloc(1);
	ret[0] = '\0';
    } else {
	ret = malloc((p - tmp) + 1);
	memset(ret, 0, (p - tmp) + 1);
	strncpy(ret, tmp, (p - tmp) - 1);
    };

    free(tmp);

    return ret;
};

#define BUFSIZE	2048

unsigned int ses_is_folder(char *name, Config *cfg)
{
    char buf[BUFSIZE];
    int isfolder;

    if (!name || !name[0] || !cfg)
	return 0;

    reg_make_path(NULL, name, buf);
    reg_read_i(buf, ISFOLDER, 0, &isfolder);

    return isfolder;
};

unsigned int ses_make_folder(char *name, Config *cfg) {
    char buf[BUFSIZE];

    if (!name || !name[0] || !cfg)
	return 0;

    reg_make_path(NULL, name, buf);
    return reg_write_i(buf, ISFOLDER, 1);
};

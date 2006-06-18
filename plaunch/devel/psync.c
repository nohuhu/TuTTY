#include "entry.h"
#include "plaunch.h"
#include "misc.h"

int __cdecl main(int argc, char **argv)
{
    session_root_t from;
    session_root_t to;
    char fromurl[BUFSIZE], tourl[BUFSIZE];
    char errmsg[BUFSIZE];

    strcpy(fromurl, "");
//    strcpy(tourl, "--session-root=file:///home/devel/plaunch/sessions.xml");
//    strcpy(fromurl, "--session-root=file:///home/devel/plaunch/sessions.xml");
    strcpy(tourl, "--session-root=registry://Software\\dwalin\\TuTTY");

    memset(errmsg, 0, BUFSIZE);

    ses_init_session_root(&from, fromurl, errmsg, BUFSIZE);
    ses_init_session_root(&to, tourl, errmsg, BUFSIZE);

    sync_session_roots(&from, &to);

    memset(errmsg, 0, BUFSIZE);

    ses_finish_session_root(&from, errmsg, BUFSIZE);
    ses_finish_session_root(&to, errmsg, BUFSIZE);

    return 0;
};
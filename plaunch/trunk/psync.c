#include "entry.h"
#include "plaunch.h"
#include "misc.h"

static void sync_callback(session_callback_t *scb)
{
    session_root_t *to = (session_root_t *) scb->public1;
    int i, max, type, ivalue;
    char name[BUFSIZE], svalue[BUFSIZE];
    void *sfrom, *sto, *en;

    if ((scb->mode == SES_MODE_PREPROCESS &&
	!scb->session->isfolder) ||
	(scb->mode == SES_MODE_POSTPROCESS &&
	scb->session->isfolder))
	return;

    sfrom = ses_open_session_r(&scb->root, scb->session->path);
    sto = ses_open_session_w(to, scb->session->path);

    if (!sfrom || !sto)
	return;

    en = ses_enum_values_start(&scb->root, sfrom);

    if (!en)
	return;

    max = ses_enum_values_count(&scb->root, en);

    for (i = 0; i < max; i++) {
	if (ses_enum_values_next(&scb->root, en, name, BUFSIZE)) {
	    type = ses_enum_values_type(&scb->root, en);
	    switch (type) {
	    case SES_VALUE_INTEGER:
		{
		    ivalue = 0;
		    ses_read_handle_i(&scb->root, sfrom, name, 0, &ivalue);
		    ses_write_handle_i(to, sto, name, ivalue);
		};
		break;
	    case SES_VALUE_STRING:
		{
		    memset(svalue, 0, BUFSIZE);
		    ses_read_handle_s(&scb->root, sfrom, name, "", svalue, BUFSIZE);
		    ses_write_handle_s(to, sto, name, svalue);
		};
		break;
	    };
	};
    };

    ses_close_session(&scb->root, sfrom);
    ses_close_session(to, sto);
};

int sync_session_roots(session_root_t *from, session_root_t *to)
{
    session_walk_t sw;

    memset(&sw, 0, sizeof(sw));
    sw.root = *from;
    sw.root_path = "";
    sw.depth = SES_MAX_DEPTH;
    sw.callback = sync_callback;
    sw.public1 = (void *) to;

    ses_walk_over_tree(&sw);

    return TRUE;
};

int __cdecl main(int argc, char **argv)
{
    session_root_t from;
    session_root_t to;
    char fromurl[BUFSIZE], tourl[BUFSIZE];
    char errmsg[BUFSIZE];

    strcpy(fromurl, "");
    strcpy(tourl, "--session-root=file://sessions2.xml");
//    strcpy(fromurl, "--session-root=file:///home/devel/plaunch/sessions.xml");
//    strcpy(tourl, "--session-root=registry://Software\\dwalin\\TuTTY");

    memset(errmsg, 0, BUFSIZE);

    ses_init_session_root(&from, fromurl, errmsg, BUFSIZE);
    ses_init_session_root(&to, tourl, errmsg, BUFSIZE);

    sync_session_roots(&from, &to);

    memset(errmsg, 0, BUFSIZE);

    ses_finish_session_root(&from, errmsg, BUFSIZE);
    ses_finish_session_root(&to, errmsg, BUFSIZE);

    return 0;
};
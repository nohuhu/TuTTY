/*
 * config.c - the platform-independent parts of the PuTTY
 * configuration box.
 */

#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"

#define PRINTER_DISABLED_STRING "None (printing disabled)"

#define HOST_BOX_TITLE "Host Name (or IP address)"
#define PHONE_NUMBER_OR_BLANK   "Phone number (or blank)"
#define PORT_BOX_TITLE "Port"

struct protocolbuttons_data {
    struct controlset *s;
    union control *host, *port, *protocol;
};
/*
 * Convenience function: determine whether this binary supports a
 * given backend.
 */
static int have_backend(int protocol)
{
    struct backend_list *p = backends;
    for (p = backends; p->name; p++) {
	if (p->protocol == protocol)
	    return 1;
    }
    return 0;
}

static void config_host_handler(union control *ctrl, void *dlg,
				 void *data, int event)
{
    Config *cfg = (Config *)data;

    if (event == EVENT_REFRESH) {
	char *str;
	int selected = FALSE;

	if (cfg->protocol == PROT_SERIAL) {
	    int i, port;
	    char portnames[BUFSIZE], *curdev, pname[100];

	    memset(portnames, 0, sizeof(portnames));
	    serial_getportnames(portnames);

	    dlg_update_start(ctrl, dlg);
	    dlg_specialedit_switch(ctrl, dlg, 1);
	    dlg_listbox_clear(ctrl, dlg);
	    curdev = strtok(portnames, "|");
	    while (curdev != NULL) {
		pname[0] = '\0';
		port = 0;
		if (sscanf(curdev, "%s\t%d", &pname, &port) == 2 &&
		    pname[0])
		    dlg_listbox_addwithid(ctrl, dlg, pname, port);
		curdev = strtok(NULL, "|");
	    };

	    for (i = 0; i < dlg_listbox_getcount(ctrl, dlg); i++) {
		if (dlg_listbox_getid(ctrl, dlg, i) == cfg->port) {
		    dlg_listbox_select(ctrl, dlg, i);
		    selected = TRUE;
		};
	    };
	    if (!selected) dlg_listbox_select(ctrl, dlg, 0);
	    dlg_update_done(ctrl, dlg);
	} else {
	    dlg_update_start(ctrl, dlg);
	    dlg_specialedit_switch(ctrl, dlg, 0);
	    str = dupprintf("%d", cfg->port);
	    dlg_editbox_set(ctrl, dlg, str);
	    sfree(str);
	    dlg_update_done(ctrl, dlg);
	};
    } else if (event == EVENT_VALCHANGE) {
	if (cfg->protocol == PROT_SERIAL) {
	    cfg->port =
		dlg_listbox_getid(ctrl, dlg, dlg_listbox_index(ctrl, dlg));
	} else {
	    char str[10];
	    int i, j;

	    dlg_editbox_get(ctrl, dlg, str, 10);
	    i = sscanf(str, "%d", &j);

	    if (i == 1)
		cfg->port = j;
	};
    };
};

/*
 * We export this function so that platform-specific config
 * routines can use it to conveniently identify the protocol radio
 * buttons in order to add to them.
 */
void config_protocolbuttons_handler(union control *ctrl, void *dlg,
				    void *data, int event)
{
    int button, defport;
    Config *cfg = (Config *)data;
    struct protocolbuttons_data *pbd =
	(struct protocolbuttons_data *) ctrl->generic.context.p;
    /*
     * This function works just like the standard radio-button
     * handler, except that it also has to change the setting of
     * the port box. We expect the context parameter to point at
     * the `union control' structure for the port box.
     */
    if (event == EVENT_REFRESH) {
	for (button = 0; button < ctrl->radio.nbuttons; button++)
	    if (cfg->protocol == ctrl->radio.buttondata[button].i)
		break;
	/* We expected that `break' to happen, in all circumstances. */
	assert(button < ctrl->radio.nbuttons);
	dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
	int oldproto = cfg->protocol;
	button = dlg_radiobutton_get(ctrl, dlg);
	assert(button >= 0 && button < ctrl->radio.nbuttons);
	cfg->protocol = ctrl->radio.buttondata[button].i;
	if (oldproto != cfg->protocol) {
	    defport = -1;
	    switch (cfg->protocol) {
	      case PROT_SSH: defport = 22; break;
	      case PROT_TELNET: defport = 23; break;
	      case PROT_RLOGIN: defport = 513; break;
          case PROT_SERIAL: defport = 1; break;
	    }
	    if (cfg->protocol == PROT_SERIAL) {
		dlg_setcontroltext(pbd->host, dlg, PHONE_NUMBER_OR_BLANK);
		dlg_refresh(pbd->host, dlg);
		dlg_refresh(pbd->port, dlg);
	    } else {
		dlg_setcontroltext(pbd->host, dlg, HOST_BOX_TITLE);
		dlg_refresh(pbd->host, dlg);
		if (defport > 0 && cfg->port != defport) {
		    cfg->port = defport;
		    dlg_refresh(pbd->port, dlg);
		} else if (cfg->protocol == PROT_RAW)
		    dlg_refresh(pbd->port, dlg);
	    };
	}
    }
}

static void loggingbuttons_handler(union control *ctrl, void *dlg,
				   void *data, int event)
{
    int button;
    Config *cfg = (Config *)data;
    /* This function works just like the standard radio-button handler,
     * but it has to fall back to "no logging" in situations where the
     * configured logging type isn't applicable.
     */
    if (event == EVENT_REFRESH) {
	for (button = 0; button < ctrl->radio.nbuttons; button++)
	    if (cfg->logtype == ctrl->radio.buttondata[button].i)
		break;
    
	/* We fell off the end, so we lack the configured logging type */
	if (button == ctrl->radio.nbuttons) {
        button=0;
        cfg->logtype=LGTYP_NONE;
	}
	dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
	button = dlg_radiobutton_get(ctrl, dlg);
	assert(button >= 0 && button < ctrl->radio.nbuttons);
	cfg->logtype = ctrl->radio.buttondata[button].i;
    }
}

static void numeric_keypad_handler(union control *ctrl, void *dlg,
				   void *data, int event)
{
    int button;
    Config *cfg = (Config *)data;
    /*
     * This function works much like the standard radio button
     * handler, but it has to handle two fields in Config.
     */
    if (event == EVENT_REFRESH) {
	if (cfg->nethack_keypad)
	    button = 2;
	else if (cfg->app_keypad)
	    button = 1;
	else
	    button = 0;
	assert(button < ctrl->radio.nbuttons);
	dlg_radiobutton_set(ctrl, dlg, button);
    } else if (event == EVENT_VALCHANGE) {
	button = dlg_radiobutton_get(ctrl, dlg);
	assert(button >= 0 && button < ctrl->radio.nbuttons);
	if (button == 2) {
	    cfg->app_keypad = FALSE;
	    cfg->nethack_keypad = TRUE;
	} else {
	    cfg->app_keypad = (button != 0);
	    cfg->nethack_keypad = FALSE;
	}
    }
}

static void cipherlist_handler(union control *ctrl, void *dlg,
			       void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;

	static const struct { char *s; int c; } ciphers[] = {
	    { "3DES",			CIPHER_3DES },
	    { "Blowfish",		CIPHER_BLOWFISH },
	    { "DES",			CIPHER_DES },
	    { "AES (SSH-2 only)",	CIPHER_AES },
	    { "Arcfour (SSH-2 only)",	CIPHER_ARCFOUR },
	    { "-- warn below here --",	CIPHER_WARN }
	};

	/* Set up the "selected ciphers" box. */
	/* (cipherlist assumed to contain all ciphers) */
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < CIPHER_MAX; i++) {
	    int c = cfg->ssh_cipherlist[i];
	    int j;
	    char *cstr = NULL;
	    for (j = 0; j < (sizeof ciphers) / (sizeof ciphers[0]); j++) {
		if (ciphers[j].c == c) {
		    cstr = ciphers[j].s;
		    break;
		}
	    }
	    dlg_listbox_addwithid(ctrl, dlg, cstr, c);
	}
	dlg_update_done(ctrl, dlg);

    } else if (event == EVENT_VALCHANGE) {
	int i;

	/* Update array to match the list box. */
	for (i=0; i < CIPHER_MAX; i++)
	    cfg->ssh_cipherlist[i] = dlg_listbox_getid(ctrl, dlg, i);

    }
}

static void kexlist_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;

	static const struct { char *s; int k; } kexes[] = {
	    { "Diffie-Hellman group 1",		KEX_DHGROUP1 },
	    { "Diffie-Hellman group 14",	KEX_DHGROUP14 },
	    { "Diffie-Hellman group exchange",	KEX_DHGEX },
	    { "-- warn below here --",		KEX_WARN }
	};

	/* Set up the "kex preference" box. */
	/* (kexlist assumed to contain all algorithms) */
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < KEX_MAX; i++) {
	    int k = cfg->ssh_kexlist[i];
	    int j;
	    char *kstr = NULL;
	    for (j = 0; j < (sizeof kexes) / (sizeof kexes[0]); j++) {
		if (kexes[j].k == k) {
		    kstr = kexes[j].s;
		    break;
		}
	    }
	    dlg_listbox_addwithid(ctrl, dlg, kstr, k);
	}
	dlg_update_done(ctrl, dlg);

    } else if (event == EVENT_VALCHANGE) {
	int i;

	/* Update array to match the list box. */
	for (i=0; i < KEX_MAX; i++)
	    cfg->ssh_kexlist[i] = dlg_listbox_getid(ctrl, dlg, i);

    }
}

static void printerbox_handler(union control *ctrl, void *dlg,
			       void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int nprinters, i;
	printer_enum *pe;

	dlg_update_start(ctrl, dlg);
	/*
	 * Some backends may wish to disable the drop-down list on
	 * this edit box. Be prepared for this.
	 */
	if (ctrl->editbox.has_list) {
	    dlg_listbox_clear(ctrl, dlg);
	    dlg_listbox_add(ctrl, dlg, PRINTER_DISABLED_STRING);
	    pe = printer_start_enum(&nprinters);
	    for (i = 0; i < nprinters; i++)
		dlg_listbox_add(ctrl, dlg, printer_get_name(pe, i));
	    printer_finish_enum(pe);
	}
	dlg_editbox_set(ctrl, dlg,
			(*cfg->printer ? cfg->printer :
			 PRINTER_DISABLED_STRING));
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
	dlg_editbox_get(ctrl, dlg, cfg->printer, sizeof(cfg->printer));
	if (!strcmp(cfg->printer, PRINTER_DISABLED_STRING))
	    *cfg->printer = '\0';
    }
}

static void codepage_handler(union control *ctrl, void *dlg,
			     void *data, int event)
{
    Config *cfg = (Config *)data;
    if (event == EVENT_REFRESH) {
	int i;
	const char *cp;
	dlg_update_start(ctrl, dlg);
	strcpy(cfg->line_codepage,
	       cp_name(decode_codepage(cfg->line_codepage)));
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; (cp = cp_enumerate(i)) != NULL; i++)
	    dlg_listbox_add(ctrl, dlg, cp);
	dlg_editbox_set(ctrl, dlg, cfg->line_codepage);
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
	dlg_editbox_get(ctrl, dlg, cfg->line_codepage,
			sizeof(cfg->line_codepage));
	strcpy(cfg->line_codepage,
	       cp_name(decode_codepage(cfg->line_codepage)));
    }
}

static void sshbug_handler(union control *ctrl, void *dlg,
			   void *data, int event)
{
    if (event == EVENT_REFRESH) {
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	dlg_listbox_addwithid(ctrl, dlg, "Auto", AUTO);
	dlg_listbox_addwithid(ctrl, dlg, "Off", FORCE_OFF);
	dlg_listbox_addwithid(ctrl, dlg, "On", FORCE_ON);
	switch (*(int *)ATOFFSET(data, ctrl->listbox.context.i)) {
	  case AUTO:      dlg_listbox_select(ctrl, dlg, 0); break;
	  case FORCE_OFF: dlg_listbox_select(ctrl, dlg, 1); break;
	  case FORCE_ON:  dlg_listbox_select(ctrl, dlg, 2); break;
	}
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_SELCHANGE) {
	int i = dlg_listbox_index(ctrl, dlg);
	if (i < 0)
	    i = AUTO;
	else
	    i = dlg_listbox_getid(ctrl, dlg, i);
	*(int *)ATOFFSET(data, ctrl->listbox.context.i) = i;
    }
}

#define SAVEDSESSION_LEN 2048

struct sessionsaver_data {
    union control *editbox, *listbox, *loadbutton, *savebutton, *delbutton;
    union control *okbutton, *cancelbutton;
    union control *mkfolderbutton, *pathview;
    struct sesslist sesslist;
    int midsession;
    char currentpath[BUFSIZE];
};

/* 
 * Helper function to load the session selected in the list box, if
 * any, as this is done in more than one place below. Returns 0 for
 * failure.
 */
static int load_selected_session(struct sessionsaver_data *ssd,
				 char *savedsession,
				 void *dlg, Config *cfg, int *maybe_launch)
{
    char *name, path[BUFSIZE];

    int i = dlg_listbox_index(ssd->listbox, dlg);
    int isdef;
    if (i < 0) {
	dlg_beep(dlg);
	return 0;
    }
    if (!strcmp(ssd->sesslist.sessions[i], "..")) {
	ses_pathname(ssd->currentpath, path, BUFSIZE);
	get_sesslist(&cfg->sessionroot, &ssd->sesslist, path, FALSE);
	get_sesslist(&cfg->sessionroot, &ssd->sesslist, path, TRUE);
	savedsession[0] = '\0';
	strcpy(ssd->currentpath, path);
	dlg_setcontroltext(ssd->pathview, dlg, savedsession);
	return 0;
    } else if (ses_is_folder(&cfg->sessionroot, ssd->sesslist.sessions[i])) {
	strncpy(path, ssd->sesslist.sessions[i], BUFSIZE);
	get_sesslist(&cfg->sessionroot, &ssd->sesslist, path, FALSE);
	get_sesslist(&cfg->sessionroot, &ssd->sesslist, path, TRUE);
	savedsession[0] = '\0';
	strcpy(ssd->currentpath, path);
	dlg_setcontroltext(ssd->pathview, dlg, savedsession);
	return 0;
    };
    isdef = !strcmp(ssd->sesslist.sessions[i], "Default Settings");
    load_settings(ssd->sesslist.sessions[i], cfg);
    if (!isdef) {
	name = ses_lastname(ssd->sesslist.sessions[i]);
	strncpy(savedsession, name, SAVEDSESSION_LEN);
	savedsession[SAVEDSESSION_LEN-1] = '\0';
	if (maybe_launch)
	    *maybe_launch = TRUE;
    } else {
	savedsession[0] = '\0';
	if (maybe_launch)
	    *maybe_launch = FALSE;
    }
    dlg_refresh(NULL, dlg);
    /* Restore the selection, which might have been clobbered by
     * changing the value of the edit box. */
    dlg_listbox_select(ssd->listbox, dlg, i);
    return 1;
}

static void sessionsaver_handler(union control *ctrl, void *dlg,
				 void *data, int event)
{
    Config *cfg = (Config *)data;
    struct sessionsaver_data *ssd =
	(struct sessionsaver_data *)ctrl->generic.context.p;
    char *savedsession;
    char buf[BUFSIZE], *tmp;

    /*
     * The first time we're called in a new dialog, we must
     * allocate space to store the current contents of the saved
     * session edit box (since it must persist even when we switch
     * panels, but is not part of the Config).
     */
    if (!ssd->editbox) {
	savedsession = NULL;
    } else if (!dlg_get_privdata(ssd->editbox, dlg)) {
	savedsession = (char *)
	    dlg_alloc_privdata(ssd->editbox, dlg, SAVEDSESSION_LEN);
	if (loaded_session_edit) {
	    ses_pathname(loaded_session_name, buf, BUFSIZE);
	    strcpy(ssd->currentpath, buf);
	    tmp = ses_lastname(loaded_session_name);
	    strcpy(savedsession, tmp);
	    dlg_setcontroltext(ssd->pathview, dlg, loaded_session_name);
	    get_sesslist(&cfg->sessionroot, &ssd->sesslist, "", FALSE);
	    get_sesslist(&cfg->sessionroot, &ssd->sesslist, ssd->currentpath, TRUE);
	} else
	    savedsession[0] = '\0';
    } else {
	savedsession = dlg_get_privdata(ssd->editbox, dlg);
    }

    if (event == EVENT_REFRESH) {
	if (ctrl == ssd->editbox) {
	    dlg_editbox_set(ctrl, dlg, savedsession);
	} else if (ctrl == ssd->listbox) {
	    int i;
	    int selected = 0;

	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < ssd->sesslist.nsessions; i++) {
		dlg_listbox_add(ctrl, dlg, ssd->sesslist.sessions[i]);
		if (!strcmp(ses_lastname(ssd->sesslist.sessions[i]),
		     savedsession))
		    selected = i;
	    };
	    dlg_listbox_select(ctrl, dlg, selected);
	    if (!strcmp(ssd->sesslist.sessions[selected], ".."))
		dlg_setcontroltext(ssd->pathview, dlg, ssd->currentpath);
	    else
		dlg_setcontroltext(ssd->pathview, dlg, ssd->sesslist.sessions[selected]);
	    dlg_update_done(ctrl, dlg);
	/*
	 * Check the session root properties, and if it's read only,
	 * disable the "Save", "Delete" and "New folder" buttons.
	 */
	} else if (ctrl == ssd->mkfolderbutton)
	    dlg_control_enable(ssd->mkfolderbutton, dlg, !cfg->sessionroot.readonly);
	else if (ctrl == ssd->delbutton)
	    dlg_control_enable(ssd->delbutton, dlg, !cfg->sessionroot.readonly);
	else if (ctrl == ssd->savebutton)
	    dlg_control_enable(ssd->savebutton, dlg, !cfg->sessionroot.readonly);
    } else if (event == EVENT_VALCHANGE) {
	if (ctrl == ssd->editbox) {
	    dlg_editbox_get(ctrl, dlg, savedsession, SAVEDSESSION_LEN);
	}
    } else if (event == EVENT_SELCHANGE) {
	if (ctrl == ssd->listbox) {
	    int i;

	    i = dlg_listbox_index(ctrl, dlg);

	    if (!strcmp(ssd->sesslist.sessions[i], ".."))
		dlg_setcontroltext(ssd->pathview, dlg, ssd->currentpath);
	    else
		dlg_setcontroltext(ssd->pathview, dlg,
				   ssd->sesslist.sessions[i]);
	}
    } else if (event == EVENT_ACTION) {
	if ((ctrl == ssd->listbox ||
	     (ssd->loadbutton && ctrl == ssd->loadbutton))) {
	    /*
	     * The user has double-clicked a session, or hit Load.
	     * We must load the selected session, and then
	     * terminate the configuration dialog _if_ there was a
	     * double-click on the list box _and_ that session
	     * contains a hostname.
	     */
	    int mbl = FALSE;
	    int i, isfolder;

	    i = dlg_listbox_index(ssd->listbox, dlg);
	    isfolder = !strcmp(ssd->sesslist.sessions[i], "..") ||
		ses_is_folder(&cfg->sessionroot, ssd->sesslist.sessions[i]);
	    
	    if ((ssd->midsession && 
		 isfolder &&
		 load_selected_session(ssd, savedsession, dlg, cfg, &mbl)) ||
	         (!ssd->midsession &&
		  load_selected_session(ssd, savedsession, dlg, cfg, &mbl)) &&
		/*
		 * With Serial backend, host name is not required.
		 * If it is present, it should contain a number
		 * that should be dialed to connect to remote host.
		 */
		(ctrl == ssd->listbox &&
		 (cfg->protocol == PROT_SERIAL || cfg->host[0]))) {
		dlg_end(dlg, 1);       /* it's all over, and succeeded */
	    } else {
		dlg_refresh(ssd->editbox, dlg);
		dlg_refresh(ssd->listbox, dlg);
	    };
	} else if (ctrl == ssd->savebutton) {
	    int isdef = !strcmp(savedsession, "Default Settings");
	    if (!savedsession[0]) {
		int i = dlg_listbox_index(ssd->listbox, dlg);
		if (i < 0) {
		    dlg_beep(dlg);
		    return;
		}
		isdef = !strcmp(ssd->sesslist.sessions[i], "Default Settings");
		if (!isdef) {
		    strncpy(savedsession, ssd->sesslist.sessions[i],
			    SAVEDSESSION_LEN);
		    savedsession[SAVEDSESSION_LEN-1] = '\0';
		} else {
		    savedsession[0] = '\0';
		}
	    } else {
		char *tmp;
		int len;

		len = strlen(ssd->currentpath) + strlen(savedsession) + 2;
		tmp = smalloc(len);
		if (strlen(ssd->currentpath))
		    sprintf(tmp, "%s\\%s", ssd->currentpath, savedsession);
		else
		    sprintf(tmp, "%s", savedsession);
		strncpy(savedsession, tmp, SAVEDSESSION_LEN);
		savedsession[SAVEDSESSION_LEN - 1] = '\0';
	    };
	    {
                char *errmsg = save_settings(savedsession, cfg);
		if (errmsg) {
		    dlg_error_msg(dlg, errmsg);
		    sfree(errmsg);
		}
	    }
	    get_sesslist(&cfg->sessionroot, &ssd->sesslist, ssd->currentpath, FALSE);
	    get_sesslist(&cfg->sessionroot, &ssd->sesslist, ssd->currentpath, TRUE);
	    {
		int i, selected = -1;
		char *name;

		for (i = 0; i < ssd->sesslist.nsessions; i++)
		    if (!strcmp(ssd->sesslist.sessions[i], savedsession))
			selected = i;
		if (selected >= 0) {
		    name = ses_lastname(ssd->sesslist.sessions[selected]);
		    strcpy(savedsession, name);
		};
	    };
	    dlg_refresh(ssd->editbox, dlg);
	    dlg_refresh(ssd->listbox, dlg);
	} else if (ctrl == ssd->mkfolderbutton) {
	    char tmp[BUFSIZE];

	    if (ssd->currentpath[0])
		sprintf(tmp, "%s\\%s", ssd->currentpath,
			savedsession[0] ? savedsession : "New folder");
	    else
		strcpy(tmp, savedsession[0] ? savedsession : "New folder");
	    ses_make_folder(&cfg->sessionroot, tmp);
	    get_sesslist(&cfg->sessionroot, &ssd->sesslist, ssd->currentpath, FALSE);
	    get_sesslist(&cfg->sessionroot, &ssd->sesslist, ssd->currentpath, TRUE);
	    savedsession[0] = '\0';
	    dlg_refresh(ssd->editbox, dlg);
	    dlg_refresh(ssd->listbox, dlg);
	} else if (!ssd->midsession &&
		   ssd->delbutton && ctrl == ssd->delbutton) {
	    int i = dlg_listbox_index(ssd->listbox, dlg);
	    if (i <= 0) {
		dlg_beep(dlg);
	    } else {
		if (ses_is_folder(&cfg->sessionroot, ssd->sesslist.sessions[i])) {
		    char msg[BUFSIZE];

		    sprintf(msg, "Are you sure you want to delete \"%s\" folder and all its contents?",
			ses_lastname(ssd->sesslist.sessions[i]));
		    if (!dlg_yesnobox(dlg, msg))
			return;
		};
		del_settings(&cfg->sessionroot, ssd->sesslist.sessions[i]);
		get_sesslist(&cfg->sessionroot, &ssd->sesslist, ssd->currentpath, FALSE);
		get_sesslist(&cfg->sessionroot, &ssd->sesslist, ssd->currentpath, TRUE);
		dlg_refresh(ssd->listbox, dlg);
	    }
	} else if (ctrl == ssd->okbutton) {
	    if (ssd->midsession) {
		/* In a mid-session Change Settings, Apply is always OK. */
		dlg_end(dlg, 1);
		return;
	    }
	    if (dlg_last_focused(ctrl, dlg) == ssd->listbox) {
		int i = dlg_listbox_index(ssd->listbox, dlg);
		if (i < 0)
		    dlg_beep(dlg);
		else if (!strcmp(ssd->sesslist.sessions[i], "..") ||
			ses_is_folder(&cfg->sessionroot, ssd->sesslist.sessions[i])) {
		    int mbl = FALSE;
		    load_selected_session(ssd, savedsession, dlg, cfg, &mbl);
		    dlg_refresh(ssd->editbox, dlg);
		    dlg_refresh(ssd->listbox, dlg);
		    return;
		};
	    }
	    /*
	     * Annoying special case. If the `Open' button is
	     * pressed while no host name is currently set, _and_
	     * the session list previously had the focus, _and_
	     * there was a session selected in that which had a
	     * valid host name in it, then load it and go.
	     */
	    if (dlg_last_focused(ctrl, dlg) == ssd->listbox &&
		!cfg_launchable(cfg)) {
		Config cfg2;
		int mbl = FALSE;
		if (!load_selected_session(ssd, savedsession, dlg,
					   &cfg2, &mbl)) {
		    dlg_beep(dlg);
		    return;
		}
		/* If at this point we have a valid session, go! */
		if (mbl && cfg_launchable(&cfg2)) {
		    *cfg = cfg2;	/* structure copy */
		    cfg->remote_cmd_ptr = NULL;
		    dlg_end(dlg, 1);
		} else
		    dlg_beep(dlg);
		return;
	    }

	    /*
	     * Serial backend doesn't require host name, so just
	     * skip the check and go.
	     */
	    if (cfg->protocol == PROT_SERIAL) {
		dlg_end(dlg, 1);
	    };

	    /*
	     * Otherwise, do the normal thing: if we have a valid
	     * session, get going.
	     */
	    if (cfg_launchable(cfg)) {
		dlg_end(dlg, 1);
	    } else
		dlg_beep(dlg);
	} else if (ctrl == ssd->cancelbutton) {
	    dlg_end(dlg, 0);
	}
    }
}

struct charclass_data {
    union control *listbox, *editbox, *button;
};

static void charclass_handler(union control *ctrl, void *dlg,
			      void *data, int event)
{
    Config *cfg = (Config *)data;
    struct charclass_data *ccd =
	(struct charclass_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == ccd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < 128; i++) {
		char str[100];
		sprintf(str, "%d\t(0x%02X)\t%c\t%d", i, i,
			(i >= 0x21 && i != 0x7F) ? i : ' ', cfg->wordness[i]);
		dlg_listbox_add(ctrl, dlg, str);
	    }
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == ccd->button) {
	    char str[100];
	    int i, n;
	    dlg_editbox_get(ccd->editbox, dlg, str, sizeof(str));
	    n = atoi(str);
	    for (i = 0; i < 128; i++) {
		if (dlg_listbox_issel(ccd->listbox, dlg, i))
		    cfg->wordness[i] = n;
	    }
	    dlg_refresh(ccd->listbox, dlg);
	}
    }
}

struct colour_data {
    union control *listbox, *redit, *gedit, *bedit, *button,
	*bold_checkbox, *underline_checkbox, *selected_checkbox;
};

static const char *const colours[] = {
    "Default Foreground", "Default Bold Foreground",
    "Default Underlined Foreground",
    "Default Background", "Default Bold Background",
    "Default Underlined Background",
    "Cursor Text", "Cursor Colour",
    "Selected Text Foreground", "Selected Text Background",
    "ANSI Black", "ANSI Black Bold", "ANSI Black Underlined",
    "ANSI Red", "ANSI Red Bold", "ANSI Red Underlined",
    "ANSI Green", "ANSI Green Bold", "ANSI Green Underlined",
    "ANSI Yellow", "ANSI Yellow Bold", "ANSI Yellow Underlined",
    "ANSI Blue", "ANSI Blue Bold", "ANSI Blue Underlined",
    "ANSI Magenta", "ANSI Magenta Bold", "ANSI Magenta Underlined",
    "ANSI Cyan", "ANSI Cyan Bold", "ANSI Cyan Underlined",
    "ANSI White", "ANSI White Bold", "ANSI White Underlined"
};

static const int itemcolour[] = {
    0, 1, 2, 0, 1, 2, 0, 0, 3, 3, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2,
    0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2
};

static const int idxcolour[2][2][2][34] = {
    {
     {
      {0, 2, 4, 5, 6, 8, 10, 12, 14, 16,
       18, 20, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1},		/* 0, 0, 0: !bold && !under && !selected */
      {0, 2, 4, 5, 32, 33, 6, 8, 10, 12,
       18, 20, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1}		/* 0, 0, 1: !bold && !under && selected */
      },
     {
      {0, 22, 2, 23, 4, 5, 6, 24, 8, 25,
       10, 26, 12, 27, 14, 28, 16, 29, 18, 30,
       20, 31, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1},		/* 0, 1, 0: !bold && under && !selected */
      {0, 22, 2, 23, 4, 5, 32, 33, 6, 24,
       8, 25, 10, 26, 12, 27, 14, 28, 16, 29,
       18, 30, 20, 31, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1}		/* 0, 1, 1: !bold && under && selected */
      },
     },
    {
     {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
       10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
       20, 21, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1},		/* 1, 0, 0: bold && !under && !selected */
      {0, 1, 2, 3, 4, 5, 32, 33, 6, 7,
       8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
       18, 19, 20, 21, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1}		/* 1, 0, 1: bold && !under && selected */
      },
     {
      {0, 1, 22, 2, 3, 23, 4, 5, 6, 7,
       24, 8, 9, 25, 10, 11, 26, 12, 13, 27,
       14, 15, 28, 16, 17, 29, 18, 19, 30, 20,
       21, 31, -1, -1},		/* 1, 1, 0: bold && under && !selected */
      {0, 1, 22, 2, 3, 23, 4, 5, 32, 33,
       6, 7, 24, 8, 9, 25, 10, 11, 26, 12,
       13, 27, 14, 15, 28, 16, 17, 29, 18, 19,
       30, 20, 21, 31}		/* 1, 1, 1: bold && under && selected */
      },
     }
};

static void colour_handler(union control *ctrl, void *dlg,
			   void *data, int event)
{
    Config *cfg = (Config *)data;
    struct colour_data *cd =
	(struct colour_data *)ctrl->generic.context.p;
    int update = FALSE, r, g, b;

    if (ctrl == cd->bold_checkbox) {
	switch (event) {
	case EVENT_REFRESH:
	    dlg_checkbox_set(cd->bold_checkbox, dlg, cfg->bold_colour);
	    break;
	case EVENT_VALCHANGE:
	    cfg->bold_colour = dlg_checkbox_get(cd->bold_checkbox, dlg);
	    dlg_refresh(cd->listbox, dlg);
	    break;
	};
    } else if (ctrl == cd->underline_checkbox) {
	switch (event) {
	case EVENT_REFRESH:
	    dlg_checkbox_set(cd->underline_checkbox, dlg,
			     cfg->under_colour);
	    break;
	case EVENT_VALCHANGE:
	    cfg->under_colour =
		dlg_checkbox_get(cd->underline_checkbox, dlg);
	    dlg_refresh(cd->listbox, dlg);
	    break;
	};
    } else if (ctrl == cd->selected_checkbox) {
	switch (event) {
	case EVENT_REFRESH:
	    dlg_checkbox_set(cd->selected_checkbox, dlg, cfg->sel_colour);
	    break;
	case EVENT_VALCHANGE:
	    cfg->sel_colour = dlg_checkbox_get(cd->selected_checkbox, dlg);
	    dlg_refresh(cd->listbox, dlg);
	    break;
	};
    };

    if (event == EVENT_REFRESH) {
	if (ctrl == cd->listbox) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; i < lenof(colours); i++)
		/* This allows us to hide list items we don't need to
		 * see: if bold-as-colour (or underline) turned off, we just hide those bold
		 * choices to decrease user confusion. And, of course, it looks
		 * lots cooler to have "jumping" controls. Feels more interactive. :)
		 */
	    {
		switch (itemcolour[i]) {
		case 0:
		    dlg_listbox_add(ctrl, dlg, colours[i]);
		    break;
		case 1:
		    if (cfg->bold_colour)
			dlg_listbox_add(ctrl, dlg, colours[i]);
		    break;
		case 2:
		    if (cfg->under_colour)
			dlg_listbox_add(ctrl, dlg, colours[i]);
		    break;
		case 3:
		    if (cfg->sel_colour)
			dlg_listbox_add(ctrl, dlg, colours[i]);
		};
	    };
	    dlg_update_done(ctrl, dlg);
	    dlg_editbox_set(cd->redit, dlg, "");
	    dlg_editbox_set(cd->gedit, dlg, "");
	    dlg_editbox_set(cd->bedit, dlg, "");
	}
    } else if (event == EVENT_SELCHANGE) {
	if (ctrl == cd->listbox) {
	    /* The user has selected a colour. Update the RGB text. */
	    int i = dlg_listbox_index(ctrl, dlg);
	    if (i < 0) {
		dlg_beep(dlg);
		return;
	    }
	    /* I know this looks a bit weird, but I just had no
	     * other choice. Other way it would break existing code and
	     * worse yet, existing saved session structure.
	     * This way, underline colours are stored in last 10
	     * positions of the colours array, not breaking anything.
	     * But the price is some weird transformations based on
	     * predefined index array.
	     */
	    i = idxcolour[cfg->bold_colour][cfg->under_colour][cfg->
							       sel_colour]
		[i];
	    r = cfg->colours[i][0];
	    g = cfg->colours[i][1];
	    b = cfg->colours[i][2];
	    update = TRUE;
	}
    } else if (event == EVENT_VALCHANGE) {
	if (ctrl == cd->redit || ctrl == cd->gedit || ctrl == cd->bedit) {
	    /* The user has changed the colour using the edit boxes. */
	    char buf[80];
	    int i, cval;

	    dlg_editbox_get(ctrl, dlg, buf, lenof(buf));
	    cval = atoi(buf);
	    if (cval > 255) cval = 255;
	    if (cval < 0)   cval = 0;

	    i = dlg_listbox_index(cd->listbox, dlg);
	    if (i >= 0) {
		i = idxcolour[cfg->bold_colour][cfg->under_colour][cfg->
								   sel_colour]
		    [i];
		if (ctrl == cd->redit)
		    cfg->colours[i][0] = cval;
		else if (ctrl == cd->gedit)
		    cfg->colours[i][1] = cval;
		else if (ctrl == cd->bedit)
		    cfg->colours[i][2] = cval;
	    }
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == cd->button) {
	    int i = dlg_listbox_index(cd->listbox, dlg);
	    if (i < 0) {
		dlg_beep(dlg);
		return;
	    }
	    /*
	     * Start a colour selector, which will send us an
	     * EVENT_CALLBACK when it's finished and allow us to
	     * pick up the results.
	     */
	    i = idxcolour[cfg->bold_colour][cfg->under_colour][cfg->
							       sel_colour]
		[i];
	    dlg_coloursel_start(ctrl, dlg, cfg->colours[i][0],
				cfg->colours[i][1], cfg->colours[i][2]);
	}
    } else if (event == EVENT_CALLBACK) {
	if (ctrl == cd->button) {
	    int i = dlg_listbox_index(cd->listbox, dlg);
	    /*
	     * Collect the results of the colour selector. Will
	     * return nonzero on success, or zero if the colour
	     * selector did nothing (user hit Cancel, for example).
	     */
	    i = idxcolour[cfg->bold_colour][cfg->under_colour][cfg->
							       sel_colour]
		[i];
	    if (dlg_coloursel_results(ctrl, dlg, &r, &g, &b)) {
		cfg->colours[i][0] = r;
		cfg->colours[i][1] = g;
		cfg->colours[i][2] = b;
		update = TRUE;
	    }
	}
    }

    if (update) {
	char buf[40];
	sprintf(buf, "%d", r); dlg_editbox_set(cd->redit, dlg, buf);
	sprintf(buf, "%d", g); dlg_editbox_set(cd->gedit, dlg, buf);
	sprintf(buf, "%d", b); dlg_editbox_set(cd->bedit, dlg, buf);
    }
}

struct ttymodes_data {
    union control *modelist, *valradio, *valbox;
    union control *addbutton, *rembutton, *listbox;
};

static void ttymodes_handler(union control *ctrl, void *dlg,
			     void *data, int event)
{
    Config *cfg = (Config *)data;
    struct ttymodes_data *td =
	(struct ttymodes_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == td->listbox) {
	    char *p = cfg->ttymodes;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
		int tabpos = strchr(p, '\t') - p;
		char *disp = dupprintf("%.*s\t%s", tabpos, p,
				       (p[tabpos+1] == 'A') ? "(auto)" :
				       p+tabpos+2);
		dlg_listbox_add(ctrl, dlg, disp);
		p += strlen(p) + 1;
		sfree(disp);
	    }
	    dlg_update_done(ctrl, dlg);
	} else if (ctrl == td->modelist) {
	    int i;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    for (i = 0; ttymodes[i]; i++)
		dlg_listbox_add(ctrl, dlg, ttymodes[i]);
	    dlg_listbox_select(ctrl, dlg, 0); /* *shrug* */
	    dlg_update_done(ctrl, dlg);
	} else if (ctrl == td->valradio) {
	    dlg_radiobutton_set(ctrl, dlg, 0);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == td->addbutton) {
	    int ind = dlg_listbox_index(td->modelist, dlg);
	    if (ind >= 0) {
		char type = dlg_radiobutton_get(td->valradio, dlg) ? 'V' : 'A';
		int slen, left;
		char *p, str[lenof(cfg->ttymodes)];
		/* Construct new entry */
		memset(str, 0, lenof(str));
		strncpy(str, ttymodes[ind], lenof(str)-3);
		slen = strlen(str);
		str[slen] = '\t';
		str[slen+1] = type;
		slen += 2;
		if (type == 'V') {
		    dlg_editbox_get(td->valbox, dlg, str+slen, lenof(str)-slen);
		}
		/* Find end of list, deleting any existing instance */
		p = cfg->ttymodes;
		left = lenof(cfg->ttymodes);
		while (*p) {
		    unsigned int t = strchr(p, '\t') - p;
		    if (t == strlen(ttymodes[ind]) &&
			strncmp(p, ttymodes[ind], t) == 0) {
			memmove(p, p+strlen(p)+1, left - (strlen(p)+1));
			continue;
		    }
		    left -= strlen(p) + 1;
		    p    += strlen(p) + 1;
		}
		/* Append new entry */
		memset(p, 0, left);
		strncpy(p, str, left - 2);
		dlg_refresh(td->listbox, dlg);
	    } else
		dlg_beep(dlg);
	} else if (ctrl == td->rembutton) {
	    char *p = cfg->ttymodes;
	    int i = 0, len = lenof(cfg->ttymodes);
	    while (*p) {
		int multisel = dlg_listbox_index(td->listbox, dlg) < 0;
		if (dlg_listbox_issel(td->listbox, dlg, i)) {
		    if (!multisel) {
			/* Populate controls with entry we're about to
			 * delete, for ease of editing.
			 * (If multiple entries were selected, don't
			 * touch the controls.) */
			char *val = strchr(p, '\t');
			if (val) {
			    int ind = 0;
			    val++;
			    while (ttymodes[ind]) {
				if (strlen(ttymodes[ind]) == (unsigned int)(val-p-1) &&
				    !strncmp(ttymodes[ind], p, val-p-1))
				    break;
				ind++;
			    }
			    dlg_listbox_select(td->modelist, dlg, ind);
			    dlg_radiobutton_set(td->valradio, dlg,
						(*val == 'V'));
			    dlg_editbox_set(td->valbox, dlg, val+1);
			}
		    }
		    memmove(p, p+strlen(p)+1, len - (strlen(p)+1));
		    i++;
		    continue;
		}
		len -= strlen(p) + 1;
		p   += strlen(p) + 1;
		i++;
	    }
	    memset(p, 0, lenof(cfg->ttymodes) - len);
	    dlg_refresh(td->listbox, dlg);
	}
    }
}

struct environ_data {
    union control *varbox, *valbox, *addbutton, *rembutton, *listbox;
};

static void environ_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct environ_data *ed =
	(struct environ_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == ed->listbox) {
	    char *p = cfg->environmt;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
		dlg_listbox_add(ctrl, dlg, p);
		p += strlen(p) + 1;
	    }
	    dlg_update_done(ctrl, dlg);
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == ed->addbutton) {
	    char str[sizeof(cfg->environmt)];
	    char *p;
	    dlg_editbox_get(ed->varbox, dlg, str, sizeof(str)-1);
	    if (!*str) {
		dlg_beep(dlg);
		return;
	    }
	    p = str + strlen(str);
	    *p++ = '\t';
	    dlg_editbox_get(ed->valbox, dlg, p, sizeof(str)-1 - (p - str));
	    if (!*p) {
		dlg_beep(dlg);
		return;
	    }
	    p = cfg->environmt;
	    while (*p) {
		while (*p)
		    p++;
		p++;
	    }
	    if ((p - cfg->environmt) + strlen(str) + 2 <
		sizeof(cfg->environmt)) {
		strcpy(p, str);
		p[strlen(str) + 1] = '\0';
		dlg_listbox_add(ed->listbox, dlg, str);
		dlg_editbox_set(ed->varbox, dlg, "");
		dlg_editbox_set(ed->valbox, dlg, "");
	    } else {
		dlg_error_msg(dlg, "Environment too big");
	    }
	} else if (ctrl == ed->rembutton) {
	    int i = dlg_listbox_index(ed->listbox, dlg);
	    if (i < 0) {
		dlg_beep(dlg);
	    } else {
		char *p, *q, *str;

		dlg_listbox_del(ed->listbox, dlg, i);
		p = cfg->environmt;
		while (i > 0) {
		    if (!*p)
			goto disaster;
		    while (*p)
			p++;
		    p++;
		    i--;
		}
		q = p;
		if (!*p)
		    goto disaster;
		/* Populate controls with the entry we're about to delete
		 * for ease of editing */
		str = p;
		p = strchr(p, '\t');
		if (!p)
		    goto disaster;
		*p = '\0';
		dlg_editbox_set(ed->varbox, dlg, str);
		p++;
		str = p;
		dlg_editbox_set(ed->valbox, dlg, str);
		p = strchr(p, '\0');
		if (!p)
		    goto disaster;
		p++;
		while (*p) {
		    while (*p)
			*q++ = *p++;
		    *q++ = *p++;
		}
		*q = '\0';
	      disaster:;
	    }
	}
    }
}

struct portfwd_data {
    union control *addbutton, *rembutton, *listbox;
    union control *sourcebox, *destbox, *direction;
#ifndef NO_IPV6
    union control *addressfamily;
#endif
};

static void portfwd_handler(union control *ctrl, void *dlg,
			    void *data, int event)
{
    Config *cfg = (Config *)data;
    struct portfwd_data *pfd =
	(struct portfwd_data *)ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	if (ctrl == pfd->listbox) {
	    char *p = cfg->portfwd;
	    dlg_update_start(ctrl, dlg);
	    dlg_listbox_clear(ctrl, dlg);
	    while (*p) {
		dlg_listbox_add(ctrl, dlg, p);
		p += strlen(p) + 1;
	    }
	    dlg_update_done(ctrl, dlg);
	} else if (ctrl == pfd->direction) {
	    /*
	     * Default is Local.
	     */
	    dlg_radiobutton_set(ctrl, dlg, 0);
#ifndef NO_IPV6
	} else if (ctrl == pfd->addressfamily) {
	    dlg_radiobutton_set(ctrl, dlg, 0);
#endif
	}
    } else if (event == EVENT_ACTION) {
	if (ctrl == pfd->addbutton) {
	    char str[sizeof(cfg->portfwd)];
	    char *p;
	    int i, type;
	    int whichbutton;

	    i = 0;
#ifndef NO_IPV6
	    whichbutton = dlg_radiobutton_get(pfd->addressfamily, dlg);
	    if (whichbutton == 1)
		str[i++] = '4';
	    else if (whichbutton == 2)
		str[i++] = '6';
#endif

	    whichbutton = dlg_radiobutton_get(pfd->direction, dlg);
	    if (whichbutton == 0)
		type = 'L';
	    else if (whichbutton == 1)
		type = 'R';
	    else
		type = 'D';
	    str[i++] = type;

	    dlg_editbox_get(pfd->sourcebox, dlg, str+i, sizeof(str) - i);
	    if (!str[i]) {
		dlg_error_msg(dlg, "You need to specify a source port number");
		return;
	    }
	    p = str + strlen(str);
	    if (type != 'D') {
		*p++ = '\t';
		dlg_editbox_get(pfd->destbox, dlg, p,
				sizeof(str) - (p - str));
		if (!*p || !strchr(p, ':')) {
		    dlg_error_msg(dlg,
				  "You need to specify a destination address\n"
				  "in the form \"host.name:port\"");
		    return;
		}
	    } else
		*p = '\0';
	    p = cfg->portfwd;
	    while (*p) {
		while (*p)
		    p++;
		p++;
	    }
	    if ((p - cfg->portfwd) + strlen(str) + 2 <=
		sizeof(cfg->portfwd)) {
		strcpy(p, str);
		p[strlen(str) + 1] = '\0';
		dlg_listbox_add(pfd->listbox, dlg, str);
		dlg_editbox_set(pfd->sourcebox, dlg, "");
		dlg_editbox_set(pfd->destbox, dlg, "");
	    } else {
		dlg_error_msg(dlg, "Too many forwardings");
	    }
	} else if (ctrl == pfd->rembutton) {
	    int i = dlg_listbox_index(pfd->listbox, dlg);
	    if (i < 0)
		dlg_beep(dlg);
	    else {
		char *p, *q, *src, *dst;
		char dir;

		dlg_listbox_del(pfd->listbox, dlg, i);
		p = cfg->portfwd;
		while (i > 0) {
		    if (!*p)
			goto disaster2;
		    while (*p)
			p++;
		    p++;
		    i--;
		}
		q = p;
		if (!*p)
		    goto disaster2;
		/* Populate the controls with the entry we're about to
		 * delete, for ease of editing. */
		{
		    static const char *const afs = "A46";
		    char *afp = strchr(afs, *p);
		    int idx = afp ? afp-afs : 0;
		    if (afp)
		    p++;
#ifndef NO_IPV6
		    dlg_radiobutton_set(pfd->addressfamily, dlg, idx);
#endif
		}
		{
		    static const char *const dirs = "LRD";
		    dir = *p;
		    dlg_radiobutton_set(pfd->direction, dlg,
					strchr(dirs, dir) - dirs);
		}
		p++;
		if (dir != 'D') {
		    src = p;
		    p = strchr(p, '\t');
		    if (!p)
			goto disaster2;
		    *p = '\0';
		    p++;
		    dst = p;
		} else {
		    src = p;
		    dst = "";
		}
		p = strchr(p, '\0');
		if (!p)
		    goto disaster2;
		dlg_editbox_set(pfd->sourcebox, dlg, src);
		dlg_editbox_set(pfd->destbox, dlg, dst);
		p++;
		while (*p) {
		    while (*p)
			*q++ = *p++;
		    *q++ = *p++;
		}
		*q = '\0';
	      disaster2:;
	    }
	}
    }
}

static int baudrates[12] = { 300, 600, 1200, 2400, 4800, 7200, 9600,
    14400, 19200, 38400, 57600, 115200
};
static const char *parities[5] =
    { "None", "Odd", "Even", "Mark", "Space" };
static const char *stopbits[3] = { "1", "1.5", "2" };
static int databits[4] = { 5, 6, 7, 8 };
static const char *flowcontrols[3] = { "None", "RTS/CTS", "XON/XOFF" };

struct serialpanel_data {
    union control *baud, *parity, *datab, *stopb, *flowctrl;
};

static void serialpanel_handler(union control *ctrl, void *dlg,
				void *data, int event)
{
    Config *cfg = (Config *) data;
    struct serialpanel_data *sd =
	(struct serialpanel_data *) ctrl->generic.context.p;
    int i, gotcha = 0;
    char str[10];

    if (event == EVENT_REFRESH) {
	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	if (ctrl == sd->baud) {
	    for (i = 0; i < lenof(baudrates); i++) {
		sprintf(str, "%d", baudrates[i]);
		dlg_listbox_add(ctrl, dlg, str);
		if (cfg->ser_baud == baudrates[i])
		    gotcha = i;
	    };
	    dlg_listbox_select(ctrl, dlg, gotcha);
	} else if (ctrl == sd->datab) {
	    for (i = 0; i < lenof(databits); i++) {
		sprintf(str, "%d", databits[i]);
		dlg_listbox_add(ctrl, dlg, str);
		if (cfg->ser_databits == databits[i])
		    gotcha = i;
	    };
	    dlg_listbox_select(ctrl, dlg, gotcha);
	} else if (ctrl == sd->parity) {
	    for (i = 0; i < lenof(parities); i++) {
		dlg_listbox_add(ctrl, dlg, parities[i]);
	    };
	    dlg_listbox_select(ctrl, dlg, cfg->ser_parity);
	} else if (ctrl == sd->stopb) {
	    for (i = 0; i < lenof(stopbits); i++) {
		dlg_listbox_add(ctrl, dlg, stopbits[i]);
	    };
	    dlg_listbox_select(ctrl, dlg, cfg->ser_stopbits);
	} else if (ctrl == sd->flowctrl) {
	    for (i = 0; i < lenof(flowcontrols); i++) {
		dlg_listbox_add(ctrl, dlg, flowcontrols[i]);
	    };
	    dlg_listbox_select(ctrl, dlg, cfg->ser_flowcontrol);
	};
	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_SELCHANGE) {
	i = dlg_listbox_index(ctrl, dlg);
	if (ctrl == sd->baud)
	    cfg->ser_baud = baudrates[i];
	else if (ctrl == sd->datab)
	    cfg->ser_databits = databits[i];
	else if (ctrl == sd->flowctrl)
	    cfg->ser_flowcontrol = i;
	else if (ctrl == sd->parity)
	    cfg->ser_parity = i;
	else if (ctrl == sd->stopb)
	    cfg->ser_stopbits = i;
    };
};

struct dialingoptions_data {
    union control *winloc, *mode, *prefix;
};

static void dialingoptions_handler(union control *ctrl, void *dlg,
				   void *data, int event)
{
    Config *cfg = (Config *) data;
    struct dialingoptions_data *dd =
	(struct dialingoptions_data *) ctrl->generic.context.p;

    if (event == EVENT_REFRESH) {
	dlg_checkbox_set(ctrl, dlg, cfg->ser_usewinloc);
	dlg_control_enable(dd->mode, dlg, !cfg->ser_usewinloc);
	dlg_control_enable(dd->prefix, dlg, !cfg->ser_usewinloc);
    } else if (event == EVENT_VALCHANGE) {
	cfg->ser_usewinloc = dlg_checkbox_get(ctrl, dlg);
	dlg_control_enable(dd->mode, dlg, !cfg->ser_usewinloc);
	dlg_control_enable(dd->prefix, dlg, !cfg->ser_usewinloc);
    };
};


static void funky_handler(union control *ctrl, void *dlg,
			  void *data, int event)
{
    Config *cfg = (Config *) data;

    dlg_stdradiobutton_handler(ctrl, dlg, data, event);

    if (event == EVENT_VALCHANGE && cfg->funky_type == FUNKY_ATT513) {
	if (dlg_yesnobox
	    (dlg,
	     "It seems you want to configure 513 terminal emulation "
	     "which is primarily used to manage Lucent/Avaya phone switches. "
	     "Do you want the terminal settings to be adjusted so they will "
	     "be more like Avaya Terminal Emulator's? This will change some "
	     "options which you can review on this and other panels.")) {
	    cfg->cursor_type = 0;	/* block cursor */
	    cfg->blink_cur = TRUE;	/* ...and it will blink */
	    cfg->bold_colour = TRUE;	/* bold is different colour */
	    cfg->under_colour = TRUE;	/* the same for underlined */
	    cfg->sel_colour = TRUE;	/* and the same for selected */
	    cfg->resize_action = RESIZE_DISABLED;
	    cfg->height = 24;
	    cfg->width = 80;
	    cfg->savelines = 0;	/* scrollback is meaningless in this case */
	    cfg->scrollbar = FALSE;	/* so we hide scrollbar */
	    cfg->scroll_on_disp = FALSE;
	    cfg->scroll_on_key = FALSE;
	    cfg->erase_to_scrollback = FALSE;
	    cfg->scrollbar_in_fullscreen = FALSE;
	    cfg->bksp_is_delete = 0;
	    cfg->bottom_buttons = 0;	/* doesn't show bottom buttons (yet) */
	    cfg->passive_telnet = 1;	/* compatibility with c-lan telnetd */
	    cfg->colours[0][0] = 255;
	    cfg->colours[0][1] = 255;
	    cfg->colours[0][2] = 128;
	    cfg->colours[1][0] = 255;
	    cfg->colours[1][1] = 255;
	    cfg->colours[1][2] = 128;
	    cfg->colours[2][0] = 0;
	    cfg->colours[2][1] = 0;
	    cfg->colours[2][2] = 128;
	    cfg->colours[3][0] = 0;
	    cfg->colours[3][1] = 0;
	    cfg->colours[3][2] = 128;
	    cfg->colours[4][0] = 0;
	    cfg->colours[4][1] = 0;
	    cfg->colours[4][2] = 0;
	    cfg->colours[5][0] = 255;
	    cfg->colours[5][1] = 255;
	    cfg->colours[5][2] = 128;
	    cfg->colours[6][0] = 0;
	    cfg->colours[6][1] = 0;
	    cfg->colours[6][2] = 0;
	    cfg->colours[7][0] = 85;
	    cfg->colours[7][1] = 85;
	    cfg->colours[7][2] = 85;
	    cfg->colours[8][0] = 187;
	    cfg->colours[8][1] = 0;
	    cfg->colours[8][2] = 0;
	    cfg->colours[9][0] = 255;
	    cfg->colours[9][1] = 85;
	    cfg->colours[9][2] = 85;
	    cfg->colours[10][0] = 0;
	    cfg->colours[10][1] = 187;
	    cfg->colours[10][2] = 0;
	    cfg->colours[11][0] = 85;
	    cfg->colours[11][1] = 255;
	    cfg->colours[11][2] = 85;
	    cfg->colours[12][0] = 187;
	    cfg->colours[12][1] = 187;
	    cfg->colours[12][2] = 0;
	    cfg->colours[13][0] = 255;
	    cfg->colours[13][1] = 255;
	    cfg->colours[13][2] = 85;
	    cfg->colours[14][0] = 0;
	    cfg->colours[14][1] = 0;
	    cfg->colours[14][2] = 187;
	    cfg->colours[15][0] = 85;
	    cfg->colours[15][1] = 85;
	    cfg->colours[15][2] = 255;
	    cfg->colours[16][0] = 187;
	    cfg->colours[16][1] = 0;
	    cfg->colours[16][2] = 187;
	    cfg->colours[17][0] = 255;
	    cfg->colours[17][1] = 85;
	    cfg->colours[17][2] = 255;
	    cfg->colours[18][0] = 0;
	    cfg->colours[18][1] = 187;
	    cfg->colours[18][2] = 187;
	    cfg->colours[19][0] = 85;
	    cfg->colours[19][1] = 255;
	    cfg->colours[19][2] = 255;
	    cfg->colours[20][0] = 187;
	    cfg->colours[20][1] = 187;
	    cfg->colours[20][2] = 187;
	    cfg->colours[21][0] = 255;
	    cfg->colours[21][1] = 255;
	    cfg->colours[21][2] = 255;
	    cfg->colours[22][0] = 255;
	    cfg->colours[22][1] = 128;
	    cfg->colours[22][2] = 64;
	    cfg->colours[23][0] = 0;
	    cfg->colours[23][1] = 0;
	    cfg->colours[23][2] = 128;
	    cfg->colours[24][0] = 0;
	    cfg->colours[24][1] = 0;
	    cfg->colours[24][2] = 0;
	    cfg->colours[25][0] = 187;
	    cfg->colours[25][1] = 0;
	    cfg->colours[25][2] = 0;
	    cfg->colours[26][0] = 0;
	    cfg->colours[26][1] = 187;
	    cfg->colours[26][2] = 0;
	    cfg->colours[27][0] = 187;
	    cfg->colours[27][1] = 187;
	    cfg->colours[27][2] = 0;
	    cfg->colours[28][0] = 0;
	    cfg->colours[28][1] = 0;
	    cfg->colours[28][2] = 187;
	    cfg->colours[29][0] = 187;
	    cfg->colours[29][1] = 0;
	    cfg->colours[29][2] = 187;
	    cfg->colours[30][0] = 0;
	    cfg->colours[30][1] = 187;
	    cfg->colours[30][2] = 187;
	    cfg->colours[31][0] = 187;
	    cfg->colours[31][1] = 187;
	    cfg->colours[31][2] = 187;
	    cfg->colours[32][0] = 255;
	    cfg->colours[32][1] = 255;
	    cfg->colours[32][2] = 0;
	    cfg->colours[33][0] = 128;
	    cfg->colours[33][1] = 0;
	    cfg->colours[33][2] = 0;
	    dlg_refresh(NULL, dlg);
	};
    };
};

struct sessionsaver_data *ssd;

static void window_icon_handler(union control *ctrl, void *dlg,
				void *data, int event)
{
    Config *cfg = (Config *)data;

    if (event == EVENT_ACTION) {
	char buf[BUFSIZE], iname[BUFSIZE], *ipointer;
	int iindex;

	memset(&iname, 0, sizeof(iname));
	memset(&buf, 0, sizeof(buf));
	iindex = 0;
	ipointer = iname;
	if (dlg_pick_icon(dlg, &ipointer, sizeof(iname), &iindex)
	    && iname[0]) {
	    sprintf(buf, "%s,%d", iname, iindex);
	    dlg_icon_set((union control *) ctrl->button.context.p, dlg,
			 buf);
	    strcpy(cfg->win_icon, buf);
	};
    };
};

void setup_config_box(Config *cfg, struct controlbox *b, struct sesslist *sesslist,
		      int midsession, int protocol, int protcfginfo)
{
    struct controlset *s;
    struct protocolbuttons_data *pbd;
    struct sessionsaver_data *ssd;
    struct charclass_data *ccd;
    struct colour_data *cd;
    struct ttymodes_data *td;
    struct environ_data *ed;
    struct portfwd_data *pfd;
    union control *c;
    char *str;

    ssd = (struct sessionsaver_data *)
	ctrl_alloc(b, sizeof(struct sessionsaver_data));
    memset(ssd, 0, sizeof(*ssd));
    ssd->midsession = midsession;

    /*
     * The standard panel that appears at the bottom of all panels:
     * Open, Cancel, Apply etc.
     */
    s = ctrl_getset(b, "", "", "");
    ctrl_columns(s, 5, 20, 20, 20, 20, 20);
    ssd->okbutton = ctrl_pushbutton(s,
				    (midsession ? "Apply" : "Open"),
				    (char)(midsession ? 'a' : 'o'),
				    HELPCTX(no_help),
				    sessionsaver_handler, P(ssd));
    ssd->okbutton->button.isdefault = TRUE;
    ssd->okbutton->generic.column = 3;
    ssd->cancelbutton = ctrl_pushbutton(s, "Cancel", 'c', HELPCTX(no_help),
					sessionsaver_handler, P(ssd));
    ssd->cancelbutton->button.iscancel = TRUE;
    ssd->cancelbutton->generic.column = 4;
    /* We carefully don't close the 5-column part, so that platform-
     * specific add-ons can put extra buttons alongside Open and Cancel. */

    /*
     * The Session panel.
     */
    str = dupprintf("Basic options for your %s session", appname);
    ctrl_settitle(b, "Session", str);
    sfree(str);

    if (!midsession) {
	s = ctrl_getset(b, "Session", "hostport",
			"Specify the destination you want to connect to");
	ctrl_columns(s, 2, 75, 25);
	pbd = ctrl_alloc(b, sizeof(struct protocolbuttons_data));
	pbd->s = s;
	pbd->host = ctrl_editbox(s, HOST_BOX_TITLE, 'n', 100,
				 HELPCTX(session_hostname),
				 dlg_stdeditbox_handler,
				 I(offsetof(Config, host)),
				 I(sizeof(((Config *) 0)->host)));
	pbd->host->generic.column = 0;
	pbd->port =
	    ctrl_specialeditbox(s, PORT_BOX_TITLE, 'p', 100,
				HELPCTX(session_hostname),
				config_host_handler,
				I(offsetof(Config, port)), I(-1));
	pbd->port->generic.column = 1;
	ctrl_columns(s, 1, 100);

	if (!have_backend(PROT_SSH))
	    pbd->protocol =
		ctrl_radiobuttons(s, "Protocol:", NO_SHORTCUT, 4,
				  HELPCTX(session_hostname),
				  config_protocolbuttons_handler, P(pbd),
				  "Serial", NO_SHORTCUT, I(PROT_SERIAL),
				  "Raw", 'r', I(PROT_RAW),
				  "Telnet", 't', I(PROT_TELNET),
				  "Rlogin", 'i', I(PROT_RLOGIN), NULL);
	else
	    pbd->protocol =
		ctrl_radiobuttons(s, "Protocol:", NO_SHORTCUT, 5,
				  HELPCTX(session_hostname),
				  config_protocolbuttons_handler, P(pbd),
				  "Serial", NO_SHORTCUT, I(PROT_SERIAL),
				  "Raw", 'r', I(PROT_RAW),
				  "Telnet", 't', I(PROT_TELNET),
				  "Rlogin", 'i', I(PROT_RLOGIN),
				  "SSH", 's', I(PROT_SSH), NULL);
    }

    /*
     * The Load/Save panel is available even in mid-session.
     */
    s = ctrl_getset(b, "Session", "savedsessions",
		    midsession ? "Save the current session settings" :
		    "Load, save or delete a stored session");
    ctrl_columns(s, 2, 75, 25);
    get_sesslist(&cfg->sessionroot, &ssd->sesslist, "", TRUE);
    ssd->editbox = ctrl_editbox(s, "Saved Sessions", 'e', 100,
				HELPCTX(session_saved),
				sessionsaver_handler, P(ssd), P(NULL));
    ssd->editbox->generic.column = 0;
    /*
     * An empty static control, to ensure New folder button appears alongside
     * (well, most of :) the edit box not its caption.
     */
    c = ctrl_text(s, " ", HELPCTX(no_help));
    c->generic.column = 1;
    /*
     * New folder button should not be available mid-session.
     */
    if (!midsession) {
	ssd->mkfolderbutton = ctrl_pushbutton(s, "New folder", 'f',
		HELPCTX(session_saved),
		sessionsaver_handler, P(ssd));
	ssd->mkfolderbutton->generic.column = 1;
    };
    /* Reset columns so that the buttons are alongside the list, rather
     * than alongside that edit box. */
    ctrl_columns(s, 1, 100);
    ctrl_columns(s, 2, 75, 25);
    ssd->listbox = ctrl_sessionlistbox(s, NULL, NO_SHORTCUT,
				       HELPCTX(session_saved),
				       sessionsaver_handler, P(ssd));
    ssd->listbox->generic.column = 0;
    ssd->listbox->listbox.height = 7;
    if (!midsession) {
	ssd->loadbutton = ctrl_pushbutton(s, "Load", 'l',
					  HELPCTX(session_saved),
					  sessionsaver_handler, P(ssd));
	ssd->loadbutton->generic.column = 1;
    } else {
	/* We can't offer the Load button mid-session, as it would allow the
	 * user to load and subsequently save settings they can't see. (And
	 * also change otherwise immutable settings underfoot; that probably
	 * shouldn't be a problem, but.) */
	ssd->loadbutton = NULL;
    }
    /* "Save" button is permitted mid-session. */
    ssd->savebutton = ctrl_pushbutton(s, "Save", 'v',
				      HELPCTX(session_saved),
				      sessionsaver_handler, P(ssd));
    ssd->savebutton->generic.column = 1;
    if (!midsession) {
	ssd->delbutton = ctrl_pushbutton(s, "Delete", 'd',
					 HELPCTX(session_saved),
					 sessionsaver_handler, P(ssd));
	ssd->delbutton->generic.column = 1;
    } else {
	/* Disable the Delete button mid-session too, for UI consistency. */
	ssd->delbutton = NULL;
    }
    ssd->pathview = ctrl_path(s, "Path", HELPCTX(session_saved));
    ctrl_columns(s, 1, 100);

    s = ctrl_getset(b, "Session", "otheropts", NULL);
    c = ctrl_radiobuttons(s, "Close window on exit:", 'w', 4,
			  HELPCTX(session_coe),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, close_on_exit)),
			  "Always", I(FORCE_ON),
			  "Never", I(FORCE_OFF),
			  "Only on clean exit", I(AUTO), NULL);

    /*
     * The Session/Logging panel.
     */
    ctrl_settitle(b, "Session/Logging", "Options controlling session logging");

    s = ctrl_getset(b, "Session/Logging", "main", NULL);
    /*
     * The logging buttons change depending on whether SSH packet
     * logging can sensibly be available.
     */
    {
	char *sshlogname, *sshrawlogname;
	if ((midsession && protocol == PROT_SSH) ||
	    (!midsession && have_backend(PROT_SSH))) {
	    sshlogname = "SSH packets";
	    sshrawlogname = "SSH packets and raw data";
        } else {
	    sshlogname = NULL;	       /* this will disable both buttons */
	    sshrawlogname = NULL;      /* this will just placate optimisers */
        }
	ctrl_radiobuttons(s, "Session logging:", NO_SHORTCUT, 2,
			  HELPCTX(logging_main),
			  loggingbuttons_handler,
			  I(offsetof(Config, logtype)),
			  "None", 't', I(LGTYP_NONE),
			  "Printable output", 'p', I(LGTYP_ASCII),
			  "All session output", 'l', I(LGTYP_DEBUG),
			  sshlogname, 's', I(LGTYP_PACKETS),
			  sshrawlogname, 'r', I(LGTYP_SSHRAW),
			  NULL);
    }
    ctrl_filesel(s, "Log file name:", 'f',
		 NULL, TRUE, "Select session log file name",
		 HELPCTX(logging_filename),
		 dlg_stdfilesel_handler, I(offsetof(Config, logfilename)));
    ctrl_text(s, "(Log file name can contain &Y, &M, &D for date,"
	      " &T for time, and &H for host name)",
	      HELPCTX(logging_filename));
    ctrl_radiobuttons(s, "What to do if the log file already exists:", 'e', 1,
		      HELPCTX(logging_exists),
		      dlg_stdradiobutton_handler, I(offsetof(Config,logxfovr)),
		      "Always overwrite it", I(LGXF_OVR),
		      "Always append to the end of it", I(LGXF_APN),
		      "Ask the user every time", I(LGXF_ASK), NULL);
    ctrl_checkbox(s, "Flush log file frequently", 'u',
		 HELPCTX(logging_flush),
		 dlg_stdcheckbox_handler, I(offsetof(Config,logflush)));

    if ((midsession && protocol == PROT_SSH) ||
	(!midsession && have_backend(PROT_SSH))) {
	s = ctrl_getset(b, "Session/Logging", "ssh",
			"Options specific to SSH packet logging");
	ctrl_checkbox(s, "Omit known password fields", 'k',
		      HELPCTX(logging_ssh_omit_password),
		      dlg_stdcheckbox_handler, I(offsetof(Config,logomitpass)));
	ctrl_checkbox(s, "Omit session data", 'd',
		      HELPCTX(logging_ssh_omit_data),
		      dlg_stdcheckbox_handler, I(offsetof(Config,logomitdata)));
    }

    /*
     * The Terminal panel.
     */
    ctrl_settitle(b, "Terminal", "Options controlling the terminal emulation");

    s = ctrl_getset(b, "Terminal", "general", "Set various terminal options");
    ctrl_checkbox(s, "Auto wrap mode initially on", 'w',
		  HELPCTX(terminal_autowrap),
		  dlg_stdcheckbox_handler, I(offsetof(Config,wrap_mode)));
    ctrl_checkbox(s, "DEC Origin Mode initially on", 'd',
		  HELPCTX(terminal_decom),
		  dlg_stdcheckbox_handler, I(offsetof(Config,dec_om)));
    ctrl_checkbox(s, "Implicit CR in every LF", 'r',
		  HELPCTX(terminal_lfhascr),
		  dlg_stdcheckbox_handler, I(offsetof(Config,lfhascr)));
    ctrl_checkbox(s, "Use background colour to erase screen", 'e',
		  HELPCTX(terminal_bce),
		  dlg_stdcheckbox_handler, I(offsetof(Config,bce)));
    ctrl_checkbox(s, "Enable blinking text", 'n',
		  HELPCTX(terminal_blink),
		  dlg_stdcheckbox_handler, I(offsetof(Config,blinktext)));
    ctrl_editbox(s, "Answerback to ^E:", 's', 100,
		 HELPCTX(terminal_answerback),
		 dlg_stdeditbox_handler, I(offsetof(Config,answerback)),
		 I(sizeof(((Config *)0)->answerback)));

    s = ctrl_getset(b, "Terminal", "ldisc", "Line discipline options");
    ctrl_radiobuttons(s, "Local echo:", 'l', 3,
		      HELPCTX(terminal_localecho),
		      dlg_stdradiobutton_handler,I(offsetof(Config,localecho)),
		      "Auto", I(AUTO),
		      "Force on", I(FORCE_ON),
		      "Force off", I(FORCE_OFF), NULL);
    ctrl_radiobuttons(s, "Local line editing:", 't', 3,
		      HELPCTX(terminal_localedit),
		      dlg_stdradiobutton_handler,I(offsetof(Config,localedit)),
		      "Auto", I(AUTO),
		      "Force on", I(FORCE_ON),
		      "Force off", I(FORCE_OFF), NULL);

    s = ctrl_getset(b, "Terminal", "printing", "Remote-controlled printing");
    ctrl_combobox(s, "Printer to send ANSI printer output to:", 'p', 100,
		  HELPCTX(terminal_printing),
		  printerbox_handler, P(NULL), P(NULL));

    /*
     * The Terminal/Keyboard panel.
     */
    ctrl_settitle(b, "Terminal/Keyboard",
		  "Options controlling the effects of keys");

    s = ctrl_getset(b, "Terminal/Keyboard", "mappings",
		    "Change the sequences sent by:");
    ctrl_radiobuttons(s, "The Backspace key", 'b', 2,
		      HELPCTX(keyboard_backspace),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, bksp_is_delete)),
		      "Control-H", I(0), "Control-? (127)", I(1), NULL);
    ctrl_radiobuttons(s, "The Home and End keys", 'e', 2,
		      HELPCTX(keyboard_homeend),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, rxvt_homeend)),
		      "Standard", I(0), "rxvt", I(1), NULL);
    ctrl_radiobuttons(s, "The Function keys and keypad", 'f', 3,
		      HELPCTX(keyboard_funkeys),
		      funky_handler,
		      I(offsetof(Config, funky_type)),
		      "ESC[n~", I(0), "Linux", I(1), "Xterm R6", I(2),
		      "VT400", I(3), "VT100+", I(4), "SCO", I(5),
		      "AT&T 513", I(6), 
/*
		      "AT&T 4410", I(7),
*/
		      "Sun Xterm", I(8),
		      NULL);

    s = ctrl_getset(b, "Terminal/Keyboard", "appkeypad",
		    "Application keypad settings:");
    ctrl_radiobuttons(s, "Initial state of cursor keys:", 'r', 3,
		      HELPCTX(keyboard_appcursor),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, app_cursor)),
		      "Normal", I(0), "Application", I(1), NULL);
    ctrl_radiobuttons(s, "Initial state of numeric keypad:", 'n', 3,
		      HELPCTX(keyboard_appkeypad),
		      numeric_keypad_handler, P(NULL),
		      "Normal", I(0), "Application", I(1), "NetHack", I(2),
		      NULL);

    /*
     * The Terminal/Bell panel.
     */
    ctrl_settitle(b, "Terminal/Bell",
		  "Options controlling the terminal bell");

    s = ctrl_getset(b, "Terminal/Bell", "style", "Set the style of bell");
    ctrl_radiobuttons(s, "Action to happen when a bell occurs:", 'b', 1,
		      HELPCTX(bell_style),
		      dlg_stdradiobutton_handler, I(offsetof(Config, beep)),
		      "None (bell disabled)", I(BELL_DISABLED),
		      "Make default system alert sound", I(BELL_DEFAULT),
		      "Visual bell (flash window)", I(BELL_VISUAL), NULL);

    s = ctrl_getset(b, "Terminal/Bell", "overload",
		    "Control the bell overload behaviour");
    ctrl_checkbox(s, "Bell is temporarily disabled when over-used", 'd',
		  HELPCTX(bell_overload),
		  dlg_stdcheckbox_handler, I(offsetof(Config, bellovl)));
    ctrl_editbox(s, "Over-use means this many bells...", 'm', 20,
		 HELPCTX(bell_overload),
		 dlg_stdeditbox_handler, I(offsetof(Config,bellovl_n)), I(-1));
    ctrl_editbox(s, "... in this many seconds", 't', 20,
		 HELPCTX(bell_overload),
		 dlg_stdeditbox_handler, I(offsetof(Config,bellovl_t)),
		 I(-TICKSPERSEC));
    ctrl_text(s, "The bell is re-enabled after a few seconds of silence.",
	      HELPCTX(bell_overload));
    ctrl_editbox(s, "Seconds of silence required", 's', 20,
		 HELPCTX(bell_overload),
		 dlg_stdeditbox_handler, I(offsetof(Config,bellovl_s)),
		 I(-TICKSPERSEC));

    /*
     * The Terminal/Features panel.
     */
    ctrl_settitle(b, "Terminal/Features",
		  "Enabling and disabling advanced terminal features");

    s = ctrl_getset(b, "Terminal/Features", "main", NULL);
    ctrl_checkbox(s, "Disable application cursor keys mode", 'u',
		  HELPCTX(features_application),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_applic_c)));
    ctrl_checkbox(s, "Disable application keypad mode", 'k',
		  HELPCTX(features_application),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_applic_k)));
    ctrl_checkbox(s, "Disable xterm-style mouse reporting", 'x',
		  HELPCTX(features_mouse),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_mouse_rep)));
    ctrl_checkbox(s, "Disable remote-controlled terminal resizing", 's',
		  HELPCTX(features_resize),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,no_remote_resize)));
    ctrl_checkbox(s, "Disable switching to alternate terminal screen", 'w',
		  HELPCTX(features_altscreen),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_alt_screen)));
    ctrl_checkbox(s, "Disable remote-controlled window title changing", 't',
		  HELPCTX(features_retitle),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,no_remote_wintitle)));
    ctrl_radiobuttons(s, "Response to remote title query (SECURITY):", 'q', 3,
		      HELPCTX(features_qtitle),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config,remote_qtitle_action)),
		      "None", I(TITLE_NONE),
		      "Empty string", I(TITLE_EMPTY),
		      "Window title", I(TITLE_REAL), NULL);
    ctrl_checkbox(s, "Disable destructive backspace on server sending ^?",'b',
		  HELPCTX(features_dbackspace),
		  dlg_stdcheckbox_handler, I(offsetof(Config,no_dbackspace)));
    ctrl_checkbox(s, "Disable remote-controlled character set configuration",
		  'r', HELPCTX(features_charset), dlg_stdcheckbox_handler,
		  I(offsetof(Config,no_remote_charset)));
    ctrl_checkbox(s, "Disable Arabic text shaping",
		  'l', HELPCTX(features_arabicshaping), dlg_stdcheckbox_handler,
		  I(offsetof(Config, arabicshaping)));
    ctrl_checkbox(s, "Disable bidirectional text display",
		  'd', HELPCTX(features_bidi), dlg_stdcheckbox_handler,
		  I(offsetof(Config, bidi)));
    ctrl_checkbox(s, "Disable feature key buttons", NO_SHORTCUT,
		  HELPCTX(features_bottombuttons), dlg_stdcheckbox_handler,
		  I(offsetof(Config, bottom_buttons)));

    /*
     * The Window panel.
     */
    str = dupprintf("Options controlling %s's window", appname);
    ctrl_settitle(b, "Window", str);
    sfree(str);

    s = ctrl_getset(b, "Window", "size", "Set the size of the window");
    ctrl_columns(s, 2, 50, 50);
    c = ctrl_editbox(s, "Columns", 'm', 100,
		     HELPCTX(window_size),
		     dlg_stdeditbox_handler, I(offsetof(Config,width)), I(-1));
    c->generic.column = 0;
    c = ctrl_editbox(s, "Rows", 'r', 100,
		     HELPCTX(window_size),
		     dlg_stdeditbox_handler, I(offsetof(Config,height)),I(-1));
    c->generic.column = 1;
    ctrl_columns(s, 1, 100);

    s = ctrl_getset(b, "Window", "scrollback",
		    "Control the scrollback in the window");
    ctrl_editbox(s, "Lines of scrollback", 's', 50,
		 HELPCTX(window_scrollback),
		 dlg_stdeditbox_handler, I(offsetof(Config,savelines)), I(-1));
    ctrl_checkbox(s, "Display scrollbar", 'd',
		  HELPCTX(window_scrollback),
		  dlg_stdcheckbox_handler, I(offsetof(Config,scrollbar)));
    ctrl_checkbox(s, "Reset scrollback on keypress", 'k',
		  HELPCTX(window_scrollback),
		  dlg_stdcheckbox_handler, I(offsetof(Config,scroll_on_key)));
    ctrl_checkbox(s, "Reset scrollback on display activity", 'p',
		  HELPCTX(window_scrollback),
		  dlg_stdcheckbox_handler, I(offsetof(Config,scroll_on_disp)));
    ctrl_checkbox(s, "Push erased text into scrollback", 'e',
		  HELPCTX(window_erased),
		  dlg_stdcheckbox_handler,
		  I(offsetof(Config,erase_to_scrollback)));

    /*
     * The Window/Appearance panel.
     */
    str = dupprintf("Configure the appearance of %s's window", appname);
    ctrl_settitle(b, "Window/Appearance", str);
    sfree(str);

    s = ctrl_getset(b, "Window/Appearance", "cursor",
		    "Adjust the use of the cursor");
    ctrl_radiobuttons(s, "Cursor appearance:", NO_SHORTCUT, 3,
		      HELPCTX(appearance_cursor),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, cursor_type)),
		      "Block", 'l', I(0),
		      "Underline", 'u', I(1),
		      "Vertical line", 'v', I(2), NULL);
    ctrl_checkbox(s, "Cursor blinks", 'b',
		  HELPCTX(appearance_cursor),
		  dlg_stdcheckbox_handler, I(offsetof(Config,blink_cur)));

    s = ctrl_getset(b, "Window/Appearance", "font",
		    "Font settings");
    ctrl_fontsel(s, "Font used in the terminal window", 'n',
		 HELPCTX(appearance_font),
		 dlg_stdfontsel_handler, I(offsetof(Config, font)));

    s = ctrl_getset(b, "Window/Appearance", "mouse",
		    "Adjust the use of the mouse pointer");
    ctrl_checkbox(s, "Hide mouse pointer when typing in window", 'p',
		  HELPCTX(appearance_hidemouse),
		  dlg_stdcheckbox_handler, I(offsetof(Config,hide_mouseptr)));

    s = ctrl_getset(b, "Window/Appearance", "border",
		    "Adjust the window border");
    ctrl_editbox(s, "Gap between text and window edge:", 'e', 20,
		 HELPCTX(appearance_border),
		 dlg_stdeditbox_handler,
		 I(offsetof(Config,window_border)), I(-1));

    /*
     * The Window/Behaviour panel.
     */
    str = dupprintf("Configure the behaviour of %s's window", appname);
    ctrl_settitle(b, "Window/Behaviour", str);
    sfree(str);

    s = ctrl_getset(b, "Window/Behaviour", "title",
		    "Adjust the behaviour of the window title");
    ctrl_editbox(s, "Window title:", 't', 100,
		 HELPCTX(appearance_title),
		 dlg_stdeditbox_handler, I(offsetof(Config,wintitle)),
		 I(sizeof(((Config *)0)->wintitle)));
    ctrl_columns(s, 3, 40, 20, 40);
//      c = ctrl_text(s, " ", HELPCTX(appearance_title));
//      c->generic.column = 0;
    c = ctrl_text(s, "Window icon:", HELPCTX(appearance_title));
    c->generic.column = 0;
    c = ctrl_icon(s, HELPCTX(appearance_title),
		  I(offsetof(Config, win_icon)));
    c->generic.column = 1;
    c = ctrl_pushbutton(s, "Change...", 'n', HELPCTX(appearance_title),
			window_icon_handler, P(c));
    c->generic.column = 2;
    ctrl_columns(s, 1, 100);
    ctrl_checkbox(s, "Separate window and icon titles", 'i',
		  HELPCTX(appearance_title),
		  dlg_stdcheckbox_handler,
		  I(CHECKBOX_INVERT | offsetof(Config,win_name_always)));

    s = ctrl_getset(b, "Window/Behaviour", "main", NULL);
    ctrl_checkbox(s, "Warn before closing window", 'w',
		  HELPCTX(behaviour_closewarn),
		  dlg_stdcheckbox_handler, I(offsetof(Config,warn_on_close)));

    /*
     * The Window/Translation panel.
     */
    ctrl_settitle(b, "Window/Translation",
		  "Options controlling character set translation");

    s = ctrl_getset(b, "Window/Translation", "trans",
		    "Character set translation on received data");
    ctrl_combobox(s, "Received data assumed to be in which character set:",
		  'r', 100, HELPCTX(translation_codepage),
		  codepage_handler, P(NULL), P(NULL));

    s = ctrl_getset(b, "Window/Translation", "tweaks", NULL);
    ctrl_checkbox(s, "Treat CJK ambiguous characters as wide", 'w',
		  HELPCTX(translation_cjk_ambig_wide),
		  dlg_stdcheckbox_handler, I(offsetof(Config,cjk_ambig_wide)));

    str = dupprintf("Adjust how %s handles line drawing characters", appname);
    s = ctrl_getset(b, "Window/Translation", "linedraw", str);
    sfree(str);
    ctrl_radiobuttons(s, "Handling of line drawing characters:", NO_SHORTCUT,1,
		      HELPCTX(translation_linedraw),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, vtmode)),
		      "Use Unicode line drawing code points",'u',I(VT_UNICODE),
		      "Poor man's line drawing (+, - and |)",'p',I(VT_POORMAN),
		      NULL);
    ctrl_checkbox(s, "Copy and paste line drawing characters as lqqqk",'d',
		  HELPCTX(selection_linedraw),
		  dlg_stdcheckbox_handler, I(offsetof(Config,rawcnp)));

    /*
     * The Window/Selection panel.
     */
    ctrl_settitle(b, "Window/Selection", "Options controlling copy and paste");
	
    s = ctrl_getset(b, "Window/Selection", "mouse",
		    "Control use of mouse");
    ctrl_checkbox(s, "Shift overrides application's use of mouse", 'p',
		  HELPCTX(selection_shiftdrag),
		  dlg_stdcheckbox_handler, I(offsetof(Config,mouse_override)));
    ctrl_radiobuttons(s,
		      "Default selection mode (Alt+drag does the other one):",
		      NO_SHORTCUT, 2,
		      HELPCTX(selection_rect),
		      dlg_stdradiobutton_handler,
		      I(offsetof(Config, rect_select)),
		      "Normal", 'n', I(0),
		      "Rectangular block", 'r', I(1), NULL);

    s = ctrl_getset(b, "Window/Selection", "charclass",
		    "Control the select-one-word-at-a-time mode");
    ccd = (struct charclass_data *)
	ctrl_alloc(b, sizeof(struct charclass_data));
    ccd->listbox = ctrl_listbox(s, "Character classes:", 'e',
				HELPCTX(selection_charclasses),
				charclass_handler, P(ccd));
    ccd->listbox->listbox.multisel = 1;
    ccd->listbox->listbox.ncols = 4;
    ccd->listbox->listbox.percentages = snewn(4, int);
    ccd->listbox->listbox.percentages[0] = 15;
    ccd->listbox->listbox.percentages[1] = 25;
    ccd->listbox->listbox.percentages[2] = 20;
    ccd->listbox->listbox.percentages[3] = 40;
    ctrl_columns(s, 2, 67, 33);
    ccd->editbox = ctrl_editbox(s, "Set to class", 't', 50,
				HELPCTX(selection_charclasses),
				charclass_handler, P(ccd), P(NULL));
    ccd->editbox->generic.column = 0;
    ccd->button = ctrl_pushbutton(s, "Set", 's',
				  HELPCTX(selection_charclasses),
				  charclass_handler, P(ccd));
    ccd->button->generic.column = 1;
    ctrl_columns(s, 1, 100);

    /*
     * The Window/Colours panel.
     */
    ctrl_settitle(b, "Window/Colours", "Options controlling use of colours");

    s = ctrl_getset(b, "Window/Colours", "general",
		    "General options for colour usage");
    ctrl_checkbox(s, "Allow terminal to specify ANSI colours", 'i',
		  HELPCTX(colours_ansi),
		  dlg_stdcheckbox_handler, I(offsetof(Config,ansi_colour)));
    ctrl_checkbox(s, "Allow terminal to use xterm 256-colour mode", '2',
		  HELPCTX(colours_xterm256), dlg_stdcheckbox_handler,
		  I(offsetof(Config,xterm_256_colour)));
    cd = (struct colour_data *) ctrl_alloc(b, sizeof(struct colour_data));
    cd->bold_checkbox =
	ctrl_checkbox(s, "Bolded text is a different colour", 'b',
		      HELPCTX(colours_bold), colour_handler, P(cd));
    cd->underline_checkbox =
	ctrl_checkbox(s, "Underlined text is a different colour", 'd',
		      HELPCTX(colours_underline), colour_handler, P(cd));
    cd->selected_checkbox =
	ctrl_checkbox(s, "Selected text is a different colour", 't',
		      HELPCTX(colours_selected), colour_handler, P(cd));

    str = dupprintf("Adjust the precise colours %s displays", appname);
    s = ctrl_getset(b, "Window/Colours", "adjust", str);
    sfree(str);
    ctrl_text(s, "Select a colour from the list, and then click the"
	      " Modify button to change its appearance.",
	      HELPCTX(colours_config));
    ctrl_columns(s, 2, 67, 33);
    cd->listbox = ctrl_listbox(s, "Select a colour to adjust:", 'u',
			       HELPCTX(colours_config), colour_handler, P(cd));
    cd->listbox->generic.column = 0;
    cd->listbox->listbox.height = 7;
    c = ctrl_text(s, "RGB value:", HELPCTX(colours_config));
    c->generic.column = 1;
    cd->redit = ctrl_editbox(s, "Red", 'r', 50, HELPCTX(colours_config),
			     colour_handler, P(cd), P(NULL));
    cd->redit->generic.column = 1;
    cd->gedit = ctrl_editbox(s, "Green", 'n', 50, HELPCTX(colours_config),
			     colour_handler, P(cd), P(NULL));
    cd->gedit->generic.column = 1;
    cd->bedit = ctrl_editbox(s, "Blue", 'e', 50, HELPCTX(colours_config),
			     colour_handler, P(cd), P(NULL));
    cd->bedit->generic.column = 1;
    cd->button = ctrl_pushbutton(s, "Modify", 'm', HELPCTX(colours_config),
				 colour_handler, P(cd));
    cd->button->generic.column = 1;
    ctrl_columns(s, 1, 100);

    /*
     * The Connection panel. This doesn't show up if we're in a
     * non-network utility such as pterm. We tell this by being
     * passed a protocol < 0.
     */
    if (protocol >= 0) {
	ctrl_settitle(b, "Connection", "Options controlling the connection");

	if (!midsession && protocol != PROT_SERIAL) {
	    s = ctrl_getset(b, "Connection", "keepalive",
			    "Sending of null packets to keep session active");
	    ctrl_editbox(s, "Seconds between keepalives (0 to turn off)",
			 'k', 20, HELPCTX(connection_keepalive),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config, ping_interval)), I(-1));
	};

	if (!midsession) {
	    s = ctrl_getset(b, "Connection", "tcp",
			    "Low-level TCP connection options");
	    ctrl_checkbox(s, "Disable Nagle's algorithm (TCP_NODELAY option)",
			  'n', HELPCTX(connection_nodelay),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,tcp_nodelay)));
	    ctrl_checkbox(s, "Enable TCP keepalives (SO_KEEPALIVE option)",
			  'p', HELPCTX(connection_tcpkeepalive),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,tcp_keepalives)));
#ifndef NO_IPV6
	    s = ctrl_getset(b, "Connection", "ipversion",
			    "Internet protocol version");
	    ctrl_radiobuttons(s, NULL, NO_SHORTCUT, 3,
			      HELPCTX(connection_ipversion),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, addressfamily)),
			  "Auto", 'u', I(ADDRTYPE_UNSPEC),
			  "IPv4", '4', I(ADDRTYPE_IPV4),
			  "IPv6", '6', I(ADDRTYPE_IPV6),
			  NULL);
#endif
	}

	if (!midsession) {
	    s = ctrl_getset(b, "Connection", "script",
			    "Login script options");
	    ctrl_columns(s, 2, 20, 80);
	    c = ctrl_checkbox(s, "Enable", 'e',
			      HELPCTX(connection_script),
			      dlg_stdcheckbox_handler,
			      I(offsetof(Config, secondary)));
	    c->generic.column = 0;
	    c = ctrl_editbox(s, "Script:", 'i', 84,
			     HELPCTX(connection_script),
			     dlg_stdeditbox_handler,
			     I(offsetof(Config, secondaryscript)),
			     I(sizeof(((Config *) 0)->secondaryscript)));
	    c->generic.column = 1;
	};

	/*
	 * A sub-panel Connection/Data, containing options that
	 * decide on data to send to the server.
	 */
	if (!midsession) {
	    ctrl_settitle(b, "Connection/Data", "Data to send to the server");

	    s = ctrl_getset(b, "Connection/Data", "login",
			    "Login details");
	    ctrl_editbox(s, "Auto-login username", 'u', 50,
			 HELPCTX(connection_username),
			 dlg_stdeditbox_handler, I(offsetof(Config,username)),
			 I(sizeof(((Config *)0)->username)));

	    s = ctrl_getset(b, "Connection/Data", "term",
			    "Terminal details");
	    ctrl_editbox(s, "Terminal-type string", 't', 50,
			 HELPCTX(connection_termtype),
			 dlg_stdeditbox_handler, I(offsetof(Config,termtype)),
			 I(sizeof(((Config *)0)->termtype)));
	    ctrl_editbox(s, "Terminal speeds", 's', 50,
			 HELPCTX(connection_termspeed),
			 dlg_stdeditbox_handler, I(offsetof(Config,termspeed)),
			 I(sizeof(((Config *)0)->termspeed)));

	    s = ctrl_getset(b, "Connection/Data", "env",
			    "Environment variables");
	    ctrl_columns(s, 2, 80, 20);
	    ed = (struct environ_data *)
		ctrl_alloc(b, sizeof(struct environ_data));
	    ed->varbox = ctrl_editbox(s, "Variable", 'v', 60,
				      HELPCTX(telnet_environ),
				      environ_handler, P(ed), P(NULL));
	    ed->varbox->generic.column = 0;
	    ed->valbox = ctrl_editbox(s, "Value", 'l', 60,
				      HELPCTX(telnet_environ),
				      environ_handler, P(ed), P(NULL));
	    ed->valbox->generic.column = 0;
	    ed->addbutton = ctrl_pushbutton(s, "Add", 'd',
					    HELPCTX(telnet_environ),
					    environ_handler, P(ed));
	    ed->addbutton->generic.column = 1;
	    ed->rembutton = ctrl_pushbutton(s, "Remove", 'r',
					    HELPCTX(telnet_environ),
					    environ_handler, P(ed));
	    ed->rembutton->generic.column = 1;
	    ctrl_columns(s, 1, 100);
	    ed->listbox = ctrl_listbox(s, NULL, NO_SHORTCUT,
				       HELPCTX(telnet_environ),
				       environ_handler, P(ed));
	    ed->listbox->listbox.height = 3;
	    ed->listbox->listbox.ncols = 2;
	    ed->listbox->listbox.percentages = snewn(2, int);
	    ed->listbox->listbox.percentages[0] = 30;
	    ed->listbox->listbox.percentages[1] = 70;
	}

    }

    if (!midsession) {
	/*
	 * The Connection/Proxy panel.
	 */
	ctrl_settitle(b, "Connection/Proxy",
		      "Options controlling proxy usage");

	s = ctrl_getset(b, "Connection/Proxy", "basics", NULL);
	ctrl_radiobuttons(s, "Proxy type:", 't', 3,
			  HELPCTX(proxy_type),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, proxy_type)),
			  "None", I(PROXY_NONE),
			  "SOCKS 4", I(PROXY_SOCKS4),
			  "SOCKS 5", I(PROXY_SOCKS5),
			  "HTTP", I(PROXY_HTTP),
			  "Telnet", I(PROXY_TELNET),
			  NULL);
	ctrl_columns(s, 2, 80, 20);
	c = ctrl_editbox(s, "Proxy hostname", 'y', 100,
			 HELPCTX(proxy_main),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,proxy_host)),
			 I(sizeof(((Config *)0)->proxy_host)));
	c->generic.column = 0;
	c = ctrl_editbox(s, "Port", 'p', 100,
			 HELPCTX(proxy_main),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,proxy_port)),
			 I(-1));
	c->generic.column = 1;
	ctrl_columns(s, 1, 100);
	ctrl_editbox(s, "Exclude Hosts/IPs", 'e', 100,
		     HELPCTX(proxy_exclude),
		     dlg_stdeditbox_handler,
		     I(offsetof(Config,proxy_exclude_list)),
		     I(sizeof(((Config *)0)->proxy_exclude_list)));
	ctrl_checkbox(s, "Consider proxying local host connections", 'x',
		      HELPCTX(proxy_exclude),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,even_proxy_localhost)));
	ctrl_radiobuttons(s, "Do DNS name lookup at proxy end:", 'd', 3,
			  HELPCTX(proxy_dns),
			  dlg_stdradiobutton_handler,
			  I(offsetof(Config, proxy_dns)),
			  "No", I(FORCE_OFF),
			  "Auto", I(AUTO),
			  "Yes", I(FORCE_ON), NULL);
	ctrl_editbox(s, "Username", 'u', 60,
		     HELPCTX(proxy_auth),
		     dlg_stdeditbox_handler,
		     I(offsetof(Config,proxy_username)),
		     I(sizeof(((Config *)0)->proxy_username)));
	c = ctrl_editbox(s, "Password", 'w', 60,
			 HELPCTX(proxy_auth),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,proxy_password)),
			 I(sizeof(((Config *)0)->proxy_password)));
	c->editbox.password = 1;
	ctrl_editbox(s, "Telnet command", 'm', 100,
		     HELPCTX(proxy_command),
		     dlg_stdeditbox_handler,
		     I(offsetof(Config,proxy_telnet_command)),
		     I(sizeof(((Config *)0)->proxy_telnet_command)));
    }

    if (!midsession || protocol == PROT_SERIAL) {
	struct serialpanel_data *spd;
	struct dialingoptions_data *dod;

	ctrl_settitle(b, "Connection/Serial",
		      "Options controlling Serial connections");
	s = ctrl_getset(b, "Connection/Serial", "settings",
			"Serial port settings");
	spd = (struct serialpanel_data *)
	    ctrl_alloc(b, sizeof(struct serialpanel_data));
	spd->baud = ctrl_droplist(s, "Baud rate:", 'b', 40,
				  HELPCTX(serial_portsettings),
				  serialpanel_handler, P(spd));
	spd->datab = ctrl_droplist(s, "Data bits:", 'd', 40,
				   HELPCTX(serial_portsettings),
				   serialpanel_handler, P(spd));
	spd->parity = ctrl_droplist(s, "Parity:", 'p', 40,
				    HELPCTX(serial_portsettings),
				    serialpanel_handler, P(spd));
	spd->stopb = ctrl_droplist(s, "Stop bits:", 's', 40,
				   HELPCTX(serial_portsettings),
				   serialpanel_handler, P(spd));
	spd->flowctrl = ctrl_droplist(s, "Flow control:", 'f', 40,
				      HELPCTX(serial_portsettings),
				      serialpanel_handler, P(spd));

	s = ctrl_getset(b, "Connection/Serial", "dialing",
			"Dialing options");

	dod = ctrl_alloc(b, sizeof(struct dialingoptions_data));

	dod->winloc =
	    ctrl_checkbox(s, "Use Windows Dialing Rules", 'w',
			  HELPCTX(serial_dialing), dialingoptions_handler,
			  P(dod));
	dod->mode =
	    ctrl_radiobuttons(s, "Dialing Mode", 'm', 2, 
			      HELPCTX(serial_dialing),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, ser_dialmode)),
			      "Tone", I(0), "Pulse", I(1), NULL);
	dod->prefix =
	    ctrl_editbox(s, "Prefix to get access to outside line", 'x',
			 30, HELPCTX(serial_dialing), dlg_stdeditbox_handler,
			 I(offsetof(Config, ser_dialprefix)),
			 I(sizeof(((Config *) 0)->ser_dialprefix)));

	ctrl_editbox(s, "Connect timeout (seconds)", 't', 30,
		     HELPCTX(serial_dialing), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_dialtimeout)), I(-1));
	ctrl_columns(s, 2, 50, 50);
	c = ctrl_editbox(s, "Redial attempts", 'r', 30,
			 HELPCTX(serial_dialing), dlg_stdeditbox_handler,
			 I(offsetof(Config, ser_redials)), I(-1));
	c->generic.column = 0;
	c = ctrl_editbox(s, "Redial delay (sec)", 'e', 30,
			 HELPCTX(serial_dialing), dlg_stdeditbox_handler,
			 I(offsetof(Config, ser_redialtimeout)), I(-1));
	c->generic.column = 1;

	ctrl_columns(s, 1, 100);
	ctrl_checkbox(s, "Print modem input/output when dialing", 'i',
		      HELPCTX(serial_dialing), dlg_stdcheckbox_handler,
		      I(offsetof(Config, ser_print_when_dialing)));

	ctrl_settitle(b, "Connection/Serial/Modem",
		      "Options related to modem dialing");
	s = ctrl_getset(b, "Connection/Serial/Modem", "modemcommands",
			"Modem command strings");
	ctrl_editbox(s, "Initialization string", 'i', 50,
		     HELPCTX(serial_modemcommands), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_init)),
		     I(sizeof(((Config *) 0)->ser_modem_init)));
	ctrl_editbox(s, "Dial in Tone mode", 't', 50,
		     HELPCTX(serial_modemcommands), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_dial_tone)),
		     I(sizeof(((Config *) 0)->ser_modem_dial_tone)));
	ctrl_editbox(s, "Dial in Pulse mode", 'p', 50,
		     HELPCTX(serial_modemcommands), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_dial_pulse)),
		     I(sizeof(((Config *) 0)->ser_modem_dial_pulse)));
	ctrl_editbox(s, "Hang up", 'u', 50,
		     HELPCTX(serial_modemcommands), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_hangup)),
		     I(sizeof(((Config *) 0)->ser_modem_hangup)));
	ctrl_editbox(s, "Return to Command mode", 'm', 50,
		     HELPCTX(serial_modemcommands), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_commandmode)),
		     I(sizeof(((Config *) 0)->ser_modem_commandmode)));
	s = ctrl_getset(b, "Connection/Serial/Modem", "modemresponses",
			"Modem response strings");
	ctrl_editbox(s, "OK response", 'k', 50,
		     HELPCTX(serial_modemresponses), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_ok)),
		     I(sizeof(((Config *) 0)->ser_modem_ok)));
	ctrl_editbox(s, "ERROR response", 'e', 50,
		     HELPCTX(serial_modemresponses), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_error)),
		     I(sizeof(((Config *) 0)->ser_modem_error)));
	ctrl_editbox(s, "BUSY response", 'b', 50,
		     HELPCTX(serial_modemresponses), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_busy)),
		     I(sizeof(((Config *) 0)->ser_modem_busy)));
	ctrl_editbox(s, "NO DIALTONE response", 'd', 50,
		     HELPCTX(serial_modemresponses), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_no_dialtone)),
		     I(sizeof(((Config *) 0)->ser_modem_no_dialtone)));
	ctrl_editbox(s, "NO CARRIER response", 'r', 50,
		     HELPCTX(serial_modemresponses), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_no_carrier)),
		     I(sizeof(((Config *) 0)->ser_modem_no_carrier)));
	ctrl_editbox(s, "CONNECT response", 'n', 50,
		     HELPCTX(serial_modemresponses), dlg_stdeditbox_handler,
		     I(offsetof(Config, ser_modem_connect)),
		     I(sizeof(((Config *) 0)->ser_modem_connect)));
    };

    /*
     * The Telnet panel exists in the base config box, and in a
     * mid-session reconfig box _if_ we're using Telnet.
     */
    if (!midsession || protocol == PROT_TELNET) {
	/*
	 * The Connection/Telnet panel.
	 */
	ctrl_settitle(b, "Connection/Telnet",
		      "Options controlling Telnet connections");

	s = ctrl_getset(b, "Connection/Telnet", "protocol",
			"Telnet protocol adjustments");

	if (!midsession) {
	    ctrl_radiobuttons(s, "Handling of OLD_ENVIRON ambiguity:",
			      NO_SHORTCUT, 2,
			      HELPCTX(telnet_oldenviron),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, rfc_environ)),
			      "BSD (commonplace)", 'b', I(0),
			      "RFC 1408 (unusual)", 'f', I(1), NULL);
	    ctrl_radiobuttons(s, "Telnet negotiation mode:", 't', 2,
			      HELPCTX(telnet_passive),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, passive_telnet)),
			      "Passive", I(1), "Active", I(0), NULL);
	}
	ctrl_checkbox(s, "Keyboard sends Telnet special commands", 'k',
		      HELPCTX(telnet_specialkeys),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,telnet_keyboard)));
	ctrl_checkbox(s, "Return key sends Telnet New Line instead of ^M",
		      'm', HELPCTX(telnet_newline),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,telnet_newline)));
    }

    if (!midsession) {

	/*
	 * The Connection/Rlogin panel.
	 */
	ctrl_settitle(b, "Connection/Rlogin",
		      "Options controlling Rlogin connections");

	s = ctrl_getset(b, "Connection/Rlogin", "data",
			"Data to send to the server");
	ctrl_editbox(s, "Local username:", 'l', 50,
		     HELPCTX(rlogin_localuser),
		     dlg_stdeditbox_handler, I(offsetof(Config,localusername)),
		     I(sizeof(((Config *)0)->localusername)));

    }

    /*
     * All the SSH stuff is omitted in PuTTYtel, or in a reconfig
     * when we're not doing SSH.
     */

    if (have_backend(PROT_SSH) && (!midsession || protocol == PROT_SSH)) {

	/*
	 * The Connection/SSH panel.
	 */
	ctrl_settitle(b, "Connection/SSH",
		      "Options controlling SSH connections");

	if (midsession && protcfginfo == 1) {
	    s = ctrl_getset(b, "Connection/SSH", "disclaimer", NULL);
	    ctrl_text(s, "Nothing on this panel may be reconfigured in mid-"
		      "session; it is only here so that sub-panels of it can "
		      "exist without looking strange.", HELPCTX(no_help));
	}

	if (!midsession) {

	    s = ctrl_getset(b, "Connection/SSH", "data",
			    "Data to send to the server");
	    ctrl_editbox(s, "Remote command:", 'r', 100,
			 HELPCTX(ssh_command),
			 dlg_stdeditbox_handler, I(offsetof(Config,remote_cmd)),
			 I(sizeof(((Config *)0)->remote_cmd)));

	    s = ctrl_getset(b, "Connection/SSH", "protocol", "Protocol options");
	    ctrl_checkbox(s, "Don't start a shell or command at all", 'n',
			  HELPCTX(ssh_noshell),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,ssh_no_shell)));
	}

	if (!midsession || protcfginfo != 1) {
	    s = ctrl_getset(b, "Connection/SSH", "protocol", "Protocol options");

	    ctrl_checkbox(s, "Enable compression", 'e',
			  HELPCTX(ssh_compress),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,compression)));
	}

	if (!midsession) {
	    s = ctrl_getset(b, "Connection/SSH", "protocol", "Protocol options");

	    ctrl_radiobuttons(s, "Preferred SSH protocol version:", NO_SHORTCUT, 4,
			      HELPCTX(ssh_protocol),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, sshprot)),
			      "1 only", 'l', I(0),
			      "1", '1', I(1),
			      "2", '2', I(2),
			      "2 only", 'y', I(3), NULL);
	}

	if (!midsession || protcfginfo != 1) {
	    s = ctrl_getset(b, "Connection/SSH", "encryption", "Encryption options");
	    c = ctrl_draglist(s, "Encryption cipher selection policy:", 's',
			      HELPCTX(ssh_ciphers),
			      cipherlist_handler, P(NULL));
	    c->listbox.height = 6;

	    ctrl_checkbox(s, "Enable legacy use of single-DES in SSH-2", 'i',
			  HELPCTX(ssh_ciphers),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,ssh2_des_cbc)));
	}

	/*
	 * The Connection/SSH/Kex panel. (Owing to repeat key
	 * exchange, this is all meaningful in mid-session _if_
	 * we're using SSH-2 or haven't decided yet.)
	 */
	if (protcfginfo != 1) {
	    ctrl_settitle(b, "Connection/SSH/Kex",
			  "Options controlling SSH key exchange");

	    s = ctrl_getset(b, "Connection/SSH/Kex", "main",
			    "Key exchange algorithm options");
	    c = ctrl_draglist(s, "Algorithm selection policy:", 's',
			      HELPCTX(ssh_kexlist),
			      kexlist_handler, P(NULL));
	    c->listbox.height = 5;

	    s = ctrl_getset(b, "Connection/SSH/Kex", "repeat",
			    "Options controlling key re-exchange");

	    ctrl_editbox(s, "Max minutes before rekey (0 for no limit)", 't', 20,
			 HELPCTX(ssh_kex_repeat),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,ssh_rekey_time)),
			 I(-1));
	    ctrl_editbox(s, "Max data before rekey (0 for no limit)", 'x', 20,
			 HELPCTX(ssh_kex_repeat),
			 dlg_stdeditbox_handler,
			 I(offsetof(Config,ssh_rekey_data)),
			 I(16));
	    ctrl_text(s, "(Use 1M for 1 megabyte, 1G for 1 gigabyte etc)",
		      HELPCTX(ssh_kex_repeat));
	}

	if (!midsession) {

	    /*
	     * The Connection/SSH/Auth panel.
	     */
	    ctrl_settitle(b, "Connection/SSH/Auth",
			  "Options controlling SSH authentication");

	    s = ctrl_getset(b, "Connection/SSH/Auth", "main", NULL);
	    ctrl_checkbox(s, "Bypass authentication entirely (SSH-2 only)", 'b',
			  HELPCTX(ssh_auth_bypass),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,ssh_no_userauth)));

	    s = ctrl_getset(b, "Connection/SSH/Auth", "methods",
			    "Authentication methods");
	    ctrl_checkbox(s, "Attempt authentication using Pageant", 'p',
			  HELPCTX(ssh_auth_pageant),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,tryagent)));
	    ctrl_checkbox(s, "Attempt TIS or CryptoCard auth (SSH-1)", 'm',
			  HELPCTX(ssh_auth_tis),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,try_tis_auth)));
	    ctrl_checkbox(s, "Attempt \"keyboard-interactive\" auth (SSH-2)",
			  'i', HELPCTX(ssh_auth_ki),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,try_ki_auth)));

	    s = ctrl_getset(b, "Connection/SSH/Auth", "params",
			    "Authentication parameters");
	    ctrl_checkbox(s, "Allow agent forwarding", 'f',
			  HELPCTX(ssh_auth_agentfwd),
			  dlg_stdcheckbox_handler, I(offsetof(Config,agentfwd)));
	    ctrl_checkbox(s, "Allow attempted changes of username in SSH-2", 'u',
			  HELPCTX(ssh_auth_changeuser),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,change_username)));
	    ctrl_filesel(s, "Private key file for authentication:", 'k',
			 FILTER_KEY_FILES, FALSE, "Select private key file",
			 HELPCTX(ssh_auth_privkey),
			 dlg_stdfilesel_handler, I(offsetof(Config, keyfile)));
	}

	if (!midsession) {
	    /*
	     * The Connection/SSH/TTY panel.
	     */
	    ctrl_settitle(b, "Connection/SSH/TTY", "Remote terminal settings");

	    s = ctrl_getset(b, "Connection/SSH/TTY", "sshtty", NULL);
	    ctrl_checkbox(s, "Don't allocate a pseudo-terminal", 'p',
			  HELPCTX(ssh_nopty),
			  dlg_stdcheckbox_handler,
			  I(offsetof(Config,nopty)));

	    s = ctrl_getset(b, "Connection/SSH/TTY", "ttymodes",
			    "Terminal modes");
	    td = (struct ttymodes_data *)
		ctrl_alloc(b, sizeof(struct ttymodes_data));
	    ctrl_columns(s, 2, 75, 25);
	    c = ctrl_text(s, "Terminal modes to send:", HELPCTX(ssh_ttymodes));
	    c->generic.column = 0;
	    td->rembutton = ctrl_pushbutton(s, "Remove", 'r',
					    HELPCTX(ssh_ttymodes),
					    ttymodes_handler, P(td));
	    td->rembutton->generic.column = 1;
	    td->rembutton->generic.tabdelay = 1;
	    ctrl_columns(s, 1, 100);
	    td->listbox = ctrl_listbox(s, NULL, NO_SHORTCUT,
				       HELPCTX(ssh_ttymodes),
				       ttymodes_handler, P(td));
	    td->listbox->listbox.multisel = 1;
	    td->listbox->listbox.height = 4;
	    td->listbox->listbox.ncols = 2;
	    td->listbox->listbox.percentages = snewn(2, int);
	    td->listbox->listbox.percentages[0] = 40;
	    td->listbox->listbox.percentages[1] = 60;
	    ctrl_tabdelay(s, td->rembutton);
	    ctrl_columns(s, 2, 75, 25);
	    td->modelist = ctrl_droplist(s, "Mode:", 'm', 67,
					 HELPCTX(ssh_ttymodes),
					 ttymodes_handler, P(td));
	    td->modelist->generic.column = 0;
	    td->addbutton = ctrl_pushbutton(s, "Add", 'd',
					    HELPCTX(ssh_ttymodes),
					    ttymodes_handler, P(td));
	    td->addbutton->generic.column = 1;
	    td->addbutton->generic.tabdelay = 1;
	    ctrl_columns(s, 1, 100);	    /* column break */
	    /* Bit of a hack to get the value radio buttons and
	     * edit-box on the same row. */
	    ctrl_columns(s, 3, 25, 50, 25);
	    c = ctrl_text(s, "Value:", HELPCTX(ssh_ttymodes));
	    c->generic.column = 0;
	    td->valradio = ctrl_radiobuttons(s, NULL, NO_SHORTCUT, 2,
					     HELPCTX(ssh_ttymodes),
					     ttymodes_handler, P(td),
					     "Auto", NO_SHORTCUT, P(NULL),
					     "This:", NO_SHORTCUT, P(NULL),
					     NULL);
	    td->valradio->generic.column = 1;
	    td->valbox = ctrl_editbox(s, NULL, NO_SHORTCUT, 100,
				      HELPCTX(ssh_ttymodes),
				      ttymodes_handler, P(td), P(NULL));
	    td->valbox->generic.column = 2;
	    ctrl_tabdelay(s, td->addbutton);

	}

	if (!midsession) {
	    /*
	     * The Connection/SSH/X11 panel.
	     */
	    ctrl_settitle(b, "Connection/SSH/X11",
			  "Options controlling SSH X11 forwarding");

	    s = ctrl_getset(b, "Connection/SSH/X11", "x11", "X11 forwarding");
	    ctrl_checkbox(s, "Enable X11 forwarding", 'e',
			  HELPCTX(ssh_tunnels_x11),
			  dlg_stdcheckbox_handler,I(offsetof(Config,x11_forward)));
	    ctrl_editbox(s, "X display location", 'x', 50,
			 HELPCTX(ssh_tunnels_x11),
			 dlg_stdeditbox_handler, I(offsetof(Config,x11_display)),
			 I(sizeof(((Config *)0)->x11_display)));
	    ctrl_radiobuttons(s, "Remote X11 authentication protocol", 'u', 2,
			      HELPCTX(ssh_tunnels_x11auth),
			      dlg_stdradiobutton_handler,
			      I(offsetof(Config, x11_auth)),
			      "MIT-Magic-Cookie-1", I(X11_MIT),
			      "XDM-Authorization-1", I(X11_XDM), NULL);
	}

	/*
	 * The Tunnels panel _is_ still available in mid-session.
	 */
	ctrl_settitle(b, "Connection/SSH/Tunnels",
		      "Options controlling SSH port forwarding");

	s = ctrl_getset(b, "Connection/SSH/Tunnels", "portfwd",
			"Port forwarding");
	ctrl_checkbox(s, "Local ports accept connections from other hosts",'t',
		      HELPCTX(ssh_tunnels_portfwd_localhost),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,lport_acceptall)));
	ctrl_checkbox(s, "Remote ports do the same (SSH-2 only)", 'p',
		      HELPCTX(ssh_tunnels_portfwd_localhost),
		      dlg_stdcheckbox_handler,
		      I(offsetof(Config,rport_acceptall)));

	ctrl_columns(s, 3, 55, 20, 25);
	c = ctrl_text(s, "Forwarded ports:", HELPCTX(ssh_tunnels_portfwd));
	c->generic.column = COLUMN_FIELD(0,2);
	/* You want to select from the list, _then_ hit Remove. So tab order
	 * should be that way round. */
	pfd = (struct portfwd_data *)ctrl_alloc(b,sizeof(struct portfwd_data));
	pfd->rembutton = ctrl_pushbutton(s, "Remove", 'r',
					 HELPCTX(ssh_tunnels_portfwd),
			    portfwd_handler, P(pfd));
	pfd->rembutton->generic.column = 2;
	pfd->rembutton->generic.tabdelay = 1;
	pfd->listbox = ctrl_listbox(s, NULL, NO_SHORTCUT,
				    HELPCTX(ssh_tunnels_portfwd),
				    portfwd_handler, P(pfd));
	pfd->listbox->listbox.height = 3;
	pfd->listbox->listbox.ncols = 2;
	pfd->listbox->listbox.percentages = snewn(2, int);
	pfd->listbox->listbox.percentages[0] = 20;
	pfd->listbox->listbox.percentages[1] = 80;
	ctrl_tabdelay(s, pfd->rembutton);
	ctrl_text(s, "Add new forwarded port:", HELPCTX(ssh_tunnels_portfwd));
	/* You want to enter source, destination and type, _then_ hit Add.
	 * Again, we adjust the tab order to reflect this. */
	pfd->addbutton = ctrl_pushbutton(s, "Add", 'd',
					 HELPCTX(ssh_tunnels_portfwd),
					 portfwd_handler, P(pfd));
	pfd->addbutton->generic.column = 2;
	pfd->addbutton->generic.tabdelay = 1;
	pfd->sourcebox = ctrl_editbox(s, "Source port", 's', 40,
				      HELPCTX(ssh_tunnels_portfwd),
				      portfwd_handler, P(pfd), P(NULL));
	pfd->sourcebox->generic.column = 0;
	pfd->destbox = ctrl_editbox(s, "Destination", 'i', 67,
				    HELPCTX(ssh_tunnels_portfwd),
				    portfwd_handler, P(pfd), P(NULL));
	pfd->direction = ctrl_radiobuttons(s, NULL, NO_SHORTCUT, 3,
					   HELPCTX(ssh_tunnels_portfwd),
					   portfwd_handler, P(pfd),
					   "Local", 'l', P(NULL),
					   "Remote", 'm', P(NULL),
					   "Dynamic", 'y', P(NULL),
					   NULL);
#ifndef NO_IPV6
	pfd->addressfamily =
	    ctrl_radiobuttons(s, NULL, NO_SHORTCUT, 3,
			      HELPCTX(ssh_tunnels_portfwd_ipversion),
			      portfwd_handler, P(pfd),
			      "Auto", 'u', I(ADDRTYPE_UNSPEC),
			      "IPv4", '4', I(ADDRTYPE_IPV4),
			      "IPv6", '6', I(ADDRTYPE_IPV6),
			      NULL);
#endif
	ctrl_tabdelay(s, pfd->addbutton);
	ctrl_columns(s, 1, 100);

	if (!midsession) {
	    /*
	     * The Connection/SSH/Bugs panel.
	     */
	    ctrl_settitle(b, "Connection/SSH/Bugs",
			  "Workarounds for SSH server bugs");

	    s = ctrl_getset(b, "Connection/SSH/Bugs", "main",
			    "Detection of known bugs in SSH servers");
	    ctrl_droplist(s, "Chokes on SSH-1 ignore messages", 'i', 20,
			  HELPCTX(ssh_bugs_ignore1),
			  sshbug_handler, I(offsetof(Config,sshbug_ignore1)));
	    ctrl_droplist(s, "Refuses all SSH-1 password camouflage", 's', 20,
			  HELPCTX(ssh_bugs_plainpw1),
			  sshbug_handler, I(offsetof(Config,sshbug_plainpw1)));
	    ctrl_droplist(s, "Chokes on SSH-1 RSA authentication", 'r', 20,
			  HELPCTX(ssh_bugs_rsa1),
			  sshbug_handler, I(offsetof(Config,sshbug_rsa1)));
	    ctrl_droplist(s, "Miscomputes SSH-2 HMAC keys", 'm', 20,
			  HELPCTX(ssh_bugs_hmac2),
			  sshbug_handler, I(offsetof(Config,sshbug_hmac2)));
	    ctrl_droplist(s, "Miscomputes SSH-2 encryption keys", 'e', 20,
			  HELPCTX(ssh_bugs_derivekey2),
			  sshbug_handler, I(offsetof(Config,sshbug_derivekey2)));
	    ctrl_droplist(s, "Requires padding on SSH-2 RSA signatures", 'p', 20,
			  HELPCTX(ssh_bugs_rsapad2),
			  sshbug_handler, I(offsetof(Config,sshbug_rsapad2)));
	    ctrl_droplist(s, "Misuses the session ID in SSH-2 PK auth", 'n', 20,
			  HELPCTX(ssh_bugs_pksessid2),
			  sshbug_handler, I(offsetof(Config,sshbug_pksessid2)));
	    ctrl_droplist(s, "Handles SSH-2 key re-exchange badly", 'k', 20,
			  HELPCTX(ssh_bugs_rekey2),
			  sshbug_handler, I(offsetof(Config,sshbug_rekey2)));
	}
    }
}

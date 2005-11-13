/*
 * Linking module for PuTTYtel: list the available backends not
 * including ssh.
 */

#include <stdio.h>
#include "putty.h"

const int be_default_protocol = PROT_TELNET;

const char *const appname = "TuTTYtel";

struct backend_list backends[] = {
    {PROT_TELNET, "telnet", &telnet_backend},
    {PROT_RLOGIN, "rlogin", &rlogin_backend},
    {PROT_RAW, "raw", &raw_backend},
    {PROT_SERIAL, "serial", &serial_backend},
    {0, NULL}
};

/*
 * Stub implementations of functions not used in non-ssh versions.
 */
void random_save_seed(void)
{
}

void random_destroy_seed(void)
{
}

void noise_ultralight(unsigned long data)
{
}
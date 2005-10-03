/*
 * PuTTY version numbering
 */

#include "build.h"

#define STR1(x) #x
#define STR(x) STR1(x)

#if defined SNAPSHOT

char ver[] = "Development build " STR(BUILDNUMBER) ", based on PuTTY 0.58"
    "\n compiled at " __DATE__ " " __TIME__;
char sshver[] = "PuTTY-Snapshot-" STR(SNAPSHOT);

#elif defined RELEASE

char ver[] = 
    "Release " STR(RELEASE) " build " STR(BUILDNUMBER) ", based on PuTTY " STR(RELEASE);
char sshver[] = "PuTTY-Release-" STR(RELEASE);

#else

char ver[] = "Unidentified build, " __DATE__ " " __TIME__;
char sshver[] = "PuTTY-Local: " __DATE__ " " __TIME__;

#endif

/*
 * SSH local version string MUST be under 40 characters. Here's a
 * compile time assertion to verify this.
 */
enum { vorpal_sword = 1 / (sizeof(sshver) <= 40) };

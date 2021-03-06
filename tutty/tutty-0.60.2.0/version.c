/*
 * PuTTY version numbering
 */

#include "build.h"

#define STR1(x) #x
#define STR(x) STR1(x)

#if defined SNAPSHOT

#if defined SVN_REV
#define SNAPSHOT_TEXT STR(SNAPSHOT) ":r" STR(SVN_REV)
#else
#define SNAPSHOT_TEXT STR(SNAPSHOT)
#endif

char ver[] = "Development snapshot " SNAPSHOT_TEXT;
char sshver[] = "TuTTY-Snapshot-" SNAPSHOT_TEXT;

#undef SNAPSHOT_TEXT

#elif defined RELEASE

char ver[] = "Release " STR(RELEASE);
char sshver[] = "TuTTY-Release-" STR(RELEASE);

#elif defined SVN_REV

char ver[] = "Custom build r" STR(SVN_REV);
char sshver[] = "TuTTY-Custom-r" STR(SVN_REV);

#else

char ver[] = "Unidentified build, " __DATE__ " " __TIME__;
char sshver[] = "PuTTY-Local: " __DATE__ " " __TIME__;

#endif

/*
 * SSH local version string MUST be under 40 characters. Here's a
 * compile time assertion to verify this.
 */
enum { vorpal_sword = 1 / (sizeof(sshver) <= 40) };

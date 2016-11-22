
#include "usloss.h"
#define DEBUG 0
extern int debugflag;

void
p1_fork(int pid)
{
    if (DEBUG && debugflag)
        USLOSS_Console("p1_fork() called: pid = %d\n", pid);
} /* p1_fork */


/* As a reminder:
 * In phase 1, p1_switch is called by the dispatcher right before the
 * dispatcher does: enableInterrupts() followed by USLOSS_ContestSwitch()
 */
void
p1_switch(int old, int new)
{
    if (DEBUG && debugflag)
        USLOSS_Console("p1_switch() called: old = %d, new = %d\n", old, new);
} /* p1_switch */

void
p1_quit(int pid)
{
    if (DEBUG && debugflag)
        USLOSS_Console("p1_quit() called: pid = %d\n", pid);
} /* p1_quit */

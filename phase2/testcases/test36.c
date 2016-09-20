
/* A simple test of USLOSS_Syscall and sys_vec.  Makes a call to system trap 
 * number 0.  Should cause USLOSS to halt.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


extern void USLOSS_Syscall(void *arg);

void enableUserMode(){
    USLOSS_PsrSet( USLOSS_PsrGet() & (~ USLOSS_PSR_CURRENT_MODE) );
}


int start2(char *arg)
{
    systemArgs args;

    USLOSS_Console("start2(): putting itself into user mode\n");
    enableUserMode();

    args.number=0;

    USLOSS_Console("start2(): calling USLOSS_Syscall executing syscall 0, ");
    USLOSS_Console("this should halt\n");

    USLOSS_Syscall((void *)&args);

    USLOSS_Console("start2(): should not see this message!\n");
    quit(0);

    return 0;

} /* start2 */

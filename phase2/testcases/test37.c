
/* A simple test of USLOSS_Syscall and sys_vec.  Makes a call to system trap 
 * number MAXSYSCALLS - 1.  Should cause USLOSS to halt.
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

    args.number = MAXSYSCALLS - 1;

    USLOSS_Console("start2(): calling USLOSS_Syscall executing syscall ");
    USLOSS_Console("MAXSYSCALLS - 1\n");
    USLOSS_Console("          This should halt\n");

    USLOSS_Syscall((void *)&args);

    USLOSS_Console("start2(): should not see this message!\n");
    quit(0);

    return 0;

} /* start2 */

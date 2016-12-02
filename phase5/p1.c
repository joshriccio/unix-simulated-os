
#include "usloss.h"
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <phase5.h>
#include <usyscall.h>
#include <libuser.h>
#include <vm.h>

#define DEBUG 0
extern int debugflag;
extern int vmInitialized;
extern Process procTable[MAXPROC];

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
    if (DEBUG && debugflag) {
        USLOSS_Console("p1_switch() called: old = %d, new = %d\n", old, new);
        USLOSS_Console("p1_switch(): new = %d, vm = %d\n", new, procTable[new].vm);
    }

    int result;

    //Check if vmInit has been called
    if(vmInitialized ){
        //USLOSS_Console("p1_switch(): The VM is initialized\n");

        // if old is a vm process
        if (procTable[old % MAXPROC].vm) {
            for(int page=0; page<vmStats.pages; page++){
                if (procTable[old % MAXPROC].pageTable[page].frame != -1) {
                    result = USLOSS_MmuUnmap(0, page);
                    if (result != USLOSS_MMU_OK) {
                        USLOSS_Console("p1_switch(old): "
                                "USLOSS_MmuUnmap Error: %d\n", result);
                    }
                }
            }
        }

        // if new is a vm process
        if (procTable[new % MAXPROC].vm) {
            for(int page=0; page<vmStats.pages; page++){
                if (procTable[new % MAXPROC].pageTable[page].frame != -1) {
                    result = USLOSS_MmuMap(0, page, 
                        procTable[new % MAXPROC].pageTable[page].frame, 
                        USLOSS_MMU_PROT_RW);
                    if(result != USLOSS_MMU_OK){
                        USLOSS_Console("p1_switch(new): USLOSS_MmuMap error: "
                                "%d\n", result);
                    }
                    // TODO: figure out status of mapping
                    USLOSS_MmuSetAccess(procTable[new % MAXPROC].pageTable[page].frame, 
                            3);
                }
            }

        }

	    //Set access to RW, the scecond parameter is the access bits,
	    //USLOSS_MMU_REF          1       /* Page has been referenced */
	    //USLOSS_MMU_DIRTY        2       /* Page has been written */
    }
} /* p1_switch */

void
p1_quit(int pid)
{
    if (DEBUG && debugflag)
        USLOSS_Console("p1_quit() called: pid = %d\n", pid);
} /* p1_quit */

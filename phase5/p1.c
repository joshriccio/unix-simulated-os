
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
extern FTE *frameTable;
extern int vmStatSem;
extern VmStats vmStats;
extern int  sempReal(int semaphore);
extern int  semvReal(int semaphore);

void
p1_fork(int pid)
{
    if (DEBUG && debugflag)
        USLOSS_Console("p1_fork() called: pid = %d\n", pid);

    if (vmInitialized){
        int pages = procTable[pid % MAXPROC].numPages;
        procTable[pid % MAXPROC].pageTable = malloc(pages * sizeof(PTE));

        for (int page = 0; page < pages; page++) {
            procTable[pid % MAXPROC].pageTable[page].frame = -1;
            procTable[pid % MAXPROC].pageTable[page].state = UNMAPPED;
        }
    }
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
        USLOSS_Console("p1_switch(): new = %d, vm = %d\n", new, 
                procTable[new].vm);
    }

    int result;

    //Check if vmInit has been called
    if (vmInitialized) {
        //USLOSS_Console("p1_switch(): The VM is initialized\n");

        // if old is a vm process
        if (procTable[old % MAXPROC].vm) {
            for(int page = 0; page < vmStats.pages; page++){
                if (procTable[old % MAXPROC].pageTable[page].state == MAPPED) {
                    //USLOSS_Console("p1_switch: unmapping old\n");
                    result = USLOSS_MmuUnmap(0, page);
                    if (result != USLOSS_MMU_OK) {
                        USLOSS_Console("p1_switch(old): "
                                "USLOSS_MmuUnmap Error: %d\n", result);
                    }
                    procTable[old % MAXPROC].pageTable[page].state = UNMAPPED;
                }
            }
        }

        // if new is a vm process
        if (procTable[new % MAXPROC].vm) {
            int frame;
            for(int page = 0; page < vmStats.pages; page++){
                frame = procTable[new % MAXPROC].pageTable[page].frame;
                if (frame != -1) {
                    //USLOSS_Console("p1_switch: mapping new\n");
                    result = USLOSS_MmuMap(0, page, frame, USLOSS_MMU_PROT_RW);
                    if(result != USLOSS_MMU_OK){
                        USLOSS_Console("p1_switch(new): USLOSS_MmuMap error: "
                                "%d\n", result);
                    }
                    //USLOSS_MmuSetAccess(frame, 3);
                    procTable[new % MAXPROC].pageTable[page].state = MAPPED;
                }
            }

        }

        sempReal(vmStatSem);
        vmStats.switches++;
        semvReal(vmStatSem);
    }
} /* p1_switch */

void
p1_quit(int pid)
{
    int result; 

    if (DEBUG && debugflag)
        USLOSS_Console("p1_quit() called: pid = %d\n", pid);

    if (vmInitialized && procTable[pid % MAXPROC].vm) {
        int frame;
        for(int page=0; page<vmStats.pages; page++){
            frame = procTable[pid % MAXPROC].pageTable[page].frame;
            if (frame != -1) {
                result = USLOSS_MmuUnmap(0, page);
                if (result != USLOSS_MMU_OK) {
                    USLOSS_Console("p1_quit(): "
                            "USLOSS_MmuUnmap Error: %d\n", result);
                }
                procTable[pid % MAXPROC].pageTable[page].frame = -1;
                frameTable[frame].state = UNUSED;
                frameTable[frame].ref = UNREFERENCED;
                frameTable[frame].dirty = CLEAN;

                sempReal(vmStatSem);
                vmStats.freeFrames++;
                semvReal(vmStatSem);
            }
        }
        free(procTable[pid % MAXPROC].pageTable);
    }
} /* p1_quit */

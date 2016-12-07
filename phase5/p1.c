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

/*----------------------------------------------------------------------
 * p1_fork
 *
 * Creates a page table for a process, if the virtual memory system
 * has been initialized.
 *
 * Results: None
 *
 * Side effects: pageTable malloced and initialized for process
 *----------------------------------------------------------------------*/
void p1_fork(int pid) {
    if (DEBUG && debugflag)
        USLOSS_Console("p1_fork() called: pid = %d\n", pid);

    if (vmInitialized) {
        /* set number of pages in process's page table and malloc memory */
        int pages = procTable[pid % MAXPROC].numPages;
        procTable[pid % MAXPROC].pageTable = malloc(pages * sizeof(PTE));

        /* initialize page table for the process */
        for (int page = 0; page < pages; page++) {
            procTable[pid % MAXPROC].pageTable[page].frame = -1;
            procTable[pid % MAXPROC].pageTable[page].accessed = 0;
            procTable[pid % MAXPROC].pageTable[page].state = UNMAPPED;
            procTable[pid % MAXPROC].pageTable[page].diskTableIndex = -1;
        }
    }
} /* p1_fork */

/*----------------------------------------------------------------------
 * p1_switch
 *
 * Unmaps and Maps pages to frames in the MMU for processes that are context
 * switching. A process must be set as a virtual memory process by the fault
 * handler in phase5.c to unmap and map.
 *
 * Results: None
 *
 * Side effects: mapping are added or removed from the MMU
 *----------------------------------------------------------------------*/
void p1_switch(int old, int new) {
    if (DEBUG && debugflag) {
        USLOSS_Console("p1_switch() called: old = %d, new = %d\n", old, new);
        USLOSS_Console("p1_switch(): new = %d, vm = %d\n", new, 
                procTable[new].vm);
    }

    int result;

    /* only perform mappings and unmappings if vm is initialized */
    if (vmInitialized) {

        /* if old is a vm process */
        if (procTable[old % MAXPROC].vm) {
            for(int page = 0; page < vmStats.pages; page++){

                /* Unmap page from MMU */
                if (procTable[old % MAXPROC].pageTable[page].state == MAPPED) {
                    result = USLOSS_MmuUnmap(0, page);
                    if (result != USLOSS_MMU_OK) {
                        USLOSS_Console("p1_switch(old): "
                                "USLOSS_MmuUnmap Error: %d\n", result);
                    }
                    procTable[old % MAXPROC].pageTable[page].state = UNMAPPED;
                }
            }
        }

        /* if new is a vm process */
        if (procTable[new % MAXPROC].vm) {
            int frame;

            /* 
             * for every page in the pageTable, see if it should be mapped
             * to a frame.
             */
            for(int page = 0; page < vmStats.pages; page++){
                frame = procTable[new % MAXPROC].pageTable[page].frame;

                /* page should only be mapped to a frame if frame is not -1 */
                if (frame != -1) {
                    result = USLOSS_MmuMap(0, page, frame, USLOSS_MMU_PROT_RW);
                    if(result != USLOSS_MMU_OK){
                        USLOSS_Console("p1_switch(new): USLOSS_MmuMap error: "
                                "%d\n", result);
                    }
                    procTable[new % MAXPROC].pageTable[page].state = MAPPED;
                }
            }

        }

        sempReal(vmStatSem);
        vmStats.switches++;
        semvReal(vmStatSem);
    }
} /* p1_switch */

/*----------------------------------------------------------------------
 * p1_quit
 *
 * Removes mappings to the MMU for processes that are quiting.
 *
 * Results: None
 *
 * Side effects: mappings are removed from the MMU
 *----------------------------------------------------------------------*/
void p1_quit(int pid) {
    int result; 

    if (DEBUG && debugflag)
        USLOSS_Console("p1_quit() called: pid = %d\n", pid);

    /* Unmap pages that are maped in the MMU */
    if (vmInitialized && procTable[pid % MAXPROC].vm) {
        int frame;
        for(int page = 0; page < vmStats.pages; page++) {
            frame = procTable[pid % MAXPROC].pageTable[page].frame;

            /* page is mapped, if frame is not -1 in pageTable */
            if (frame != -1) {
                result = USLOSS_MmuUnmap(0, page);
                if (result != USLOSS_MMU_OK) {
                    USLOSS_Console("p1_quit(): "
                            "USLOSS_MmuUnmap Error: %d\n", result);
                }
                
                /* update frame table */
                procTable[pid % MAXPROC].pageTable[page].frame = -1;
                frameTable[frame].state = UNUSED;
                frameTable[frame].ref = UNREFERENCED;
                frameTable[frame].dirty = CLEAN;

                /* update MMU */
                result = USLOSS_MmuSetAccess(frame, UNREFERENCED + CLEAN);
                if (result != USLOSS_MMU_OK) {
                    USLOSS_Console("p1_quit: USLOSS_MmuSetAccess "
                        "Error: %d\n", result);
                }  

                sempReal(vmStatSem);
                vmStats.freeFrames++;
                semvReal(vmStatSem);
            }
        }
        free(procTable[pid % MAXPROC].pageTable); // free malloced memory
    }
} /* p1_quit */

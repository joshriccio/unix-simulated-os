/* ------------------------------------------------------------------------
   phase5.c

   University of Arizona
   Computer Science 452
 
   @author Joshua Riccio
   @author Austin George
   ------------------------------------------------------------------------ */

#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <phase5.h>
#include <usyscall.h>
#include <libuser.h>
#include <vm.h>
#include <string.h>
#include <providedPrototypes.h>

extern void mbox_create(systemArgs *args_ptr);
extern void mbox_release(systemArgs *args_ptr);
extern void mbox_send(systemArgs *args_ptr);
extern void mbox_receive(systemArgs *args_ptr);
extern void mbox_condsend(systemArgs *args_ptr);
extern void mbox_condreceive(systemArgs *args_ptr);
extern int  semcreateReal(int init_value);
extern int  sempReal(int semaphore);
extern int  semvReal(int semaphore);
extern int getPID_real(int *pid);
extern int diskSizeReal(int unit, int *sectorSize, int *sectorsInTrack, 
        int *tracksInDisk);
extern int start5(char *arg);

static void vmInit(systemArgs *sysargsPtr);
void *vmInitReal(int mappings, int pages, int frames, int pagers);
static void vmDestroy(systemArgs *sysargsPtr);
void vmDestroyReal(void);
static void FaultHandler(int  type, void *arg);
static int Pager(char *buf);
int getPID5();
void debugPageTable(int pid);
void debugFrameTable(int pid);
Process procTable[MAXPROC];
FaultMsg faults[MAXPROC]; /* Note that a process can have only
                           * one fault at a time, so we can
                           * allocate the messages statically
                           * and index them by pid. */
VmStats  vmStats;
void *vmRegion;
FTE *frameTable;
int diskTableSize;
DTE *diskTable;
int numPagers;
int *pagerPids;
int pagerMbox;
int vmInitialized = 0;
int vmStatSem;
int clockHand;
int clockSem;
int frameSem;

/*
 *----------------------------------------------------------------------
 *
 * start4 --
 *
 * Initializes the VM system call handlers. 
 *
 * Results:
 *      MMU return status
 *
 * Side effects:
 *      The MMU is initialized.
 *
 *----------------------------------------------------------------------
 */
int start4(char *arg){
    int pid;
    int result;
    int status;

    /* to get user-process access to mailbox functions */
    systemCallVec[SYS_MBOXCREATE]      = mbox_create;
    systemCallVec[SYS_MBOXRELEASE]     = mbox_release;
    systemCallVec[SYS_MBOXSEND]        = mbox_send;
    systemCallVec[SYS_MBOXRECEIVE]     = mbox_receive;
    systemCallVec[SYS_MBOXCONDSEND]    = mbox_condsend;
    systemCallVec[SYS_MBOXCONDRECEIVE] = mbox_condreceive;
    

    /* user-process access to VM functions */
    systemCallVec[SYS_VMINIT]    = vmInit;
    systemCallVec[SYS_VMDESTROY] = vmDestroy;

    result = Spawn("Start5", start5, NULL, 8*USLOSS_MIN_STACK, 2, &pid);
    if (result != 0) {
        USLOSS_Console("start4(): Error spawning start5\n");
        Terminate(1);
    }

    //Wait for start5 to terminate
    result = Wait(&pid, &status);
    if (result != 0) {
        USLOSS_Console("start4(): Error waiting for start5\n");
        Terminate(1);
    }

    Terminate(0);
    return 0; // not reached

} /* start4 */

/*
 *----------------------------------------------------------------------
 *
 * VmInit --
 *
 * Stub for the VmInit system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      VM system is initialized.
 *
 *----------------------------------------------------------------------
 */
static void vmInit(systemArgs *args) {
    void *result;

    CheckMode();

    int mappings = ((int) (long) args->arg1);
    int pages = ((int) (long) args->arg2);
    int frames = ((int) (long) args->arg3);
    int pagers = ((int) (long) args->arg4);

    result = vmInitReal(mappings, pages, frames, pagers);

    if (((int) (long) result) < 0) {
        args->arg4 = result;
    } else {
        args->arg4 = ((void *) (long) 0);
    }

    args->arg1 = result;

} /* vmInit */


/*
 *----------------------------------------------------------------------
 *
 * vmDestroy --
 *
 * Stub for the VmDestroy system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      VM system is cleaned up.
 *
 *----------------------------------------------------------------------
 */
static void vmDestroy(systemArgs *sysargsPtr){
   CheckMode();
   vmDestroyReal();
} /* vmDestroy */


/*
 *----------------------------------------------------------------------
 *
 * vmInitReal --
 *
 * Called by vmInit.
 * Initializes the VM system by configuring the MMU and setting
 * up the page tables.
 *
 * Results:
 *      Address of the VM region.
 *
 * Side effects:
 *      The MMU is initialized.
 *
 *----------------------------------------------------------------------
 */
void *vmInitReal(int mappings, int pages, int frames, int pagers){
   int status;
   int numPagesInVmRegion;

   CheckMode();

    // error checking
    if (mappings < 1 || pages < 1 || frames < 1 || pagers < 1) {
        return ((void *)(long)-1);
    }

    if (pagers > MAXPAGERS) {
        return ((void *)(long)-1);
    }

    if (mappings != pages) {
        return ((void *)(long)-1);
    }

   status = USLOSS_MmuInit(mappings, pages, frames);
   if (status != USLOSS_MMU_OK) {
      USLOSS_Console("vmInitReal: couldn't init MMU, status %d\n", status);
      abort();
   }
   
   vmInitialized = 1;

   /*
    * Zero out, then initialize, the vmStats structure
    */
   int sectorSize, sectorsInTrack, numTracksOnDisk;

   diskSizeReal(1, &sectorSize, &sectorsInTrack, &numTracksOnDisk);

   vmStats.pages = pages;
   vmStats.frames = frames;
   vmStats.diskBlocks = numTracksOnDisk * 2;
   vmStats.freeFrames = frames;
   vmStats.freeDiskBlocks = numTracksOnDisk * 2;
   vmStats.faults = 0;
   vmStats.switches = 0;
   vmStats.new = 0;
   vmStats.pageIns = 0;
   vmStats.pageOuts = 0;
   vmStats.replaced = 0;

    vmStatSem = semcreateReal(1);  // MUTEX for vmStats

   USLOSS_IntVec[USLOSS_MMU_INT] = FaultHandler;

   /* initialize disk table */
   diskTable = malloc(sectorsInTrack * numTracksOnDisk * sizeof(DTE));
   diskTableSize = sectorsInTrack * numTracksOnDisk;

   int trackNum = 0;
   for (int i = 0; i < diskTableSize; i += 2) {
       diskTable[i].track = trackNum;
       diskTable[i + 1].track = trackNum++;
       diskTable[i].sector = 0;
       diskTable[i + 1].sector = sectorsInTrack / 2;
       diskTable[i].state = UNUSED;
       diskTable[i + 1].state = UNUSED;
   }


   /*
    * Initialize page tables, and fault mbox.
    */
   for (int i = 0; i < MAXPROC; i++) {
       procTable[i].pid = -1;
       procTable[i].vm = 0;
       procTable[i].numPages = pages;

       faults[i].pid = -1;
       faults[i].replyMbox = MboxCreate(1, 0);
       faults[i].addr = NULL;
   }

   /*
    * Initialize frame tables.
    */
   frameTable = malloc(frames * sizeof(FTE));
   for (int i = 0; i < frames; i++) {
      frameTable[i].state = UNUSED;
      frameTable[i].ref = UNREFERENCED;
      frameTable[i].dirty = CLEAN;
      frameTable[i].pid = -1;
      frameTable[i].page = NULL;
   }   
   
   frameSem = semcreateReal(1);

   /* initialize clock hand for clock algorithm */
   clockHand = 0;
   clockSem = semcreateReal(1);

   /*
    * Fork the pagers.
    */
   numPagers = pagers;
   pagerPids = malloc(pagers * sizeof(int));
   char buf[100];
   pagerMbox = MboxCreate(pagers, sizeof(int));
   for (int i = 0; i < numPagers; i++){
      pagerPids[i] = fork1("pagerProcess", Pager, buf, USLOSS_MIN_STACK, 
              PAGER_PRIORITY);
   }

   vmRegion = USLOSS_MmuRegion(&numPagesInVmRegion);
   return vmRegion;
} /* vmInitReal */


/*
 *----------------------------------------------------------------------
 *
 * PrintStats --
 *
 *      Print out VM statistics.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Stuff is printed to the USLOSS_Console.
 *
 *----------------------------------------------------------------------
 */
void PrintStats(void){
     USLOSS_Console("VmStats\n");
     USLOSS_Console("pages:          %d\n", vmStats.pages);
     USLOSS_Console("frames:         %d\n", vmStats.frames);
     USLOSS_Console("diskBlocks:     %d\n", vmStats.diskBlocks);
     USLOSS_Console("freeFrames:     %d\n", vmStats.freeFrames);
     USLOSS_Console("freeDiskBlocks: %d\n", vmStats.freeDiskBlocks);
     USLOSS_Console("switches:       %d\n", vmStats.switches);
     USLOSS_Console("faults:         %d\n", vmStats.faults);
     USLOSS_Console("new:            %d\n", vmStats.new);
     USLOSS_Console("pageIns:        %d\n", vmStats.pageIns);
     USLOSS_Console("pageOuts:       %d\n", vmStats.pageOuts);
     USLOSS_Console("replaced:       %d\n", vmStats.replaced);
} /* PrintStats */


/*
 *----------------------------------------------------------------------
 *
 * vmDestroyReal --
 *
 * Called by vmDestroy.
 * Frees all of the global data structures
 *
 * Results:
 *      None
 *
 * Side effects:
 *      The MMU is turned off.
 *
 *----------------------------------------------------------------------
 */
void vmDestroyReal(void){

   CheckMode();
   USLOSS_MmuDone();
   /*
    * Kill the pagers here.
    */
   int terminateMsg = -1;
   for (int i = 0; i < numPagers; i++) {
       MboxSend(pagerMbox, &terminateMsg, sizeof(int));
   }

   for (int i = 0; i < numPagers; i++) {
       zap(pagerPids[i]);
   }

   /* 
    * Print vm statistics.
    */
   PrintStats();

   //TODO: free memory 
   //free framesTable and pager PID table

   vmInitialized = 0;  //TODO: might be after MmuDone
} /* vmDestroyReal */

/*
 *----------------------------------------------------------------------
 *
 * FaultHandler
 *
 * Handles an MMU interrupt. Simply stores information about the
 * fault in a queue, wakes a waiting pager, and blocks until
 * the fault has been handled.
 * 
 * Parameters: int  type = USLOSS_MMU_INT , void *arg  = Offset within VM region
 * Results:
 * None.
 *
 * Side effects:
 * The current process is blocked until the fault is handled.
 *
 *----------------------------------------------------------------------
 */
static void FaultHandler(int  type, void *arg){
   int cause;

    int offset = (int) (long) arg;
    void * vmPlusOffset = vmRegion + offset;

    /*USLOSS_Console("offset = %d\n", (int) (long) arg);
    USLOSS_Console("page# = %d\n", page);
    USLOSS_Console("vmRegion = %lp\n", vmRegion);*/ 
    //USLOSS_Console("temp = %lp\n", temp);
    
   assert(type == USLOSS_MMU_INT);
   cause = USLOSS_MmuGetCause();
   assert(cause == USLOSS_MMU_FAULT);

   // update vmStats
   sempReal(vmStatSem);
   vmStats.faults++;
   semvReal(vmStatSem);

   int pid;
   getPID_real(&pid);

   procTable[pid % MAXPROC].pid = pid;
   procTable[pid % MAXPROC].vm = 1;

   /*
    * Fill in faults[pid % MAXPROC], send it to the pagers, and wait for the
    * reply.
    */
   faults[pid % MAXPROC].pid = pid;
   faults[pid % MAXPROC].addr = vmPlusOffset;

   MboxSend(pagerMbox, &pid, sizeof(int));

   MboxReceive(faults[pid % MAXPROC].replyMbox, NULL, 0);
} /* FaultHandler */

/*
 *----------------------------------------------------------------------
 *
 * Pager 
 *
 * Kernel process that handles page faults and does page replacement.
 *
 * Results:
 * None.
 *
 * Side effects:
 * None.
 *
 *----------------------------------------------------------------------
 */
static int Pager(char *buf){
    int pid;

    while (1) {

        /* Wait for fault to occur (receive from mailbox) */
        MboxReceive(pagerMbox, &pid, sizeof(int));

        if (pid == -1 ) {
            break;
        }
	//debugPageTable(pid);
	//debugFrameTable(0);
        /* update frameTable to match MMU */
        int accessBit;
        for (int i = 0; i < vmStats.frames; i++) {
            //TODO: sempReal(frameSem);
            int result = USLOSS_MmuGetAccess(i, &accessBit);
            if (result != USLOSS_MMU_OK) {
                USLOSS_Console("Pager_accessBit: USLOSS_MmuGetAccess Error: "
                        "%d\n", result);
            }
            frameTable[i].ref = accessBit & REFERENCED;
            frameTable[i].dirty = accessBit & DIRTY;
            //semvReal(frameSem);
        }

        int freeFrame = -1;
        int page = -1;

        /* Look for free frame */
        for (int i = 0; i < vmStats.frames; i++) {
            if (frameTable[i].state == UNUSED){
                freeFrame = i; // save frame index, break out of loop
		//USLOSS_Console("Pager: Frame %d is unused\n", i);
                sempReal(vmStatSem);
                vmStats.freeFrames--;
                semvReal(vmStatSem);
                break;
            }
        }

        /* If there isn't one then use clock algorithm to
         * replace a page (perhaps write to disk) */
        if (freeFrame == -1) {
            int found = 0;
            //sempReal(clockSem);
            for (int i = 0; i < vmStats.frames; i++) {
                if (frameTable[clockHand].ref == UNREFERENCED) {
                    if (frameTable[clockHand].dirty == CLEAN) {
                        freeFrame = clockHand;
                        frameTable[clockHand].page->frame = -1;
                        found = 1;
			//USLOSS_Console("pass 1 Found UNref & Clean frame, %d\n", clockHand);
                clockHand++;
                if (clockHand == vmStats.frames) {
                    clockHand = 0;
                }

                        break;
                    }
                } else {
                    frameTable[clockHand].ref = UNREFERENCED;
                    USLOSS_MmuSetAccess(clockHand, 
                            frameTable[clockHand].dirty);
                }
                clockHand++;
                if (clockHand == vmStats.frames) {
                    clockHand = 0;
                }
            }

            /* No unreferenced and clean frames on first pass, this is second
             * pass */
            if (!found) {
                found = 0;
                for (int i = 0; i < vmStats.frames; i++) {
                    if (frameTable[clockHand].ref == UNREFERENCED) {
                        if (frameTable[clockHand].dirty == CLEAN) {
                            freeFrame = clockHand;
                            frameTable[clockHand].page->frame = -1;
                            found = 1;
			   // USLOSS_Console("pass 2 Found UNref & Clean frame, %d\n", clockHand);
                clockHand++;
                if (clockHand == vmStats.frames) {
                    clockHand = 0;
                }

                            break;
                        }
                    }
                    clockHand++;
                    if (clockHand == vmStats.frames) {
                        clockHand = 0;
                    }
                }
            }

            if (!found) {
                freeFrame = clockHand;
                page = frameTable[freeFrame].page->pageNum;
                void * addr = vmRegion + (page * USLOSS_MmuPageSize());
                char buffer[USLOSS_MmuPageSize()];

                int diskLocation;
                int firstUnused = 1;
                /* find location on swap disk for page */
                for (int i = 0; i < diskTableSize; i++) {
                    if (diskTable[i].pid == pid && diskTable[i].page == page) {
                        diskLocation = i;
                        break;
                    } else if (firstUnused && diskTable[i].state == UNUSED) {
                        firstUnused = 0;
                        diskLocation = i;

                        sempReal(vmStatSem);
                        vmStats.freeDiskBlocks--;
                        semvReal(vmStatSem);
                    }
                }
		//USLOSS_Console("Using clockhand, %d\n", clockHand);
                /* save frame to buffer */
                int result = USLOSS_MmuMap(0, page, freeFrame, 
                        USLOSS_MMU_PROT_RW);
                if (result != USLOSS_MMU_OK) {
                    USLOSS_Console("Pager(): USLOSS_MmuMap Error: %d\n"
                            , result);
                }

                memcpy(buffer, addr, USLOSS_MmuPageSize());

                result = USLOSS_MmuUnmap(0, page);
                if (result != USLOSS_MMU_OK) {
                    USLOSS_Console("Pager(): USLOSS_MmuUnmap Error: "
                            "%d\n", result);
                }

                // release clockSem before writing
                //semvReal(clockSem);
                diskWriteReal(1, diskTable[diskLocation].track, 
                        diskTable[diskLocation].sector, USLOSS_MmuPageSize() / 
                        USLOSS_DISK_SECTOR_SIZE, buffer);

                frameTable[clockHand].ref = UNREFERENCED;
                frameTable[clockHand].dirty = CLEAN;
                USLOSS_MmuSetAccess(clockHand, 
                        frameTable[clockHand].dirty);

                /* update disk table and page table state */
                diskTable[diskLocation].state = USED;
                diskTable[diskLocation].pid = frameTable[freeFrame].pid;
                diskTable[diskLocation].page = page;
                frameTable[freeFrame].page->diskTableIndex = diskLocation;
                frameTable[freeFrame].page->frame = -1;

                sempReal(vmStatSem);
                vmStats.pageOuts++;
                semvReal(vmStatSem);

                clockHand++;
                if (clockHand == vmStats.frames) {
                    clockHand = 0;
                }
            }
        }


        /* find page number of fault*/
        page = (int)(long) ( (char *) faults[pid % MAXPROC].addr - 
                (char *) vmRegion) / USLOSS_MmuPageSize();
        
        /* update page table */
        procTable[pid].pageTable[page].frame = freeFrame;
        procTable[pid].pageTable[page].pageNum = page;
        
        /* update frame table */
        frameTable[freeFrame].state = USED; 
        frameTable[freeFrame].page = &procTable[pid].pageTable[page];
        frameTable[freeFrame].pid = pid;

        /* Load page into frame from disk or initialize frame */
         if (procTable[pid % MAXPROC].pageTable[page].diskTableIndex != -1) {
            int i = procTable[pid].pageTable[page].diskTableIndex;

            // read from disk
            char buffer[USLOSS_MmuPageSize()];
            //TODO:USLOSS_Console("Pager(): Reading page from disk\n");
            diskReadReal(1, diskTable[i].track,
                             diskTable[i].sector, USLOSS_MmuPageSize() /
                             USLOSS_DISK_SECTOR_SIZE, buffer);

            /* save buffer to frame */
            int result = USLOSS_MmuMap(0, page, freeFrame,
                USLOSS_MMU_PROT_RW);
            if (result != USLOSS_MMU_OK) {
                USLOSS_Console("Pager(): USLOSS_MmuMap Error: %d\n"
                    , result);
            }

            memcpy(faults[pid % MAXPROC].addr, buffer, USLOSS_MmuPageSize());
            
            result = USLOSS_MmuSetAccess(freeFrame, UNREFERENCED + CLEAN);
            frameTable[freeFrame].ref = UNREFERENCED;
            frameTable[freeFrame].dirty = CLEAN;

            procTable[pid % MAXPROC].pageTable[page].frame = freeFrame;

            if (result != USLOSS_MMU_OK) {
                USLOSS_Console("Pager(): USLOSS_MmuSetAccess Error: %d\n",
                        result);
            }

            result = USLOSS_MmuUnmap(0, page);
            if (result != USLOSS_MMU_OK) {
                USLOSS_Console("Pager(): USLOSS_MmuUnmap Error: "
                    "%d\n", result);
            }
            
            sempReal(vmStatSem);
            vmStats.pageIns++;
            semvReal(vmStatSem);
         } else if (procTable[pid].pageTable[page].accessed == 0) {
            int result = USLOSS_MmuMap(0, page, freeFrame, USLOSS_MMU_PROT_RW);
            if (result != USLOSS_MMU_OK) {
                USLOSS_Console("Pager(): USLOSS_MmuMap Error: %d\n", result);
            }

            memset(faults[pid % MAXPROC].addr, 0, USLOSS_MmuPageSize());

            frameTable[freeFrame].ref = UNREFERENCED; 
            frameTable[freeFrame].dirty = CLEAN; 

            result = USLOSS_MmuSetAccess(freeFrame, UNREFERENCED + CLEAN);
            if (result != USLOSS_MMU_OK) {
                USLOSS_Console("Pager(): USLOSS_MmuSetAccess Error: %d\n", 
                        result);
            }

            result = USLOSS_MmuUnmap(0, page);
            if (result != USLOSS_MMU_OK) {
                USLOSS_Console("Pager(): USLOSS_MmuUnmap Error: %d\n", result);
            }

            procTable[pid].pageTable[page].accessed = 1;

            sempReal(vmStatSem);
            vmStats.new++;
            semvReal(vmStatSem);
         }

        /* Unblock waiting (faulting) process */
        MboxSend(faults[pid % MAXPROC].replyMbox, NULL, 0);
    }
    return 0;
} /* Pager */

int getPID5() {
    int pid;
    getPID_real(&pid);
    return pid;
}

void debugPageTable(int pid){
    USLOSS_Console("-Process %d Page Table-\n", pid);
    for(int i=0; i<vmStats.pages; i++){
	USLOSS_Console("Page %d -> Frame %d \n",i, procTable[pid].pageTable[i].frame);
    }
}

void debugFrameTable(int pid){
    USLOSS_Console("-Current Frame Table-\n", pid);
    for(int i=0; i<vmStats.frames; i++){
	if(frameTable[i].page != NULL)
        USLOSS_Console("Frame %d -> Process %d: Page %d: Ref %d: Dirty: %d\n"
		,i, frameTable[i].pid,frameTable[i].page->pageNum, frameTable[i].ref, frameTable[i].dirty);
	else
	USLOSS_Console("Frame unused\n");
    }
}


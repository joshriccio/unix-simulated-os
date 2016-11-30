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
static void FaultHandler(int  type, void *arg);
static int Pager(char *buf);
int getPID5();

Process procTable[MAXPROC];
FaultMsg faults[MAXPROC]; /* Note that a process can have only
                           * one fault at a time, so we can
                           * allocate the messages statically
                           * and index them by pid. */
VmStats  vmStats;
void *vmRegion;
FTE *frameTable;
int *pagerPids;
int pagerMbox;
int vmInitialized;
int vmStatSem;

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
    
    // ... more stuff goes here ...
    SemCreate(1, &vmStatSem);  // MUTEX for vmStats

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

    //TODO : mappings <= TAGS * pages 

   status = USLOSS_MmuInit(mappings, pages, frames);
   if (status != USLOSS_MMU_OK) {
      USLOSS_Console("vmInitReal: couldn't init MMU, status %d\n", status);
      abort();
   }
   USLOSS_IntVec[USLOSS_MMU_INT] = FaultHandler;


   /*
    * Initialize page tables, and fault mbox.
    */
   for (int i = 0; i < MAXPROC; i++) {
       procTable[i].pid = -1;
       procTable[i].numPages = pages;
       procTable[i].pageTable = malloc(pages * sizeof(PTE));
       
       faults[i].pid = -1;
       faults[i].replyMbox = MboxCreate(1, 0);
       faults[i].addr = NULL;
   }

   /*
    * Initialize frame tables.
    */
   frameTable = malloc(frames * sizeof(FTE));
   for (int i=0; i<frames; i++){
      frameTable[i].state = UNUSED;
      frameTable[i].pid = -1;
      frameTable[i].page = NULL;
   }   
   /*
    * Fork the pagers.
    */
   pagerPids = malloc(pagers * sizeof(int));
   char buf[100];
   pagerMbox = MboxCreate(pagers, sizeof(int));
   for (int i=0; i<frames; i++){
      pagerPids[i] = fork1("pagerProcess", Pager, buf, USLOSS_MIN_STACK, 
              PAGER_PRIORITY);
   }

   /*
    * Zero out, then initialize, the vmStats structure
    */
   int sectorSize, sectorsInTrack, numTracksOnDisk;

   diskSizeReal(1, &sectorSize, &sectorsInTrack, &numTracksOnDisk);

   memset((char *) &vmStats, 0, sizeof(VmStats));
   vmStats.pages = pages;
   vmStats.frames = frames;
   vmStats.diskBlocks = numTracksOnDisk;
   vmStats.freeFrames = frames;
   vmStats.freeDiskBlocks = numTracksOnDisk;
   vmStats.switches = 0;
   vmStats.faults = 0;
   vmStats.new = 0;
   vmStats.pageIns = 0;
   vmStats.pageOuts = 0;
   vmStats.replaced = 0;

   vmRegion = USLOSS_MmuRegion(&numPagesInVmRegion);
   vmInitialized = 1;
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
   /* 
    * Print vm statistics.
    */
   USLOSS_Console("vmStats:\n");
   USLOSS_Console("pages: %d\n", vmStats.pages);
   USLOSS_Console("frames: %d\n", vmStats.frames);
   USLOSS_Console("disk blocks: %d\n", vmStats.diskBlocks); //Changed from vmStats.blocks
   /* and so on... */

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

   assert(type == USLOSS_MMU_INT);
   cause = USLOSS_MmuGetCause();
   assert(cause == USLOSS_MMU_FAULT);

   // update vmStats
   sempReal(vmStatSem);
   vmStats.faults++;
   semvReal(vmStatSem);

   //int pid = getPID5(); //SEG FAULT
   int pid;
   getPID_real(&pid);

   if (procTable[pid % MAXPROC].pid != pid) {
       procTable[pid % MAXPROC].pid = pid;
   }

   /*
    * Fill in faults[pid % MAXPROC], send it to the pagers, and wait for the
    * reply.
    */
   faults[pid % MAXPROC].pid = pid;
   faults[pid % MAXPROC].addr = vmRegion + offset;

   MboxSend(pagerMbox, &pid, sizeof(int));

   MboxReceive(faults[pid % MAXPROC].replyMbox, NULL, 0);
   USLOSS_Console("FaultHandler(): Done\n");
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
    while(1) {
        /* Wait for fault to occur (receive from mailbox) */
        MboxReceive(pagerMbox, &pid, sizeof(int));
        /* Look for free frame */
        int freeFrame = -1;
	int page = -1;
        for(int i=0; i<vmStats.frames; i++){
	    if(frameTable[i].state == UNUSED){
                //Save frame index, break out of loop
                freeFrame = i;
		vmStats.freeFrames--;
            }
        }
        /* If there isn't one then use clock algorithm to
         * replace a page (perhaps write to disk) */
        if(freeFrame == -1){
            //No frame was found
        }else{
	    page = ((int)(long)(faults[pid % MAXPROC].addr - vmRegion)) / USLOSS_MmuPageSize();
            procTable[pid].pageTable[page].state = INCORE;
            procTable[pid].pageTable[page].frame = freeFrame;
            //update frameTable
            frameTable[freeFrame].state = 1; 
            frameTable[freeFrame].page = &procTable[pid].pageTable[page];
            frameTable[freeFrame].pid = pid;
        }
        /* Load page into frame from disk, if necessary */
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

/* ------------------------------------------------------------------------
   phase1.c

   University of Arizona
   Computer Science 452
   Fall 2016

   @author Austin George
   @author Joshua Riccio
------------------------------------------------------------------------ */

#include "phase1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */
int sentinel (char *);
extern int start1 (char *);
void dispatcher(void);
void launch();
static void checkDeadlock();
void addProcToReadyList(procPtr proc);
void printReadyList();
int getProcSlot();
void zeroProcStruct(int pid);
void removeFromChildList(procPtr process);
void removeFromQuitList(procPtr process);
void clock_handler();
int readtime();
void disableInterrupts();
void addToQuitChildList(procPtr ptr);
int getpid();
void timeSlice();
int readCurStartTime();
int isBlocked(int index);
int zap(int pid);
int isZapped();
void removeFromReadyList(procPtr process);
void unblockZappers(procPtr ptr);
/* -------------------------- Globals ------------------------------------- */

// Patrick's debugging global variable...
int debugflag = 0;

// the process table
procStruct ProcTable[MAXPROC];

// Process lists
static procPtr ReadyList;

// current process ID
procPtr Current;

// the next pid to be assigned
unsigned int nextPid = SENTINELPID;


/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - startup
   Purpose - Initializes process lists and clock interrupt vector.
             Start up sentinel process and the test process.
   Parameters - none, called by USLOSS
   Returns - nothing
   Side Effects - lots, starts the whole thing
   ----------------------------------------------------------------------- */
void startup()
{
    int result; // value returned by call to fork1()

    // initialize the process table
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): initializing process table, ProcTable[]\n");
    for (int i = 0; i < MAXPROC; i++) {
        zeroProcStruct(i);
    }

    // Initialize the Ready list, etc.
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): initializing the Ready list\n");
    ReadyList = NULL;

    // Initialize the clock interrupt handler
    USLOSS_IntVec[USLOSS_CLOCK_INT] = clock_handler;

    // startup a sentinel process
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): calling fork1() for sentinel\n");
    result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK,
                    SENTINELPRIORITY);
    if (result < 0) {
        if (DEBUG && debugflag) {
            USLOSS_Console("startup(): fork1 of sentinel returned error, ");
            USLOSS_Console("halting...\n");
        }
        USLOSS_Halt(1);
    }
  
    // start the test process
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): calling fork1() for start1\n");
    result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);
    if (result < 0) {
        USLOSS_Console("startup(): fork1 for start1 returned an error, ");
        USLOSS_Console("halting...\n");
        USLOSS_Halt(1);
    }

    USLOSS_Console("startup(): Should not see this message! ");
    USLOSS_Console("Returned from fork1 call that created start1\n");

    return;
} /* startup */

/* ------------------------------------------------------------------------
   Name - finish
   Purpose - Required by USLOSS
   Parameters - none
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void finish()
{
    if (DEBUG && debugflag)
        USLOSS_Console("in finish...\n");
} /* finish */

/* ------------------------------------------------------------------------
|  Name - fork1
|
|  Purpose - Gets a new process from the process table and initializes
|            information of the process.  Updates information in the
|            parent process to reflect this child process creation.
|
|  Parameters - the process procedure address, the size of the stack and
|               the priority to be assigned to the child process.
|
|  Returns - The process id of the created child. 
|            -1 if no child could be created or if priority is not between 
|            max and min priority. A value of -2 is returned if stacksize is 
|            less than USLOSS_MIN_STACK
|
|  Side Effects - ReadyList is changed, ProcTable is changed.
*-------------------------------------------------------------------------- */
int fork1(char *name, int (*startFunc)(char *), char *arg,
          int stacksize, int priority) {

    int procSlot = -1; // The location in process table to store PCB

    /* test if in kernel mode; halt if in user mode; disabling interrupts */
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("fork1(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }
    if (DEBUG && debugflag) {
        USLOSS_Console("fork1(): Process %s is disabling interrupts.\n", 
                name);
    }
    disableInterrupts();

    if (DEBUG && debugflag) {
        USLOSS_Console("fork1(): creating process %s\n", name);
    }

    /* Return -1 if priority is out of bounds */
    if ((nextPid != SENTINELPID) && (priority > MINPRIORITY || 
                              priority < MAXPRIORITY)) {
        if (DEBUG && debugflag) {
            USLOSS_Console("fork1(): Process %s priority is out of "
                    "bounds!\n", name);
        }
        return -1;
    }

    /* Return -2 if stack size is too small */
    if (stacksize < USLOSS_MIN_STACK) {
        if (DEBUG && debugflag) {
            USLOSS_Console("fork1(): Process %s stack size too small!\n", 
                           name);
        }
        return -2;
    }

    /* find an empty slot in the process table using getProcSlot() */
    procSlot = getProcSlot();
    if (procSlot == -1) {
        if (DEBUG && debugflag) {
            USLOSS_Console("fork1(): Process %s - no empty slot.\n", 
                           name);
        }
        return -1;
    }

    /* Halt USLOSS if process name is too long */
    if ( strlen(name) >= (MAXNAME - 1) ) {
        USLOSS_Console("fork1(): Process name is too long.  Halting...\n");
        USLOSS_Halt(1);
    }
    
    /* initializing procStruct in ProcTable for index procSlot */
    ProcTable[procSlot].pid = nextPid;
    strcpy(ProcTable[procSlot].name, name);
    ProcTable[procSlot].startFunc = startFunc; 

    /* initialization and error checking for process argument */
    if (arg == NULL) {
        ProcTable[procSlot].startArg[0] = '\0';
    } else if ( strlen(arg) >= (MAXARG - 1) ) {
        USLOSS_Console("fork1(): argument too long.  Halting...\n");
        USLOSS_Halt(1);
    } else {
        strcpy(ProcTable[procSlot].startArg, arg);
    }
    ProcTable[procSlot].stackSize = stacksize;
    if ((ProcTable[procSlot].stack = malloc(stacksize)) == NULL) {
        USLOSS_Console("fork1(): malloc fail!  Halting...\n");
        USLOSS_Halt(1);
    }
    ProcTable[procSlot].priority = priority;

    /* set parent, child, and sibling pointers */
    if (Current != NULL) {                    // Current is the parent process
        if (Current->childProcPtr == NULL) {  // Current has no children
            Current->childProcPtr = &ProcTable[procSlot];
        } else {  // Current has children
            procPtr child = Current->childProcPtr;

            /* Insert child at end of Sib List */
            while (child->nextSiblingPtr != NULL) {
                child = child->nextSiblingPtr;
            }
            child->nextSiblingPtr = &ProcTable[procSlot]; 
        }
    } 
    ProcTable[procSlot].parentPtr = Current; // value could be NULL
    
    /* Initialize context for this process, but use launch function pointer 
     * for the initial value of the process's program counter (PC) */
    USLOSS_ContextInit(&(ProcTable[procSlot].state), USLOSS_PsrGet(),
                       ProcTable[procSlot].stack,
                       ProcTable[procSlot].stackSize,
                       launch);

    p1_fork(ProcTable[procSlot].pid); // for future phase(s)
    

    /* Make process ready and add to ready list */
    ProcTable[procSlot].status = READY;
    addProcToReadyList(&ProcTable[procSlot]);

    nextPid++;  // increment for next process to start at this pid

    /* Sentinel does not call dispatcher when it is first created */
    if (ProcTable[procSlot].pid != SENTINELPID) {
        dispatcher();
    }
    return ProcTable[procSlot].pid;
} /* fork1 */

/*-------------------------------------------------------------------------
|  Name - launch
|
|  Purpose - Dummy function to enable interrupts and launch a given process
|            upon startup.
|
|  Parameters - none
|
|  Returns - nothing
|
|  Side Effects - enable interrupts
*-------------------------------------------------------------------------- */
void launch()
{
    int result;

    if (DEBUG && debugflag)
        USLOSS_Console("launch(): started\n");

    // Enable interrupts
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);

    // Call the function passed to fork1, and capture its return value
    result = Current->startFunc(Current->startArg);

    if (DEBUG && debugflag)
        USLOSS_Console("launch(): Process %d returned to launch\n", 
                Current->pid);

    quit(result);

} /* launch */


/* ------------------------------------------------------------------------
|  Name - join
|  Purpose - Wait for a child process (if one has been forked) to quit.  If 
|            one has already quit, don't wait.
|
|  Parameters - a pointer to an int where the termination code of the 
|               quitting process is to be stored.
|
|  Returns - the process id of the quitting child joined on.
|            -1 if the process was zapped in the join
|            -2 if the process has no children
|
|  Side Effects - If no child process has quit before join is called, the 
|                 parent is removed from the ready list and blocked.
*-------------------------------------------------------------------------- */
int join(int *status) {
    int childPID = -3;  // The child PID to return
    procPtr child;      // The child this process is joinging with

    /* Make sure PSR is in kernal mode */
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("join(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }

    /* Disable Interrupts */
    if (DEBUG && debugflag) {
        USLOSS_Console("join(): Process %s is disabling interrupts.\n", 
                       Current->name);
    }
    disableInterrupts();
    
    /* Process has no children */
    if (Current->childProcPtr == NULL && Current->quitChildPtr == NULL) {
        if (DEBUG && debugflag)
            USLOSS_Console("join(): Process %s has no children.\n", 
                    Current->name);
        return -2;
    }

    /* No children has called quit */
    if (Current->quitChildPtr == NULL) { 
        Current->status = JOIN_BLOCKED;
        removeFromReadyList(Current);
        if (DEBUG && debugflag) {
            USLOSS_Console("join(): %s is JOIN_BLOCKED.\n", Current->name);
            dumpProcesses();
            printReadyList();
        }
        dispatcher();
    }

    /* A child has quit and reactivated the parent */
    child = Current->quitChildPtr;
    if (DEBUG && debugflag) {
        USLOSS_Console("join(): Child %s has status of quit.\n", child->name);
        dumpProcesses();
        printReadyList();
    }
    childPID = child->pid;
    *status = child->quitStatus;
    removeFromQuitList(child);
    zeroProcStruct(childPID);

    /* Process was zapped while JOIN_BLOCKED */
    if(isZapped()){
        return -1;
    }
    return childPID;
} /* join */


/* ------------------------------------------------------------------------
|  Name - quit
|
|  Purpose - Stops the child process and notifies the parent of the death by
|            putting child quit info on the parents child completion code
|            list.
|
|  Parameters - the code to return to the grieving parent
|
|  Returns - nothing
|
|  Side Effects - changes the parent of pid child completion status list.
*-------------------------------------------------------------------------- */
void quit(int status) {
    int currentPID; // the PID of the currently running process

    /* Make sure PSR is in kernal mode */
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("quit(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }
    if (DEBUG && debugflag) {
        USLOSS_Console("quit(): Process %s is disabling interrupts.\n", 
                       Current->name);
    }
    disableInterrupts();

    if (DEBUG && debugflag) {
        USLOSS_Console("quit(): Quitting %s, status is %d.\n", 
                Current->name, status);
    }

    /* The process has an active child */
    if (Current->childProcPtr != NULL) {
        USLOSS_Console("quit(): process %d, '%s', has active children."
                        " Halting...\n", Current->pid, Current->name);
        USLOSS_Halt(1);
    }

    Current->quitStatus = status;
    Current->status = QUIT;
    removeFromReadyList(Current);

    /* For all processes that zapped this process, add to ready list and 
     * set status to READY. */
    if (isZapped()) {
        /*procPtr ptr = Current->whoZapped;
        while (ptr != NULL) {
            ptr->status = READY;
            addProcToReadyList(ptr);
            ptr = ptr->nextWhoZapped;
        }*/
        unblockZappers(Current->whoZapped);
    }

    /* The process that is quitting is a child and has its own quit child */
    if (Current->parentPtr != NULL && Current->quitChildPtr != NULL) {

        /* Clean up all children on child quit list */
        while (Current->quitChildPtr != NULL) {
            int childPID = Current->quitChildPtr->pid;
            removeFromQuitList(Current->quitChildPtr);
            zeroProcStruct(childPID);
        }

        /* Clean up self and activate parent */
        Current->parentPtr->status = READY;
        removeFromChildList(Current);
        addToQuitChildList(Current->parentPtr);
        addProcToReadyList(Current->parentPtr);
        printReadyList();                        // only prints in debug mode
        currentPID = Current->pid;

    /* Process is only a child */
    } else if (Current->parentPtr != NULL) {
        addToQuitChildList(Current->parentPtr);
        removeFromChildList(Current);
        if(Current->parentPtr->status == JOIN_BLOCKED){
           addProcToReadyList(Current->parentPtr);
           Current->parentPtr->status = READY;
        }
        printReadyList();                        // only prints in debug mode

    /* Process is a parent */
    } else {
        while (Current->quitChildPtr != NULL) {
            int childPID = Current->quitChildPtr->pid;
            removeFromQuitList(Current->quitChildPtr);
            zeroProcStruct(childPID);
        }
        currentPID = Current->pid;
        zeroProcStruct(Current->pid);
    }
    p1_quit(currentPID);
    if (DEBUG && debugflag) {
        dumpProcesses();
    }
    dispatcher();
} /* quit */

/* ------------------------------------------------------------------------
|  Name - zap
|
|  Purpose - Marks a process pid as being zapped. zape does not return until
|            the zapped process has called quit. USLOSS will halt if a 
|            process tries to zap itself or attempts to zap a nonexistent
|            process.
|
|  Parameters - pid (IN) - The process to mark as zapped. 
|
|  Returns - 0: The zapped process has called quit.
|           -1: The calling process itself was zapped while in zap.
|
|  Side Effects - The process being zapped zapped marker is set to true.
|                 The process calling zap is added to the zapped process's
|                 list of processes that have zapped it.
*-------------------------------------------------------------------------- */
int zap(int pid) {
    procPtr zapPtr; // The process to zap

    /* Make sure PSR is in kernal mode */
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("zap(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }
    if (DEBUG && debugflag) {
        USLOSS_Console("zap(): Process %s is disabling interrupts.\n", 
                       Current->name);
    }
    disableInterrupts();

    /* Current process tried to zap itself */
    if(Current->pid == pid) {
        USLOSS_Console("zap(): process %d tried to zap itself."
                       "  Halting...\n", pid);
        USLOSS_Halt(1);
    }

    /* Process to zap does not exist */
    if (ProcTable[pid % MAXPROC].status == EMPTY || 
            ProcTable[pid % MAXPROC].pid != pid) {

        USLOSS_Console("zap(): process being zapped does not exist."
                       "  Halting...\n");
        USLOSS_Halt(1);
    }
   
    /* Process to zap has finished running, but is still waiting for parent */
    if (ProcTable[pid % MAXPROC].status == QUIT) {
        if (DEBUG && debugflag) {
            USLOSS_Console("zap(): process being zapped has quit but not"
                    " joined.\n");
        }

        /* Process was zapped by another process */
        if (isZapped()) {
            return -1;
        }   
     return 0;
    }
    if (DEBUG && debugflag) {
        USLOSS_Console("zap(): Process %d is zapping process %d.\n",
                Current->pid, pid);
    }
    Current->status = ZAP_BLOCKED;
    removeFromReadyList(Current);
    zapPtr = &ProcTable[pid % MAXPROC];
    zapPtr->zapped = 1;

    /* Add this process to the list of process who have zapped the process */
    if (zapPtr->whoZapped == NULL) {
        zapPtr->whoZapped = Current;
    } else {
        procPtr ptr = zapPtr->whoZapped;
        zapPtr->whoZapped = Current;
        zapPtr->whoZapped->nextWhoZapped = ptr;
    }
    dispatcher();
    if (isZapped()) {
        return -1;
    }
    return 0;
}/* zap */

int isZapped() {
    return Current->zapped;
}

/* ------------------------------------------------------------------------
|  Name - dispatcher
|
|  Purpose - dispatches ready processes.  The process with the highest
|            priority (the first on the ready list) is scheduled to
|            run.  The old process is swapped out and the new process
|            swapped in.
|
|  Parameters - none
|
|  Returns - nothing
|
|  Side Effects - the context of the machine is changed
*------------------------------------------------------------------------- */
void dispatcher(void) {
    if (DEBUG && debugflag) {
        USLOSS_Console("dispatcher(): started.\n");
    }
    
    /* Dispacher is called for the first time for starting process (start1) */
    if (Current == NULL) {
        Current = ReadyList;
        if (DEBUG && debugflag) {
            USLOSS_Console("dispatcher(): dispatching %s.\n", Current->name);
        }
        Current->startTime = USLOSS_Clock();

        /* Enable Interrupts - returning to user code */
        USLOSS_PsrSet( USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT );
        USLOSS_ContextSwitch(NULL, &Current->state);
    } else {
        procPtr old = Current;
        if (old->status == RUNNING) {
            old->status = READY;
        }
        Current = ReadyList;
        removeFromReadyList(Current);
        Current->status = RUNNING;
        addProcToReadyList(Current);
        if (DEBUG && debugflag) {
            USLOSS_Console("dispatcher(): dispatching %s.\n", 
                    Current->name);
        }
        Current->startTime = USLOSS_Clock();
        p1_switch(old->pid, Current->pid);

        /* Enable Interrupts - returning to user code */
        USLOSS_PsrSet( USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT );
        USLOSS_ContextSwitch(&old->state, &Current->state);
    }

    if (DEBUG && debugflag){
        USLOSS_Console("dispatcher(): Printing process table");
        dumpProcesses();
    }
} /* dispatcher */


/* ------------------------------------------------------------------------
|  Name - sentinel
|
|  Purpose - The purpose of the sentinel routine is two-fold.  One
|            responsibility is to keep the system going when all other
|            processes are blocked.  The other is to detect and report
|            simple deadlock states.
|
|  Parameters - none
|
|  Returns - nothing
|  
|  Side Effects -  if system is in deadlock, print appropriate error
|                  and halt.
*------------------------------------------------------------------------- */
int sentinel (char *dummy) {
    if (DEBUG && debugflag) {
        USLOSS_Console("sentinel(): called\n");
    }
    while (1) {
        checkDeadlock();
        if (DEBUG && debugflag) {
            USLOSS_Console("sentinel(): before WaitInt()\n");
        }
        USLOSS_WaitInt();
    }
} /* sentinel */

/* ------------------------------------------------------------------------
|  Name - checkDeadlock
|
|  Purpose - Checks to determine if a deadlock has occured. In phase1, a
|            deadlock will occur if checkDeadlock is called and there
|            are any processes, other then Sentinel, with a status other 
|            then empty in the process table.
|
|  Parameters - none
|
|  Returns - nothing
|  
|  Side Effects - The USLOSS simulation is terminated. Either with an exit
|                 code of 0, if all process completed normally or an exit
|                 code of 1, a process other than Sentinel is in the process
|                 table.
*------------------------------------------------------------------------- */
static void checkDeadlock(){
    int numProc = 0; // Number of processes in the process table

    /* Check the status of every entry in the process table. Increment
     * numProc if a process status in not EMPTY
     */
    for (int i = 0; i < MAXPROC; i++) {
        if (ProcTable[i].status != EMPTY) {
            numProc++;
        }
    }

    /* A deadlock has occured */
    if(numProc > 1){
        USLOSS_Console("checkDeadlock(): numProc = %d. Only Sentinel"
                       " should be left. Halting...\n", numProc);
        USLOSS_Halt(1);
    }

    USLOSS_Console("All processes completed.\n");
    USLOSS_Halt(0);
} /* checkDeadlock */

/*
 * Disables the interrupts.
 */
void disableInterrupts()
{
    // turn the interrupts OFF iff we are in kernel mode
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        //not in kernel mode
        USLOSS_Console("Kernel Error: Not in kernel mode, may not ");
        USLOSS_Console("disable interrupts\n");
        USLOSS_Halt(1);
    } else
        // We ARE in kernel mode
        USLOSS_PsrSet( USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT );
} /* disableInterrupts */

/*---------------------------- addProcToReadyList -----------------------
|  Function addProcToReadyList
|
|  Purpose:  Adds a new process to the ready list. Process is added to 
|            the list based on priority. Lower priorities are placed at
|            the front of the list. A process is placed at the end of
|            all processes with the same pritority.
|
|  Parameters:  proc (IN) -- The process to be added to the ready list.
|
|  Returns:  None
|
|  Side Effects:  proc is added to the correct location in ready list.
*-------------------------------------------------------------------*/
void addProcToReadyList(procPtr proc) {
    if (DEBUG && debugflag){
      USLOSS_Console("addProcToReadyList(): Adding process %s to ReadyList\n",
                     proc->name);
    }
    /* Process being added is the Sentinel process */
    if (ReadyList == NULL) {
        ReadyList = proc;
    } else {

        /* all priorities in list are less than proc */
        if(ReadyList->priority > proc->priority) {
            procPtr temp = ReadyList;
            ReadyList = proc;
            proc->nextProcPtr = temp;

        /* Add process before first greater priority */
        } else {
            procPtr next = ReadyList->nextProcPtr;
            procPtr last = ReadyList;
            while (next->priority <= proc->priority) {
                last = next;
                next = next->nextProcPtr;
            }
            last->nextProcPtr = proc;
            proc->nextProcPtr = next;
        }
    }
    if (DEBUG && debugflag){
      USLOSS_Console("addProcToReadyList(): Process %s added to ReadyList\n",
                     proc->name);
     printReadyList(); 
    }
} /* addProcToReadyList */

/*---------------------------- printReadyList -----------------------
|  Function printReadyList
|
|  Purpose:  Prints a string representation of the ready list using
|            the USLOSS_Console containing name, priority of process,
|            and process ID. Debugging must be enable.
|
|  Parameters:  None
|
|  Returns:  None
*-------------------------------------------------------------------*/
void printReadyList(){
    char str[10000], str1[40];
    
    procPtr head = ReadyList;
    
    sprintf(str, "%s(%d:PID=%d)", head->name, head->priority, head->pid);

    while (head->nextProcPtr != NULL) {
        head = head->nextProcPtr;
        sprintf(str1, " -> %s(%d:PID=%d)", head->name, head->priority, 
                head->pid);
        strcat(str, str1);
    }
    if (DEBUG && debugflag){
      USLOSS_Console("printReadyList(): %s\n", str);
    }
} /* printReadyList */

/*---------------------------- getProcSlot -----------------------
|  Function getProcSlot
|
|  Purpose:  Finds an empty index in the process table (ProcTable). 
|
|  Parameters:  None
|
|  Returns:  -1 if no slot is available or the index of the next
|            empty slot in the process table.
*-------------------------------------------------------------------*/
int getProcSlot() {
    int hashedIndex = nextPid % MAXPROC;
    int counter = 0;
    while (ProcTable[hashedIndex].status != EMPTY) {
        nextPid++;
        hashedIndex = nextPid % MAXPROC;
        if (counter >= MAXPROC) {
            return -1;
        }
        counter++;
    }
    return hashedIndex;
} /* getProcSlot */

/*---------------------------- zeroProcStruct -----------------------
|  Function zeroProcStruct
|
|  Purpose:  Initializes a ProcStruct. Members are set to 0, NULL, or -1.
|            A process's quit status is set to a value of -666.
|
|  Parameters:
|      pid (IN) --  The process ID to be zeroed
|
|  Returns:  None
|
|  Side Effects:  The members of the ProcStruct for pid are changed.
*-------------------------------------------------------------------*/
void zeroProcStruct(int pid) {
    int index = pid % MAXPROC;

    ProcTable[index].pid = -1;
    ProcTable[index].stackSize = -1;
    ProcTable[index].stack = NULL; 
    ProcTable[index].priority = -1;
    ProcTable[index].status = EMPTY;
    ProcTable[index].childProcPtr = NULL;
    ProcTable[index].nextSiblingPtr = NULL;
    ProcTable[index].nextProcPtr = NULL;
    ProcTable[index].quitChildPtr = NULL;
    ProcTable[index].nextQuitSibling = NULL;
    ProcTable[index].whoZapped = NULL;
    ProcTable[index].nextWhoZapped = NULL;
    ProcTable[index].name[0] = '\0';
    ProcTable[index].startArg[0] = '\0';
    ProcTable[index].startFunc = NULL;
    ProcTable[index].parentPtr = NULL;
    ProcTable[index].quitStatus = -666;
    ProcTable[index].startTime = -1;
    ProcTable[index].zapped = 0;
} /* zeroProcStruct */

/*---------------------------- dumpProcesses -----------------------
n dumpProcesses
|
|  Purpose:  Loops through all procesess and prints all active
|            processes (non empty processes).
|
|  Parameters:
|            void
|
|  Returns:  void
|
|  Side Effects:  Process printed to screen using USLOSS_Console
*-------------------------------------------------------------------*/
void dumpProcesses(){
    char *ready = "READY";
    char *running = "RUNNING";
    char *blocked = "BLOCKED";
    char *join_blocked = "JOIN_BLOCKED";
    char *quit = "QUIT";
    char *zap_blocked = "ZAP_BLOCKED";
    USLOSS_Console("\n     PID       Name   Priority        Status     Parent\n");
    for(int i=0; i<50; i++){
        char buf[30];
        char *status = buf;
        char *parent;
        if(ProcTable[i].status != EMPTY){
           switch(ProcTable[i].status) {
               case READY : status = ready;
                   break;
               case RUNNING : status = running;
                   break;
               case BLOCKED  : status = blocked;
                   break;
               case JOIN_BLOCKED : status = join_blocked;
                   break;
               case QUIT  : status = quit;
                   break;               
               case ZAP_BLOCKED  : status = zap_blocked;
                   break;
               default : sprintf(status, "%d", ProcTable[i].status);
           }
           if(ProcTable[i].parentPtr != NULL){
               parent = ProcTable[i].parentPtr->name;
           }else{
               parent = "NULL";
           }
           USLOSS_Console("%8d %10s %10d %13s %10s\n", ProcTable[i].pid, 
                          ProcTable[i].name, ProcTable[i].priority, 
                          status, parent); 
        }
    }
}/* dumpProcesses */

/*------------------------------------------------------------------
|  Function removeFromChildList
|
|  Purpose:  Finds process in parent's childlist, removes process, reasigns
|            all important processes
|
|  Parameters:
|            procPtr process, process to be deleted
|
|  Returns:  void
|
|  Side Effects:  Process is removed from parent's childList
*-------------------------------------------------------------------*/
void removeFromChildList(procPtr process) {
    procPtr temp = process;
    // process is at the head of the linked list
    if (process == process->parentPtr->childProcPtr) {
        process->parentPtr->childProcPtr = process->nextSiblingPtr;
    } else { // process is in the middle or end of linked list
        temp = process->parentPtr->childProcPtr;
        while (temp->nextSiblingPtr != process) {
            temp = temp->nextSiblingPtr;
        }
        temp->nextSiblingPtr = temp->nextSiblingPtr->nextSiblingPtr;
    }
    if (DEBUG && debugflag) {
       USLOSS_Console("removeFromChildList(): Process %d removed.\n", 
                      temp->pid);
    }
}/* removeFromChildList */

/*------------------------------------------------------------------
|  Function removeFromQuitList
|
|  Purpose: Removes process from parent's quit list 
|
|  Parameters:
|            procPtr process, process to be removed
|
|  Returns:  void
|
|  Side Effects:  Process is removed from parent's quitList
*-------------------------------------------------------------------*/
void removeFromQuitList(procPtr process) {
    process->parentPtr->quitChildPtr = process->nextQuitSibling;

    if (DEBUG && debugflag) {
       USLOSS_Console("removeFromQuitList(): Process %d removed.\n", 
                      process->pid);
    }
}/* removeFromQuitList */

void clock_handler() {
    timeSlice();
}

/*------------------------------------------------------------------
|  Function addToQuitChildList
|
|  Purpose:  Adds a process to it's parent's quit child list
|
|  Parameters:
|            procPtr ptr - the parent process to add the child to
|
|  Returns:  void
|
|  Side Effects: the process is added back of the quit child list
*-------------------------------------------------------------------*/
void addToQuitChildList(procPtr ptr) {
    if (ptr->quitChildPtr == NULL) {
        ptr->quitChildPtr = Current;
        return;
    }
    procPtr child = ptr->quitChildPtr;
    while (child->nextQuitSibling != NULL) {
        child = child->nextQuitSibling;
    }
    child->nextQuitSibling = Current;
}/* addToQuitChildList */

int getpid(){
    return Current->pid;
}

int readCurStartTime() {
    return Current->startTime;
}

void timeSlice() {
    if (readtime() >= TIME_SLICE) {
        dispatcher();
    }
    return;
}

int readtime() {
    return USLOSS_Clock() - readCurStartTime();
}

int isBlocked(int index) {
    if (ProcTable[index].status > 7) {
        return 1;
    }
    return 0;
}

/*------------------------------------------------------------------
|  Function blockMe
|
|  Purpose:  Blocks a process and removes from Readylist
|
|  Parameters:
|            int newStatus - the status for the process to block on
|
|  Returns:  int - the return code
|
|  Side Effects:  Process status is changed, removed from readyList
*-------------------------------------------------------------------*/
int blockMe(int newStatus){
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("blockMe(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }
    if (DEBUG && debugflag) {
        USLOSS_Console("blockMe(): Process %s is disabling interrupts.\n", 
                       Current->name);
    }
    disableInterrupts();

    if(newStatus < 11){
        USLOSS_Console("blockMe(): called with invalid status of %d."
                       " Halting...\n", newStatus);
        USLOSS_Halt(1);
      USLOSS_Halt(1);
    }
    Current->status = newStatus;
    removeFromReadyList(Current);
    dispatcher();
    if (DEBUG && debugflag) {
        USLOSS_Console("blockMe(): Process %s is unblocked.\n", 
                       Current->name);
    }
    if(isZapped()){
      return -1;
    }
    return 0;
}/*blockMe */

/*------------------------------------------------------------------
|  Function unBlockProc
|
|  Purpose:  Unblocks a process blocked by blockMe
|
|  Parameters:
|            int pid - the pid of the process to unblock
|
|  Returns:  int - the return code
|
|  Side Effects:  Process status is changed, added back to readyList
*-------------------------------------------------------------------*/
int unblockProc(int pid){
    if (ProcTable[pid % MAXPROC].pid != pid) {
        return -2;
    }
    if (Current->pid == pid) {
        return -2;
    }
    if (ProcTable[pid % MAXPROC].status < 11) {
        return -2;
    }
    if (isZapped()) {
        return -1;
    }
    ProcTable[pid % MAXPROC].status = READY;
    addProcToReadyList(&ProcTable[pid % MAXPROC]);
    dispatcher();
    return 0;
}/* unblockProc */

/*------------------------------------------------------------------
|  Function removeFromReadyList
|
|  Purpose:  Finds process in ReadyList, removes process, reasigns
|            all important processes|
|  
|  Parameters:
|            procPtr process, process to be deleted
|
|  Returns:  void
|
|  Side Effects:  Process is removed from ReadyList
*-------------------------------------------------------------------*/
void removeFromReadyList(procPtr process) {
    if(process == ReadyList){
        ReadyList = ReadyList->nextProcPtr;
    }else{
        procPtr proc = ReadyList;
        while(proc->nextProcPtr != process){
            proc = proc->nextProcPtr;
        }
        proc->nextProcPtr = proc->nextProcPtr->nextProcPtr;
    }
    if (DEBUG && debugflag) {
        USLOSS_Console("removeFromReadyList(): Process %d removed from"
                       " ReadyList.\n", process->pid);
    }
}/* removeFromReadyList */

void unblockZappers(procPtr ptr) {
    if (ptr == NULL) {
        return;
    }
    unblockZappers(ptr->nextWhoZapped);
    ptr->status = READY;
    addProcToReadyList(ptr);
}

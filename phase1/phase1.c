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
int allChildrenQuit(procPtr parent); // TODO: possibly remove
procPtr firstChildWithStatus(procPtr parent, int status);
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
   Name - fork1
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.
   Returns - the process id of the created child or -1 if no child could
             be created or if priority is not between max and min priority.
   Side Effects - ReadyList is changed, ProcTable is changed, Current
                  process information changed
   ------------------------------------------------------------------------ */
int fork1(char *name, int (*startFunc)(char *), char *arg,
          int stacksize, int priority)
{
    // test if in kernel mode; halt if in user mode; disabling interrupts
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("fork1(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }
    if (DEBUG && debugflag) {
        USLOSS_Console("fork1(): Process %s is disabling interrupts.\n", name);
    }
    disableInterrupts();

    int procSlot = -1;

    if (DEBUG && debugflag)
        USLOSS_Console("fork1(): creating process %s\n", name);

    // Return if stack size is too small
    if (stacksize < USLOSS_MIN_STACK) {
        if (DEBUG && debugflag) {
            USLOSS_Console("fork1(): Process %s stack size too small!\n", 
                           name);
        }
        return -2;
    }

    // find an empty slot in the process table
    procSlot = getProcSlot();
    if (procSlot == -1) {
        if (DEBUG && debugflag) {
            USLOSS_Console("fork1(): Process %s - no empty slot.\n", 
                           name);
        }
        return -1;
    }

    // fill-in entry in process table
    if ( strlen(name) >= (MAXNAME - 1) ) {
        USLOSS_Console("fork1(): Process name is too long.  Halting...\n");
        USLOSS_Halt(1);
    }
    
    // initializing procStruct in ProcTable
    ProcTable[procSlot].pid = nextPid;
    strcpy(ProcTable[procSlot].name, name);
    ProcTable[procSlot].startFunc = startFunc; 
    // check argument
    if (arg == NULL) {
        ProcTable[procSlot].startArg[0] = '\0';
    } else if ( strlen(arg) >= (MAXARG - 1) ) {
        USLOSS_Console("fork1(): argument too long.  Halting...\n");
        USLOSS_Halt(1);
    } else {
        strcpy(ProcTable[procSlot].startArg, arg);
    }
    ProcTable[procSlot].stackSize = stacksize;
    ProcTable[procSlot].stack = malloc(stacksize);
    ProcTable[procSlot].priority = priority;
    // setting parent, child, and sibling pointers
    if (Current != NULL) { // Current is the parent process
        if (Current->childProcPtr == NULL) {  // Current has no children
            Current->childProcPtr = &ProcTable[procSlot];
        } else {  // Current has children
            procPtr child = Current->childProcPtr;
            while (child->nextSiblingPtr != NULL) { // Find last sibling in L 
                child = child->nextSiblingPtr;
            }
            // Insert child at end of Sib List
            child->nextSiblingPtr = &ProcTable[procSlot]; 
        }
    } 
    ProcTable[procSlot].parentPtr = Current; // value could be NULL
    
    // Initialize context for this process, but use launch function pointer for
    // the initial value of the process's program counter (PC)
    USLOSS_ContextInit(&(ProcTable[procSlot].state), USLOSS_PsrGet(),
                       ProcTable[procSlot].stack,
                       ProcTable[procSlot].stackSize,
                       launch);

    // for future phase(s)
    p1_fork(ProcTable[procSlot].pid);

    // Make process ready and add to ready list
    ProcTable[procSlot].status = READY;
    addProcToReadyList(&ProcTable[procSlot]);

    nextPid++;  // increment for next process to start at this pid

    if (ProcTable[procSlot].pid != SENTINELPID) {
        dispatcher();
    }

    return ProcTable[procSlot].pid;
} /* fork1 */

/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts
   ------------------------------------------------------------------------ */
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
   Name - join
   Purpose - Wait for a child process (if one has been forked) to quit.  If 
             one has already quit, don't wait.
   Parameters - a pointer to an int where the termination code of the 
                quitting process is to be stored.
   Returns - the process id of the quitting child joined on.
             -1 if the process was zapped in the join
             -2 if the process has no children
   Side Effects - If no child process has quit before join is called, the 
                  parent is removed from the ready list and blocked.
   ------------------------------------------------------------------------ */
int join(int *status)
{
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("join(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }
    
    int childPID = -3;
    procPtr child;
    // Process has no children
    if (Current->childProcPtr == NULL && Current->quitChildPtr == NULL) {
        if (DEBUG && debugflag)
            USLOSS_Console("join(): Process %s has no children.\n", 
                    Current->name);
        return -2;
    }

    // Process has a child but without a status of quitldPtr;
    if (Current->quitChildPtr == NULL) { 
        Current->status = JOIN_BLOCKED;
        ReadyList = ReadyList->nextProcPtr;
        if (DEBUG && debugflag) {
            USLOSS_Console("join(): %s is JOIN_BLOCKED.\n", Current->name);
            dumpProcesses();
            printReadyList();
        }
        dispatcher();
    }
    //Process was zapped while JOIN_BLOCKED 
    if(isZapped()){
        return -1;
    }
    // A child has quit and reactivated the parent
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
    return childPID;
} /* join */


/* ------------------------------------------------------------------------
   Name - quit
   Purpose - Stops the child process and notifies the parent of the death by
             putting child quit info on the parents child completion code
             list.
   Parameters - the code to return to the grieving parent
   Returns - nothing
   Side Effects - changes the parent of pid child completion status list.
   ------------------------------------------------------------------------ */
void quit(int status)
{
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("quit(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }

    if (DEBUG && debugflag)
        USLOSS_Console("quit(): Quitting %s, status is %d.\n", 
                Current->name, status);

    if (Current->childProcPtr != NULL) { // The process has an active child
        USLOSS_Console("quit(): process %d, '%s', has active children."
                        " Halting...\n", Current->pid, Current->name);
        USLOSS_Halt(1);
    }

    if (isZapped()) {
        Current->whoZapped->status = READY;
        addProcToReadyList(Current->whoZapped);
    }

    Current->quitStatus = status;
    Current->status = QUIT;
    removeFromReadyList(Current);
    //ReadyList = ReadyList->nextProcPtr; // take off ready list
    int currentPID;
    // The process that is quitting is a child
    if (Current->parentPtr != NULL) {
        Current->parentPtr->status = READY;
        addToQuitChildList(Current->parentPtr);
        removeFromChildList(Current);
        addProcToReadyList(Current->parentPtr);
        printReadyList();
    } else {  // process is a parent
        while (Current->quitChildPtr != NULL) {
            int childPID = Current->quitChildPtr->pid;
            removeFromQuitList(Current->quitChildPtr);
            zeroProcStruct(childPID);
        }
        currentPID = Current->pid;
        zeroProcStruct(Current->pid);
    }
    p1_quit(currentPID);
    if (DEBUG && debugflag)
        dumpProcesses();
    dispatcher();
} /* quit */

// zap
int zap(int pid) {
    procPtr zapPtr;
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        USLOSS_Console("zap(): called while in user mode, by process %d."
                       " Halting...\n", Current->pid);
        USLOSS_Halt(1);
    }
    if(Current->pid == pid) {
        USLOSS_Console("zap(): Process %d tried to zap self."
                       " Halting...\n", pid);
        USLOSS_Halt(1);
    }
    if (ProcTable[pid % MAXPROC].status == EMPTY || 
            ProcTable[pid % MAXPROC].pid != pid) {

        USLOSS_Console("zap(): Process %d does not exist. Halting...\n", pid);
        USLOSS_Halt(1);
    }
    if (DEBUG && debugflag)
        USLOSS_Console("zap(): Process %d is zapping process %d.\n",
                Current->pid, pid);
    Current->status = ZAP_BLOCKED;
    ReadyList = ReadyList->nextProcPtr;
    zapPtr = &ProcTable[pid % MAXPROC];
    zapPtr->zapped = 1;
    zapPtr->whoZapped = Current;
    dispatcher();
    if (isZapped()) {
        return -1;
    }
    return 0;
}

int isZapped() {
    return Current->zapped;
}

/* ------------------------------------------------------------------------
   Name - dispatcher
   Purpose - dispatches ready processes.  The process with the highest
             priority (the first on the ready list) is scheduled to
             run.  The old process is swapped out and the new process
             swapped in.
   Parameters - none
   Returns - nothing
   Side Effects - the context of the machine is changed
   ----------------------------------------------------------------------- */
void dispatcher(void)
{
    if (DEBUG && debugflag)
        USLOSS_Console("dispatcher(): started.\n");

    if (Current == NULL) { // Dispatcher called for first time
        Current = ReadyList;
        if (DEBUG && debugflag)
            USLOSS_Console("dispatcher(): dispatching %s.\n", Current->name);
        Current->startTime = USLOSS_Clock();
        // enable interrupts
        USLOSS_PsrSet( USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT );
        USLOSS_ContextSwitch(NULL, &Current->state);
    } else {
        procPtr old = Current;
        Current = ReadyList;
        if (DEBUG && debugflag)
            USLOSS_Console("dispatcher(): dispatching %s.\n", 
                    Current->name);
        Current->startTime = USLOSS_Clock();
        p1_switch(old->pid, Current->pid);
        // enable interrupts
        USLOSS_PsrSet( USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT );
        USLOSS_ContextSwitch(&old->state, &Current->state);
    }
    if (DEBUG && debugflag){
        USLOSS_Console("dispatcher(): Printing process table");
        dumpProcesses();
    }
} /* dispatcher */


/* ------------------------------------------------------------------------
   Name - sentinel
   Purpose - The purpose of the sentinel routine is two-fold.  One
             responsibility is to keep the system going when all other
             processes are blocked.  The other is to detect and report
             simple deadlock states.
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in deadlock, print appropriate error
                   and halt.
   ----------------------------------------------------------------------- */
int sentinel (char *dummy)
{
    if (DEBUG && debugflag)
        USLOSS_Console("sentinel(): called\n");
    while (1)
    {
        checkDeadlock();
        if (DEBUG && debugflag)
            USLOSS_Console("sentinel(): before WaitInt()\n");
        USLOSS_WaitInt();
    }
} /* sentinel */


/* check to determine if deadlock has occurred... */
static void checkDeadlock()
{
    if (ProcTable[0].status != EMPTY) {
        USLOSS_Console("checkDeadLock(): numProc = %d. Only Sentinel"
                       " should be left. Halting...\n", ProcTable[0].pid);
        USLOSS_Halt(1);
    }
    for (int i = 2; i < MAXPROC; i++) {
        if (ProcTable[i].status != EMPTY) { // process is blocked in any way
            USLOSS_Console("checkDeadLock(): numProc = %d. Only Sentinel"
                           " should be left. Halting...\n", ProcTable[i].pid);
            USLOSS_Halt(1);
        }
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
|            the front of the list.
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

    if (ReadyList == NULL) {
        ReadyList = proc; //In this case proc is the sentinel process
    } else {
        // all priorities in list are less than proc
        if(ReadyList->priority > proc->priority) {
            procPtr temp = ReadyList;
            ReadyList = proc;
            proc->nextProcPtr = temp;
        } else { // add proc before first greater priority
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

}

/*---------------------------- printReadyList -----------------------
|  Function printReadyList
|
|  Purpose:  Prints a string representation of the ready list using
|            the USLOSS_Console containing name and priority of process.
|            Debugging must be enable.
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
        sprintf(str1, " -> %s(%d:PID=%d)", head->name, head->priority, head->pid);
        strcat(str, str1);
    }

    if (DEBUG && debugflag){
      USLOSS_Console("printReadyList(): %s\n", str);
    }
}

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
}

/*---------------------------- zeroProcStruct -----------------------
|  Function zeroProcStruct
|
|  Purpose:  Initializes a ProcStruct. Members are set to 0 or NULL,
|            except in the case of priority which is set to the highest
|            priority of five.
|
|  Parameters:
|      index (IN) --  The index of the ProcStruct in the ProcTable
|
|  Returns:  None
|
|  Side Effects:  The members of the ProcStruct at index are changed.
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
    //ProcTable[index].zapPtr = NULL;
    ProcTable[index].whoZapped = NULL;
    ProcTable[index].name[0] = '\0';
    ProcTable[index].startArg[0] = '\0';
    ProcTable[index].startFunc = NULL;
    ProcTable[index].parentPtr = NULL;
    ProcTable[index].quitStatus = -666;
    ProcTable[index].startTime = -1;
    ProcTable[index].zapped = 0;

}

int allChildrenQuit(procPtr parent) {
    if (parent->childProcPtr != NULL) { // parent has a child
        procPtr child = parent->childProcPtr;
        while(child != NULL) {
            if (child->status != QUIT) {
                return 0;
            }
            child = child->nextSiblingPtr;
        }
    }
    return 1;
}

procPtr firstChildWithStatus(procPtr parent, int status) {
    if (parent->childProcPtr != NULL) { // parent has a child
        procPtr child = parent->childProcPtr;
        while(child != NULL) {
            if (child->status == status) {
                return child;
            }
            child = child->nextSiblingPtr;
        }
    }
    return NULL;
}

void dumpProcesses(){
    char *ready = "READY";
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
}

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
}

void removeFromQuitList(procPtr process) {
    process->parentPtr->quitChildPtr = process->nextQuitSibling;

    if (DEBUG && debugflag) {
       USLOSS_Console("removeFromQuitList(): Process %d removed.\n", 
                      process->pid);
    }
}

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
       USLOSS_Console("removeFromReadyList(): Process %d removed from ReadyList.\n",
                      process->pid);
    }
}


void clock_handler() {
    timeSlice();
}


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
}

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

int blockMe(int newStatus){
    if(newStatus < 10){
      //print error message
      USLOSS_Halt(1);
    }
    Current->status = newStatus;
    //Remove from ready list
    dispatcher();
    if(isZapped()){
      return -1;
    }
    return 0;
}

int unblockProc(int pid){
return 0;
}


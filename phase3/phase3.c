/* ------------------------------------------------------------------------
   phase3.c

   University of Arizona
   Computer Science 452

   @author Joshua Riccio
   @author Austin George
   ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <sems.h>
#include <libuser.h>
#include <string.h>

/* ------------------------- Prototypes ----------------------------------- */
void setUserMode();
void spawn(systemArgs *args);
int spawnReal(char *name, int (* userFunc)(char *), char *arg, int stackSize, 
        int priority);
int spawnLaunch(char *arg);
void checkKernelMode(char * processName);
void wait(systemArgs *args);
void terminate(systemArgs *args);
int waitReal(int *status);
void addChildToList(procPtr3 child);
void removeFromChildList(procPtr3 process);
void semCreate(systemArgs *args);
void addToSemBlockList(procPtr3 process, int semIndex);
void semP(systemArgs *args);
void semV(systemArgs *args);
void semFree(systemArgs *args);
void getPid(systemArgs *args);
void getTimeOfDay(systemArgs *args);
void cpuTime(systemArgs *args);

extern int start3(char *arg);

/* -------------------------- Globals ------------------------------------- */
procStruct3 procTable[MAXPROC]; // Process Table

semStruct semTable[MAXSEMS]; // Semaphore Table

/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start2
   Purpose - Initializes process table, semaphore table, and system call 
             vector.
   Parameters - arg: function arguments. not used.
   Returns - int: zero for a normal quit. Should not be used.
   Side Effects - lots since it initializes the phase3 data structures.
   ----------------------------------------------------------------------- */
int start2(char *arg) {
    int pid;    // start3 process ID
    int status; // start3 quit status

    /* Check kernel mode here. */
    checkKernelMode("start2");

    // initialize all sturcts in the process table to EMPTY
    for (int i = 0; i < MAXPROC; i++) {
        procTable[i].status = EMPTY;
    }

    // initialize semaphore table
    for (int i = 0; i < MAXSEMS; i++) {
        semTable[i].status = EMPTY;
    }

    // initialize systemCallVec to system call functions
    systemCallVec[SYS_SPAWN] = spawn;
    systemCallVec[SYS_WAIT] = wait;
    systemCallVec[SYS_TERMINATE] = terminate;
    systemCallVec[SYS_SEMCREATE] = semCreate;
    systemCallVec[SYS_SEMP] = semP;
	systemCallVec[SYS_SEMV] = semV;
	systemCallVec[SYS_SEMFREE] = semFree;
	systemCallVec[SYS_GETPID] = getPid;
	systemCallVec[SYS_GETTIMEOFDAY] = getTimeOfDay;
	systemCallVec[SYS_CPUTIME] = cpuTime;

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscallHandler; spawnReal is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes USLOSS_Syscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawnReal().
     *
     * Here, we only call spawnReal(), since we are already in kernel mode.
     *
     * spawnReal() will create the process by using a call to fork1 to
     * create a process executing the code in spawnLaunch().  spawnReal()
     * and spawnLaunch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawnReal() will
     * return to the original caller of Spawn, while spawnLaunch() will
     * begin executing the function passed to Spawn. spawnLaunch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawnReal() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */
    pid = spawnReal("start3", start3, NULL, USLOSS_MIN_STACK, 3);

    // failed to create start3 process
    if (pid < 0) {
        quit(pid);
    }

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    pid = waitReal(&status);

    // failed to join with start3 child process
    if (pid < 0) {
        quit(pid);
    }

    quit(0);
    return 0;
} /* start2 */

/* ------------------------------------------------------------------------
   Name - spawn
   Purpose - Create a user-level process
   Parameters - systemArgs *args, the arguments passed from libuser.c
             args->arg1: address of the function to spawn
             args->arg2: parameter passed to spawned function
             args->arg3: stack size (in bytes)
             args->arg4: priority
             args->arg5: character string containing process’s name
   Returns - void, sets arg values
             args->arg1: PID of the newly created process; -1 if a process 
                         could not be created
             args->arg2: -1 if illegal values are given as input; 0 otherwise
   Side Effects - spawnReal is called using the above parameters
   ----------------------------------------------------------------------- */
void spawn(systemArgs *args) {
    long pid;

    // check for invalid arguments
    if ((long) args->number != SYS_SPAWN) {
        args->arg4 = (void *) -1;
        return;
    }
    if ((long) args->arg3 < USLOSS_MIN_STACK) {
        args->arg4 = (void *) -1;
        return;
    }
    if ((long) args->arg4 > MINPRIORITY || (long) args->arg4 < MAXPRIORITY ) {
        args->arg4 = (void *) -1;
        return;
    }

    pid = spawnReal((char *) args->arg5, args->arg1, args->arg2, 
            (long) args->arg3, (long) args->arg4);

    args->arg1 = (void *) pid; // newly created process id; or -1
    args->arg4 = (void *) 0;   // inputs to function valid
    setUserMode();
}

/* ------------------------------------------------------------------------
   Name - spawnReal
   Purpose - Called by spawn to create a user-level process using the phase1
             function call
   Parameters - userFunc: address of the function to spawn
                     arg: parameter passed to spawned function
               stackSize: stack size (in bytes)
                priority: priority
                    name: character string containing process’s name
   Returns - int: PID of the newly created process; -1 if a process could not 
                  be created
   Side Effects - process information is added to the process table
   ----------------------------------------------------------------------- */
int spawnReal(char *name, int (* userFunc)(char *), char *arg, int stackSize, 
        int priority) {

    int childPID;  // child pid to return
    int mailboxID; // private mailbox for newly created process

    childPID = fork1(name, spawnLaunch, arg, stackSize, priority);

    if (childPID < 0) {  // error durnin fork1
        return childPID;
    }

    // If parent process has a higher priority then child, parent will
    // initialize process table for child
    if (procTable[childPID % MAXPROC].status == EMPTY) {
        mailboxID = MboxCreate(0, 0);
        procTable[childPID % MAXPROC].mboxID = mailboxID;
        procTable[childPID % MAXPROC].status = ACTIVE;
    } else {
        mailboxID = procTable[childPID % MAXPROC].mboxID;
    }

    // add child information to the process table
    procTable[childPID % MAXPROC].pid = childPID;
    strcpy(procTable[childPID % MAXPROC].name, name);
    procTable[childPID % MAXPROC].priority = priority;
    procTable[childPID % MAXPROC].userFunc = userFunc;
    procTable[childPID % MAXPROC].childProcPtr = NULL;
    procTable[childPID % MAXPROC].nextSiblingPtr = NULL;
    procTable[childPID % MAXPROC].nextSemBlock = NULL;
    if (arg == NULL) {
        procTable[childPID % MAXPROC].startArg[0] = 0;
    } else {
        strcpy(procTable[childPID % MAXPROC].startArg, arg);
    }
    procTable[childPID % MAXPROC].stackSize = stackSize;

    // start2 should not add its information to process table
    if (getpid() != START2_PID) {
        procTable[childPID % MAXPROC].parentPtr = 
            &procTable[getpid() % MAXPROC];
        addChildToList(&procTable[childPID % MAXPROC]);
    }

    // wake up child if blocked in spawnLaunch
    MboxCondSend(mailboxID, NULL, 0); 

    return childPID;
}

/* ------------------------------------------------------------------------
   Name - spawnLaunch
   Purpose - Called by phase1 Launch to execute the user-level process code
             passed to spawn.
   Parameters - arg: parameter passed to spawned function
   Returns - int: no value is returned
   Side Effects - If process does not call Terminate, spawnLaunch will call
                  Terminate
   ----------------------------------------------------------------------- */
int spawnLaunch(char *arg) {
    int mailboxID;           // process private mailbox
    int pid = getpid();      // process ID
    int userFuncReturnValue; // value returned from userFunc

    // parent has not set up process table, child will set up
    if (procTable[pid % MAXPROC].status == EMPTY) {
        procTable[pid % MAXPROC].status = ACTIVE;
        mailboxID = MboxCreate(0, 0);
        procTable[pid % MAXPROC].mboxID = mailboxID;
        MboxReceive(mailboxID, NULL, 0);
    }

    // Terminate if child was zapped while blocked waiting for parent
    if (isZapped()) {
        setUserMode();
        Terminate(99);
    }

    setUserMode();

    // calls userFunc for child process to execute
    userFuncReturnValue = procTable[pid % MAXPROC].userFunc(
            procTable[pid % MAXPROC].startArg);
    
    Terminate(userFuncReturnValue);
    return 0;
}

/* ------------------------------------------------------------------------
   Name - wait
   Purpose - Wait for a child process to terminate.
   Parameters - None
   Returns - void, sets arg values
             args->arg1: process ID of terminating child
             args->arg2: termination code of the child
   Side Effects - process is blocked if no children have terminated
   ----------------------------------------------------------------------- */
void wait(systemArgs *args) {
    int status;
    long kidPID;

    // check if correct system call
    if ((long) args->number != SYS_WAIT) {
        args->arg2 = (void *) -1;
        return;
    }

    kidPID = waitReal(&status);

    procTable[getpid() % MAXPROC].status = ACTIVE;
    
    if (kidPID == -2) {
        args->arg1 = (void *) 0;
        args->arg2 = (void *) -2;
    } else {
        args->arg1 = (void *) kidPID;
        args->arg2 = ((void *) (long) status);
    }
    setUserMode();
}

/* ------------------------------------------------------------------------
   Name - waitReal
   Purpose - Called by wait. Sets process table status of calling process and
             calls phase1 join.
   Parameters - status: the termination code of the child
   Returns - int: process ID of terminating child
   Side Effects - process is blocked if no children have terminated
   ----------------------------------------------------------------------- */
int waitReal(int *status) {
    procTable[getpid() % MAXPROC].status = WAIT_BLOCK;
    return join(status);
}

/* ------------------------------------------------------------------------
   Name - terminate
   Purpose - Terminates the invoking process and all of its children, and 
             synchronizes with its parent’s Wait system call. Processes are 
             terminated by zap’ing them.
   Parameters - systemArgs *args, the arguments passed from libuser.c
                args->arg1: termination code for the process
   Returns - None
   Side Effects - process status in process table is set to EMPTY
   ----------------------------------------------------------------------- */
void terminate(systemArgs *args) {
    procPtr3 parent = &procTable[getpid() % MAXPROC]; // the calling process

    // if the process has children, zap them
    if (parent->childProcPtr != NULL) {
        while (parent->childProcPtr != NULL) {
            zap(parent->childProcPtr->pid);
        }
    }

    // When children call terminate they remove themselves from their parents
    if (parent->pid != START3_PID && parent->parentPtr != NULL) {
        removeFromChildList(&procTable[getpid() % MAXPROC]);
    }

    parent->status = EMPTY; // process should no longer be used
    quit(((int) (long) args->arg1));
}

/* ------------------------------------------------------------------------
   Name - semCreate
   Purpose - Creates a user-level semaphore.
   Parameters - systemArgs *args, the arguments passed from libuser.c
                args->arg1: initial semaphore value
   Returns - void, sets arg values
             args->arg1: index of semaphore
             args->arg4: -1 if initial value is negative or no semaphore
                         available; 0 otherwise.
   Side Effects - semaphore is created in the semaphore table
   ----------------------------------------------------------------------- */
void semCreate(systemArgs *args) {
    int index; // index of newly created semaphore

    // initial semaphore value cannot be negaive
    if ((long) args->arg1 < 0) {
        args->arg4 = ((void *) (long) -1);
        return;
    }

    // find empty semaphore in the semaphore table
    for (index = 0; index < MAXSEMS; index++) {
        if (semTable[index].status == EMPTY) {
            break;
        }
    }

    // no available semaphore
    if (index == MAXSEMS) {
        args->arg4 = ((void *) (long) -1);
        return;
    }

    // initialize values of semaphore
    semTable[index].status = ACTIVE;
    semTable[index].count = (long) args->arg1;
    semTable[index].blockedList = NULL;
    semTable[index].mboxID = MboxCreate(1, 0);

    // return values set in args
    args->arg1 = ((void *) (long) index);
    args->arg4 = ((void *) (long) 0);
    setUserMode();
}

/* ------------------------------------------------------------------------
   Name - semP
   Purpose - Decrements the semaphore count by one. Blocks the process if the
             count is zero.
   Parameters - systemArgs *args, the arguments passed from libuser.c
                args->arg1: index of semaphore in semaphore table
   Returns - void, sets arg values
             args->arg4: -1 if semaphore index is invalid, 0 otherwise.
   Side Effects - Places a process on the block semphore list, if the count
                  is zero.
   ----------------------------------------------------------------------- */
void semP(systemArgs *args) {
    int semIndex = ((int) (long) args->arg1); // index to semaphore table

	// if the index requested is an invalid index, return -1 through arg4
    if (semIndex < 0 || semIndex > MAXSEMS) {
        args->arg4 = ((void *) (long) -1);
        return;
    }

    semStruct *semaphore = &semTable[semIndex];

	// if the requested semaphore is not active, return -1 through arg4
    if (semaphore->status == EMPTY) {
        args->arg4 = ((void *) (long) -1);
        return;
    }

    // process enters semaphore critical section
    MboxSend(semaphore->mboxID, NULL, 0);

    // block process on the semaphore block list
    if (semaphore->count < 1) {
        addToSemBlockList(&procTable[getpid() % MAXPROC], semIndex);
        MboxReceive(semaphore->mboxID, NULL, 0); // release mutex

        // block process on private mailbox
        MboxReceive(procTable[getpid() % MAXPROC].mboxID, NULL, 0);

        // mailbox was released while process was waiting to enter
        if (semaphore->status == EMPTY) {
            setUserMode();
            Terminate(1);
        }
    } else {
        MboxReceive(semaphore->mboxID, NULL, 0); // release mutex
    }
    semaphore->count--;
    args->arg4 = ((void *) (long) 0);
    setUserMode();
}

/* ------------------------------------------------------------------------
   Name - semV
   Purpose - Increments the semaphore count by one.
   Parameters - systemArgs *args, the arguments passed from libuser.c
   Returns - void, sets arg values
   Side Effects - Wakes up any blocked semaphores if there are any blocked.
   ----------------------------------------------------------------------- */
void semV(systemArgs *args) {
    int semIndex = ((int) (long) args->arg1);
	
	//If the index requested is an invalid index, return
    if (semIndex < 0 || semIndex > MAXSEMS) {
        args->arg4 = ((void *) (long) -1);
        return;
    }

    semStruct *semaphore = &semTable[semIndex];

	//If the requested semaphore is not active, return
    if (semaphore->status == EMPTY) {
        args->arg4 = ((void *) (long) -1);
        return;
    }
	
	//Enter mutex area, no other processes may enter while in.
    MboxSend(semaphore->mboxID, NULL, 0);
	//increment semaphore count
    semaphore->count++;
    
	//Wake up the first blocked process if one exists
    if (semaphore->blockedList != NULL) {
        int blockProcMboxID = semaphore->blockedList->mboxID;
        semaphore->blockedList = semaphore->blockedList->nextSemBlock;
        //Release mutex
		MboxReceive(semaphore->mboxID, NULL, 0);
        MboxSend(blockProcMboxID, NULL, 0);
    } else {
        MboxReceive(semaphore->mboxID, NULL, 0);
    }
    args->arg4 = ((void *) (long) 0);
    setUserMode();
}

/* ------------------------------------------------------------------------
   Name - semFree
   Purpose - Frees a semaphore, making it available in the semtable
   Parameters - systemArgs *args, the arguments passed from libuser.c
   Returns - void, sets arg values
   Side Effects - Makes a semaphore available in the semTable
   ----------------------------------------------------------------------- */
void semFree(systemArgs *args) {
    int semIndex = ((int) (long) args->arg1);

	//If the requested semaphore is invalid, return
    if (semIndex < 0 || semIndex > MAXSEMS) {
        args->arg4 = ((void *) (long) -1);
        return;
    }

    semStruct *semaphore = &semTable[semIndex];

	//If the semaphore is not in use, return
    if (semaphore->status == EMPTY) {
        args->arg4 = ((void *) (long) -1);
        return;
    }

    semaphore->status = EMPTY;

    // Handle block list
    if (semaphore->blockedList != NULL) {
		//While the semaphore has blocked processes, wake them up
        while (semaphore->blockedList != NULL) {
            int privateMboxID = semaphore->blockedList->mboxID;
            MboxSend(privateMboxID, NULL, 0);
            semaphore->blockedList = semaphore->blockedList->nextSemBlock;
        }
        args->arg4 = ((void *) (long) 1);
    } else {
        args->arg4 = ((void *) (long) 0);
    }
    setUserMode();
}

/* Halt USLOSS if process is not in kernal mode */
void checkKernelMode(char * processName) {
    if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0) {
        USLOSS_Console("check_kernal_mode(): called while in user mode, by"
                " process %s. Halting...\n", processName);
        USLOSS_Halt(1);
    }   
}

/*Returns the phase1 gtpid() value*/
void getPid(systemArgs *args) {
    args->arg1 = ((void *) (long) getpid());
    setUserMode();
}

/* Returns the USLOSS_Clock time*/
void getTimeOfDay(systemArgs *args) {
    args->arg1 = ((void *) (long) USLOSS_Clock());
    setUserMode();
}

/* Returns the phase1 readtime value*/
void cpuTime(systemArgs *args) {
    args->arg1 = ((void *) (long) readtime());
    setUserMode();
}

/* Sets the mode from kernel mode to user mode */
void setUserMode() {
    USLOSS_PsrSet(USLOSS_PsrGet() & 14);
}

/* ------------------------------------------------------------------------
   Name - addChildToList
   Purpose - Inserts a child to the end of the parent's child list
   Parameters - procPtr3 process, process to beadded
   Returns - void
   Side Effects - adds child to parent's child list
   ----------------------------------------------------------------------- */
void addChildToList(procPtr3 child) {
    procPtr3 parent = &procTable[getpid() % MAXPROC];
    
	//If process has no children, add it to head
    if (parent->childProcPtr == NULL) {
        parent->childProcPtr = child;
    } else {
		//add child to end of list
        procPtr3 sibling = parent->childProcPtr;
        while (sibling->nextSiblingPtr != NULL) {
            sibling = sibling->nextSiblingPtr;
        }
        sibling->nextSiblingPtr = child;
    }
}

/* ------------------------------------------------------------------------
   Name - removeFromChildList
   Purpose - Finds process in parent's childlist, removes process, reasigns
             all important processes
   Parameters - procPtr3 process, process to be deleted
   Returns - void
   Side Effects - Removes child from parent's child list
   ----------------------------------------------------------------------- */
void removeFromChildList(procPtr3 process) {
    procPtr3 temp = process;
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
}/* removeFromChildList */

/* ------------------------------------------------------------------------
   Name - addToSemBlockList
   Purpose - Inserts a process into a semaphore's blocklist
   Parameters - The process to be inserted, the index of the sempahore.
   Returns - void
   Side Effects - A new process is added to the semaphore blocklist
   ----------------------------------------------------------------------- */
void addToSemBlockList(procPtr3 process, int semIndex) {
    procPtr3 blockList = semTable[semIndex].blockedList;
    
	//If there are not processes blocked 
    if (blockList == NULL) {
        semTable[semIndex].blockedList = process;
    } else {
        procPtr3 temp = blockList;
		//find the end of the semaphore's blocklist
        while (temp->nextSemBlock != NULL) {
            temp = temp->nextSemBlock;
        }
        temp->nextSemBlock = process;
    }
}

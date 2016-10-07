#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <sems.h>

void setUserMode();
void spawn(systemArgs *args);
int spawnReal(int (* userFunc)(char *), char *arg, int stackSize, int priority,
        char *name);
void spawnLaunch();
void checkKernelMode(char * processName);

procStruct3 procTable[MAXPROC]; // Process Table

// Semaphore Table

int start2(char *arg) {
    int pid;
    int status;

    /* Check kernel mode here. */
    checkKernelMode();

    // initialize all sturcts in the process table to EMPTY
    for (int i = 0; i < MAXPROC; i++) {
        procTable[i].status = EMPTY;
    }

    // TODO: initialize semaphore table

    // initialize systemCallVec to system call functions
    systemCallVec[SYS_SPAWN] = spawn;
    systemCallVec[SYS_WAIT] = wait;
    systemCallVec[SYS_TERMINATE] = terminate;

    // TODO: finish initialized systemCallVec

    // TODO: place start2 in process table

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

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    pid = waitReal(&status);

} /* start2 */

/* 
 *check_kernel_mode
 */
void checkKernelMode(char * processName) {
    if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0) {
        USLOSS_Console("check_kernal_mode(): called while in user mode, by"
                " process %s. Halting...\n", processName);
        USLOSS_Halt(1);
    }   
}

void spawn(systemArgs *args) {
    int pid;

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
    pid = spawnReal(args->arg1, args->arg2, (int) args->arg3, (int) args->arg4, (char *) args->arg5);

    args->arg1 = (void *) pid; // newly created process id; or -1
    args->arg4 = (void *) 0;   // inputs to function valid
}

int spawnReal(int (* userFunc)(char *), char *arg, int stackSize, int priority,
        char *name) {

    int childPID;
    int mailboxID;

    childPID = fork1(name, spawnLaunch, arg, stackSize, priority);

    // add child information to the process table
    procTable[childPID % MAXPROC].pid = childPID;
    procTable[childPID % MAXPROC].parentPtr = &procTable[getpid() % MAXPROC];
    procTable[childPID % MAXPROC].name = name;
    procTable[childPID % MAXPROC].priority = priority;
    procTable[childPID % MAXPROC].userFunc = userFunc;
    procTable[childPID % MAXPROC].startArg = arg;
    procTable[childPID % MAXPROC].stackSize = stackSize;
    procTable[childPID % MAXPROC].status = ACTIVE;

    mailboxID = procTable[childPID % MAXPROC].mboxID;

    MboxSend(mailboxID, NULL, 0); // wake up child blocked in spawnLaunch

    return childPID;
}

void spawnLaunch() {
    int mailboxID;
    int pid = getpid();

    mailboxID = MboxCreate(0, 0);

    procTable[pid % MAXPROC].mboxID = mailboxID;
    
    MboxReceive(mailboxID, NULL, 0);

    setUserMode();

    // calls userFunc for child process to execute
    procTable[pid % MAXPROC].userFunc(procTable[pid % MAXPROC].startArg);
}

void setUserMode() {
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_MODE);
}

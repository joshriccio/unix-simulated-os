#include <usloss.h>
#include <usyscall.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <providedPrototypes.h>
#include <driver.h>
#include <stdlib.h> /* needed for atoi() */
#include <stdio.h>  /* sprintf */
#include <string.h>  /* strcpy */

/* ------------------------- Prototypes ----------------------------------- */

static int ClockDriver(char *);
static int DiskDriver(char *);
static int TermDriver(char *arg);
void sleep(systemArgs *args);
int sleepReal(int seconds);
void diskRead(systemArgs *args);
void diskWrite(systemArgs *args);
void diskSize(systemArgs *args);
void termRead(systemArgs *args);
void termWrite(systemArgs *args);
void checkKernelMode(char * processName);
void enableInterrupts();
void addToProcessTable();
void removeFromProcessTable();

/* -------------------------- Globals ------------------------------------- */

// Process Table
procStruct4 procTable[MAXPROC];

int clockSemaphore;

procPtr4 headSleepList;

void start3() {
    char	name[128];
    char    argBuffer[10];
    int		i;
    int		clockPID;
    int		diskPID[USLOSS_DISK_UNITS];
    int		termPID[USLOSS_TERM_UNITS];
    int		pid;
    int		status;

    // Check kernel mode here.
    checkKernelMode("start3");

    // initialize all process table structs to EMPTY
    for (int i = 0; i < MAXPROC; i++) {
        procTable[i].status = EMPTY;
    }

    // initialize headSleepList
    headSleepList = NULL;

    // initialize system call vector
    systemCallVec[SYS_SLEEP] = sleep;
    systemCallVec[SYS_DISKREAD] = diskRead;
    systemCallVec[SYS_DISKWRITE] = diskWrite;
    systemCallVec[SYS_DISKSIZE] = diskSize;
    systemCallVec[SYS_TERMREAD] = termRead;
    systemCallVec[SYS_TERMWRITE] = termWrite;

    // Create clock device driver 
    clockSemaphore = semcreateReal(0);
    clockPID = fork1("Clock driver", ClockDriver, NULL, USLOSS_MIN_STACK, 2);
    if (clockPID < 0) {
        USLOSS_Console("start3(): Can't create clock driver\n");
        USLOSS_Halt(1);
    }

    // add clockPID to process table
    strcpy(procTable[clockPID % MAXPROC].name, "Clock driver");
    procTable[clockPID % MAXPROC].pid = clockPID;
    procTable[clockPID % MAXPROC].status = ACTIVE;

    // start3 blocks until clock driver is running
    sempReal(clockSemaphore);

    /*
     * Create the disk device drivers here.  You may need to increase
     * the stack size depending on the complexity of your
     * driver, and perhaps do something with the pid returned.
     */

    // create disk driver processes
    for (i = 0; i < USLOSS_DISK_UNITS; i++) {
        sprintf(argBuffer, "%d", i);
        sprintf(name, "diskDriver%d", i);
        pid = fork1(name, DiskDriver, argBuffer, USLOSS_MIN_STACK, 2);
        if (pid < 0) {
            USLOSS_Console("start3(): Can't create disk driver %d\n", i);
            USLOSS_Halt(1);
        }

        diskPID[i] = pid; // store pid of disk driver processes for zapping

        strcpy(procTable[pid % MAXPROC].name, name);
        procTable[pid % MAXPROC].pid = pid;
        procTable[pid % MAXPROC].status = ACTIVE;
    }

    // May be other stuff to do here before going on to terminal drivers

    // Create terminal device drivers.
    for (i = 0; i < USLOSS_TERM_UNITS; i++) {
        sprintf(argBuffer, "%d", i);
        sprintf(name, "termDriver%d", i);
        pid = fork1(name, TermDriver, argBuffer, USLOSS_MIN_STACK, 2);
        if (pid < 0) {
            USLOSS_Console("start3(): Can't create term driver %d\n", i);
            USLOSS_Halt(1);
        }

        termPID[i] = pid; // store pid of term driver processes for zapping

        strcpy(procTable[pid % MAXPROC].name, name);
        procTable[pid % MAXPROC].pid = pid;
        procTable[pid % MAXPROC].status = ACTIVE;
    }
    

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * I'm assuming kernel-mode versions of the system calls
     * with lower-case first letters, as shown in provided_prototypes.h
     */
    pid = spawnReal("start4", start4, NULL, 4 * USLOSS_MIN_STACK, 3);
    pid = waitReal(&status);

    /*
     * Zap the device drivers
     */
    zap(clockPID);  // clock driver
    for (i = 0; i < USLOSS_DISK_UNITS; i++) {  // disk drivers
        zap(diskPID[i]);
    }
    for (i = 0; i < USLOSS_TERM_UNITS; i++) {  // term drivers
        zap(termPID[i]);
    }

    // join for drivers
    while (join(&status) != -2)

    // eventually, at the end:
    quit(0);
    
} // start3

static int ClockDriver(char *arg) {
    int result;
    int status;

    // Let the parent know we are running and enable interrupts.
    semvReal(clockSemaphore);
    enableInterrupts();

    // Infinite loop until we are zap'd
    while(! isZapped()) {
        result = waitDevice(USLOSS_CLOCK_DEV, 0, &status);
        if (result != 0) {
            return 0;
        }
	/*
	 * Compute the current time and wake up any processes
	 * whose time has come.
	 */
        while (headSleepList != NULL && 
                headSleepList->awakeTime <= USLOSS_Clock()) {

            // remove from sleep list
            int mboxID = headSleepList->mboxID;
            headSleepList = headSleepList->sleepPtr;
            MboxSend(mboxID, NULL, 0);
        }
    }
    return 0;
}

static int DiskDriver(char *arg) {
    int status;
    int result;
    while(! isZapped()) {
        result = waitDevice(USLOSS_DISK_DEV, atoi(arg), &status);
        if (result != 0) {
            return 0;
        }
    }
    return 0;
}

static int TermDriver(char *arg) {
    int status;
    int result;
    while(! isZapped()) {
        result = waitDevice(USLOSS_TERM_DEV, atoi(arg), &status);
        if (result != 0) {
            return 0;
        }
    }
    return 0;
}

void sleep(systemArgs *args) {
    int seconds = ((int) (long) args->arg1);
    if (sleepReal(seconds) < 0) {
        args->arg4 = ((void *) (long) -1);
    } else {
        args->arg4 = ((void *) (long) 0);
    }
}

int sleepReal(int seconds) {
    if (seconds < 0) {
        return -1;
    }

    // add process to phase 4 process table
    addToProcessTable();

    int awakeTime = USLOSS_Clock() + (1000000 * seconds);
    procTable[getpid() % MAXPROC].awakeTime = awakeTime;
    if (headSleepList == NULL) {
        headSleepList = &procTable[getpid() % MAXPROC];
    } else {
        procPtr4 temp = headSleepList;
        while (temp->awakeTime < awakeTime) {
            temp = temp->sleepPtr;
        }
        procPtr4 temp2 = temp->sleepPtr;
        temp->sleepPtr = &procTable[getpid() % MAXPROC];
        temp->sleepPtr->sleepPtr = temp2;
    }

    // block on private mailbox
    MboxReceive(procTable[getpid() % MAXPROC].mboxID, NULL, 0);

    // remove from process table
    removeFromProcessTable();

    return 0;
}

void diskRead(systemArgs *args) {

}

void diskWrite(systemArgs *args) {

}

void diskSize(systemArgs *args) {

}

void termRead(systemArgs *args) {

}

void termWrite(systemArgs *args) {

}

/* Halt USLOSS if process is not in kernal mode */
void checkKernelMode(char * processName) {
    if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0) {
        USLOSS_Console("check_kernal_mode(): called while in user mode, by"
                " process %s. Halting...\n", processName);
        USLOSS_Halt(1);
    }   
}

/* Enables Interrupts */
void enableInterrupts() {
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
}

void addToProcessTable() {
    procTable[getpid() % MAXPROC].pid = getpid();
    procTable[getpid() % MAXPROC].status = ACTIVE;
    procTable[getpid() % MAXPROC].mboxID = MboxCreate(0,0);
    procTable[getpid() % MAXPROC].sleepPtr = NULL;
}

void removeFromProcessTable() {
    MboxRelease(procTable[getpid() % MAXPROC].mboxID);
    procTable[getpid() % MAXPROC].pid = -1;
    procTable[getpid() % MAXPROC].status = EMPTY;
    procTable[getpid() % MAXPROC].mboxID = -1;
    procTable[getpid() % MAXPROC].sleepPtr = NULL;
}

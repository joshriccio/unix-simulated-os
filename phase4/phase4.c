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
int diskReadReal(int unit, int startTrack, int startSector, int sectors, 
        void *buffer);
void diskWrite(systemArgs *args);
int diskWriteReal(int unit, int startTrack, int startSector, int sectors, 
        void *buffer);
void diskSize(systemArgs *args);
int diskSizeReal(int unit, int *sectorSize, int *sectorsInTrack, 
        int *tracksInDisk);
void termRead(systemArgs *args);
void termWrite(systemArgs *args);
void checkKernelMode(char * processName);
void enableInterrupts();
void addToProcessTable();
void removeFromProcessTable();
int diskReadHandler();
int diskWriteHandler();
int deviceOutput(USLOSS_DeviceRequest *devRequest);
void insertDiskRequest(diskDriverInfoPtr info);
/* -------------------------- Globals ------------------------------------- */

// Process Table
procStruct4 procTable[MAXPROC];

int clockSemaphore;
int diskSemaphore[USLOSS_DISK_UNITS];

int tracksOnDisk[USLOSS_DISK_UNITS];

procPtr4 headSleepList;
diskDriverInfoPtr headDiskList;


/* ------------------------------------------------------------------------
   Name - start3
   Purpose - Initializes process table, semaphore table, and system call 
             vector, forks all drivers, and cleans up when start4 is complete.
   Parameters - none.
   Returns - void
   Side Effects - lots since it initializes the phase4 requests
   ----------------------------------------------------------------------- */
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
        procTable[i].pid = -1;
    }

    // initialize headSleepList TODO:
    headSleepList = NULL;
    headDiskList = NULL;

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
        diskSemaphore[i] = semcreateReal(0);
        sprintf(argBuffer, "%d", i);
        sprintf(name, "diskDriver%d", i);
        pid = fork1(name, DiskDriver, argBuffer, USLOSS_MIN_STACK, 2);
        if (pid < 0) {
            USLOSS_Console("start3(): Can't create disk driver %d\n", i);
            USLOSS_Halt(1);
        }

        diskPID[i] = pid; // store pid of disk driver processes for zapping

        int sector, track;
        diskSizeReal(i, &sector, &track, &tracksOnDisk[i]);

        strcpy(procTable[pid % MAXPROC].name, name);
        procTable[pid % MAXPROC].pid = pid;
        procTable[pid % MAXPROC].status = ACTIVE;
    }

    // May be other stuff to do here before going on to terminal drivers

    // Create terminal device drivers.
    /*for (i = 0; i < USLOSS_TERM_UNITS; i++) {
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
    }*/
    

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

        //Unblock the disk drivers
        semvReal(diskSemaphore[i]);
        zap(diskPID[i]);
    }
    /*for (i = 0; i < USLOSS_TERM_UNITS; i++) {  // term drivers
        zap(termPID[i]);
    }*/

    quit(0);
    
} // start3

/* ------------------------------------------------------------------------
   Name - ClockDriver
   Purpose - Wakes up sleeping processes by grabing the head of the sleep
             list and unblocking it. Runs in a loop
   Parameters - char* arg, not used
   Returns - int, returns zero
   Side Effects - Wakes up sleeping processes
   ----------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------------
   Name - DiskDriver
   Purpose - Grabs the various disk requests from the headDiskList queue and
             processes each request in order
   Parameters - char* arg, not used
   Returns - int, returns zero
   Side Effects - Wakes up blocked disk request processes
   ----------------------------------------------------------------------- */
static int DiskDriver(char *arg) {
    int result;
    int unit = atoi(arg);

    while(! isZapped()) {
        //If there is a request, process it, else block and wait
        if (headDiskList != NULL){
                switch (headDiskList->requestType) {
                    case USLOSS_DISK_READ:
                        result = diskReadHandler(); 
                        break;
                    case USLOSS_DISK_WRITE:
                        result = diskWriteHandler();
                        break;
                    default:
                        USLOSS_Console("DiskDriver: Invalid disk request.\n");
                        USLOSS_Console("DiskDriver: %d.\n", 
                                headDiskList->requestType);
                }
        } else {
            //Block and wait for a new request
            sempReal(diskSemaphore[unit]);
        }

        if (result < 0) {
            USLOSS_Console("DiskDriver: Read/Write Fail!\n");
        }
    }	
    return 0;
}

/* ------------------------------------------------------------------------
   Name - diskReadHandler
   Purpose - Function called by DiskDriver to process actual disk read 
             request. reads data and writes to buffer.
   Parameters - none.
   Returns - void
   Side Effects - Writes data to buffer
   ----------------------------------------------------------------------- */
int diskReadHandler() {
    char sectorBuffer[512];
    int bufferIndex = 0;
    int unit = headDiskList->unit;
    int currentTrack = headDiskList->startTrack;
    int currentSector = headDiskList->startSector;

    USLOSS_DeviceRequest devRequest;
    devRequest.opr = USLOSS_DISK_SEEK;
    devRequest.reg1 = ((void *) (long) currentTrack);
    
    // perform initial seek operation
    if (deviceOutput(&devRequest) < 0) {
        return -1;
    }

    // perform all read operations. may need to change track.
    for (int i = 0; i < headDiskList->sectors; i++) {
        if (currentSector == USLOSS_DISK_TRACK_SIZE) {
            currentSector = 0;
            currentTrack++;

            // change track to next track 
            if (currentTrack == tracksOnDisk[unit]) {  // disk cannot wrap
                return -1;
            }
            devRequest.opr = USLOSS_DISK_SEEK;
            devRequest.reg1 = ((void *) (long) currentTrack);
            
            if (deviceOutput(&devRequest) < 0) { // seek to next track
                return -1;
            }
        }

        // build a device request struct for read
        devRequest.opr = USLOSS_DISK_READ;
        devRequest.reg1 = ((void *) (long) currentSector);
        devRequest.reg2 = sectorBuffer;

        // perform a read
        if (deviceOutput(&devRequest) < 0) {
            headDiskList = headDiskList->next;  // remove request from queue
            return -1;
        }

        // copy what was read to users buffer
        memcpy(((char *) headDiskList->buffer) + bufferIndex, sectorBuffer, 
                512);
        bufferIndex += 512;

        currentSector++;
    }

    MboxSend(headDiskList->mboxID, NULL, 0); // wake up calling process
    headDiskList = headDiskList->next;  // remove request from queue
    return 0;
}

/* ------------------------------------------------------------------------
   Name - diskWriteHandler
   Purpose - Function called by DiskDriver to process actual disk write
             request. writes data to disk
   Parameters - none.
   Returns - void
   Side Effects - Writes data to disk
   ----------------------------------------------------------------------- */
int diskWriteHandler(){
    int status;
    int unit = headDiskList->unit;
    int currentTrack = headDiskList->startTrack;
    int currentSector = headDiskList->startSector;

    USLOSS_DeviceRequest devRequest;
    devRequest.opr = USLOSS_DISK_SEEK;
    devRequest.reg1 = ((void *) (long) currentTrack);
    
    // seek to initial track to write
    if (deviceOutput(&devRequest) < 0) {
        return -1;
    }

    for (int i = 0; i < headDiskList->sectors; i++) {

        // Used all sectors on track
        if (currentSector == USLOSS_DISK_TRACK_SIZE) {
            currentSector = 0;
            currentTrack++;

            // used all tracks on disk
            if (currentTrack == tracksOnDisk[unit]) {  // disk cannot wrap
                return -1;
            }
            devRequest.opr = USLOSS_DISK_SEEK;
            devRequest.reg1 = ((void *) (long) currentTrack);

            // seek to next track to write
            if (deviceOutput(&devRequest) < 0) {
                USLOSS_Console("seek fail\n");
                return -1;
            }
        }
        devRequest.opr = USLOSS_DISK_WRITE;
        devRequest.reg1 = ((void *) (long) currentSector);
        devRequest.reg2 = headDiskList->buffer + (512 * i);
        
        // write sector to disk
        if (deviceOutput(&devRequest) < 0) {
            headDiskList = headDiskList->next;  // remove request from queue
            headDiskList->status = status;
            USLOSS_Console("write fail\n");
            return -1;
        }
        
        currentSector++;
    }

    MboxSend(headDiskList->mboxID, NULL, 0);  // wake up calling process
    headDiskList = headDiskList->next;  // remove request from queue
    return 0;
}

/* ------------------------------------------------------------------------
   Name - 
   Purpose - 
             
   Parameters - none.
   Returns - void
   Side Effects - none.
   ----------------------------------------------------------------------- */
int deviceOutput(USLOSS_DeviceRequest *devRequest){
    int status;
    int result;

    USLOSS_DeviceOutput(USLOSS_DISK_DEV, headDiskList->unit, devRequest);
    result = waitDevice(USLOSS_DISK_DEV, headDiskList->unit, &status);
    if (status == USLOSS_DEV_ERROR) {
        headDiskList->status = status;
        return -1;
    }
    if (result != 0) {
        return -2;
    }
    headDiskList->status = status;
    return 0;
}

/* ------------------------------------------------------------------------
   Name - TermDriver
   Purpose - TermDriver processes all terminal requests by reading from 
             head of queue and processesing each request.
   Parameters - char *arg, not used
   Returns - int, returns 0
   Side Effects - none.
   ----------------------------------------------------------------------- */
static int TermDriver(char *arg) {
    /*int status;
    int result;
    while(! isZapped()) {
        result = waitDevice(USLOSS_TERM_DEV, atoi(arg), &status);
        if (result != 0) {
            return 0;
        }
    }*/
    return 0;
}

/* ------------------------------------------------------------------------
   Name - sleep
   Purpose - Processes systemArgs and calls sleep real, this function
             blocks sleeping processes for int seconds time.
   Parameters - systemArgs args, arg1 = seconds to sleep
   Returns - void
   Side Effects - calls sleepReal()
   ----------------------------------------------------------------------- */
void sleep(systemArgs *args) {
    int seconds = ((int) (long) args->arg1);
    if (sleepReal(seconds) < 0) {
        args->arg4 = ((void *) (long) -1);
    } else {
        args->arg4 = ((void *) (long) 0);
    }
}

/* ------------------------------------------------------------------------
   Name - sleepReal
   Purpose - This function blocks sleeping processes for int seconds time.
   Parameters - int seconds, the seconds to sleep for
   Returns - int, result
   Side Effects - blocks sleeping process
   ----------------------------------------------------------------------- */
int sleepReal(int seconds) {
    if (seconds < 0) {
        return -1;
    }

    // add process to phase 4 process table
    addToProcessTable();

    // process to add to the sleep list and block
    procPtr4 toAdd = &procTable[getpid() % MAXPROC];

    int awakeTime = USLOSS_Clock() + (1000000 * seconds);
    toAdd->awakeTime = awakeTime;

    // add process to the sleep list
    if (headSleepList == NULL) {
        headSleepList = toAdd;
    } else {
            procPtr4 temp = headSleepList;
        if (toAdd->awakeTime >= headSleepList->awakeTime) {
            procPtr4 temp2 = headSleepList->sleepPtr;
            while (temp2 != NULL && toAdd->awakeTime > temp2->awakeTime) {
                temp = temp->sleepPtr;
                temp2 = temp2->sleepPtr;
            }
            temp->sleepPtr = toAdd;
            toAdd->sleepPtr = temp2;
        } else {
            toAdd->sleepPtr = temp;
            headSleepList = toAdd;
        }
    }

    // block on private mailbox
    MboxReceive(procTable[getpid() % MAXPROC].mboxID, NULL, 0);

    // remove from process table
    removeFromProcessTable();

    return 0;
}

/* ------------------------------------------------------------------------
   Name - diskRead
   Purpose - Processes systemArgs and calls diskReadReal to add a new 
             diskRead request to the disk request queue.
   Parameters - systemArgs args
   Returns - void
   Side Effects - calls diskReadReal
   ----------------------------------------------------------------------- */
void diskRead(systemArgs *args) {
    int result;
    
    void *buffer = args->arg1;
    int sectors = ((int) (long) args->arg2);
    int startTrack = ((int) (long) args->arg3);
    int startSector = ((int) (long) args->arg4);
    int unit = ((int) (long) args->arg5);

    result = diskReadReal(unit, startTrack, startSector, sectors, buffer);
    
    //If invalid arguments, store -1 in arg1, else store 0 in arg4
    if (result == -1) {
        args->arg4 = ((void *) (long) -1);
    } else {
        args->arg4 = ((void *) (long) 0);
    }
    //Store the result of the disk read in arg1
    args->arg1 = ((void *) (long) result);
}

/* ------------------------------------------------------------------------
   Name - diskReadReal
   Purpose - Adds a new diskRead request to the disk request queue and 
             blocks until request is completed.
   Parameters - int unit, int startTrack, int startSector, int sectors,
                void *buffer
   Returns - int, the result
   Side Effects - Adds new request to queue
   ----------------------------------------------------------------------- */
int diskReadReal(int unit, int startTrack, int startSector, int sectors, 
        void *buffer) {

    diskDriverInfo info;

    if (unit < 0 || unit > 1) {
        return -1;
    }
    if (startTrack < 0 || startTrack > tracksOnDisk[unit] - 1) {
        return -1;
    }
    if (startSector < 0 || startSector > USLOSS_DISK_TRACK_SIZE - 1) {
        return -1;
    }

    addToProcessTable();

    info.unit = unit;
    info.startTrack = startTrack;
    info.startSector = startSector;
    info.sectors = sectors;
    info.buffer = buffer;
    info.mboxID = procTable[getpid() % MAXPROC].mboxID;
    info.requestType = USLOSS_DISK_READ;
    info.next = NULL;

    insertDiskRequest(&info);

    semvReal(diskSemaphore[unit]); // wake up driver

    MboxReceive(procTable[getpid() % MAXPROC].mboxID, NULL, 0);
    //Remove process from table
    removeFromProcessTable();
    return info.status;
}

void insertDiskRequest(diskDriverInfoPtr info) {
    if (headDiskList == NULL) {
        headDiskList = info;
    } else {
        diskDriverInfoPtr tempA = headDiskList;
        diskDriverInfoPtr tempB = headDiskList->next;
        if (info->startTrack > headDiskList->startTrack) {
            while (tempB != NULL && tempB->startTrack < info->startTrack && tempB->startTrack > tempA->startTrack) {
                tempA = tempA->next;
                tempB = tempB->next;
            }
            tempA->next = info;
            info->next = tempB;
        } else {
            while (tempB != NULL && tempA->startTrack < tempB->startTrack) {
                tempA = tempA->next;
                tempB = tempB->next;
            }
            while (tempB != NULL && tempB->startTrack < info->startTrack) {
                tempA = tempA->next;
                tempB = tempB->next;
            }
            tempA->next = info;
            info->next = tempB;
        }
    }
}
/* ------------------------------------------------------------------------
   Name - diskWrite
   Purpose - Processes systemArgs and calls diskWriteReal to add a new
             diskWrite request to the disk request queue.
   Parameters - systemArgs args
   Returns - void
   Side Effects - calls diskWriteReal
   ----------------------------------------------------------------------- */
void diskWrite(systemArgs *args) {
    int result;
    
    void *buffer = args->arg1;
    int sectors = ((int) (long) args->arg2);
    int startTrack = ((int) (long) args->arg3);
    int startSector = ((int) (long) args->arg4);
    int unit = ((int) (long) args->arg5);

    result = diskWriteReal(unit, startTrack, startSector, sectors, buffer);
    
    //If invalid arguments, store -1 in arg1, else store 0 in arg4
    if (result == -1) {
        args->arg4 = ((void *) (long) -1);
    } else {
        args->arg4 = ((void *) (long) 0);
    }
    //Store the result of the disk read in arg1
    args->arg1 = ((void *) (long) result);
}

/* ------------------------------------------------------------------------
   Name - diskWriteReal
   Purpose - Adds a new diskRead request to the disk request queue and 
             blocks until request is completed.
   Parameters - int unit, int startTrack, int startSector, int sectors,
                void *buffer
   Returns - int, the result
   Side Effects - Adds new request to queue
   ----------------------------------------------------------------------- */
int diskWriteReal(int unit, int startTrack, int startSector, int sectors, 
        void *buffer) {

    diskDriverInfo info;

    if (unit < 0 || unit > 1) {
        return -1;
    }
    if (startTrack < 0 || startTrack > tracksOnDisk[unit] - 1) {
        return -1;
    }
    if (startSector < 0 || startSector > USLOSS_DISK_TRACK_SIZE - 1) {
        return -1;
    }

    addToProcessTable();

    info.unit = unit;
    info.startTrack = startTrack;
    info.startSector = startSector;
    info.sectors = sectors;
    info.buffer = buffer;
    info.mboxID = procTable[getpid() % MAXPROC].mboxID;
    info.requestType = USLOSS_DISK_WRITE;
    info.next = NULL;

    insertDiskRequest(&info);

    diskDriverInfoPtr temp = headDiskList;
    while (temp != NULL) {
        USLOSS_Console("%d, ", temp->startTrack);
        temp = temp->next;
    }
    USLOSS_Console("\n");

    semvReal(diskSemaphore[unit]);

    MboxReceive(procTable[getpid() % MAXPROC].mboxID, NULL, 0);
    
    // Remove process from table
    removeFromProcessTable();
    return info.status;
}

/* ------------------------------------------------------------------------
   Name - diskSize
   Purpose - Processes systemArgs and calls diskSizeReal to add a new
             diskSize request to the disk request queue.
   Parameters - systemArgs args
   Returns - void
   Side Effects - calls diskSizeReal
   ----------------------------------------------------------------------- */
void diskSize(systemArgs *args) {
    int result;

    int sectorSize;
    int sectorsInTrack;
    int tracksInDisk;
    
    int unit = ((int) (long) args->arg1);

    result = diskSizeReal(unit, &sectorSize, &sectorsInTrack, &tracksInDisk);
    
    //If invalid arguments, store -1 in arg1, else store 0 in arg4
    if (result == -1) {
        args->arg4 = ((void *) (long) -1);
    } else {
        args->arg4 = ((void *) (long) 0);
    }
    //Store the result of the disk read in arg1
    args->arg1 = ((void *) (long) sectorSize);
    args->arg2 = ((void *) (long) sectorsInTrack);
    args->arg3 = ((void *) (long) tracksInDisk);
}

int diskSizeReal(int unit, int *sectorSize, int *sectorsInTrack, 
        int *tracksInDisk) {
    int status;
    int result;
    
    if (unit < 0 || unit > 1) {
        return -1;
    }
    
    addToProcessTable();

    USLOSS_DeviceRequest devRequest;
    devRequest.opr = USLOSS_DISK_TRACKS;
    devRequest.reg1 = (void *) tracksInDisk;

    USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &devRequest);
    result = waitDevice(USLOSS_DISK_DEV, unit, &status);

    if (status == USLOSS_DEV_ERROR) {
        return -1;
    }
    if (result != 0) {
        return -1;
    }

    *sectorSize = USLOSS_DISK_SECTOR_SIZE;
    *sectorsInTrack = USLOSS_DISK_TRACK_SIZE;

    removeFromProcessTable();
    return 0;
}

/* ------------------------------------------------------------------------
   Name - termRead
   Purpose - Processes systemArgs and calls termReadReal to add a new
             termRead request to the terminal request queue.
   Parameters - systemArgs args
   Returns - void
   Side Effects - calls termReadReal
   ----------------------------------------------------------------------- */
void termRead(systemArgs *args) {

}

/* ------------------------------------------------------------------------
   Name - termWrite
   Purpose - Processes systemArgs and calls termWriteReal to add a new
             termWrite request to the terminal request queue.
   Parameters - systemArgs args
   Returns - void
   Side Effects - calls termWriteReal
   ----------------------------------------------------------------------- */
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

/* Adds incomming process to procTable, creates proc mBox */
void addToProcessTable() {
    if (getpid() !=  procTable[getpid() % MAXPROC].pid) {
        procTable[getpid() % MAXPROC].pid = getpid();
        procTable[getpid() % MAXPROC].status = ACTIVE;
        procTable[getpid() % MAXPROC].mboxID = MboxCreate(0,0);
        procTable[getpid() % MAXPROC].sleepPtr = NULL;
    }
}

/* Removes outgoing process from procTable, releases proc mBox */
void removeFromProcessTable() {
    MboxRelease(procTable[getpid() % MAXPROC].mboxID);
    procTable[getpid() % MAXPROC].pid = -1;
    procTable[getpid() % MAXPROC].status = EMPTY;
    procTable[getpid() % MAXPROC].mboxID = -1;
    procTable[getpid() % MAXPROC].sleepPtr = NULL;
}

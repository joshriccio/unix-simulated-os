/*
 *  File:  libuser.c
 *
 *  Description:  This file contains the interface declarations
 *                to the OS kernel support package.
 *
 */

#include <string.h>
#include <phase1.h>
#include <phase2.h>
#include <phase5.h>
#include <libuser.h>
#include <usyscall.h>
#include <usloss.h>

#define CHECKMODE {						\
	if (USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) { 				\
	    USLOSS_Console("Trying to invoke syscall from kernel\n");	\
	    USLOSS_Halt(1);						\
	}							\
}

/*
 *  Routine:  Spawn
 *
 *  Description: This is the call entry to fork a new user process.
 *
 *  Arguments:    char *name    -- new process's name
 *		  PFV func      -- pointer to the function to fork
 *		  void *arg	-- argument to function
 *                int stacksize -- amount of stack to be allocated
 *                int priority  -- priority of forked process
 *                int  *pid      -- pointer to output value
 *                (output value: process id of the forked process)
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int Spawn(char *name, int (*func)(char *), char *arg, int stack_size,
	int priority, int *pid)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SPAWN;
    sysArg.arg1 = (void *) func;
    sysArg.arg2 = arg;
    sysArg.arg3 = (void *) ( (long) stack_size);
    sysArg.arg4 = (void *) ( (long) priority);
    sysArg.arg5 = name;
    USLOSS_Syscall(&sysArg);
    *pid = (int) (long) sysArg.arg1;
    return (int) (long) sysArg.arg4;
} /* end of Spawn */


/*
 *  Routine:  Wait
 *
 *  Description: This is the call entry to wait for a child completion
 *
 *  Arguments:    int *pid -- pointer to output value 1
 *                (output value 1: process id of the completing child)
 *                int *status -- pointer to output value 2
 *                (output value 2: status of the completing child)
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int Wait(int *pid, int *status)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_WAIT;
    USLOSS_Syscall(&sysArg);
    *pid    = (int) (long) sysArg.arg1;
    *status = (int) (long) sysArg.arg2;
    return (int) (long) sysArg.arg4;

} /* End of Wait */


/*
 *  Routine:  Terminate
 *
 *  Description: This is the call entry to terminate
 *               the invoking process and its children
 *
 *  Arguments:   int status -- the commpletion status of the process
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
void Terminate(int status)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_TERMINATE;
    sysArg.arg1 = (void *) ( (long) status);
    USLOSS_Syscall(&sysArg);
    return;

} /* End of Terminate */


/*
 *  Routine:  SemCreate
 *
 *  Description: Create a semaphore.
 *
 *  Arguments:    int value -- initial semaphore value
 *		  int *semaphore -- semaphore handle
 *                (output value: completion status)
 */
int SemCreate(int value, int *semaphore)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMCREATE;
    sysArg.arg1 = (void *) ( (long) value);
    USLOSS_Syscall(&sysArg);
    *semaphore = (int) (long) sysArg.arg1;
    return (int) (long) sysArg.arg4;
} /* end of SemCreate */


/*
 *  Routine:  SemP
 *
 *  Description: "P" a semaphore.
 *
 *
 *  Arguments:    int semaphore -- semaphore handle
 *                (output value: completion status)
 *
 */
int SemP(int semaphore)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMP;
    sysArg.arg1 = (void *) ( (long) semaphore);
    USLOSS_Syscall(&sysArg);
    return (int) (long) sysArg.arg4;
} /* end of SemP */


/*
 *  Routine:  SemV
 *
 *  Description: "V" a semaphore.
 *
 *
 *  Arguments:    int semaphore -- semaphore handle
 *                (output value: completion status)
 *
 */
int SemV(int semaphore)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMV;
    sysArg.arg1 = (void *) ( (long) semaphore);
    USLOSS_Syscall(&sysArg);
    return (int) (long) sysArg.arg4;
} /* end of SemV */


/*
 *  Routine:  SemFree
 *
 *  Description: Free a semaphore.
 *
 *
 *  Arguments:    int semaphore -- semaphore handle
 *                (output value: completion status)
 *
 */
int SemFree(int semaphore)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SEMFREE;
    sysArg.arg1 = (void *) ( (long) semaphore);
    USLOSS_Syscall(&sysArg);
    return (int) (long) sysArg.arg4;
} /* end of SemFree */


/*
 *  Routine:  GetTimeofDay
 *
 *  Description: This is the call entry point for getting the time of day.
 *
 *  Arguments:    int *tod  -- pointer to output value
 *                (output value: the time of day)
 *
 */
void GetTimeofDay(int *tod)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_GETTIMEOFDAY;
    USLOSS_Syscall(&sysArg);
    *tod = (int) (long) sysArg.arg1;
    return;
} /* end of GetTimeofDay */


/*
 *  Routine:  CPUTime
 *
 *  Description: This is the call entry point for the process' CPU time.
 *
 *
 *  Arguments:    int *cpu  -- pointer to output value
 *                (output value: the CPU time of the process)
 *
 */
void CPUTime(int *cpu)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_CPUTIME;
    USLOSS_Syscall(&sysArg);
    *cpu = (int) (long) sysArg.arg1;
    return;
} /* end of CPUTime */


/*
 *  Routine:  GetPID
 *
 *  Description: This is the call entry point for the process' PID.
 *
 *
 *  Arguments:    int *pid  -- pointer to output value
 *                (output value: the PID)
 *
 */
void GetPID(int *pid)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_GETPID;
    USLOSS_Syscall(&sysArg);
    *pid = (int) (long) sysArg.arg1;
    return;
} /* end of GetPID */

/* end libuser.c */
/*
 *  Routine:  Sleep
 *
 *  Description: This is the call entry point for timed delay.
 *
 *  Arguments:    int seconds -- number of seconds to sleep
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Sleep(int seconds)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SLEEP;
    sysArg.arg1 = (void *) (long) seconds;
    USLOSS_Syscall(&sysArg);
    return (int) (long) sysArg.arg4;
} /* end of Sleep */


/*
 *  Routine:  TermRead
 *
 *  Description: This is the call entry point for terminal input.
 *
 *  Arguments:    char *buffer    -- pointer to the input buffer
 *                int   bufferSize   -- maximum size of the buffer
 *                int   unitID -- terminal unit number
 *                int  *numCharsRead      -- pointer to output value
 *                (output value: number of characters actually read)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int TermRead(char *buffer, int bufferSize, int unitID, int *numCharsRead)     
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_TERMREAD;
    sysArg.arg1 = (void *) buffer;
    sysArg.arg2 = (void *) (long) bufferSize;
    sysArg.arg3 = (void *) (long) unitID;
    USLOSS_Syscall(&sysArg);
    *numCharsRead = (int) (long) sysArg.arg2;
    return (int) (long) sysArg.arg4;
} /* end of TermRead */


/*
 *  Routine:  TermWrite
 *
 *  Description: This is the call entry point for terminal output.
 *
 *  Arguments:    char *buffer    -- pointer to the output buffer
 *                int   bufferSize   -- number of characters to write
 *                int   unitID -- terminal unit number
 *                int  *numCharsWritten      -- pointer to output value
 *                (output value: number of characters actually written)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int TermWrite(char *buffer, int bufferSize, int unitID, int *numCharsWritten)    
{
    systemArgs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_TERMWRITE;
    sysArg.arg1 = (void *) buffer;
    sysArg.arg2 = (void *) (long) bufferSize;
    sysArg.arg3 = (void *) (long) unitID;
    USLOSS_Syscall(&sysArg);
    *numCharsWritten = (int) (long) sysArg.arg2;
    return (int) (long) sysArg.arg4;
} /* end of TermWrite */


/*
 *  Routine:  DiskRead
 *
 *  Description: This is the call entry point for disk input.
 *
 *  Arguments:    void* diskBuffer  -- pointer to the input buffer
 *                int   unit -- which disk to read
 *                int   track  -- first track to read
 *                int   first -- first sector to read
 *                int   sectors -- number of sectors to read
 *                int   *status    -- pointer to output value
 *                (output value: completion status)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int DiskRead(void *diskBuffer, int unit, int track, int first, int sectors,
    int *status)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_DISKREAD;
    sysArg.arg1 = diskBuffer;
    sysArg.arg2 = (void *) (long) sectors;
    sysArg.arg3 = (void *) (long) track;
    sysArg.arg4 = (void *) (long) first;
    sysArg.arg5 = (void *) (long) unit;
    USLOSS_Syscall(&sysArg);
    *status = (int) (long) sysArg.arg1;
    return (int) (long) sysArg.arg4;
} /* end of DiskRead */


/*
 *  Routine:  DiskWrite
 *
 *  Description: This is the call entry point for disk output.
 *
 *  Arguments:    void* diskBuffer  -- pointer to the output buffer
 *		  int   unit -- which disk to write
 *                int   track  -- first track to write
 *                int   first -- first sector to write
 *		  int	sectors -- number of sectors to write
 *                int   *status    -- pointer to output value
 *                (output value: completion status)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int DiskWrite(void *diskBuffer, int unit, int track, int first, int sectors, 
    int *status)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_DISKWRITE;
    sysArg.arg1 = diskBuffer;
    sysArg.arg2 = (void *) (long) sectors;
    sysArg.arg3 = (void *) (long) track;
    sysArg.arg4 = (void *) (long) first;
    sysArg.arg5 = (void *) (long) unit;
    USLOSS_Syscall(&sysArg);
    *status = (int) (long) sysArg.arg1;
    return (int) (long) sysArg.arg4;
} /* end of DiskWrite */


/*
 *  Routine:  DiskSize
 *
 *  Description: This is the call entry point for getting the disk size.
 *
 *  Arguments:    int	unit -- which disk
 *		  int	*sector -- # bytes in a sector
 *		  int	*track -- # sectors in a track
 *		  int   *disk -- # tracks in the disk
 *                (output value: completion status)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int DiskSize(int unit, int *sector, int *track, int *disk)
{
    systemArgs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_DISKSIZE;
    sysArg.arg1 = (void *) (long) unit;
    USLOSS_Syscall(&sysArg);
    *sector = (int) (long) sysArg.arg1;
    *track  = (int) (long) sysArg.arg2;
    *disk   = (int) (long) sysArg.arg3;
    return (int) (long) sysArg.arg4;
} /* end of DiskSize */

/*
 *  Routine:  Mbox_Create
 *
 *  Description: This is the call entry point to create a new mail box.
 *
 *  Arguments:    int   numslots -- number of mailbox slots
 *                int   slotsize -- size of the mailbox buffer
 *                int  *mboxID   -- pointer to output value
 *                (output value: id of created mailbox)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Mbox_Create(int numslots, int slotsize, int *mboxID)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_MBOXCREATE;
    sysArg.arg1 = (void *) (long) numslots;
    sysArg.arg2 = (void *) (long) slotsize;
    USLOSS_Syscall(&sysArg);
    *mboxID = (int) (long) sysArg.arg1;
    return (int) (long) sysArg.arg4;
} /* end of Mbox_Create */


/*
 *  Routine:  Mbox_Release
 *
 *  Description: This is the call entry point to release a mailbox
 *
 *  Arguments: int mbox  -- id of the mailbox
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Mbox_Release(int mboxID)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_MBOXRELEASE;
    sysArg.arg1 = (void *) (long) mboxID;
    USLOSS_Syscall(&sysArg);
    return (int) (long) sysArg.arg4;
} /* end of Mbox_Release */


/*
 *  Routine:  Mbox_Send
 *
 *  Description: This is the call entry point mailbox send.
 *
 *  Arguments:    int mboxID    -- id of the mailbox to send to
 *                int msgSize   -- size of the message
 *                void *msgPtr  -- message to send
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Mbox_Send(int mboxID, void *msgPtr, int msgSize)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_MBOXSEND;
    sysArg.arg1 = (void *) (long) mboxID;
    sysArg.arg2 = (void *) (long) msgPtr;
    sysArg.arg3 = (void *) (long) msgSize;
    USLOSS_Syscall(&sysArg);
    return (int) (long) sysArg.arg4;
} /* end of Mbox_Send */


/*
 *  Routine:  Mbox_Receive
 *
 *  Description: This is the call entry point for terminal input.
 *
 *  Arguments:    int mboxID    -- id of the mailbox to receive from
 *                int msgSize   -- size of the message
 *                void *msgPtr  -- message to receive
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Mbox_Receive(int mboxID, void *msgPtr, int msgSize)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_MBOXRECEIVE;
    sysArg.arg1 = (void *) (long) mboxID;
    sysArg.arg2 = (void *) (long) msgPtr;
    sysArg.arg3 = (void *) (long) msgSize;
    USLOSS_Syscall( &sysArg );
        /*
         * This doesn't belong here. The copy should by done by the
         * system call.
         */
        if ( (int) (long) sysArg.arg4 == -1 )
                return (int) (long) sysArg.arg4;
        memcpy( (char*)msgPtr, (char*)sysArg.arg2, (int) (long) sysArg.arg3);
        return 0;

} /* end of Mbox_Receive */


/*
 *  Routine:  Mbox_CondSend
 *
 *  Description: This is the call entry point mailbox conditional send.
 *
 *  Arguments:    int mboxID    -- id of the mailbox to send to
 *                int msgSize   -- size of the message
 *                void *msgPtr  -- message to send
 *
 *  Return Value: 0 means success, -1 means error occurs, 1 means mailbox
 *                was full
 *
 */
int Mbox_CondSend(int mboxID, void *msgPtr, int msgSize)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_MBOXCONDSEND;
    sysArg.arg1 = (void *) (long) mboxID;
    sysArg.arg2 = (void *) (long) msgPtr;
    sysArg.arg3 = (void *) (long) msgSize;
    USLOSS_Syscall(&sysArg);
    return ((int) (long) sysArg.arg4);
} /* end of Mbox_CondSend */


/*
 *  Routine:  Mbox_CondReceive
 *
 *  Description: This is the call entry point mailbox conditional
 *               receive.
 *
 *  Arguments:    int mboxID    -- id of the mailbox to receive from
 *                int msgSize   -- size of the message
 *                void *msgPtr  -- message to receive
 *
 *  Return Value: 0 means success, -1 means error occurs, 1 means no
 *                message was available
 *
 */
int Mbox_CondReceive(int mboxID, void *msgPtr, int msgSize)
{
    systemArgs sysArg;

    CHECKMODE;
    sysArg.number = SYS_MBOXCONDRECEIVE;
    sysArg.arg1 = (void *) (long) mboxID;
    sysArg.arg2 = (void *) (long) msgPtr;
    sysArg.arg3 = (void *) (long) msgSize;
    USLOSS_Syscall( &sysArg );
    return ((int) (long) sysArg.arg4);
} /* end of Mbox_CondReceive */


/*
 *  Routine:  VmInit
 *
 *  Description: Initializes the virtual memory system.
 *
 *  Arguments:    int mappings -- # of mappings in the MMU
 *                int pages -- # pages in the VM region
 *                int frames -- # physical page frames
 *                int pagers -- # pagers to use
 *
 *  Return Value: address of VM region, NULL if there was an error
 *
 */
int VmInit(int mappings, int pages, int frames, int pagers, void **region)
{
    systemArgs sysArg;
    int result;

    CHECKMODE;

    sysArg.number = SYS_VMINIT;
    sysArg.arg1 = (void *) (long) mappings;
    sysArg.arg2 = (void *) (long) pages;
    sysArg.arg3 = (void *) (long) frames;
    sysArg.arg4 = (void *) (long) pagers;

    USLOSS_Syscall(&sysArg);

    *region = sysArg.arg1;  // return address of VM Region

    result = (int) (long) sysArg.arg4;

    if (sysArg.arg4 == 0) {
        return 0;
    } else {
        return result;
    }
} /* VmInit */


/*
 *  Routine:  VmDestroy
 *
 *  Description: Tears down the VM system
 *
 *  Arguments:
 *
 *  Return Value:
 *
 */

int
VmDestroy(void) {
    systemArgs     sysArg;

    CHECKMODE;
    sysArg.number = SYS_VMDESTROY;
    USLOSS_Syscall(&sysArg);
    return (int) (long) sysArg.arg1;
} /* VmDestroy */


/* end libuser.c */

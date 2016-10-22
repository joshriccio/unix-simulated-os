/*
 *  File:  libuser.c
 *
 *  Description:  This file contains the interface declarations
 *                to the OS kernel support package.
 *
 */

//#include <phase1.h>
//#include <phase2.h>
#include <usyscall.h>
#include <usloss.h>

#define CHECKMODE {    \
    if (USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) { \
        USLOSS_Console("Trying to invoke syscall from kernel\n"); \
        USLOSS_Halt(1);  \
    }  \
}

typedef USLOSS_Sysargs systemArgs;

int Sleep(int seconds) {
    systemArgs sysArg;
        
    CHECKMODE;
    sysArg.number = SYS_SLEEP;
    sysArg.arg1 = ((void *) (long) seconds);

    USLOSS_Syscall(&sysArg);

    return (long) sysArg.arg4;
}

int DiskRead (void *diskBuffer, int unit, int startTrack, int startSector, 
              int sectors, int *status) {
    systemArgs sysArg;
        
    CHECKMODE;
    sysArg.number = SYS_DISKREAD;
    sysArg.arg1 = diskBuffer;
    sysArg.arg2 = ((void *) (long) sectors);
    sysArg.arg3 = ((void *) (long) startTrack);
    sysArg.arg4 = ((void *) (long) startSector);
    sysArg.arg5 = ((void *) (long) unit);

    USLOSS_Syscall(&sysArg);

    *status = (long) sysArg.arg1;
    return (long) sysArg.arg4;
}

int DiskWrite (void *diskBuffer, int unit, int startTrack, int startSector, 
              int sectors, int *status) {
    systemArgs sysArg;
        
    CHECKMODE;
    sysArg.number = SYS_DISKREAD;
    sysArg.arg1 = diskBuffer;
    sysArg.arg2 = ((void *) (long) sectors);
    sysArg.arg3 = ((void *) (long) startTrack);
    sysArg.arg4 = ((void *) (long) startSector);
    sysArg.arg5 = ((void *) (long) unit);

    USLOSS_Syscall(&sysArg);

    *status = (long) sysArg.arg1;
    return (long) sysArg.arg4;
}

int DiskSize (int unit, int *sectorSize, int *sectorsInTrack, 
              int *tracksInDisk) {
    systemArgs sysArg;
        
    CHECKMODE;
    sysArg.number = SYS_DISKSIZE;
    sysArg.arg1 = ((void *) (long) unit);

    USLOSS_Syscall(&sysArg);

    *sectorSize = (long) sysArg.arg1;
    *sectorsInTrack = (long) sysArg.arg2;
    *tracksInDisk = (long) sysArg.arg3;
    return (long) sysArg.arg4;
}

int TermRead (char *buffer, int bufferSize, int unitID, int *numCharsRead) {
    systemArgs sysArg;
        
    CHECKMODE;
    sysArg.number = SYS_TERMREAD;
    sysArg.arg1 = buffer;
    sysArg.arg2 = ((void *) (long) bufferSize);
    sysArg.arg3 = ((void *) (long) unitID);

    USLOSS_Syscall(&sysArg);

    *numCharsRead = (long) sysArg.arg2;
    return (long) sysArg.arg4;
}

int TermWrite(char *buffer, int bufferSize, int unitID, int *numCharsWrite) {
    systemArgs sysArg;
        
    CHECKMODE;
    sysArg.number = SYS_TERMWRITE;
    sysArg.arg1 = buffer;
    sysArg.arg2 = ((void *) (long) bufferSize);
    sysArg.arg3 = ((void *) (long) unitID);

    USLOSS_Syscall(&sysArg);

    *numCharsWrite = (long) sysArg.arg2;
    return (long) sysArg.arg4;
}

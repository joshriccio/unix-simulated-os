/*
 * simple2.c
 * Reads bytes from page 0 of the vmRegion to be sure they are
 * all set to 0.
 *
 */
#include <phase5.h>
#include <usyscall.h>
#include <libuser.h>
#include <usloss.h>
#include <string.h>
#include <assert.h>

#define Tconsole USLOSS_Console

#define TEST        "simple2"
#define PAGES       1
#define CHILDREN    1
#define FRAMES      1
#define PRIORITY    5
#define ITERATIONS  1
#define PAGERS      1
#define MAPPINGS    PAGES


extern void *vmRegion;

int sem;

int
Child(char *arg)
{
    int   pid;
    int   i, memOkay;
    char *buffer;

    GetPID(&pid);
    Tconsole("\nChild(%d): starting\n", pid);

    memOkay = 1;
    buffer = (char *) vmRegion;
    for ( i = 0; i < USLOSS_MmuPageSize(); i++ )
        if ( buffer[i] != 0 )
            memOkay = 0;

    if ( memOkay )
        Tconsole("Child(%d): vmRegion is filled with 0's\n", pid);
    else
        Tconsole("Child(%d): vmRegion is NOT zero-filled!\n", pid);

    assert(vmStats.faults == 1);

    SemV(sem);

    Tconsole("Child(%d): terminating\n\n", pid);

    Terminate(119);
    return 0;
} /* Child */

int
start5(char *arg)
{
    int  pid;
    int  status;

    Tconsole("start5(): Running:    %s\n", TEST);
    Tconsole("start5(): Pagers:     %d\n", PAGERS);
    Tconsole("          Mappings:   %d\n", MAPPINGS);
    Tconsole("          Pages:      %d\n", PAGES);
    Tconsole("          Frames:     %d\n", FRAMES);
    Tconsole("          Children:   %d\n", CHILDREN);
    Tconsole("          Iterations: %d\n", ITERATIONS);
    Tconsole("          Priority:   %d\n", PRIORITY);

    status = VmInit( MAPPINGS, PAGES, FRAMES, PAGERS, &vmRegion );
    Tconsole("start5(): after call to VmInit, status = %d\n\n", status);
    assert(status == 0);
    assert(vmRegion != NULL);

    SemCreate(0, &sem);

    Spawn("Child", Child, NULL, USLOSS_MIN_STACK * 7, PRIORITY, &pid);

    SemP( sem);

    Wait(&pid, &status);
    assert(status == 119);

    Tconsole("start5 done\n");
    //PrintStats();
    VmDestroy();
    Terminate(1);

    return 0;
} /* start5 */

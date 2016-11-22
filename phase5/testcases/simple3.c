/*
 * simple3.c
 * Writes bytes into all 3 pages of the vmRegion.
 * Should see 3 page faults.
 *
 */
#include <phase5.h>
#include <usyscall.h>
#include <libuser.h>
#include <usloss.h>
#include <string.h>
#include <assert.h>

#define Tconsole USLOSS_Console

#define TEST        "simple3"
#define PAGES       3
#define CHILDREN    1
#define FRAMES      3
#define PRIORITY    5
#define ITERATIONS  1
#define PAGERS      1
#define MAPPINGS    PAGES


extern void *vmRegion;

int sem;

int
Child(char *arg)
{
    int  pid;
    int  i;
    char *buffer;

    GetPID(&pid);
    Tconsole("\nChild(%d): starting\n", pid);

    buffer = (char *) vmRegion;
    for (i = 0; i < PAGES * USLOSS_MmuPageSize(); i++) {
        buffer[i] = i % 256;
    }

    assert(vmStats.faults == PAGES);

    SemV(sem);

    Tconsole("Child(%d): terminating\n\n", pid);
    Terminate(121);
    return 0;
} /* Child */


int
start5(char *arg)
{
    int pid;
    int status;

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

    status = SemCreate(0, &sem);
    assert(status == 0);

    Spawn("Child", Child,  0, USLOSS_MIN_STACK*7, PRIORITY, &pid);

    SemP( sem);
    Wait(&pid, &status);
    assert(status == 121);

    Tconsole("start5(): done\n");
    //PrintStats();
    VmDestroy();
    Terminate(1);

    return 0;
} /* start5 */

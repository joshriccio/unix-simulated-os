/*
 * simple.c
 *
 * One process, Writing and Reading the same data from the page with
 * a context switch in between.
 * No disk I/O should occur.  0 replaced pages and 1 page 
 * faults.  Check determanistic statistics.
 */

#include <phase5.h>
#include <usyscall.h>
#include <libuser.h>
#include <usloss.h>
#include <string.h>
#include <assert.h>

#define Tconsole USLOSS_Console

#define TEST        "simple1"
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
    int    pid;
    char   str[64]= "This is the first page";

    GetPID(&pid);
    Tconsole("\nChild(%d): starting\n", pid);

    Tconsole("Child(%d): str = %s\n", pid, str);
    Tconsole("Child(%d): strlen(str) = %d\n", pid, strlen(str));

    memcpy(vmRegion, str, strlen(str)+1);  // +1 to copy nul character

    Tconsole("Child(%d): after memcpy\n", pid);

    if (strcmp(vmRegion, str) == 0)
        Tconsole("Child(%d): strcmp first attempt worked!\n", pid);
    else
        Tconsole("Child(%d): Wrong string read, first attempt\n", pid);

    assert(vmStats.faults == 1);
    assert(vmStats.new == 1);

    SemV(sem);  // forces a context switch with start5

    if (strcmp(vmRegion, str) == 0)
        Tconsole("Child(%d): strcmp second attempt worked!\n", pid);
    else
        Tconsole("Child(%d): Wrong string read, second attempt\n", pid);

    Tconsole("Child(%d): checking various vmStats\n", pid);
    assert(vmStats.faults == 1);
    assert(vmStats.new == 1);
    assert(vmStats.pageOuts == 0);
    assert(vmStats.pageIns == 0);

    Tconsole("Child(%d): terminating\n\n", pid);

    Terminate(117);
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
    assert(status == 117);

    Tconsole("start5(): done\n");
    //PrintStats();
    VmDestroy();
    Terminate(1);

    return 0;
} /* start5 */

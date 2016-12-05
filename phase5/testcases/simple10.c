/*
 * simple10.c
 *
 * Two processes write every page, where frames = pages.
 * There are 2 Pagers.
 */
#include <phase5.h>
#include <usyscall.h>
#include <libuser.h>
#include <usloss.h>
#include <string.h>
#include <assert.h>

#define Tconsole USLOSS_Console

#define TEST        "simple10"
#define PAGES       4
#define CHILDREN    2
#define FRAMES      PAGES
#define PRIORITY    5
#define ITERATIONS  1
#define PAGERS      2
#define MAPPINGS    PAGES

extern void *vmRegion;

int sem;

int
Child(char *arg)
{
    int     pid;
    int     page;
    int     i;
    int    *buffer;
    //VmStats before;
    int     value;

    GetPID(&pid);
    Tconsole("\nChild(%d): starting\n", pid);

    //buffer = (char *) vmRegion;


    for (i = 0; i < ITERATIONS; i++) {
        Tconsole("\nChild(%d): iteration %d\n", pid, i);

        Tconsole("Child(%d): writing one location to each of %d pages\n",
                 pid, PAGES);
        // Write one int to the first location on each of the pages
        // in the VM region.
        for (page = 0; page < PAGES; page++) {
            buffer = (int *) (vmRegion + (page * USLOSS_MmuPageSize()));
            *buffer = pid;
            Tconsole("Child(%d): wrote %d to page %d\n", pid, pid, page);
        }

        Tconsole("Child(%d): reading one location from each of %d pages\n",
                 pid, PAGES);
        // Read one int from the first location on each of the pages
        // in the VM region.
        for (page = 0; page < PAGES; page++) {
            value = * ((int *) (vmRegion + (page * USLOSS_MmuPageSize())));
            Tconsole("Child(%d): page %d, value %d\n", pid, page, value);
            assert(value == pid);
        }

        Tconsole("Child(%d): vmStats.faults = %d\n", pid, vmStats.faults);
    }

    Tconsole("\n");
    SemV(sem);
    Terminate(135);
    return 0;
} /* Child */


int
start5(char *arg)
{
    int  pid[CHILDREN];
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

    for (int i = 0; i < CHILDREN; i++)
        Spawn("Child", Child,  0,USLOSS_MIN_STACK*7,PRIORITY, &pid[i]);

    for (int i = 0; i < CHILDREN; i++)
        SemP( sem);


    for (int i = 0; i < CHILDREN; i++) {
        Wait(&pid[i], &status);
        assert(status == 135);
    }

    Tconsole("start5(): done\n");
    //PrintStats();
    VmDestroy();
    Terminate(1);

    return 0;
} /* start5 */

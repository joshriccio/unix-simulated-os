#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);
char buf[256];


int start1(char *arg)
{
    int status, kidpid, i, j;

    printf("start1(): started\n");

    for (j = 0; j < 2; j++) {
        for (i = 2; i < MAXPROC; i++) {
            kidpid = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
            printf("start1(): after fork of child %d\n", kidpid);
        }

        dumpProcesses();

        for (i = 2; i < MAXPROC; i++) {
            kidpid = join (&status);
            printf("start1(): after join of child %d, status = %d\n",
            kidpid, status);
        }

    }
    return 0;
}

int XXp1(char *arg)
{
    printf("XXp1(): started, pid = %d\n", getpid());
    quit(-getpid());
    return 0;
}


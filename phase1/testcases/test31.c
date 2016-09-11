#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/*
 * The purpose of this test is to demonstrate that
 * an attempt by start1 to zap itself causes an abort
 * 
 * Expected output:
 * start1(): started
 * start1(): after fork of child 3
 * start1(): zapping myself, should cause abort, calling zap(2)
 * zap(): process 2 tried to zap itself.  Halting...
 */

char buf[256];
int pid1;

int XXp1(char *);

int start1(char *arg)
{
    int status, kidpid;

    printf("start1(): started\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    printf("start1(): after fork of child %d\n", pid1);

    printf("start1(): zapping myself, should cause abort, calling zap(%d)\n",
           getpid());
    zap(getpid());

    printf("start1(): performing first join\n");
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status);
    printf("%s", buf);

    return 0;
} /* start1 */

int XXp1(char *arg)
{
    printf("XXp1(): started\n");
    printf("XXp1(): arg = `%s'\n", arg);

    quit(-2);

    return 0;
} /* XXp1 */

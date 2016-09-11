#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/* The purpose of this test is to demonstrate that
 * an attempt to zap yourself causes an abort

 * Expected output:
 * start1(): started
 * start1(): after fork of child 3
 * start1(): performing first join
 * XXp1(): started
 * XXp1(): arg = `XXp1'
 * XXp1(): zapping myself, should cause abort, calling zap(3)
 * zap(): process 3 tried to zap itself.  Halting...
 */

int XXp1(char *);
char buf[256];
int pid1;

int start1(char *arg)
{
    int status, kidpid;

    printf("start1(): started\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    printf("start1(): after fork of child %d\n", pid1);

    printf("start1(): performing first join\n");
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status);
    printf("%s", buf);

    return 0;
} /* start1 */

int XXp1(char *arg)
{
    int zapResult;

    printf("XXp1(): started\n");
    printf("XXp1(): arg = `%s'\n", arg);

    printf("XXp1(): zapping myself, should cause abort, calling zap(%d)\n",
           pid1);
    zapResult = zap(pid1);
    printf("XXp1(): after zap attempt, zapResult = %d\n", zapResult);

    quit(-1);

    return 0;
} /* XXp1 */

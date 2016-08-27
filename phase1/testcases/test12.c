/* this test case checks for deadlocks: 
      start1 forks XXp1 and is blocked on the join of XXp1 
      XXp1 then runs and zaps start1.
   This will result in deadlock, which USLOSS should report.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);
char buf[256];

int start1(char *arg)
{
    int status, pid1, kidpid;
    printf("start1(): started\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    printf("start1(): after fork of child %d\n", pid1);
    printf("start1(): performing join\n");
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status); 
    printf("%s", buf);
    quit(0);
    return 0; /* so gcc will not complain about its absence... */
}

int XXp1(char *arg)
{
    int status;

    printf("XXp1(): started\n");
    printf("XXp1(): arg = `%s'\n", arg);
    printf("XXp1(): about to zap parent -- should result in deadlock\n");
    status = zap(2);
    printf("XXp1(): after zap'ing parent , status = %d\n", status);
    quit(-3);
    return 0;
}

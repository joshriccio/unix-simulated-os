#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/*
 * The purpose of this test is to test when we join and our children have not
 * yet quit (ie we have to block).  One will be done where we have been zapped
 * before calling join, many will be done where we have not been zapped.
 * Expected output:
 * start1(): started
 * start1(): after fork of child 3
 * start1(): after fork of child 4
 * start1(): after fork of child 5
 * start1(): performing first join
 * XXp1(): started
 * XXp1(): arg = `XXp1()'
 * start1(): exit status for child 3 is -1
 * start1(): performing second join
 * XXp2(): started
 * XXp2(): arg = `XXp2'
 * XXp2(): calling zap(5)
 * XXp3(): started
 * XXp3(): arg = `XXp3'
 * XXp3(): after fork of child 6
 * XXp3(): performing first join
 * XXp4(): started
 * XXp4(): arg = `XXp4FromXXp3a'
 * XXp3(): exit status for child -1 is -4
 * start1(): exit status for child 5 is -3
 * XXp2(): return value of zap(5) is 0
 * start1(): exit status for child 4 is -2
 * All processes completed.
 */

int XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);
char buf[256];
int pid3;

int start1(char *arg)
{
    int status, pid1, pid2, kidpid;

    printf("start1(): started\n");
    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    printf("start1(): after fork of child %d\n", pid1);

    pid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 3);
    printf("start1(): after fork of child %d\n", pid2);

    pid3 = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 4);
    printf("start1(): after fork of child %d\n", pid3);

    printf("start1(): performing first join\n");
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status);
    printf("%s", buf);

    printf("start1(): performing second join\n");
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status);
    printf("%s", buf);

    printf("start1(): performing third join\n");
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status);
    printf("%s", buf);

    return 0;
} /* start1 */

int XXp1(char *arg)
{
    printf("XXp1(): started\n");
    printf("XXp1(): arg = `%s'\n", arg);

    quit(-1);

    return 0;
} /* XXp1 */

int XXp2(char *arg)
{
    int zap_result;
    printf("XXp2(): started\n");
    printf("XXp2(): arg = `%s'\n", arg);

    printf("XXp2(): calling zap(%d)\n", pid3);
    zap_result = zap(pid3);
    printf("XXp2(): return value of zap(%d) is %d\n", pid3, zap_result);

    quit(-2);

    return 0;
} /* XXp2 */

int XXp3(char *arg)
{
    int pid1, kidpid, status;

    printf("XXp3(): started\n");
    printf("XXp3(): arg = `%s'\n", arg);

    pid1 = fork1("XXp4", XXp4, "XXp4FromXXp3a", USLOSS_MIN_STACK, 5);
    printf("XXp3(): after fork of child %d\n", pid1);

    printf("XXp3(): performing first join\n");
    kidpid = join(&status);
    sprintf(buf,"XXp3(): exit status for child %d is %d\n", kidpid, status);
    printf("%s", buf);

    quit(-3);

    return 0;
} /* XXp3 */

int XXp4(char *arg)
{
    printf("XXp4(): started\n");
    printf("XXp4(): arg = `%s'\n", arg);

    quit(-4);

    return 0;
} /* XXp4 */

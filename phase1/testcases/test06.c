/* this test is a variation of test case 2 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *);
char buf[256];

int start1(char *arg)
{
    int status, pid1, kid_pid;

    printf("start1(): started\n");
    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    printf("start1(): after fork of child %d\n", pid1);
    printf("start1(): performing join\n");
    kid_pid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kid_pid, status); 
    printf("%s", buf);
    return 0;
}

int XXp1(char *arg)
{
    int kid_pid;
    int status;

    printf("XXp1(): started\n");
    printf("XXp1(): arg = `%s'\n", arg);
    printf("XXp1(): executing fork of first child\n");
    kid_pid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 5);
    printf("XXp1(): fork1 of first child returned pid = %d\n", kid_pid);
    printf("XXp1(): executing fork of second child\n");
    kid_pid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 5);
    printf("XXp1(): fork1 of second child returned pid = %d\n", kid_pid);
    kid_pid = join(&status);
    printf("XXp1(): first join returned kid_pid = %d, status = %d\n",
            kid_pid, status);
    kid_pid = join(&status);
    printf("XXp1(): second join returned kid_pid = %d, status = %d\n",
            kid_pid, status);
    quit(-3);
    return 0;
}

int XXp2(char *arg)
{
    printf("XXp2(): started\n");
    printf("XXp2(): arg = `%s'\n", arg);
    quit(5);
    return 0;
}


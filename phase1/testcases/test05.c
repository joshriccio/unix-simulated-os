#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *);
char buf[256];

int start1(char *arg)
{
    int status, pid1, kidpid;

    printf("start1(): started\n");
    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    printf("start1(): after fork of child %d\n", pid1);
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for child %d is %d\n", kidpid, status); 
    printf("%s", buf);
    return 0;
}

int XXp1(char *arg)
{
    int pid1, kidpid, status;

    printf("XXp1(): started\n");
    printf("XXp1(): arg = `%s'\n", arg);
    pid1 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 2);
    printf("XXp1(): after fork of child %d\n", pid1);
    kidpid = join(&status);
    sprintf(buf,"XXp1(): exit status for child %d is %d\n", kidpid, status); 
    printf("%s", buf);
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


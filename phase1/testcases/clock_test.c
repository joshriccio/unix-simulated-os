
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);
int XXp2(char *);
int XXp3(char *);
char buf[256];
int timeA, timeB;
int start1(char *arg)
{
    int status, pid1, kidpid;

    printf("start1(): started\n");
    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    printf("start1(): after fork of child %d\n", pid1);
    printf("start1(): performing join\n");
    kidpid = join(&status);
    sprintf(buf,"start1(): exit status for process %d is %d\n", kidpid, status); 
    printf("%s", buf);
    quit(0);
    return 0; /* so gcc will not complain about its absence... */
}

int XXp1(char *arg)
{
    int status, kidpid;
    printf("XXp1(): started\n");
    printf("XXp1(): arg = `%s'\n", arg);
    int pid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 2);
    printf("XXp1(): after fork of child %d\n", pid);
    kidpid = join(&status);
    sprintf(buf,"xxp1(): exit status for process %d is %d\n", kidpid, status);
    printf("%s", buf);
    quit(-3);
    return 0;
}

int XXp2(char *arg)
{
    int status;
    printf("XXp2(): started\n");
    printf("XXp2(): arg = `%s'\n", arg);
    int pid = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 2);
    printf("XXp2: after fork of child %d\n", pid);
    int i=0;
    timeA = USLOSS_Clock();
    for(i=0; i< 200000000; i++){
    	if(i%100000000 == 0){
	   dumpProcesses();
           printf("XXp2(): iteration = %d, SystemTime = %d\n", i, USLOSS_Clock());
           printf("XXp2 time - XXp3 time = %d\n", timeA-timeB);
	}	
    }
    int kidpid = join(&status);
    sprintf(buf,"xxp1(): exit status for process %d is %d\n", kidpid, status);
    printf("%s", buf);
    quit(-4);
    return 0;
}

int XXp3(char *arg)
{
    printf("XXp3(): started\n");
    printf("XXp3(): arg = `%s'\n", arg);
    int i=0;
    timeB = USLOSS_Clock();
    for(i=0; i< 200000000; i++){
        if(i%100000000 == 0){
           dumpProcesses();
	   printf("XXp3(): iteration = %d, SystemTime = %d\n", i, USLOSS_Clock());
	   printf("XXp3 time - XXp2 time = %d\n", timeB-timeA);
        }
    }
    quit(-5);
    return 0;
}


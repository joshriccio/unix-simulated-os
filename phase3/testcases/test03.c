/*
 * Simple Spawn test.
 */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>

int Child1(char *);
int Child2(char *);

int start3(char *arg)
{
   int pid;

   printf("start3(): started.  Calling Spawn for Child1\n");
   Spawn("Child1", Child1, NULL, USLOSS_MIN_STACK, 2, &pid);
   printf("start3(): after spawn of %d\n", pid);
   printf("start3(): Parent done. Calling Terminate.\n");
   Terminate(8);

   return 0;
} /* start3 */


int Child1(char *arg) 
{
   int pid;

   printf("Child1(): starting\n");
   Spawn("Child2", Child2, NULL, USLOSS_MIN_STACK, 1, &pid);
   printf("Child1(): done\n");
   Terminate(9);

   return 9;
} /* Child1 */


int Child2(char *arg) 
{

   printf("Child2(): starting\n");
   printf("Child2(): done\n");
   Terminate(7);

   return 7;
} /* Child2 */

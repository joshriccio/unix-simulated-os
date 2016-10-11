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

   printf("Child1(): starting\n");
   printf("Child1(): done\n");
   Terminate(9);

   return 9;
} /* Child1 */

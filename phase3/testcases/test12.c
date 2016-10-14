/*
 * Three process test of GetTimeofDay and CPUTime.
 */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>

int Child1(char *);

int semaphore;

int start3(char *arg)
{
   int pid, status;

   printf("start3(): started\n");

   printf("start3(): calling Spawn for Child1a\n");
   Spawn("Child1a", Child1, "Child1a", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child1b\n");
   Spawn("Child1b", Child1, "Child1b", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child1c\n");
   Spawn("Child1c", Child1, "Child1c", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child2\n");
   Wait(&pid, &status);
   Wait(&pid, &status);
   Wait(&pid, &status);
   printf("start3(): Parent done. Calling Terminate.\n");
   Terminate(8);

   return 0;
} /* start3 */


int Child1(char *my_name) 
{
   int i, j, temp, time;

   printf("%s(): starting\n", my_name);
   for (j = 0; j < 3; j++) {
      for (i = 0; i < 1000; i++)
         temp = 2 + temp;
      GetTimeofDay(&time);
      printf("%s(): current time of day = %d\n", my_name, time);
      CPUTime(&time);
      printf("%s(): current CPU time = %d\n", my_name, time);
   }
   printf("%s(): done\n", my_name);

   return 9;
} /* Child1 */

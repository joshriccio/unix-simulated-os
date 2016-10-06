
/*
 * Two process semaphore test.
 */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>

int Child1(char *);
int Child2(char *);

int semaphore;

int start3(char *arg)
{
   int pid, status;
   int sem_result;

   printf("start3(): started.  Creating semaphore.\n");
   sem_result = SemCreate(0, &semaphore);
   if (sem_result != 0) {
      printf("start3(): got non-zero semaphore result. Terminating...\n");
      Terminate(1);
   }
   printf("start3(): calling Spawn for Child1\n");
   Spawn("Child1", Child1, NULL, USLOSS_MIN_STACK, 2, &pid);
   printf("start3(): after spawn of %d\n", pid);
   printf("start3(): calling Spawn for Child2\n");
   Spawn("Child2", Child2, NULL, USLOSS_MIN_STACK, 2, &pid);
   printf("start3(): after spawn of %d\n", pid);
   Wait(&pid, &status);
   Wait(&pid, &status);
   printf("start3(): Parent done. Calling Terminate.\n");
   Terminate(8);

   return 0;
} /* start3 */


int Child1(char *arg) 
{

   printf("Child1(): starting, P'ing semaphore\n");
   SemP(semaphore);
   printf("Child1(): done\n");

   return 9;
} /* Child1 */


int Child2(char *arg) 
{

   printf("Child2(): starting, V'ing semaphore\n");
   SemV(semaphore);
   printf("Child2(): done\n");

   return 9;
} /* Child1 */

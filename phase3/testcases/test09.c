/*
 * Three process test of freeing a semaphore.
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
   int pid;
   int sem_result;

   printf("start3(): started.  Creating semaphore.\n");
   sem_result = SemCreate(0, &semaphore);
   if (sem_result != 0) {
      printf("start3(): got non-zero semaphore result. Terminating...\n");
      Terminate(1);
   }
   printf("start3(): calling Spawn for Child1a\n");
   Spawn("Child1a", Child1, "Child1a", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child1b\n");
   Spawn("Child1b", Child1, "Child1b", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child1c\n");
   Spawn("Child1c", Child1, "Child1c", USLOSS_MIN_STACK, 1, &pid);
   printf("start3(): calling Spawn for Child2\n");
   Spawn("Child2", Child2, NULL, USLOSS_MIN_STACK, 2, &pid);
   printf("start3(): after spawn of Child2\n");
   printf("start3(): Parent done. Calling Terminate.\n");
   Terminate(8);

   return 0;
} /* start3 */


int Child1(char *arg) 
{

   printf("%s(): starting, P'ing semaphore\n", arg);
   SemP(semaphore);
   printf("%s(): done\n", arg);

   return 9;
} /* Child1 */


int Child2(char *arg) 
{

   printf("Child2(): starting, free'ing semaphore\n");
   SemFree(semaphore);
   printf("Child2(): done\n");

   return 9;
} /* Child1 */

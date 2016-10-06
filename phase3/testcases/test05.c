
/*
 * Max Sem Create test.
 */

#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usloss.h>
#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>

int start3(char *arg)
{
   int semaphore;
   int sem_result;
   int i;

   printf("start3(): started.  Calling SemCreate\n");
   for (i = 0; i < MAXSEMS + 2; i++) {
      sem_result = SemCreate(0, &semaphore);
      printf("i = %3d, sem_result = %2d\n", i, sem_result);
   }
   Terminate(8);

   return 0;
} /* start3 */


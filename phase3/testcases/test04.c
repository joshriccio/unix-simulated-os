/*
 * Simple Sem Create test.
 */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>

int start3(char *arg)
{
    int semaphore;
    int sem_result;

   printf("start3(): started.  Calling SemCreate\n");
   sem_result = SemCreate(0, &semaphore);
   printf("start3(): sem_result = %d, semaphore = %d\n", sem_result, semaphore);
   sem_result = SemCreate(0, &semaphore);
   printf("start3(): sem_result = %d, semaphore = %d\n", sem_result, semaphore);
   Terminate(8);

   return 0;
} /* start3 */


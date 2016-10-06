/*
 * Simple Spawn test.
 */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <usyscall.h>
#include <libuser.h>
#include <stdio.h>


int start3(char *arg)
{

   printf("start3(): started, and immediately return'ing a -3\n");

   return -3;

} /* start3 */

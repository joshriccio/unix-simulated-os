/* TERMTEST
 * Send a negative length to TermRead. Send an invalid terminal
 * number to TermWrite. The routines should return -1.
 */

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <usyscall.h>
#include <libuser.h>
#include <assert.h>


int start4(char *arg)
{
    int len, result;
    char a[13];
    char b[13] = "abcdefghijklm";
  
    USLOSS_Console("start4(): Read a negative number of characters from\n");
    USLOSS_Console("          terminal 1. Write to terminal -1. Should get \n");
    USLOSS_Console("          -1 from both operations since they have\n");
    USLOSS_Console("          invalid arguments.\n");

    result = TermRead(a, -13, 1, &len);
    assert(result == -1);

    result = TermWrite(b, 13, -1, &len);
    assert(result == -1);

    USLOSS_Console("start4(): Done with test of illegal terminal parameters\n");
    Terminate(3);

    return 0;
} /* start4 */

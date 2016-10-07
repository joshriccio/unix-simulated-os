/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS         200
#define EMPTY           0
#define ACTIVE          1
#define MINPRIORITY     5
#define MAXPRIORITY     1

#endif /* _PHASE3_H */

typedef struct procStruct3 procStruct3;
typedef struct procStruct3 * procPtr3;

struct procStruct3 {
   procPtr3        nextProcPtr;
   procPtr3        childProcPtr;
   procPtr3        nextSiblingPtr;
   procPtr3        parentPtr;      // parent process
   procPtr3        quitChildPtr;
   procPtr3        nextQuitSibling;
   procPtr3        whoZapped;
   procPtr3        nextWhoZapped;
   char            name[MAXNAME];  // process name
   char            startArg[MAXARG]; // function arguments
   USLOSS_Context  state;          
   short           pid;           // process ID
   int             priority;      // process priority
   int (* userFunc) (char *);     // process code
   unsigned int    stackSize;     //
   int             status;        // EMPTY or ACTIVE
   int             quitStatus;            
   int             startTime;
   int             zapped;
   int             mboxID;        // mailbox to block on
};

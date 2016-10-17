#ifndef _SEMS_H
#define _SEMS_H

// min and max priorities
#define MINPRIORITY     5
#define MAXPRIORITY     1

// specifice process pids
#define START2_PID      3
#define START3_PID      4

// process table status
#define EMPTY           0
#define ACTIVE          1
#define WAIT_BLOCK      11

#endif

typedef struct procStruct3 *procPtr3;

typedef struct semStruct {
    int count;               // value of the semaphore
    procPtr3 blockedList;    // processes waiting to enter semaphore
    int status;              // EMPTY or ACTIVE
    int mboxID;              // mailbox to block on
} semStruct;

typedef struct procStruct3 {
   procPtr3        childProcPtr;     // process's children 
   procPtr3        nextSiblingPtr;   // next process on parent child list 
   procPtr3        parentPtr;        // parent process
   procPtr3        nextSemBlock;     // next process on semaphore block list 
   char            name[MAXNAME];    // process name
   char            startArg[MAXARG]; // function arguments
   short           pid;              // process ID
   int             priority;         // process priority
   int (* userFunc) (char *);        // process code
   unsigned int    stackSize;        // stack size
   int             status;           // EMPTY or ACTIVE
   int             mboxID;           // mailbox to block on
} procStruct3;

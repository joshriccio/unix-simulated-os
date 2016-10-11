#ifndef _SEMS_H
#define _SEMS_H

#define EMPTY 0
#define ACTIVE 1

#endif

// typedef struct semStruct * semPtr;

typedef struct semStruct {
    int count;
    procPtr3 blockedList;
    int status;
} semStruct;

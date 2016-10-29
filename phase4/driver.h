// process table status
#define EMPTY           0
#define ACTIVE          1

typedef struct procStruct4 *procPtr4;
typedef struct diskDriverInfo *diskDriverInfoPtr;

typedef struct procStruct4 {
   procPtr4        childProcPtr;     // process's children 
   procPtr4        nextSiblingPtr;   // next process on parent child list 
   procPtr4        parentPtr;        // parent process
   procPtr4        sleepPtr;         // sleep list
   int             awakeTime;        // time to be woken up in microseconds
   char            name[MAXNAME];    // process nam
   char            startArg[MAXARG]; // function arguments
   short           pid;              // process ID
   int             priority;         // process priority
   int (* userFunc) (char *);        // process code
   unsigned int    stackSize;        // stack size
   int             status;           // EMPTY or ACTIVE
   int             mboxID;           // mailbox to block on
} procStruct4;

typedef struct diskDriverInfo {
    int requestType;
    int unit;
    int startTrack;
    int startSector;
    int sectors;
    void *buffer;
    int mboxID;
    int status;
    diskDriverInfoPtr next;
} diskDriverInfo;

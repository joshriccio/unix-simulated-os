#define DEBUG 1 //Degub constant

typedef struct procStruct procStruct;
typedef struct procStruct * procPtr;

struct procStruct {
   procPtr         nextProcPtr;
   procPtr         childProcPtr;
   procPtr         nextSiblingPtr;
   procPtr         parentPtr;
   procPtr         quitChildPtr;
   procPtr         nextQuitSibling;
   procPtr         whoZapped;
   procPtr         nextWhoZapped;
   char            name[MAXNAME];     /* process's name */
   char            startArg[MAXARG];  /* args passed to process */
   USLOSS_Context  state;             /* current context for process */
   short           pid;               /* process id */
   int             priority;
   int (* startFunc) (char *);        /* function where process begins -- launch */
   char           *stack;
   unsigned int    stackSize;
   int             status;            /* READY, BLOCKED, QUIT, etc. */
   int             quitStatus;
   int             startTime;
   int             zapped;
};

struct psrBits {
    unsigned int curMode:1;
    unsigned int curIntEnable:1;
    unsigned int prevMode:1;
    unsigned int prevIntEnable:1;
    unsigned int unused:28;
};

union psrValues {
   struct psrBits bits;
   unsigned int integerPart;
};

int getPsrCurMode(int psrValue) {
    return psrValue & 1;
}

int getPsrCurInteruptMode(int psrValue) {
    return psrValue & 2;
}

int getPsrPrevMode(int psrValue) {
    return psrValue & 4;
}

int getPsrPrevInteruptMode(int psrValue) {
    return psrValue & 8;
}

/* Constants defined for phase1 */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY (MINPRIORITY + 1)
#define TIME_SLICE 80000

/* Process statuses */
#define READY 1
#define RUNNING 2
#define QUIT 4
#define EMPTY 5
#define BLOCKED 8
#define JOIN_BLOCKED 9
#define ZAP_BLOCKED 10

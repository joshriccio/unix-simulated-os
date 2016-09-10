/* Patrick's DEBUG printing constant... */
#define DEBUG 1

typedef struct procStruct procStruct;

typedef struct procStruct * procPtr;

//int psrInit(struct psrBits * psr_bits, unsigned int psrValue);

struct procStruct {
   procPtr         nextProcPtr;
   procPtr         childProcPtr;
   procPtr         nextSiblingPtr;
   procPtr         parentPtr;
   procPtr         quitChildPtr;
   procPtr         nextQuitSibling;
   char            name[MAXNAME];     /* process's name */
   char            startArg[MAXARG];  /* args passed to process */
   USLOSS_Context  state;             /* current context for process */
   short           pid;               /* process id */
   int             priority;
   int (* startFunc) (char *);   /* function where process begins -- launch */
   char           *stack;
   unsigned int    stackSize;
   int             status;        /* READY, BLOCKED, QUIT, etc. */
   int             quitStatus;
   int             startTime;
   /* other fields as needed... */
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

// TODO: Untested bits
/*int psrInit(struct psrBits * psr_bits, unsigned int psrValue) {

    psr_bits->curMode = psrValue & 1;
    psr_bits->curIntEnable = psrValue & 2;
    psr_bits->prevMode = psrValue & 4;
    psr_bits->prevIntEnable = psrValue & 8;

    return 0;
}*/

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

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY (MINPRIORITY + 1)
#define TIME_SLICE 80000

//TODO Add more status contants
#define READY 1
#define QUIT 4
#define EMPTY 5
#define BLOCKED 8
#define JOIN_BLOCKED 9
#define ZAP_BLOCKED 10

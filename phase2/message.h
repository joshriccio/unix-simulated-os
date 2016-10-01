#define DEBUG2 1
#define EMPTY 0
#define USED 1
#define ACTIVE 1
#define FAILED 666 // process status

#define SEND_BLOCK 11
#define RECV_BLOCK 12

typedef struct mailbox   mailbox;
typedef struct mboxProc  mboxProc;
typedef struct mailSlot  mailSlot;
typedef struct mailbox  *mailboxPtr;
typedef struct mailSlot *slotPtr;
typedef struct mboxProc *mboxProcPtr;

struct mboxProc {
    short pid;
    int status;
    void * message;
    int msgSize;
    int mboxReleased;
    mboxProcPtr nextBlockSend;
    mboxProcPtr nextBlockRecv;
};

struct mailbox {
    int       mboxID;
    int       numSlots;
    int       slotsUsed;
    int       slotSize;
    mboxProcPtr blockSendList;
    mboxProcPtr blockRecvList;
    slotPtr   slotList;
    int       status;
};

struct mailSlot {
    int       slotID;
    int       mboxID;
    int       status;
    char      message[MAX_MESSAGE];
    int       msgSize;
    slotPtr   nextSlot;
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

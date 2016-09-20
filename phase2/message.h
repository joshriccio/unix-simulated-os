#define DEBUG2 1

typedef struct mailbox   mailbox;
typedef struct mboxProc  mboxProc;
typedef struct mailSlot  mailSlot;
typedef struct mailSlot *slotPtr;
typedef struct mboxProc *mboxProcPtr;

struct mboxProc {
    short pid;
    int zapped;
    int status;
};

struct mailbox {
    int       mboxID;
    int       numSlots;
    slotPtr   slotList;
    int       isEmpty;
};

struct mailSlot {
    int       slotID;
    int       mboxID;
    int       status;
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

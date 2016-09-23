/* ------------------------------------------------------------------------
   phase2.c

   University of Arizona
   Computer Science 452

   ------------------------------------------------------------------------ */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <stdlib.h>

#include "message.h"

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *);
void check_kernel_mode(char * processName);
void disableInterrupts();
void enableInterrupts();
int check_io();
void zeroMailbox(int mboxID);
void zeroSlot(int slotID);
void zeroMboxProc(int pid);
/* -------------------------- Globals ------------------------------------- */

int debugflag2 = 0;

// the mail boxes 
mailbox MailBoxTable[MAXMBOX];

mailSlot SlotTable[MAXSLOTS];

mboxProc MboxProcTable[MAXPROC];
// also need array of mail slots, array of function ptrs to system call 
// handlers, ...




/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start1
   Purpose - Initializes mailboxes and interrupt vector.
             Start the phase2 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - one to indicate normal quit.
   Side Effects - lots since it initializes the phase2 data structures.
   ----------------------------------------------------------------------- */
int start1(char *arg)
{
    int kid_pid;
    int status;

    if (DEBUG2 && debugflag2)
        USLOSS_Console("start1(): at beginning\n");

    check_kernel_mode("start1");

    // Disable interrupts
    disableInterrupts();

    // Initialize the mail box table
    for (int i = 0; i < MAXMBOX; i++) {
        MailBoxTable[i].mboxID = i;
        zeroMailbox(i);
    }
    // slots
    for (int i = 0; i < MAXSLOTS; i++) {
        SlotTable[i].slotID = i;
        zeroSlot(i);
    }
    // process table
    for (int i = 0; i < MAXPROC; i++) {
        zeroMboxProc(i);
    }

    // Initialize USLOSS_IntVec and system call handlers,
    // allocate mailboxes for interrupt handlers.  Etc... 

    enableInterrupts();

    // Create a process for start2, then block on a join until start2 quits
    if (DEBUG2 && debugflag2)
        USLOSS_Console("start1(): fork'ing start2 process\n");
    kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);
    if ( join(&status) != kid_pid ) {
        USLOSS_Console("start2(): join returned something other than ");
        USLOSS_Console("start2's pid\n");
    }

    return 0;
} /* start1 */


/* ------------------------------------------------------------------------
   Name - MboxCreate
   Purpose - gets a free mailbox from the table of mailboxes and initializes it 
   Parameters - maximum number of slots in the mailbox and the max size of a msg
                sent to the mailbox.
   Returns - -1 to indicate that no mailbox was created, or a value >= 0 as the
             mailbox id.
   Side Effects - initializes one element of the mail box array. 
   ----------------------------------------------------------------------- */
int MboxCreate(int slots, int slot_size) {
    check_kernel_mode("MboxCreate");
    disableInterrupts();

    // slots should be a positive integer
    if (slots < 0) {
        return -1;
    }

    for (int i = 0; i < MAXMBOX; i++) {
        if (MailBoxTable[i].status) {
            MailBoxTable[i].numSlots = slots;
            MailBoxTable[i].slotSize = slot_size;
            MailBoxTable[i].status = USED;
            enableInterrupts();
            return i;
        }
    }
    enableInterrupts();
    return -1;
} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name - MboxSend
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size) {
    check_kernel_mode("MboxSend");
    disableInterrupts();

    // error check parameters
    if (mbox_id > MAXMBOX || mbox_id < 0) {
        return -1;
    }

    mailboxPtr mbptr = &MailBoxTable[mbox_id];

    if (msg_size > mbptr->slotSize) {
        return -1;
    }

    // Add process to Process Table
    int pid = getPid();
    MboxProcTable[pid % MAXPROC].pid = pid;
    MboxProcTable[pid % MAXPROC].status = USED;

    // Block if no available slots. Add to next blockSendList
    if (mbptr->numSlots >= mbptr->slotsUsed) {
        blockMe(SEND_BLOCK);
        if (mbptr->blockSendList == NULL) {
            mbptr->blockSendList = &MboxProcTable[pid % MAXPROC];
        } else {
            mboxProcPtr temp = mbptr->blockSendList;
            while (temp->nextBlockSend != NULL) {
                temp = nextBlockSend;
            }
            temp->nextBlockSend = &MboxProcTable[pid % MAXPROC];
        }
    }
    
    // find an empty slot in SlotTable
    int slot;
    for (int i = 0; i < MAXSLOTS; i++) {
        if (SlotTable[i].status == EMPTY) {
           slot = i;
           break;
        }
    }

    // check if process blocked on reveive


    enableInterrupts();
    return -1;
} /* MboxSend */


/* ------------------------------------------------------------------------
   Name - MboxReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
             Block the receiving process if no msg available.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - actual size of msg if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size) {
    return -1;
} /* MboxReceive */

void check_kernel_mode(char * processName) {
    if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0) {
        USLOSS_Console("check_kernal_mode(): called while in user mode, by"
                " process %s. Halting...\n", processName);
        USLOSS_Halt(1);
    } 
}

void enableInterrupts() {
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
}

/*
 * Disables the interrupts.
 */
void disableInterrupts() {
    USLOSS_PsrSet( USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT );
} /* disableInterrupts */

int check_io() {
    return 0;
}

void zeroMailbox(int mboxID) {
    MailBoxTable[mboxID].numSlots = -1;
    MailBoxTable[mboxID].slotsUsed = -1;
    MailBoxTable[mboxID].slotSize = -1;
    MailBoxTable[mboxID].blockSendList = NULL;
    MailBoxTable[mboxID].blockRecvList = NULL;
    MailBoxTable[mboxID].slotList = NULL;
    MailBoxTable[mboxID].status = EMPTY;
}

void zeroSlot(int slotID) {
    SlotTable[slotID].mboxID = -1;
    SlotTable[slotID].status = EMPTY;
    SlotTable[slotID].nextSlot = NULL;
}

void zeroMboxProc(int pid) {
   MboxProcTable[pid % MAXPROC].pid = -1; 
   MboxProcTable[pid % MAXPROC].zapped = -1; 
   MboxProcTable[pid % MAXPROC].status = EMPTY; 
}

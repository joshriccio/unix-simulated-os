/* ------------------------------------------------------------------------
   phase2.c

   University of Arizona
   Computer Science 452

   @author Joshua Riccio
   @author Austin George
   ------------------------------------------------------------------------ */

#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <stdlib.h>
#include <string.h>

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
int MboxRelease(int mailboxID);
int MboxCondSend(int mailboxID, void *message, int messageSize);
int MboxCondReceive(int mailboxID, void *message,int maxMessageSize);
int waitDevice(int type, int unit, int *status);
void nullsys(systemArgs *args);
void clockHandler2(int dev, long unit);
void diskHandler(int dev, long unit);
void termHandler(int dev, long unit);
void syscallHandler(int dev, void *unit);
slotPtr initSlot(int slotIndex, int mboxID, void *msg_ptr, int msg_size);
int getSlotIndex();
int addSlotToList(slotPtr slotToAdd, mailboxPtr mbptr);
/* -------------------------- Globals ------------------------------------- */
int debugflag2 = 0;

// mailbox table and slot array
mailbox MailBoxTable[MAXMBOX];
mailSlot SlotTable[MAXSLOTS];

// Process table
mboxProc MboxProcTable[MAXPROC];

// System call vector
void (*systemCallVec[MAXSYSCALLS])(systemArgs *args);

// Counter used by clock
int clockCounter = 0;

/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start1
   Purpose - Initializes mailboxes and interrupt vector.
             Start the phase2 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - one to indicate normal quit.
   Side Effects - lots since it initializes the phase2 data structures.
   ----------------------------------------------------------------------- */
int start1(char *arg) {
    int kid_pid;
    int status;

    check_kernel_mode("start1");
    disableInterrupts();

    // Initialize the mail box table
    for (int i = 0; i < MAXMBOX; i++) {
        MailBoxTable[i].mboxID = i;
        zeroMailbox(i);
    }

    // create first seven boxes for interrupt handlers
    for (int i = 0; i < 7; i++) {
        MboxCreate(0,0);
    }

    // initialize slot array
    for (int i = 0; i < MAXSLOTS; i++) {
        SlotTable[i].slotID = i;
        zeroSlot(i);
    }

    // initialize process table
    for (int i = 0; i < MAXPROC; i++) {
        zeroMboxProc(i);
    }

    // Initialize USLOSS_IntVec and system call handlers,
    USLOSS_IntVec[USLOSS_CLOCK_INT] = (void*)clockHandler2;
    USLOSS_IntVec[USLOSS_DISK_INT] = (void*)diskHandler;
    USLOSS_IntVec[USLOSS_TERM_INT] = (void*)termHandler;
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = (void*)syscallHandler;

    for (int i = 0; i < MAXSYSCALLS; i++) {
        systemCallVec[i] = nullsys;
    }

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

    // Error checking for parameters
    if (slots < 0) {
        enableInterrupts();
        return -1;
    }
    if (slot_size < 0 || slot_size > MAX_MESSAGE) {
        enableInterrupts();
        return -1;
    }

    // setup next available mailbox from the mailbox table
    for (int i = 0; i < MAXMBOX; i++) {
        if (MailBoxTable[i].status == EMPTY) {
            MailBoxTable[i].numSlots = slots;
            MailBoxTable[i].slotsUsed = 0;
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

    // error checking for parameters and empty mailbox
    if (MailBoxTable[mbox_id].status == EMPTY) {
        enableInterrupts();
        return -1;
    }
    if (mbox_id > MAXMBOX || mbox_id < 0) {
        enableInterrupts();
        return -1;
    }

    mailboxPtr mbptr = &MailBoxTable[mbox_id]; // pointer to mailbox

    if (mbptr->numSlots != 0 && msg_size > mbptr->slotSize) {
        enableInterrupts();
        return -1;
    }

    // Add process to Process Table
    int pid = getpid();
    MboxProcTable[pid % MAXPROC].pid = pid;
    MboxProcTable[pid % MAXPROC].status = ACTIVE;
    MboxProcTable[pid % MAXPROC].message = msg_ptr;
    MboxProcTable[pid % MAXPROC].msgSize = msg_size;

    // Block if no available slots and no process on recv list. 
    // Add to next blockSendList
    if (mbptr->numSlots <= mbptr->slotsUsed && mbptr->blockRecvList == NULL) {
        if (mbptr->blockSendList == NULL) {
            mbptr->blockSendList = &MboxProcTable[pid % MAXPROC];
        } else {
            mboxProcPtr temp = mbptr->blockSendList;
            while (temp->nextBlockSend != NULL) {
                temp = temp->nextBlockSend;
            }
            temp->nextBlockSend = &MboxProcTable[pid % MAXPROC];
        }
        blockMe(SEND_BLOCK);
        if(MboxProcTable[pid % MAXPROC].mboxReleased){
          enableInterrupts();  
          return -3;
        }
        return isZapped() ? -3 : 0;
    }

    // check if process on recieve block list
    if (mbptr->blockRecvList != NULL) {

        // message size bigger than receive buffer size
        if (msg_size > mbptr->blockRecvList->msgSize) {
            mbptr->blockRecvList->status = FAILED;
            int pid = mbptr->blockRecvList->pid;
            mbptr->blockRecvList = mbptr->blockRecvList->nextBlockRecv;
            unblockProc(pid);
            enableInterrupts();
            return -1;
        }
       
        // copy the message to the receive process buffer
        memcpy(mbptr->blockRecvList->message, msg_ptr, msg_size);
        mbptr->blockRecvList->msgSize = msg_size;
        int recvPid = mbptr->blockRecvList->pid;
        mbptr->blockRecvList = mbptr->blockRecvList->nextBlockRecv;
        unblockProc(recvPid);
        enableInterrupts();
        return isZapped() ? -3 : 0;
    }
    
    // find an empty slot in SlotTable
    int slot = getSlotIndex();
    if (slot == -2) {
        USLOSS_Console("MboxSend(): No slots in system. Halting...\n");
        USLOSS_Halt(1);
    }

    // initialize slot
    slotPtr slotToAdd = initSlot(slot, mbptr->mboxID, msg_ptr, msg_size);

    // place found slot on slotList
    addSlotToList(slotToAdd, mbptr);

    enableInterrupts();
    return isZapped() ? -3 : 0;
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
    check_kernel_mode("MboxReceive");
    disableInterrupts();

    // error checking for parameters
    if (MailBoxTable[mbox_id].status == EMPTY) {
        enableInterrupts();
        return -1;
    }

    mailboxPtr mbptr = &MailBoxTable[mbox_id]; // pointer to mailbox

    if (msg_size < 0) {
        enableInterrupts();
        return -1;
    }

    // Add process to Process Table
    int pid = getpid();
    MboxProcTable[pid % MAXPROC].pid = pid;
    MboxProcTable[pid % MAXPROC].status = ACTIVE;
    MboxProcTable[pid % MAXPROC].message = msg_ptr;
    MboxProcTable[pid % MAXPROC].msgSize = msg_size;

    // mailbox is has zero slots and there is a process on send list
    if (mbptr->numSlots == 0 && mbptr->blockSendList != NULL) {
        mboxProcPtr sender = mbptr->blockSendList;
        memcpy(msg_ptr, sender->message, sender->msgSize);
        mbptr->blockSendList = mbptr->blockSendList->nextBlockSend;
        unblockProc(sender->pid);
        return sender->msgSize;
    }
    
    slotPtr slotptr = mbptr->slotList; // pointer to first slot in list

    // block because no message available
    if (slotptr == NULL) {  

        // receive process adds itself to receive list
        if (mbptr->blockRecvList == NULL) {
            mbptr->blockRecvList = &MboxProcTable[pid % MAXPROC];
        } else {
            mboxProcPtr temp = mbptr->blockRecvList;
            while (temp->nextBlockRecv != NULL) {
                temp = temp->nextBlockRecv;
            }
            temp->nextBlockRecv = &MboxProcTable[pid % MAXPROC];
        }

        // block until sender arrives at mailbox
        blockMe(RECV_BLOCK);
        
        // process was zapped or mailbox was released
        if(MboxProcTable[pid % MAXPROC].mboxReleased || isZapped()){
           enableInterrupts(); 
           return -3;
        }

        // failed to receive message from sender
        if(MboxProcTable[pid % MAXPROC].status == FAILED) {
            enableInterrupts();
            return -1;
        }

        enableInterrupts();
        return MboxProcTable[pid % MAXPROC].msgSize;

    } else { // there is a message available on the slot list

        // message size is bigger than receive buffer size
        if (slotptr->msgSize > msg_size) {
            enableInterrupts();
            return -1;
        }

        // copy message into receive messsage buffer
        memcpy(msg_ptr, slotptr->message, slotptr->msgSize);
        mbptr->slotList = slotptr->nextSlot;
        int msgSize = slotptr->msgSize;
        zeroSlot(slotptr->slotID);
        mbptr->slotsUsed--;

        // there is a message on the send list waiting for a slot
        if (mbptr->blockSendList != NULL) {

            // get slot from slot array
            int slotIndex = getSlotIndex();

            // initialize slot with message and message size
            slotPtr slotToAdd = initSlot(slotIndex, mbptr->mboxID,
                    mbptr->blockSendList->message, 
                    mbptr->blockSendList->msgSize);

            // add slot to the slot list
            addSlotToList(slotToAdd, mbptr);
        
            // wake up a process blocked on send list
            int pid = mbptr->blockSendList->pid;
            mbptr->blockSendList = mbptr->blockSendList->nextBlockSend;
            unblockProc(pid);
        }
        enableInterrupts();
        return isZapped() ? -3 : msgSize;
    }
} /* MboxReceive */

/* ------------------------------------------------------------------------
   Name - MboxRelease
   Purpose - Releases the mailbox, and alerts any blocked processes 
             waiting on mailbox.
   Parameters - one, mailboxID, the ID of the mailbox to release
   Returns - 0 = normal, -1 = abnormal, -3 = zapped
   Side Effects - Zeros the mailbox and alert the blocks procs
   ----------------------------------------------------------------------- */
int MboxRelease(int mailboxID) {
    check_kernel_mode("MboxRelease");
    disableInterrupts();

    // error checking for parameters
    if (mailboxID < 0 || mailboxID >= MAXMBOX) {
        enableInterrupts();
        return -1;
    }
    if (MailBoxTable[mailboxID].status == EMPTY) {
        enableInterrupts();
        return -1;
    }
    mailboxPtr mbptr = &MailBoxTable[mailboxID];

    // no processes on send and receive block lists
    if (mbptr->blockSendList == NULL && mbptr->blockRecvList == NULL) {
        zeroMailbox(mailboxID);
        enableInterrupts();
        return isZapped() ? -3 : 0;
    } else {
        mbptr->status = EMPTY; // mark mailbox as being empty

        // mark all processes on block send and recveive list as being released
        while (mbptr->blockSendList != NULL) {
            mbptr->blockSendList->mboxReleased = 1;
            int pid = mbptr->blockSendList->pid;
            mbptr->blockSendList = mbptr->blockSendList->nextBlockSend;
            unblockProc(pid);
            disableInterrupts();
        }
        while (mbptr->blockRecvList != NULL) {
            mbptr->blockRecvList->mboxReleased = 1;
            int pid = mbptr->blockRecvList->pid;
            mbptr->blockRecvList = mbptr->blockRecvList->nextBlockRecv;
            unblockProc(pid);
            disableInterrupts();
        }
    }
    zeroMailbox(mailboxID); // zero members of the mailbox struct
    enableInterrupts();
    return isZapped() ? -3 : 0;
}

/* ------------------------------------------------------------------------
   Name - MboxCondSend
   Purpose - Put a message into a slot for the indicated mailbox.
             return -2 if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size){
    check_kernel_mode("MboxCondSend");
    disableInterrupts();

    // error check parameters
    if (mbox_id > MAXMBOX || mbox_id < 0) {
        enableInterrupts();
        return -1;
    }

    mailboxPtr mbptr = &MailBoxTable[mbox_id];

    if (mbptr->numSlots != 0 && msg_size > mbptr->slotSize) {
        enableInterrupts();
        return -1;
    }

    // Add process to Process Table
    int pid = getpid();
    MboxProcTable[pid % MAXPROC].pid = pid;
    MboxProcTable[pid % MAXPROC].status = ACTIVE;

    // No empty slots in mailbox or no slots in system
    if (mbptr->numSlots != 0 && mbptr->numSlots == mbptr->slotsUsed) {
        return -2;
    }

    // zero slot mailbox and no process blocked on recveive list
    if (mbptr->blockRecvList == NULL && mbptr->numSlots == 0) {
        return -1;
    }

    // check if process on recieve block list
    if (mbptr->blockRecvList != NULL) {
        if (msg_size > mbptr->blockRecvList->msgSize) {
            enableInterrupts();
            return -1;
        }

        // copy message into blocked receive process message buffer
        memcpy(mbptr->blockRecvList->message, msg_ptr, msg_size);
        mbptr->blockRecvList->msgSize = msg_size;
        int recvPid = mbptr->blockRecvList->pid;
        mbptr->blockRecvList = mbptr->blockRecvList->nextBlockRecv;
        unblockProc(recvPid);
        enableInterrupts();
        return isZapped() ? -3 : 0;
    }
    
    // find an empty slot in SlotTable
    int slot = getSlotIndex();
    if (slot == -2) {
        return -2;
    }

    // initialize slot
    slotPtr slotToAdd = initSlot(slot, mbptr->mboxID, msg_ptr, msg_size);

    // place found slot on slotList
    addSlotToList(slotToAdd, mbptr);
    
    enableInterrupts();
    return isZapped() ? -3 : 0;
}

/* ------------------------------------------------------------------------
   Name - MboxCondReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
             return -2 if no msg available.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - actual size of msg if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxCondReceive(int mbox_id, void *msg_ptr,int msg_size){
    check_kernel_mode("MboxCondReceive");
    disableInterrupts();

    // error checking for parameters
    if (MailBoxTable[mbox_id].status == EMPTY) {
        enableInterrupts();
        return -1;
    }

    mailboxPtr mbptr = &MailBoxTable[mbox_id]; // pointer to mailbox

    if (msg_size < 0) {
        enableInterrupts();
        return -1;
    }

    // Add process to Process Table
    int pid = getpid();
    MboxProcTable[pid % MAXPROC].pid = pid;
    MboxProcTable[pid % MAXPROC].status = ACTIVE;
    MboxProcTable[pid % MAXPROC].message = msg_ptr;
    MboxProcTable[pid % MAXPROC].msgSize = msg_size;

    // mailbox has zero slots and there is a process on send list
    if (mbptr->numSlots == 0 && mbptr->blockSendList != NULL) {
        mboxProcPtr sender = mbptr->blockSendList;
        memcpy(msg_ptr, sender->message, sender->msgSize);
        mbptr->blockSendList = mbptr->blockSendList->nextBlockSend;
        unblockProc(sender->pid);
        return sender->msgSize;
    }
    
    slotPtr slotptr = mbptr->slotList; // pointer to first slot in list

    // no message available in slots.
    if (slotptr == NULL) {  
        enableInterrupts();
        return -2;

    } else { // there is a message available on the slot list

        // message size is bigger than receive buffer size
        if (slotptr->msgSize > msg_size) {
            enableInterrupts();
            return -1;
        }

        // copy message into receive messsage buffer
        memcpy(msg_ptr, slotptr->message, slotptr->msgSize);
        mbptr->slotList = slotptr->nextSlot;
        int msgSize = slotptr->msgSize;
        zeroSlot(slotptr->slotID);
        mbptr->slotsUsed--;

        // there is a message on the send list waiting for a slot
        if (mbptr->blockSendList != NULL) {

            // get slot from slot array
            int slotIndex = getSlotIndex();

            // initialize slot with message and message size
            slotPtr slotToAdd = initSlot(slotIndex, mbptr->mboxID,
                    mbptr->blockSendList->message, 
                    mbptr->blockSendList->msgSize);

            // add slot to the slot list
            addSlotToList(slotToAdd, mbptr);
        
            // wake up a process blocked on send list
            int pid = mbptr->blockSendList->pid;
            mbptr->blockSendList = mbptr->blockSendList->nextBlockSend;
            unblockProc(pid);
        }
        enableInterrupts();
        return isZapped() ? -3 : msgSize;
    }
}

/* ------------------------------------------------------------------------
   Name - waitDevice
   Purpose - Block the process on the device until the device sends msg.
   Parameters - type, unit, status
   Returns - -1 if zapped, 0 otherwise
   Side Effects - none.
   ----------------------------------------------------------------------- */
int waitDevice(int type, int unit, int *status){
    check_kernel_mode("waitDevice");
    disableInterrupts();

    int returnCode;               // -1 if process was zapped, 0 otherwise
    int deviceID;                 // the index of the i/o mailbox
    int clockID = 0;              // index of the clock i/o mailbox
    int diskID[] = {1, 2};        // indexes of the disk i/o mailboxes
    int termID[] = {3, 4, 5, 6};  // indexes of the terminal i/o mailboxes

    // determine the index of the i/o mailbox for the given device type and unit
    switch (type) {
        case USLOSS_CLOCK_INT:
            deviceID = clockID;
            break;
        case USLOSS_DISK_INT:
            if (unit >  1 || unit < 0) {
                USLOSS_Console("waitDevice(): invalid unit\n");
            }
            deviceID = diskID[unit];
            break;
        case USLOSS_TERM_INT:
            if (unit >  3 || unit < 0) {
                USLOSS_Console("waitDevice(): invalid unit\n");
            }
            deviceID = termID[unit];
            break;
        default:
            USLOSS_Console("waitDevice(): invalid device or unit type\n");
    }

    // wait for status of device
    returnCode = MboxReceive(deviceID, status, sizeof(int));
    return returnCode == -3 ? -1 : 0;
}

/* 
 *check_kernel_mode
 */
void check_kernel_mode(char * processName) {
    if((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0) {
        USLOSS_Console("check_kernal_mode(): called while in user mode, by"
                " process %s. Halting...\n", processName);
        USLOSS_Halt(1);
    } 
}

/*
 *enable_interrupts 
 */
void enableInterrupts() {
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
}

/*
 * Disables the interrupts.
 */
void disableInterrupts() {
    USLOSS_PsrSet( USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT );
} /* disableInterrupts */

/*
 * checks if any procs are blocked on io mailbox
 */
int check_io() {
    for (int i = 0; i < 7; i++) { 
        if (MailBoxTable[i].blockRecvList != NULL) {
            return 1;
        }
    }
    return 0;
}

/*
 *Zeros all elements of the mailbox for that id
 */
void zeroMailbox(int mboxID) {
    MailBoxTable[mboxID].numSlots = -1;
    MailBoxTable[mboxID].slotsUsed = -1;
    MailBoxTable[mboxID].slotSize = -1;
    MailBoxTable[mboxID].blockSendList = NULL;
    MailBoxTable[mboxID].blockRecvList = NULL;
    MailBoxTable[mboxID].slotList = NULL;
    MailBoxTable[mboxID].status = EMPTY;
}

/*
 *Zeros all elements of the slot for that id
 */
void zeroSlot(int slotID) {
    SlotTable[slotID].mboxID = -1;
    SlotTable[slotID].status = EMPTY;
    SlotTable[slotID].nextSlot = NULL;
}

/*
 *Zeros all elements of the process for that id
 */
void zeroMboxProc(int pid) {
   MboxProcTable[pid % MAXPROC].pid = -1; 
   MboxProcTable[pid % MAXPROC].status = EMPTY; 
   MboxProcTable[pid % MAXPROC].message = NULL; 
   MboxProcTable[pid % MAXPROC].msgSize = -1; 
   MboxProcTable[pid % MAXPROC].mboxReleased = 0; 
   MboxProcTable[pid % MAXPROC].nextBlockSend = NULL; 
   MboxProcTable[pid % MAXPROC].nextBlockRecv = NULL; 
}

/* an error method to handle invalid syscalls */
void nullsys(systemArgs *args)
{
    USLOSS_Console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    USLOSS_Halt(1);
} /* nullsys */

/* ------------------------------------------------------------------------
   Name - clockHandler2
   Purpose - called when interrupt vector is activated for this device
   Parameters - device, unit
   Returns - void
   Side Effects - increases clock counted by 1.
   ----------------------------------------------------------------------- */
void clockHandler2(int dev, long unit) {
    check_kernel_mode("clockHandler2");
    disableInterrupts();

    if (dev != USLOSS_CLOCK_INT || unit != 0) {
        USLOSS_Console("clockHandler2(): wrong device or unit\n");
        USLOSS_Halt(1);
    }
    int status;

    clockCounter++;
    if (clockCounter >= 5) {
        USLOSS_DeviceInput(USLOSS_CLOCK_INT, 0, &status);
        MboxCondSend(0, &status, sizeof(int));
        clockCounter = 0;
    }
    timeSlice();
    enableInterrupts();
} /* clockHandler */

/* ------------------------------------------------------------------------
   Name - diskHandler
   Purpose - called when interrupt vector is activated for this device
   Parameters - device, unit
   Returns - void
   Side Effects - none
   ----------------------------------------------------------------------- */
void diskHandler(int dev, long unit) {
    check_kernel_mode("diskHandler");
    disableInterrupts();

    if (dev != USLOSS_DISK_INT || unit < 0 || unit > 1) {
        USLOSS_Console("diskHandler(): wrong device or unit\n");
        USLOSS_Halt(1);
    }
    int status;
    int mailboxID = unit + 1;

    USLOSS_DeviceInput(USLOSS_DISK_INT, unit, &status);
    MboxCondSend(mailboxID, &status, sizeof(int));
    enableInterrupts();
} /* diskHandler */

/* ------------------------------------------------------------------------
   Name - termHandler
   Purpose - called when interrupt vector is activated for this device
   Parameters - device, unit
   Returns - void
   Side Effects - none
   ----------------------------------------------------------------------- */
void termHandler(int dev, long unit) {
    check_kernel_mode("termHandler");
    disableInterrupts();

    if (dev != USLOSS_TERM_INT || unit < 0 || unit > 3) {
        USLOSS_Console("termHandler(): wrong device or unit\n");
        USLOSS_Halt(1);
    }
    int status;
    int mailboxID = unit + 3;

    USLOSS_DeviceInput(USLOSS_TERM_INT, unit, &status);
    MboxCondSend(mailboxID, &status, sizeof(int));
    enableInterrupts();
} /* termHandler */

/* ------------------------------------------------------------------------
   Name - syscallHandler
   Purpose - called when interrupt vector is activated for this device
   Parameters - device, unit
   Returns - void
   Side Effects - none
   ----------------------------------------------------------------------- */
void syscallHandler(int dev, void *unit) {
    check_kernel_mode("syscallHandler");
    disableInterrupts();

    systemArgs *args = unit;
    int sysCall = args->number;

    if (dev != USLOSS_SYSCALL_INT || sysCall < 0 || sysCall >= MAXSYSCALLS) {
        USLOSS_Console("syscallHandler(): sys number %d is wrong.  Halting...\n",
                sysCall);
        USLOSS_Halt(1);
    }
    (*systemCallVec[sysCall])(args);
    enableInterrupts();
} /* syscallHandler */

/*
 * Returns the index of the next available slot from the slot array or -2 if
 * no available slot.
 */
int getSlotIndex() {
    for (int i = 0; i < MAXSLOTS; i++) {
        if (SlotTable[i].status == EMPTY) {
           return i;
        }
    }
    return -2;
}

/*
 *Initializes a new slot in the slot tables
 */
slotPtr initSlot(int slotIndex, int mboxID, void *msg_ptr, int msg_size) {
    SlotTable[slotIndex].mboxID = mboxID;
    SlotTable[slotIndex].status = USED;
    memcpy(SlotTable[slotIndex].message, msg_ptr, msg_size);
    SlotTable[slotIndex].msgSize = msg_size;
    return &SlotTable[slotIndex];
}

/*
 *Adds a slot to the slot list for a mailbox
 */
int addSlotToList(slotPtr slotToAdd, mailboxPtr mbptr) {
    slotPtr head = mbptr->slotList;
    if (head == NULL) {
        mbptr->slotList = slotToAdd;
    } else {
        while (head->nextSlot != NULL) {
            head = head->nextSlot;
        }
        head->nextSlot = slotToAdd;
    }
    return ++mbptr->slotsUsed;
}

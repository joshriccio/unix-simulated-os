/*
 * vm.h
 */


/*
 * All processes use the same tag.
 */
#define TAG 0

/*
 * Different states for a page.
 */
#define UNUSED 500
#define INCORE 501
#define REFERENCED 502
#define CLEAN 503
#define DIRTY 504
/* You'll probably want more states */


/*
 * Page table entry.
 */
typedef struct PTE {
    int  state;      // See above.
    int  frame;      // Frame that stores the page (if any). -1 if none.
    int  diskBlock;  // Disk block that stores the page (if any). -1 if none.
    // Add more stuff here
} PTE;

/*
 * Per-process information.
 */
typedef struct Process {
    int pid; 	     // process ID
    int vm;          // 1 if using VM, 0 other wise
    int  numPages;   // Size of the page table.
    PTE  *pageTable; // The page table for the process.
    // Add more stuff here */
} Process;

/*
 * Information about page faults. This message is sent by the faulting
 * process to the pager to request that the fault be handled.
 */
typedef struct FaultMsg {
    int  pid;        // Process with the problem.
    void *addr;      // Address that caused the fault.
    int  replyMbox;  // Mailbox to send reply. 1 slot
    // Add more stuff here.
} FaultMsg;

/*
 * Frame table entry.
 */
typedef struct FTE {
    int state;      // See above.
    int pid;        // pid of process using this frame 
    int page;       // Page mapped to frame, -1 if none.
    // Add more stuff here
} FTE;

#define CheckMode() assert(USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE)

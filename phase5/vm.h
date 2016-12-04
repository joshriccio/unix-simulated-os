/*
 * vm.h
 */


/*
 * All processes use the same tag.
 */
#define TAG 0

/* constants for a page */
#define UNMAPPED 0
#define MAPPED 1

/* constants for a frame */
#define UNUSED 0
#define USED 1
#define UNREFERENCED 0
#define REFERENCED USLOSS_MMU_REF
#define CLEAN 0
#define DIRTY USLOSS_MMU_DIRTY

/*
 * Page table entry.
 */
typedef struct PTE {
    int  state;      // constant of a page
    int accessed;
    int  frame;      // Frame that stores the page (if any). -1 if none.
    int pageNum;
    int  diskTableIndex;  // -1 if none, index into disk table array.
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
    int state;      // constant of a frame
    int ref;        // constant of a frame
    int dirty;      // constant of a frame
    int pid;        // pid of process using this frame 
    PTE *page;       // Page mapped to frame, -1 if none.
} FTE;

/*
 * Disk table entry.
 */
typedef struct DTE {
    int state;     // USED 1 or UNUSED 0
    int pid;
    int page;
    int track;
    int sector;
} DTE;
#define CheckMode() assert(USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE)

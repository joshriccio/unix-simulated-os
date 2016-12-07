/*
 * vm.h
 *
 * University of Arizona
 * Computer Science 452
 *
 * @author Joshua Riccio
 * @author Austin George
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
    int  state;      // UNMAPPED or MAPPED
    int accessed;    // page has been written to a frame
    int  frame;      // Frame that stores the page (if any). -1 if none.
    int pageNum;     // number of the page being stored
    int  diskTableIndex;  // -1 if none or index into disk table array.
} PTE;

/*
 * Per-process information.
 */
typedef struct Process {
    int pid; 	     // process ID
    int vm;          // 1 if using VM, 0 other wise
    int  numPages;   // Size of the page table.
    PTE  *pageTable; // The page table for the process.
} Process;

/*
 * Information about page faults. This message is sent by the faulting
 * process to the pager to request that the fault be handled.
 */
typedef struct FaultMsg {
    int  pid;        // Process with the problem.
    void *addr;      // Address that caused the fault.
    int  replyMbox;  // Mailbox to send reply. 1 slot
} FaultMsg;

/*
 * Frame table entry.
 */
typedef struct FTE {
    int state;      // UNUSED or USED
    int ref;        // REFERENCED or UNREFERENCED
    int dirty;      // CLEAN or DIRTY
    int pid;        // pid of process using this frame 
    PTE *page;      // Page mapped to frame, -1 if none.
} FTE;

/*
 * Disk table entry.
 */
typedef struct DTE {
    int state;     // USED or UNUSED 
    int pid;       // process ID being stored in the disk block
    int page;      // page number being stored
    int track;     // track were the page is stored
    int sector;    // sector were the page is stored
} DTE;
#define CheckMode() assert(USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE)

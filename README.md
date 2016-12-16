# CSC452_USLOSS
Unix Operating System kernel that runs on simulated hardware (USLOSS).
<br /><br />
How-To build:
To compile and run the operating system, first extract the usloss-2.9.1.tgz found in the usloss directory, 
then from the phase5 directory copy all files and run make test??. This generates a test executable that will start and run the OS.
<br /><br />
Phase1:
Phase 1 contains the init functions used for initializing the operating system and forking new kernel processes.<br />
int   fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority);<br />
int   join(int *status);<br />
void  quit(int status);<br />
int   zap(int pid);<br />
int   isZapped(void);<br />
int   getpid(void);<br />
void  dumpProcesses(void);<br />
int   blockMe(int block_status);<br />
int   unblockProc(int pid);<br />
int   readCurStartTime(void);<br />
void  timeSlice(void);<br />
void  dispatcher(void);<br />
int   readtime(void);<br />
<br />
Phase 2 contains the functions needed for process blocking on mailbox/ message handling.<br />
int MboxCreate(int slots, int slot_size);<br />
int MboxRelease(int mbox_id);<br />
int MboxSend(int mbox_id, void *msg_ptr, int msg_size);<br />
int MboxReceive(int mbox_id, void *msg_ptr, int msg_max_size);<br />
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size);<br />
int MboxCondReceive(int mbox_id, void *msg_ptr, int msg_max_size);<br />
int waitDevice(int type, int unit, int *status);<br />
<br />
Phase3 conatins functions needed for the spawning of user mode functions, as well as semaphore creation and semv/semp operations.
int  Spawn(char *name, int (*func)(char *), char *arg, int stack_size, int priority, int *pid);<br />
int  Wait(int *pid, int *status);<br />
void Terminate(int status);<br />
void GetTimeofDay(int *tod);<br />
void CPUTime(int *cpu);<br />
void GetPID(int *pid);<br />
int  SemCreate(int value, int *semaphore);<br />
int  SemP(int semaphore);<br />
int  SemV(int semaphore);<br />
int  SemFree(int semaphore);<br />

Phase4 contains drivers for terminal read/write and disk read/write, as well as process sleeping.<br />
int  Sleep(int seconds);<br />
int  DiskRead(void *dbuff, int unit, int track, int first, int sectors,int *status);<br />
int  DiskWrite(void *dbuff, int unit, int track, int first, int sectors,int *status);<br />
int  DiskSize(int unit, int *sector, int *track, int *disk);<br />
int  TermRead(char *buff, int bsize, int unit_id, int *nread);<br />
int  TermWrite(char *buff, int bsize, int unit_id, int *nwrite);<br />

Phase5 contains functions and drivers to handle virtual memory<br />
static void FaultHandler(int  type, void *arg);<br />
static int Pager(char *buf);<br />
void *vmInitReal(int mappings, int pages, int frames, int pagers);<br />
void vmDestroyReal(void);<br />

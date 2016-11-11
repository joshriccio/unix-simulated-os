# CSC452_USLOSS
Unix Operating System kernel that runs on simulated hardware (USLOSS).

How-To build:
To compile and run the operating system, first extract the usloss-2.9.1.tgz found in the usloss directory, 
then from the phase5 directory copy all files and run make test??. This generates a test executable that will start and run the OS.

Phase1:
Phase 1 contains the init functions used for initializing the operating system and forking new kernel processes.
int   fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority);
int   join(int *status);
void  quit(int status);
int   zap(int pid);
int   isZapped(void);
int   getpid(void);
void  dumpProcesses(void);
int   blockMe(int block_status);
int   unblockProc(int pid);
int   readCurStartTime(void);
void  timeSlice(void);
void  dispatcher(void);
int   readtime(void);

Phase 2 contains the functions needed for process blocking on mailbox/ message handling.
int MboxCreate(int slots, int slot_size);
int MboxRelease(int mbox_id);
int MboxSend(int mbox_id, void *msg_ptr, int msg_size);
int MboxReceive(int mbox_id, void *msg_ptr, int msg_max_size);
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size);
int MboxCondReceive(int mbox_id, void *msg_ptr, int msg_max_size);
int waitDevice(int type, int unit, int *status);

Phase3 conatins functions needed for the spawning of user mode functions, as well as semaphore creation and semv/semp operations.
int  Spawn(char *name, int (*func)(char *), char *arg, int stack_size, int priority, int *pid);
int  Wait(int *pid, int *status);
void Terminate(int status);
void GetTimeofDay(int *tod);
void CPUTime(int *cpu);
void GetPID(int *pid);
int  SemCreate(int value, int *semaphore);
int  SemP(int semaphore);
int  SemV(int semaphore);
int  SemFree(int semaphore);

Phase4 contains drivers for terminal read/write and disk read/write, as well as process sleeping.
int  Sleep(int seconds);
int  DiskRead(void *dbuff, int unit, int track, int first, int sectors,int *status);
int  DiskWrite(void *dbuff, int unit, int track, int first, int sectors,int *status);
int  DiskSize(int unit, int *sector, int *track, int *disk);
int  TermRead(char *buff, int bsize, int unit_id, int *nread);
int  TermWrite(char *buff, int bsize, int unit_id, int *nwrite);

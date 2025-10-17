#define NPROC        64  // maximum number of processes
#define NCPU          3  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name

#define SCHED_RR     0     // Round Robin (原始xv6)
#define SCHED_FCFS   1     // First Come First Serve
#define SCHED_PRIORITY 2   // Priority Scheduling
#define SCHED_SJF    3     // Shortest Job First (基于预测)

#define SCHED_POLICY SCHED_RR // 调度策略: SCHED_RR, SCHED_FCFS, SCHED_PRIORITY, SCHED_SJF
#define DEBUG 0


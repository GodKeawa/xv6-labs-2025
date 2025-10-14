#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int count = 0;
int num_procs = 4;

// 简单的工作负载函数
void
work(int id, int workload)
{
  printf("[Process %d] PID:%d Started\n", id, getpid());
  // 执行一些计算工作
  for(int i = 0; i < workload; i++) {
    // 每完成一部分工作
    if(i % 5 == 0) {
      printf("[Process %d] Working... %d/%d\n", id, i, workload);
    }
    
    // 模拟计算工作
    uint64 sum = 0;
    for (int i = 0; i < 1000; i++) {
      for (int j = 0; j < 100000; j++) {
        sum += j;
      }
      for (int k = 10000; k >= 0; k--) {
        sum -= k;
      }
    }
  }
  printf("[Process %d] Finished\n", id);
}

int
main(int argc, char *argv[])
{
  printf("\n========================================\n");
  printf("   Scheduler Test Program\n");
  printf("========================================\n\n");
  
  int workload = 20;  // 每个进程的工作量
  
  // 不同调度算法下的优先级设置
  #if SCHED_POLICY == SCHED_PRIORITY
    setpriority(0);
    // int priorities[] = {0, 10, 20, 30}; // 优先级：高到低
    int priorities[] = {30, 20, 10, 0};  // 优先级：低到高
    printf("Using PRIORITY scheduling\n");
    printf("Process priorities: ");
    for(int i = 0; i < num_procs; i++) {
      printf("P%d=%d ", i, priorities[i]);
    }
    printf("\n\n");
  #else
    printf("Using NonPriority scheduling (RR/FCFS/SJF)\n\n");
  #endif

  printf("Creating %d processes...\n\n", num_procs);
  
  // 创建测试进程
  for(count = 0; count < num_procs; count++) {
    int pid = fork();
    
    if(pid < 0) {
      printf("Fork failed!\n");
      exit(1);
    }
    else if(pid == 0) {
      // 子进程
      yield();
      #if SCHED_POLICY == SCHED_PRIORITY        
        // 如果是优先级调度，设置优先级
        setpriority(priorities[count]);
      #endif
      // 执行工作
      work(count, workload);
      exit(0);
    }
    // 父进程继续循环创建下一个进程
    printf("[Parent] Created process %d", pid);
  }
  
  // 父进程等待所有子进程完成
  for(int i = 0; i < num_procs; i++) {
    wait(0);
  }
   
  printf("\n========================================\n");
  printf("   All processes completed!\n");
  printf("========================================\n\n");
  
  exit(0);
}

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
#if SCHED_POLICY == SCHED_PRIORITY
// 设置进程静态优先级
uint64
sys_setpriority(void)
{
  int priority;
  
  argint(0, &priority);
    
  // 优先级范围检查 (0-31)
  if(priority < 0 || priority > 31)
    return -1;
  
  struct proc *p = myproc();
  acquire(&p->lock);
  p->static_priority = priority;
  p->dynamic_priority = priority;
  p->priority = priority;
  release(&p->lock);
  
  return 0;
}
#else
uint64 sys_setpriority(void) { return 0; } // 也可以不定义，因为有条件编译
#endif

// 获取进程调度信息
uint64
sys_getschedinfo(void)
{
  struct proc *p = myproc();
  
  acquire(&p->lock);
  printf("PID: %d, Name: %s\n", p->pid, p->name);
    
#if SCHED_POLICY == SCHED_PRIORITY
  printf("  Static Priority: %d\n", p->static_priority);
  printf("  Dynamic Priority: %d\n", p->dynamic_priority);
#elif SCHED_POLICY == SCHED_SJF
  printf("  Predicted Burst: %d\n", p->predicted_burst);
  printf("  Total Bursts: %d\n", p->total_bursts);
#endif
  
  release(&p->lock);
  return 0;
}

uint64
sys_yield(void)
{
  yield(); // return void
  return 0;
}

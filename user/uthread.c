#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/* Possible states of a thread: */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4

struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};


struct thread {
  struct context  context;           // 添加一个context的instance, 方便进行上下文切换 
  char            stack[STACK_SIZE]; /* the thread's stack */
  int             state;             /* FREE, RUNNING, RUNNABLE */
};
struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(uint64, uint64);

void layout() {
  return;
  for (int i = 0; i < MAX_THREAD; i++) {
    struct thread* t = all_thread + i;
    printf("thread%d:%l:%l:%l:%l; ", i, t, &t->context, &t->stack, &t->state);
  }
  printf("\n");
}

void debugger() {
  return;
  printf("current_thread:%l #", current_thread);
  for (int i = 0; i < MAX_THREAD; i++) {
    struct thread* t = all_thread + i;
    printf("thread%d: %l:%d;", i, t, t->state);
  }
  printf("\n");
}
              
void 
thread_init(void)
{
  // main() is thread 0, which will make the first invocation to
  // thread_schedule(). It needs a stack so that the first thread_switch() can
  // save thread 0's state.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void 
thread_schedule(void)
{
  struct thread *t, *next_thread;
  debugger();
  /* Find another runnable thread. */
  next_thread = 0;
  t = current_thread + 1;
  for(int i = 0; i < MAX_THREAD; i++){
    if(t >= all_thread + MAX_THREAD) // 循环检测，只检测4次
      t = all_thread;
    if(t->state == RUNNABLE) {
      next_thread = t;
      break;
    }
    t = t + 1;
  }
  debugger();
  if (next_thread == 0) {
    printf("thread_schedule: no runnable threads\n");
    exit(-1);
  }
  // DEBUG
  // printf("switching context: %l -> %l\n", current_thread, next_thread);
  // printf("func: %l -> %l\n", current_thread->context.ra, next_thread->context.ra);
  if (current_thread != next_thread) {         /* switch threads?*/
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
    debugger();
    thread_switch((uint64)&t->context, (uint64)&next_thread->context); // 上下文切换
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break; // 找到线程空位
  }
  t->state = RUNNABLE;
  // YOUR CODE HERE
  // 创建一个线程，即伪造一个运行环境，包括所有寄存器和栈
  // 先给线程加上一个context的字段，用来保存上下文
  t->context.ra = (uint64)func;        // 伪造函数起始点
  t->context.sp = (uint64)&(t->stack[STACK_SIZE]); // 伪造栈空间,注意栈是从高地址向低地址增长的
  // 其他寄存器保持默认，调度时上下文切换会使得函数被调用
  debugger();
}

void 
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule(); // 调度在全部a,b,c线程启动完成之前，都会最终回到main
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void 
thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while(b_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while(a_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while(a_started == 0 || b_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int 
main(int argc, char *argv[]) 
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);
  current_thread->state = FREE;
  debugger();
  layout();
  thread_schedule();
  exit(0);
}

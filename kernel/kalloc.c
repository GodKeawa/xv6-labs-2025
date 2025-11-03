// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmem[NCPU];
char kmem_names[NCPU][16];

// 内部的free函数，直接带有cpuid进行free
void
_free(void* pa, int id) {
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("_free");

  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
}

// 内部的freerange，带有cpuid，释放一段内存
void
_freerange(void* pa_start, void* pa_end, int id) {
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    _free(p, id);
}

void
kinit()
{
  void* base = (void*)end;
  void* tail = (void*)PHYSTOP;
  uint64 length = (tail - base) / NCPU; // 均匀分配内存段
  for (int i = 0; i < NCPU; i++) {
    snprintf(kmem_names[i], 16, "kmem-%d", i);
    initlock(&kmem[i].lock, kmem_names[i]);
    _freerange(base + i * length, base + (i + 1) * length, i);// 分配对应段的内存
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  
  push_off();// 关闭中断，保证cpuid可以正常使用
  int id = cpuid();               // 直接把释放的内存放入当前cpu的freelist即可
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  int id;
  struct run *r;
  // 循环访问所有cpu，但从自己开始
  push_off();
  for (int i = cpuid(), j = 0; j < NCPU; i++, j++) {
    id = i % NCPU; // 保存当前id
    acquire(&kmem[id].lock);
    r = kmem[id].freelist;
    if(r) { // 先看看自己有没有内存，有就直接拿出来
      kmem[id].freelist = r->next;
      release(&kmem[id].lock);
      break;
    } else { // 如果自己没有，就去访问其他cpu的内存，不断迭代
      release(&kmem[id].lock); // 把这一次循环的锁先释放
    }
  }
  pop_off();
  if(r) memset((char*)r, 5, PGSIZE);
  return (void*)r;
}

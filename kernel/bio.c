// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock locks[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf heads[NBUCKET];
  // 每个锁锁住一个桶
} bcache;

char bcache_names[NBUCKET][16];

void
binit(void)
{
  for (int i = 0; i < NBUCKET; i++) {
    snprintf(bcache_names[i], 16, "bcache-%d", i);
    initlock(&bcache.locks[i], bcache_names[i]);

    // Create linked list heads of buffers
    bcache.heads[i].prev = &bcache.heads[i];
    bcache.heads[i].next = &bcache.heads[i];
  }
  
  struct buf *b;
  int id;
  // 预分配buffer到每个桶, 循环分配，保证基本均匀
  for(b = bcache.buf, id = 0; b < bcache.buf+NBUF; b++, id = (id+1)%NBUCKET) {
    b->next = bcache.heads[id].next;
    b->prev = &bcache.heads[id];
    initsleeplock(&b->lock, "buffer");
    bcache.heads[id].next->prev = b;
    bcache.heads[id].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = blockno % NBUCKET;

  acquire(&bcache.locks[id]);

  // Is the block already cached?
  for(b = bcache.heads[id].next; b != &bcache.heads[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.heads[id].prev; b != &bcache.heads[id]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 预分配的数量可能不足，这里同样可能需要抢buffer过来,从自己的下一个桶开始循环
  for (int i = (id + 1) % NBUCKET; i != id; i = (i+1)%NBUCKET) {
    acquire(&bcache.locks[i]); // 先拿到锁
    for(b = bcache.heads[i].prev; b != &bcache.heads[i]; b = b->prev){
      if(b->refcnt == 0) { // 找到一个可用的buffer
        b->prev->next = b->next;  // 先把b拿出来
        b->next->prev = b->prev;
        release(&bcache.locks[i]);  // 释放这个桶的锁
        // 设置好b的状态
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // 把b装进桶，放在head后面，表示最近刚访问
        b->next = bcache.heads[id].next;
        b->prev = &bcache.heads[id];
        bcache.heads[id].next->prev = b;
        bcache.heads[id].next = b;
        // 释放这个桶，返回新buffer
        release(&bcache.locks[id]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    // 这次循环没找到，释放锁
    release(&bcache.locks[i]);
  }
  // 完全没buffer位置了
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  
  int id = b->blockno % NBUCKET;
  acquire(&bcache.locks[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.heads[id].next;
    b->prev = &bcache.heads[id];
    bcache.heads[id].next->prev = b;
    bcache.heads[id].next = b;
  }
  
  release(&bcache.locks[id]);
}

void
bpin(struct buf *b) {
  int id = b->blockno % NBUCKET; 
  acquire(&bcache.locks[id]);
  b->refcnt++;
  release(&bcache.locks[id]);
}

void
bunpin(struct buf *b) {
  int id = b->blockno % NBUCKET; 
  acquire(&bcache.locks[id]);
  b->refcnt--;
  release(&bcache.locks[id]);
}



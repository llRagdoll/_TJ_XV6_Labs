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

#define BUCKET_NUM 13
#define BUFFER_SIZE 5



struct {
  struct spinlock lock;
  struct buf buf[BUFFER_SIZE];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache[BUCKET_NUM];

extern uint ticks;

void
binit(void)
{
  //struct buf *b;
  for(int i=0;i<BUCKET_NUM;++i){
  initlock(&bcache[i].lock, "bcache");

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
    for(int j=0;j<BUFFER_SIZE;++j){
      // b->next = bcache.head.next;
      // b->prev = &bcache.head;
      initsleeplock(&bcache[i].buf[j].lock, "buffer");
      // bcache.head.next->prev = b;
      // bcache.head.next = b;
    }
}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucket_num;
  bucket_num=(blockno%BUCKET_NUM);

  acquire(&bcache[bucket_num].lock);

  // Is the block already cached?
  for(int i=0;i<BUFFER_SIZE;++i){
    b=&bcache[bucket_num].buf[i]; 
    if(b->dev==dev&&b->blockno==blockno){
      b->lastuse=ticks;
      b->refcnt++;  
      release(&bcache[bucket_num].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint least=0xFFFFFFFF;
  int index=-1;
  for(int i=0;i<BUFFER_SIZE;++i){
    b=&bcache[bucket_num].buf[i]; 
    if(b->refcnt==0&&b->lastuse<least){
      index=i;
      least=b->lastuse;
    }
  }
  if(index==-1){
    panic("bget:no available buffer");
  }
  b = &bcache[bucket_num].buf[index];
  b->lastuse = ticks;
  b->dev = dev;
  b->blockno = blockno;
  b->refcnt = 1;
  b->valid = 0;
  
  release(&bcache[bucket_num].lock);
  acquiresleep(&b->lock);

  return b;
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
  
  int bucket_num=(b->blockno%BUCKET_NUM);
  acquire(&bcache[bucket_num].lock);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  release(&bcache[bucket_num].lock);
  
}

void
bpin(struct buf *b) {
  int bucket_num=(b->blockno%BUCKET_NUM);
  acquire(&bcache[bucket_num].lock);
  b->refcnt++;
  release(&bcache[bucket_num].lock);
}

void
bunpin(struct buf *b) {
  int bucket_num=(b->blockno%BUCKET_NUM);
  acquire(&bcache[bucket_num].lock);
  b->refcnt--;
  release(&bcache[bucket_num].lock);
}



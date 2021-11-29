#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id = 0;
    shm_table.shm_pages[i].frame = 0;
    shm_table.shm_pages[i].refcnt = 0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {

  //you write this
  //64 pages of memory in the shm_table to see if segment id exists
    // if no, then needs to be allocated
      // acquire lock while doing this
    // if yes, increase reference count use "mappages" function
    // to add mapping betweeen VA and PA
  struct proc* p = myproc();
  char* pa;
  int i;
  int found = 0, allocated = 0;
  acquire(&(shm_table.lock));  // grabbing the lock... should we use uspinlock instead?
  for (i = 0; i < 64; ++i){
    if(shm_table.shm_pages[i].id == id){
      found = 1; allocated = 1;
      // then segment already exists
      ++shm_table.shm_pages[i].refcnt; // increment reference count
      pa = shm_table.shm_pages[i].frame;
      cprintf("old pa=%x, v2p=%x\n", pa, V2P(pa));
      *pointer = (char*)PGROUNDUP(p->sz);
      mappages(p->pgdir, *pointer, PGSIZE, V2P(pa), PTE_W|PTE_U);
      break;
    }
  }
  // if we made it here then we need to allocate a new page
  if(!found){
    for(i = 0; i < 64; ++i){
      if(shm_table.shm_pages[i].id == 0){  // empty page has id 0
        allocated = 1;
        shm_table.shm_pages[i].id = id;
        shm_table.shm_pages[i].frame = kalloc();
        pa = shm_table.shm_pages[i].frame;
        shm_table.shm_pages[i].refcnt = 1;
        cprintf("new pa=%x, v2p=%x\n", pa, V2P(pa));
        *pointer = (char*)PGROUNDUP(p->sz);
        mappages(p->pgdir, *pointer, PGSIZE, V2P(pa), PTE_W|PTE_U);
        break;
      }
    }
  }
  release(&(shm_table.lock));
  // since pointer is the virtual address, just set the pointer to point to VA
  if(!allocated){
    // something went REALLY REALLY wrong
    panic("shm_open");
  }
  p->sz += PGSIZE;
  return 0; //added to remove compiler warning -- you should decide what to return
    // i dont think it matters what we return because we're not ever directly using that value
}


int shm_close(int id) {
//you write this too!
  int closed = 0, i;
  cprintf("closing start\n");
  acquire(&(shm_table.lock));
  for(i = 0; i < 64; ++i){
    if(shm_table.shm_pages[i].id == id){
      --shm_table.shm_pages[i].refcnt;
      if(!shm_table.shm_pages[i].refcnt){
        shm_table.shm_pages[i].id = 0;
      }
      closed = 1;
      break;
    }
  }
  release(&(shm_table.lock));
  if(!closed){ // we couldn't find that id in the shm_table
    panic("shm_close");
  }
  cprintf("closing done\n");
  return 0; //added to remove compiler warning -- you should decide what to return
}

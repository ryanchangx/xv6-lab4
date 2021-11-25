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
  void* va;
  int i;
  int found = 0, allocated = 0;
  acquire(&(shm_table.lock));  // grabbing the lock... should we use uspinlock instead?
  for (i = 0; i < 64; ++i){
    if(shm_table.shm_pages[i].id == id){
      found = 1; allocated = 1;
      // then segment already exists
      ++shm_table.shm_pages[i].refcnt; // increment reference count
      va = shm_table.shm_pages[i].frame;
      // use mappages
      mappages(p->pgdir, (void*)PGROUNDUP(p->sz), PGSIZE, V2P(va), PTE_W|PTE_U);
      *pointer = (char*)va;
      break;
    }
  }
  // if we made it here then we need to allocate a new page
  if(!found){
    for(i = 0; i < 64; ++i){
      if(shm_table.shm_pages[i].id == 0){  // empty page has id 0
        allocated = 1;
        shm_table.shm_pages[i].id = id;
        va = shm_table.shm_pages[i].frame = kalloc();
        shm_table.shm_pages[i].refcnt = 1;
        cprintf("va=%d\n", va);
        mappages(p->pgdir, va, PGSIZE, V2P(va), PTE_W|PTE_U);
        *pointer = (char*)va;
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
  return (int)p->pgdir; //added to remove compiler warning -- you should decide what to return
    // i dont think it matters what we return because we're not ever directly using that value
}


int shm_close(int id) {
//you write this too!
int closed = 0, i;
  for(i = 0; i < 64; ++i){
    if(shm_table.shm_pages[i].id == id){
      --shm_table.shm_pages[i].refcnt;
      if(!shm_table.shm_pages[i].refcnt){
        shm_table.shm_pages[i].id = 0;
        shm_table.shm_pages[i].frame = 0;
        shm_table.shm_pages[i].refcnt = 0;
      }
      closed = 1;
      break;
    }
  }
  if(!closed){ // we couldn't find that id in the shm_table
    panic("shm_close");
  }
  return 0; //added to remove compiler warning -- you should decide what to return
}

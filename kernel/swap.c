#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define MAX_MRU_PAGES 128   // limit MRU list size
#define MAX_SWAP_BLOCKS 1024

// Global MRU list
struct {
  struct spinlock lock;
  struct mru_node *head;
  struct mru_node *tail;
  int size;
} mru_list;

// Swap storage
char swap_storage[MAX_SWAP_BLOCKS][PGSIZE];
struct spinlock swap_lock;

void
mru_init(void)
{
  initlock(&mru_list.lock, "mru");
  initlock(&swap_lock, "swap");
  mru_list.head = 0;
  mru_list.tail = 0;
  mru_list.size = 0;
}

// Move page to front (most recently used)
void
mru_access(struct proc *p, uint64 va)
{
  struct mru_node *node;
  va = PGROUNDDOWN(va);

  acquire(&mru_list.lock);

  // Check if page already in MRU list
  for(node = mru_list.head; node; node = node->next){
    if(node->p == p && node->va == va){
      if(node == mru_list.head){
        release(&mru_list.lock);
        return;
      }

      // Remove from current position
      if(node->prev) node->prev->next = node->next;
      if(node->next) node->next->prev = node->prev;
      if(node == mru_list.tail) mru_list.tail = node->prev;

      // Insert at head
      node->next = mru_list.head;
      node->prev = 0;
      mru_list.head->prev = node;
      mru_list.head = node;

      release(&mru_list.lock);
      return;
    }
  }

  // Not in list -> allocate new node
  node = (struct mru_node*)kalloc();
  if(!node){
    release(&mru_list.lock);
    return;
  }
  memset(node, 0, sizeof(*node));
  node->p = p;
  node->va = va;

  node->next = mru_list.head;
  node->prev = 0;
  if(mru_list.head) mru_list.head->prev = node;
  mru_list.head = node;
  if(!mru_list.tail) mru_list.tail = node;
  mru_list.size++;

  // Limit MRU list size
  if(mru_list.size > MAX_MRU_PAGES){
    struct mru_node *old_tail = mru_list.tail;
    if(old_tail->prev) old_tail->prev->next = 0;
    mru_list.tail = old_tail->prev;
    mru_list.size--;
    kfree((char*)old_tail);
  }

  release(&mru_list.lock);
}

// Remove a specific page from MRU
void
mru_remove(struct proc *p, uint64 va)
{
  struct mru_node *node, *next;
  va = PGROUNDDOWN(va);

  acquire(&mru_list.lock);
  for(node = mru_list.head; node; node = next){
    next = node->next;
    if(node->p == p && node->va == va){
      if(node->prev) node->prev->next = node->next;
      else mru_list.head = node->next;

      if(node->next) node->next->prev = node->prev;
      else mru_list.tail = node->prev;

      mru_list.size--;
      kfree((char*)node);
      break;
    }
  }
  release(&mru_list.lock);
}

// Remove all pages of a process
void
mru_remove_proc(struct proc *p)
{
  struct mru_node *node, *next;

  acquire(&mru_list.lock);
  for(node = mru_list.head; node; node = next){
    next = node->next;
    if(node->p == p){
      if(node->prev) node->prev->next = node->next;
      else mru_list.head = node->next;

      if(node->next) node->next->prev = node->prev;
      else mru_list.tail = node->prev;

      mru_list.size--;
      kfree((char*)node);
    }
  }
  release(&mru_list.lock);
}

// Evict least recently used page
int
swapout(void)
{
  struct mru_node *victim;
  struct proc *p;
  uint64 va;
  pte_t *pte;
  uint64 pa;
  int i;

  acquire(&mru_list.lock);
  if(!mru_list.tail){
    release(&mru_list.lock);
    return -1;
  }

  victim = mru_list.tail;  // LRU page
  p = victim->p;
  va = victim->va;

  // Remove from MRU list
  if(victim->prev) victim->prev->next = 0;
  mru_list.tail = victim->prev;
  if(!mru_list.tail) mru_list.head = 0;
  mru_list.size--;
  release(&mru_list.lock);

  pte = walk(p->pagetable, va, 0);
  if(!pte || !(*pte & PTE_V)){
    kfree(victim);
    return -1;
  }

  pa = PTE2PA(*pte);

  // Allocate swap slot
  acquire(&swap_lock);
  for(i=0;i<MAX_SWAP_BLOCKS;i++){
    if(p->swapblocks[i]==0){
      p->swapblocks[i]=1;
      break;
    }
  }
  release(&swap_lock);

  if(i==MAX_SWAP_BLOCKS){
    kfree(victim);
    return -1;
  }

  // Copy page to swap storage
  acquire(&swap_lock);
  memmove(swap_storage[i], (void*)pa, PGSIZE);
  release(&swap_lock);

  // Store swap index in PTE but keep flags
  *pte = (i << 10) | (*pte & 0x3FF);  // lower 10 bits: flags
  *pte &= ~PTE_V;

  kfree((char*)pa);
  sfence_vma();
  p->num_swapouts++;
  kfree(victim);
  return 0;
}

// Swap in a page
int
swapin(struct proc *p, uint64 va)
{
  pte_t *pte;
  int swapidx;
  char *mem;

  va = PGROUNDDOWN(va);
  pte = walk(p->pagetable, va, 1);
  if(!pte) return -1;

  if(*pte & PTE_V) return 0; // already in memory

  mem = kalloc();
  if(!mem){
    if(swapout()<0) return -1;
    mem = kalloc();
    if(!mem) return -1;
  }

  swapidx = (*pte >> 10) & 0xFFFFF;
  if(swapidx < MAX_SWAP_BLOCKS && p->swapblocks[swapidx]){
    acquire(&swap_lock);
    memmove(mem, swap_storage[swapidx], PGSIZE);
    p->swapblocks[swapidx] = 0;
    release(&swap_lock);
    p->num_swapins++;
  } else {
    memset(mem, 0, PGSIZE);
  }

  *pte = PA2PTE((uint64)mem) | PTE_V | PTE_U | PTE_R | PTE_W | PTE_X;

  mru_access(p, va);
  return 0;
}

// Debug: dump MRU list
void
dumpmru_internal(void)
{
  struct mru_node *node;

  printf("=== MRU List (Most Recently Used first) ===\n");
  acquire(&mru_list.lock);
  if(!mru_list.head){
    printf("MRU list is empty\n");
    release(&mru_list.lock);
    return;
  }

  printf("Total pages in MRU: %d\n", mru_list.size);
  for(node = mru_list.head; node; node = node->next){
    if(node->p)
      printf("PID: %d, VA: 0x%lx\n", node->p->pid, node->va);
  }
  release(&mru_list.lock);
  printf("===========================================\n");
}

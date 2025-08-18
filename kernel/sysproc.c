#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

extern struct proc proc[]; 

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

uint64
sys_getprocsinfo(void) {
    uint64 addr;
    argaddr(0, &addr);

    struct uproc up[NPROC];
    int count = 0;

    for (int i = 0; i < NPROC; i++) {
        struct proc *p = &proc[i];
        acquire(&p->lock);
        if (p->state != UNUSED) {
            up[count].pid = p->pid;
            up[count].state = p->state;
            up[count].ticks = p->ticks;
            safestrcpy(up[count].name, p->name, sizeof(p->name));
            count++;
        }
        release(&p->lock);
    }

    if (copyout(myproc()->pagetable, addr, (char *)up, count * sizeof(struct uproc)) < 0)
        return -1;

    return count;
}

// sets the priority of process
uint64
sys_set_priority(void)
{
  int n;
  argint(0, &n); 

  if(n < 1 || n > 1000)
    return -1;

  struct proc *p = myproc();
  acquire(&p->lock);
  p->priority = n;
  release(&p->lock);

  return 0;
}

// gets the priority of process
uint64
sys_get_priority(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  int priority = p->priority;
  release(&p->lock);
  return priority;
}

// sets the tickets of process
uint64
sys_set_tickets(void)
{
  int a0, a1;
  
  // always fetch two integers (even if program passed only one,
  // the second will just be garbage, so we check validity)
  argint(0, &a0);
  argint(1, &a1);

  // Case 1: only one argument used → self set
  if(a1 == 0){
    int n = a0;
    if(n < 1 || n > 1000)
      return -1;

    struct proc *p = myproc();
    acquire(&p->lock);
    p->tickets = n;
    release(&p->lock);
    return 0;
  }

  // Case 2: two arguments → parent sets child
  int pid = a0;
  int n   = a1;

  if(n < 1 || n > 1000)
    return -1;

  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->tickets = n;
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }

  return -1; // pid not found
}

// gets the tickets of process
uint64
sys_get_tickets(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  int tickets = p->tickets;
  release(&p->lock);
  return tickets;
}


uint64
sys_get_ticks(void)
{
  int pid;
  argint(0, &pid);
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      uint64 t = p->ticks;
      release(&p->lock);
      return t;
    }
    release(&p->lock);
  }
  return -1;
}

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NCHILD 3
#define RUNTIME_TICKS 500

int
main(void)
{
  int i;
  int pids[NCHILD];
  int tickets[NCHILD] = {20, 20, 20}; // start equal

  printf("\nLottery Scheduler Test 2: Dynamic Tickets\n");
  printf("========================================\n");

  // fork children
  for(i = 0; i < NCHILD; i++){
    pids[i] = fork();
    if(pids[i] < 0){
      printf("fork failed\n");
      exit(1);
    }
    if(pids[i] == 0){
      // child
      set_tickets(tickets[i]);  // self set
      volatile unsigned long x = 0;
      while(1){
        x += 1;   // busy loop
      }
      exit(0);
    }
  }

  // Let them run for a while (phase 1)
  sleep(RUNTIME_TICKS / 2);

  printf("\n--- Phase 1 (equal tickets: 20 each) ---\n");
  for(i = 0; i < NCHILD; i++){
    int t = get_ticks(pids[i]);
    printf("Child PID %d | tickets=%d | ticks=%d\n", pids[i], tickets[i], t);
  }

  // Change tickets dynamically (parent controls children)
  set_tickets(pids[0], 10);
  set_tickets(pids[1], 50);
  set_tickets(pids[2], 100);
  tickets[0] = 10; tickets[1] = 50; tickets[2] = 100;

  printf("\n--- Changed tickets: 10, 50, 100 ---\n");

  // Run for phase 2
  sleep(RUNTIME_TICKS / 2);

  printf("\n--- Phase 2 (after ticket change) ---\n");
  for(i = 0; i < NCHILD; i++){
    int t = get_ticks(pids[i]);
    printf("Child PID %d | tickets=%d | ticks=%d\n", pids[i], tickets[i], t);
  }

  // kill children
  for(i = 0; i < NCHILD; i++){
    kill(pids[i]);
    wait(0);
  }

  exit(0);
}

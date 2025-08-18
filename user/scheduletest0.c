#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NCHILD 3
#define RUNTIME_TICKS 400

int
main(void)
{
  int i;
  int pids[NCHILD];
  int tickets[NCHILD] = {10, 30, 60};

  printf("Lottery Scheduler Test\n");
  printf("======================\n");
  printf("Creating %d children with tickets: ", NCHILD);
  for(i = 0; i < NCHILD; i++){
    printf("%d ", tickets[i]);
  }
  printf("\n\n");

  // fork children
  for(i = 0; i < NCHILD; i++){
    pids[i] = fork();
    if(pids[i] < 0){
      printf("fork failed\n");
      exit(1);
    }
    if(pids[i] == 0){
      // child
      set_tickets(tickets[i]);
      volatile unsigned long x = 0;
      while(1){
        x += 1;   // busy loop
      }
      exit(0);
    }
  }

  // let them run for a while
  sleep(RUNTIME_TICKS);

  printf("Final CPU usage (ticks):\n");
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

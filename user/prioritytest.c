#include "kernel/types.h"
#include "user/user.h"

#define NCHILD      9       
#define RUNTIME     1000     // Each child process will run for approximately this many uptime() ticks.
#define SPIN_ITERS  200000  

// This function performs a busy-wait loop. It's a simple way to make a process
// consume CPU time without performing any I/O, which is necessary for
// testing CPU scheduling algorithms. The volatile keyword prevents the compiler
// from optimizing away the loop.
void
spin(int loops)
{
  volatile int i;
  for (i = 0; i < loops; i++) { }
}

struct result {
  int pid;          
  int prio;        
  int iterations;   // The number of times the spin loop completed. This is
                    // a measure of how much CPU time the process received.
};

int
main(void)
{
  int prio_pattern[3] = {5, 10, 20}; 
  int fds[2];

  if (pipe(fds) < 0) {
    printf("prioritytest: pipe failed\n");
    exit(1);
  }

  printf("prioritytest: creating %d processes (RUNTIME=%d ticks)\n", NCHILD, RUNTIME);

  for (int i = 0; i < NCHILD; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("prioritytest: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      // This is the child process.
      close(fds[0]); // Child processes don't need to read from the pipe.
      
      int pr = prio_pattern[i % 3];

      set_priority(pr);

      int start = uptime();
      int count = 0;
      while (uptime() - start < RUNTIME) {
        spin(SPIN_ITERS);
        count++;
      }

      struct result r;
      r.pid = getpid();
      r.prio = pr;
      r.iterations = count;

      int off = 0;
      char *buf = (char*)&r;
      int towrite = sizeof(r);
      while (off < towrite) {
        int n = write(fds[1], buf + off, towrite - off);
        if (n <= 0) break;
        off += n;
      }

      close(fds[1]); // Close the write end of the pipe.
      exit(0); // Terminate the child process.
    }
  }

  // This is the parent process.
  struct result results[NCHILD];
  int got = 0;
  while (got < NCHILD) {
    struct result r;
    int off = 0;
    char *buf = (char*)&r;
    int need = sizeof(r);
    while (off < need) {
      int n = read(fds[0], buf + off, need - off);
      if (n <= 0) break; // Pipe closed or error.
      off += n;
    }
    if (off == need) {
      results[got++] = r;
    } else {
      break;
    }
  }
  close(fds[0]); 

  for (int i = 0; i < NCHILD; i++) wait(0);

  // Simple bubble sort to organize the results 
  for (int i = 0; i < got; i++) {
    for (int j = i + 1; j < got; j++) {
      if (results[j].prio < results[i].prio ||
         (results[j].prio == results[i].prio && results[j].iterations > results[i].iterations)) {
        struct result tmp = results[i];
        results[i] = results[j];
        results[j] = tmp;
      }
    }
  }

  printf("\nResults (fixed runtime = %d ticks):\n", RUNTIME);
  for (int i = 0; i < got; i++) {
    printf("PID %d  priority %d  iterations %d\n",
           results[i].pid, results[i].prio, results[i].iterations);
  }

  int sum5=0,c5=0, sum10=0,c10=0, sum20=0,c20=0;
  for (int i = 0; i < got; i++) {
    if (results[i].prio == 5)  { sum5 += results[i].iterations;  c5++; }
    if (results[i].prio == 10) { sum10 += results[i].iterations; c10++; }
    if (results[i].prio == 20) { sum20 += results[i].iterations; c20++; }
  }

  if (c5 || c10 || c20) {
    printf("\nAverages by priority (iterations per %d ticks):\n", RUNTIME);
    if (c5)  printf(" priority 5 :  %d (count %d)\n",  sum5 / c5,  c5);
    if (c10) printf(" priority 10:  %d (count %d)\n",  sum10 / c10, c10);
    if (c20) printf(" priority 20:  %d (count %d)\n",  sum20 / c20, c20);
  }

  printf("\nprioritytest: done\n");
  exit(0);
}
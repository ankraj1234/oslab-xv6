#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PAGES 3000
#define PGSIZE 4096

int
main(void)
{
  int pid = getpid();
  struct pagestat st;
  
  printf("MRU Memory Test Program\n");
  printf("========================\n");
  printf("Process PID: %d\n\n", pid);
  
  // Allocate memory
  printf("Allocating %d pages (%d KB)...\n", PAGES, (PAGES * PGSIZE) / 1024);
  char *mem = sbrk(PAGES * PGSIZE);
  if(mem == (char*)-1){
    printf("sbrk failed\n");
    exit(1);
  }
  printf("Memory allocated at: 0x%lx\n\n", (uint64)mem);

  getpagestat(pid, &st);
  printf("Initial Statistics:\n");
  printf("=================\n");
  printf("Total Page Faults: %d\n", st.num_pagefaults);
  printf("Total Swap-ins: %d\n", st.num_swapins);
  printf("Total Swap-outs: %d\n\n", st.num_swapouts);
  
  // Write to all pages
  printf("Writing to all pages...\n");
  for(int i = 0; i < PAGES; i++)
    mem[i * PGSIZE] = i;
  getpagestat(pid, &st);
  printf("After initial writes:\n");
  printf("  Page Faults: %d\n", st.num_pagefaults);
  printf("  Swap-ins: %d\n", st.num_swapins);
  printf("  Swap-outs: %d\n\n", st.num_swapouts);
  
  // Random access pattern
  printf("Random access pattern (100 accesses)...\n");
  for(int i = 0; i < 10000; i++){
    int page = (i * 7) % PAGES;  // Simple pseudo-random
    mem[page * PGSIZE]++;
    if((i + 1) % 200 == 0){
      getpagestat(pid, &st);
      printf("After %d accesses:\n", i + 1);
      printf("  Page Faults: %d, Swap-ins: %d, Swap-outs: %d\n",
             st.num_pagefaults, st.num_swapins, st.num_swapouts);
    }
  }
  printf("\n");
  
  getpagestat(pid, &st);
  printf("Final Statistics:\n");
  printf("=================\n");
  printf("Total Page Faults: %d\n", st.num_pagefaults);
  printf("Total Swap-ins: %d\n", st.num_swapins);
  printf("Total Swap-outs: %d\n\n", st.num_swapouts);

  // Verify data
  printf("Verifying data integrity...\n");
  int errors = 0;
  for(int i = 0; i < PAGES; i++){
    int expected = i + (100 / PAGES);  // Approximate expected value
    if(mem[i * PGSIZE] < i || mem[i * PGSIZE] > expected + 10)
      errors++;
  }
  printf("Verification complete. Errors: %d\n\n", errors);
  
  // Dump MRU list
  printf("Dumping MRU list:\n");
  dumpmru();
  exit(0);
}
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(int argc, char *argv[])
{
  char *page;
  int pid;
  
  printf("COW Fork Test Starting...\n");
  
  // Allocate a page of memory
  page = sbrk(PGSIZE);
  if(page == (char*)-1){
    printf("sbrk failed\n");
    exit(1);
  }
  
  // Write initial value to the page
  printf("Parent: Writing value 65 (char 'A') to page\n");
  memset(page, 65, PGSIZE);  // 65 is ASCII for 'A'
  
  printf("Parent: Value before fork: %d (expected 65 for 'A')\n", (int)page[0]);
  sleep(1);
  
  // Fork
  pid = fork();
  
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }
  
  if(pid == 0){
    // Child process
    sleep(2);
    printf("\nChild: Started (pid=%d)\n", getpid());
    printf("Child: Initial value read: %d (expected 65 for 'A')\n", (int)page[0]);
    
    // Verify child sees parent's value
    if(page[0] != 65){
      printf("Child: ERROR - expected 65 but got %d\n", (int)page[0]);
      exit(1);
    }
    
    // Child writes a new value
    printf("Child: Writing value 66 (char 'B') to page\n");
    memset(page, 66, PGSIZE);  // 66 is ASCII for 'B'
    
    printf("Child: Value after write: %d (expected 66 for 'B')\n", (int)page[0]);
    
    // Verify child's write succeeded
    if(page[0] != 66){
      printf("Child: ERROR - write failed, got %d\n", (int)page[0]);
      exit(1);
    }
    
    printf("Child: Exiting\n\n");
    sleep(1);
    exit(0);
  } else {
    // Parent process
    printf("Parent: Waiting for child (pid=%d)\n", pid);
    wait(0);
    
    sleep(1);
    
    // Parent reads the page - should still see 65 (A)
    printf("Parent: Value after child exit: %d\n", (int)page[0]);
    
    printf("\n=== TEST RESULTS ===\n");
    printf("Expected parent value: 65 (char 'A')\n");
    printf("Actual parent value: %d\n", (int)page[0]);
    
    if(page[0] == 65){
      printf("\n=== SUCCESS ===\n");
      printf("Parent still has value 65 ('A')!\n");
      printf("Child had value 66 ('B').\n");
      printf("COW working correctly - child's write was isolated.\n");
    } else if(page[0] == 66){
      printf("\n=== FAILED ===\n");
      printf("Parent's value changed to 66 ('B') - same as child!\n");
      printf("COW NOT working - child and parent share the same memory.\n");
    } else {
      printf("\n=== FAILED ===\n");
      printf("Parent's value is unexpected: %d\n", (int)page[0]);
      printf("Expected 65, something is wrong with memory handling.\n");
    }
  }
  
  exit(0);
}
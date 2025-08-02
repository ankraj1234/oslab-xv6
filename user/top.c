#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char *states[] = {
  "UNUSED",
  "USED",
  "SLEEPING",
  "RUNNABLE",
  "RUNNING",
  "ZOMBIE"
};

int
main(void)
{
  struct uproc up[64];
  int n = getprocsinfo(up);
  if (n < 0) {
    printf("Error: getprocsinfo failed\n");
    exit(1);
  }

  printf("PID\t STATE\t\t TICKS\tNAME\n");
  for (int i = 0; i < n; i++) {
    printf("%d \t %s \t %d \t%s\n",
      up[i].pid,
      states[up[i].state],
      up[i].ticks,
      up[i].name);
  }

  exit(0);
}

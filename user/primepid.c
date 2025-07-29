#include "kernel/types.h"
#include "user/user.h"

int main(){
    int pid;
    printf("Parent PID: %d\n", getpid());

    for (int i = 0; i < 10; i++) {
        pid = fork();
        if (pid == 0) {
            // child process
            sleep(i + 1); // staggers output 
            printf("Child PID: %d\n", getpid());
            exit(0);
        }
    }

    for (int i = 0; i < 10; i++) wait(0);
    exit(0);
}

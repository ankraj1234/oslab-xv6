// Description: A simple ping-pong program that measures the time taken for a series of message exchanges between a parent and child process.
// Chap1 Q1 from the book (pingpong.c didn't exist before)

#include "kernel/types.h"
#include "user/user.h"

int main() {
    int p1[2], p2[2];
    int N = 1000;
    char buf[1] = {'x'};

    pipe(p1);
    pipe(p2);

    int start_ticks, end_ticks;

    if (fork() == 0) {
        // Child process
        for (int i = 0; i < N; i++) {
            read(p1[0], buf, 1);
            write(p2[1], buf, 1);
        }
        exit(0);
    } else {
        // Parent process
        start_ticks = uptime();

        for (int i = 0; i < N; i++) {
            write(p1[1], buf, 1);
            read(p2[0], buf, 1);
        }

        end_ticks = uptime();
        wait(0);

        int total_ticks = end_ticks - start_ticks;
        if (total_ticks == 0) total_ticks = 1; // prevent division by zero

        int total_exchanges = N;
        int exchanges_per_second = (1000 * total_exchanges) / (total_ticks * 10);

        printf("Total ticks: %d\n", total_ticks);
        printf("Exchanges per second: %d\n", exchanges_per_second);

    }

    exit(0);
}


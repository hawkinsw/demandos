#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

int main() {
    volatile uint64_t ctr = 0;
    for (;;) {
        printf("About to sleep!\n");
        struct timespec t;
        t.tv_sec = 5;
        nanosleep(&t, NULL);
        printf("Done sleeping!\n");

    }
}
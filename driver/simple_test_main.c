#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/random.h>


int main() {
    struct timeval timev;

    int result = gettimeofday(&timev, NULL);

    for (int i = 0; i<10; i++) {
        printf("There will be no way this works (%d)!\n", i);
    }
    return 0;
}
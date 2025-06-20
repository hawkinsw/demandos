#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>


int main() {
    int fd = open("testing.txt", O_CREAT);
    write(fd, "Hello, World!\n", 14);

    struct timeval timev;

    int result = gettimeofday(&timev, NULL);

    for (int i = 0; i<10; i++) {
        printf("There will be no way this works (%d)!\n", i);
    }
    return 0;
}
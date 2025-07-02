#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/random.h>


int main() {
    // Make a buffer for some random bytes!
    char buffer[8] = {0, };

    if (getrandom(buffer, 8, GRND_RANDOM) < 0) {
        printf("Error occurred getting randomness!\n");
        return 0;
    }

    char what_read[20] = {0, };

    // For resetting the contents of the disk.
    //char buffer[512] = {0, };

    int fd = open("testing.txt", O_CREAT);

    // Write Hello, World to the disk!
    write(fd, "Hello, World!\n", strlen("Hello, World!"));

    // Then, seek to 15 and write some other data.
    lseek(fd, 15, SEEK_SET);
    write(fd, buffer, 8);

    // Then, seek back to 0 and read Hello, World.
    lseek(fd, 0, SEEK_SET);
    read(fd, what_read, strlen("Hello, World!"));

    // Now, print what we read!
    printf("what_read: -%s-\n", what_read);

    return 0;
}
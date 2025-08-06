#include "event.h"
#include "system.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>

uint64_t mount_hd() {
  register uint64_t syscall_no asm("a7") = 210;
  register uint64_t a0 asm("a0") = 0;
  register uint64_t a1 asm("a1") = 0;
  uint64_t result = 0;

  asm volatile("scall\n" : "+r"(a0) : "r"(syscall_no), "r"(a1) : "memory");

  result = a0;

  return result;
}

uint64_t run_internal_unit_tests() {
  register uint64_t syscall_no asm("a7") = 211;
  register uint64_t a0 asm("a0") = 0;
  register uint64_t a1 asm("a1") = 0;
  uint64_t result = 0;

  asm volatile("scall\n" : "+r"(a0) : "r"(syscall_no), "r"(a1) : "memory");

  result = a0;

  return result;
}

int main() {
  volatile uint64_t ctr = 0;
  struct event evt;

  evt.id = 0;

  printf("Preparing to run the internal unit tests.\n");

  mount_hd();

  if (run_internal_unit_tests()) {
    printf("There were errors when running the internal unit tests.\n");
    return 1;
  } else {
    printf("Internal unit tests ran successfully!\n");
  }

  printf("Preparing to run user-facing unit tests.\n");

  // Testing whether simple ext2 read works.
  printf("Attempting to read from /random4: ");
  char actual[8] = {
      0,
  };

  int fd = open("/eve", 0);
  if (fd < 0) {
    printf("Error opening /eve");
    return 1;
  }
  char expected[] = "dam\n";
  lseek(fd, 1, SEEK_SET);
  read(fd, actual, 7);
  if (strcmp(actual, expected)) {
    printf("Error reading from file (actual: %s vs expected: %s)\n", actual,
           expected);
    return 1;
  } else {
    printf("Success\n");
  }

  // Testing whether reading from a non-0 block group works.
  printf("Attempting to read from /dir9/random1: ");
  int fd2 = open("/dir9/random1", 0);
  if (fd2 < 0) {
    printf("Error opening /dir9/random1");
    return 1;
  }
  char random1_expected[] = {0x4c, 0x5a, 0xbf, 0x19, 0x00, 0xa4, 0xd1};
  char random1_actual[8] = {
      0,
  };
  read(fd2, random1_actual, 7);
  if (memcmp(random1_actual, random1_expected, 7)) {
    printf("Error reading from file (actual: ");
    for (size_t s = 0; s < 8; s++) {
      printf("0x%02x ", random1_actual[s]);
    }
    printf(" vs expected: ");
    for (size_t s = 0; s < 8; s++) {
      printf("0x%02x ", random1_expected[s]);
    }
    printf(")\n");
  } else {
    printf("Success\n");
  }
  printf("End of user-facing unit tests.\n");

  // close(fd);
  return 0;
}

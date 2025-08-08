#include "event.h"
#include "system.h"
#include <fcntl.h>
#include <stdbool.h>
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
  } else {
    printf("... okay.\n");
  }
  char random1_expected[] = {0x4c, 0x5a, 0xbf, 0x19, 0x00, 0xa4, 0xd1};
  char random1_actual[8] = {
      0,
  };

  bool random1_file_read_test_failure = false;
  for (size_t i = 0; i < 800; i++) {
    printf("Doing iteration %d\n", i);
    lseek(fd2, 0, SEEK_SET);
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
      random1_file_read_test_failure = true;
      break;
    }
  }
  if (!random1_file_read_test_failure) {
      printf("(random1_file_read) Success\n");
  }

  bool getrandom_test_failure = false;
  char random_buf[8] = {
      0,
  };
  for (size_t i = 0; i < 10000; i++) {
    ssize_t getrandom_result = getrandom(random_buf, 8, GRND_RANDOM);
    if (getrandom_result != 8) {
      printf("Expected to get 8 bytes of random data by only got %d\n",
             getrandom_result);
      break;
    }
  }

  if (!getrandom_test_failure) {
    printf("(getrandom) Success\n");
  }

  printf("End of user-facing unit tests.\n");

  // close(fd);
  return 0;
}

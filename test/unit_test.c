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

  // Determine whether a read from the disk is successful ...
  int fd = open("/eve", 0);
  char actual[8] = {
      0,
  };
  char expected[] = "dam\n";
  lseek(fd, 1, SEEK_SET);
  read(fd, actual, 7);
  if (strcmp(actual, expected)) {
    printf("Error reading from file (actual: %s vs expected: %s)\n", actual, expected);
    return 1;
  } else {
    printf("Success\n");
  }
  //close(fd);

  return 0;
}

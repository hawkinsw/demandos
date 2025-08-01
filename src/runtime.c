#include "runtime.h"
#include "event.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/random.h>
#include <unistd.h>

uint64_t wait_for_event(uint64_t _timeout, struct event *_event) {
  register uint64_t syscall_no asm("a7") = 209;
  register uint64_t a0 asm("a0") = _timeout;
  register uint64_t a1 asm("a1") = (uint64_t)_event;
  uint64_t result = 0;

  asm volatile("scall\n" : "+r"(a0) : "r"(syscall_no), "r"(a1) : "memory");

  result = a0;

  return result;
}

uint64_t mount_hd() {
  register uint64_t syscall_no asm("a7") = 210;
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

  char buffer[8] = {
      0,
  };

  printf("Starting up the runtime\n");

  mount_hd();

  for (;;) {
    uint64_t wait_result = wait_for_event(5 * 1e7, &evt);
    if (wait_result == WAIT_TIMEOUT) {
      printf("There was a timeout!\n");
    } else if (wait_result == WAIT_EVENT_OCCURRED) {
      printf("An event occurred!\n");
      printf("Wait result? %llu (event id: %llu)\n", wait_result, evt.id);
    }
  }
}

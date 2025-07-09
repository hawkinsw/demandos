#include "system.h"

#include <stddef.h>
#include <stdint.h>

extern uint64_t _kernel_metadata;

void set_stimecmp(uint64_t future) {
  asm("csrw stimecmp, %0\n" : : "r"(future));
}

uint64_t get_stime() {
  volatile uint64_t current_stime;
  asm("csrr %0, time\n" : "=r"(current_stime));
  return current_stime;
}

void yield() {
  asm("csrsi sstatus, 2\n");
  WRITE_FENCE();
  asm("csrsi sstatus, 0\n");
  WRITE_FENCE();
}

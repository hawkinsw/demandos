#include "demandos.h"

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

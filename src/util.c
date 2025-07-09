#include "util.h"
#include "ecall.h"

void breakpoint(void *value) {
  char we_are_here[] = "We are at a breakpoint (with 1 value).\n";
  eprint_str(we_are_here);
}

uint64_t sys_breakpoint(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
                        uint64_t e, uint64_t f) {
  char we_are_here[] = "We are at a breakpoint (with 5 values).\n";
  eprint_str(we_are_here);
  return 0;
}

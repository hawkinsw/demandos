#include "build_config.h"
#include "ecall.h"
#include "system.h"
#include <asm-generic/unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

void eprint(uint64_t p) {
  register uint64_t a7 asm("a7") = 1;
  register uint64_t a0 asm("a0") = p;
  asm volatile("scall");
}

uint64_t epoweroff() {
#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char msg[] = "Powering off ...\n";
    eprint_str(msg);
  }
#endif
  sys_poweroff();
  __builtin_unreachable();
}

void eprint_num(uint64_t num) {
  uint64_t orig = num;
  num %= 1000;
  if (orig != num) {
    eprint('~');
  }
  uint64_t v = num / 100;
  eprint('0' + v);
  num -= v * 100;

  v = num / 10;
  eprint('0' + v);
  num -= v * 10;

  eprint('0' + num);
}

void eprint_str(char *str) {
  size_t i = 0;
  for (size_t i = 0; str[i] != '\0'; i++) {
    eprint(str[i]);
  }
}

void eprint_strn(char *str, size_t n) {
  size_t i = 0;
  for (size_t i = 0; str[i] != '\0' && i < n; i++) {
    eprint(str[i]);
  }
}

void eprint_buffer(char *msg, uint8_t *buffer, size_t size) {
  eprint_str(msg);
  eprint(':');
  eprint('\n');

  for (int i = 0; i < size; i++) {
    eprint_num(buffer[i]);
    eprint('-');
  }
  eprint('\n');
}

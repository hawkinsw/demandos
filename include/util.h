#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>

void breakpoint(void *value);

uint64_t sys_breakpoint(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
                        uint64_t e, uint64_t f);


#endif
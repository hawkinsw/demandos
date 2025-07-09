#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <stddef.h>
#include <stdint.h>

#define WRITE_FENCE() __asm__ __volatile__("fence ow, ow" : : : "memory")

void set_stimecmp(uint64_t future);
uint64_t get_stime();
void yield();
void sys_poweroff();

#endif

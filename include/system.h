#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <stddef.h>
#include <stdint.h>

#define WRITE_FENCE() __asm__ __volatile__("fence ow, ow" : : : "memory")
#define READ_FENCE() __asm__ __volatile__("fence ir, ir" : : : "memory")

void set_stimecmp(uint64_t future);
void unset_stimecmp();
uint64_t get_stime();
void sys_poweroff();
void init_stime();

#endif

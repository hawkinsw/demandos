#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <stddef.h>

#define WRITE_FENCE() __asm__ __volatile__("fence ow, ow" : : : "memory")
#endif
#ifndef SMEMORY_H
#define SMEMORY_H

#include <stddef.h>

#define ALIGN_UP(start, align) \
    ((start + align - 1) & ( ~(align - 1) ))

void *salign(size_t size, size_t align);

#endif

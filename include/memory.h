#ifndef _MEMORY_H
#define _MEMORY_H

#include "demandos.h"
#include <stddef.h>


#define ALIGN_UP(start, align) \
    ((start + align - 1) & ( ~(align - 1) ))

void *forever_alloc_aligned(size_t size, size_t align);

void initialize_heap();

void * DEMANDOS_INTERNAL(malloc)(size_t size);
void DEMANDOS_INTERNAL(free)(void *loc);

#endif

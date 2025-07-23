#ifndef _MEMORY_H
#define _MEMORY_H

#include "demandos.h"
#include <stddef.h>

void initialize_heap();

void * DEMANDOS_INTERNAL(malloc)(size_t size);
void DEMANDOS_INTERNAL(free)(void *loc);

#endif

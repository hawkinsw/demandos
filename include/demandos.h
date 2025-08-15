#ifndef _DEMANDOS_H
#define _DEMANDOS_H

#include <stdint.h>

#include "event.h"

#define DEMANDOS_INTERNAL(x) _demandos_internal_##x

uint64_t wait_for_event(uint64_t _timeout, struct event *_event);

uint64_t mount_hd();

#endif

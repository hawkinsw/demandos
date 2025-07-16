#ifndef _RUNTIME_H
#define _RUNTIME_H

#include <stdint.h>
#include "event.h"

#define WAIT_TIMEOUT ((uint64_t)-1)
#define WAIT_EVENT_OCCURRED ((uint64_t)1)

uint64_t wait_for_event(uint64_t timeout, struct event *event);

#endif
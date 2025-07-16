#ifndef _EVENT_H
#define _EVENT_H

#include <stdint.h>

struct event {
    uint64_t id;
    void *data;
};

#endif

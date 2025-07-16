#ifndef _OS_H
#define _OS_H

#include <stdint.h>

typedef void (*at_time_deferred_t)(void*);

void set_deferred(uint64_t when, at_time_deferred_t deferred, void *cookie);

void do_timer_deferreds(uint64_t now);

void yield();

struct sleep {
  uint64_t wakeup_time;
  uint64_t should_wake;
  void *cookie;
  at_time_deferred_t deferred;
};

struct process {
  uint64_t brk;
  uint64_t set_child_tid;
  uint64_t clear_child_tid;
  struct sleep asleep;
};

struct kernel {
  struct sleep deferred;
};

#endif

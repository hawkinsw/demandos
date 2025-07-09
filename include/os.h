#ifndef _OS_H
#define _OS_H

#include <stdint.h>

struct sleep {
  uint64_t wakeup_time;
  uint64_t should_wake;
};

struct kernel_metadata {
  uint64_t brk;
  uint64_t set_child_tid;
  uint64_t clear_child_tid;
  struct sleep asleep;
};

#endif
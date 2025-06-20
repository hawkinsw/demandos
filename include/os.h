#ifndef _OS_H
#define _OS_H

#include <stdint.h>

struct kernel_metadata {
  uint64_t brk;
  uint64_t set_child_tid;
  uint64_t clear_child_tid;
};


#endif
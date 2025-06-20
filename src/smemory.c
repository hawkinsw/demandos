
#include "os.h"
#include "smemory.h"

#include <stddef.h>
#include <stdint.h>

extern uint64_t _kernel_metadata;

void *smalloc(size_t size) {
  struct kernel_metadata *md = (struct kernel_metadata *)&_kernel_metadata;
  uint64_t alloced = md->brk;
  md->brk += size;
  return (void*)alloced;
}

void *salign(size_t size, size_t align) {
  struct kernel_metadata *md = (struct kernel_metadata *)&_kernel_metadata;

  md->brk = ALIGN_UP(md->brk, align);

  uint64_t alloced = md->brk;

  md->brk += size;
  return (void*)alloced;
}
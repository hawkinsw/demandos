
#include "os.h"
#include "smemory.h"

#include <stddef.h>
#include <stdint.h>

extern uint64_t _current;

void *smalloc(size_t size) {
  struct process *md = (struct process *)&_current;
  uint64_t alloced = md->brk;
  md->brk += size;
  return (void*)alloced;
}

void *salign(size_t size, size_t align) {
  struct process *md = (struct process *)&_current;

  md->brk = ALIGN_UP(md->brk, align);

  uint64_t alloced = md->brk;

  md->brk += size;
  return (void*)alloced;
}
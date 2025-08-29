#include "memory.h"
#include "build_config.h"
#include "demandos.h"
#include "ecall.h"
#include <stddef.h>
#include <stdint.h>

extern void *_heap_start;
extern void *_heap_end;

void *heap_start = NULL;
void *heap_end = NULL;

#define ARENA_ELEMENT_COUNT 10

struct arena_element_32 {
  uint8_t used;
  uint8_t buffer[32];
};
struct arena_element_32 *arena_start_32 = NULL;
void *arena_end_32 = NULL;

struct arena_element_64 {
  uint8_t used;
  uint8_t buffer[64];
};
struct arena_element_64 *arena_start_64 = NULL;
void *arena_end_64 = NULL;

struct arena_element_128 {
  uint8_t used;
  uint8_t buffer[128];
};
struct arena_element_128 *arena_start_128 = NULL;
void *arena_end_128 = NULL;

struct arena_element_256 {
  uint8_t used;
  uint8_t buffer[256];
};
struct arena_element_256 *arena_start_256 = NULL;
void *arena_end_256 = NULL;

struct arena_element_512 {
  uint8_t used;
  uint8_t buffer[512];
};
struct arena_element_512 *arena_start_512 = NULL;
void *arena_end_512 = NULL;

#define TRY_ALLOC_FROM_DEBUG(x)                                                \
  if (size < x) {                                                              \
    for (size_t i = 0; i < ARENA_ELEMENT_COUNT; i++) {                         \
      if (!arena_start_##x[i].used) {                                          \
        {                                                                      \
          char msg[] = "Allocating from the " #x " arena at index: ";          \
          eprint_str(msg);                                                     \
          eprint_num(i);                                                       \
          eprint('\n');                                                        \
        }                                                                      \
        arena_start_##x[i].used = 1;                                           \
        return arena_start_##x[i].buffer;                                      \
      } else {                                                                 \
        {                                                                      \
          char msg[] =                                                         \
              "Skipping already used " #x " arena element at index: ";         \
          eprint_str(msg);                                                     \
          eprint_num(i);                                                       \
          eprint('\n');                                                        \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

#define TRY_ALLOC_FROM(x)                                                      \
  if (size < x) {                                                              \
    for (size_t i = 0; i < ARENA_ELEMENT_COUNT; i++) {                         \
      if (!arena_start_##x[i].used) {                                          \
        arena_start_##x[i].used = 1;                                           \
        return arena_start_##x[i].buffer;                                      \
      }                                                                        \
    }                                                                          \
  }

void initialize_heap() {
  heap_start = (void *)&_heap_start;
  heap_end = (void *)&_heap_end;

  arena_start_32 = heap_start;
  arena_end_32 = arena_start_32 + ARENA_ELEMENT_COUNT;

  arena_start_64 = arena_end_32;
  arena_end_64 = arena_start_64 + ARENA_ELEMENT_COUNT;
  {
    if (arena_end_32 != arena_start_64) {
      char msg[] = "Error occurred initializing the 32/64 arena.\n";
      eprint_str(msg);
      epoweroff();
    }
  }

  arena_start_128 = arena_end_64;
  arena_end_128 = arena_start_128 + ARENA_ELEMENT_COUNT;
  {
    if (arena_end_64 != arena_start_128) {
      char msg[] = "Error occurred initializing the 64/128 arena.\n";
      eprint_str(msg);
      epoweroff();
    }
  }

  arena_start_256 = arena_end_128;
  arena_end_256 = arena_start_256 + ARENA_ELEMENT_COUNT;
  {
    if (arena_end_128 != arena_start_256) {
      char msg[] = "Error occurred initializing the 128/256 arena.\n";
      eprint_str(msg);
      epoweroff();
    }
  }

  arena_start_512 = arena_end_256;
  arena_end_512 = arena_start_512 + ARENA_ELEMENT_COUNT;
  {
    if (arena_end_256 != arena_start_512) {
      char msg[] = "Error occurred initializing the 256/512 arena.\n";
      eprint_str(msg);
      epoweroff();
    }
  }

  if (arena_end_512 > heap_end) {
    char msg[] = "Not enough room in the heap to allocate all arenas.\n";
    eprint_str(msg);
    epoweroff();
  }
}

void *DEMANDOS_INTERNAL(malloc)(size_t size) {

#if DEBUG_LEVEL > DEBUG_TRACE
  TRY_ALLOC_FROM_DEBUG(32)
  TRY_ALLOC_FROM_DEBUG(64)
  TRY_ALLOC_FROM_DEBUG(128)
  TRY_ALLOC_FROM_DEBUG(256)
  TRY_ALLOC_FROM_DEBUG(512)
#else
  TRY_ALLOC_FROM(32)
  TRY_ALLOC_FROM(64)
  TRY_ALLOC_FROM(128)
  TRY_ALLOC_FROM(256)
  TRY_ALLOC_FROM(512)
#endif

  {
    char msg[] = "Found nothing to allocate -- return NULL\n";
    eprint_str(msg);
    epoweroff();
  }
  return NULL;
}

extern uint64_t _forever_storage_start;
extern uint64_t _forever_storage_stop;

void *forever_alloc_aligned(size_t size, size_t align) {
  static void *forever_storage_frontier = (void *)&_forever_storage_start;

  void *forever_storage_stop = (void *)&_forever_storage_stop;

  void *forever_storage_aligned_start =
      (void *)ALIGN_UP((size_t)forever_storage_frontier, align);

  if (forever_storage_aligned_start + size > forever_storage_stop) {
    eprint_str("There is no more forever space left!\n");
    return NULL;
  }

  void *alloced = forever_storage_aligned_start;

  forever_storage_frontier = forever_storage_aligned_start + size;

  return (void *)alloced;
}

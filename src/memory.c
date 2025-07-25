#include "memory.h"
#include "demandos.h"
#include "e.h"
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

#define TRY_ALLOC_FROM(x)                                                      \
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

  TRY_ALLOC_FROM(32)
  TRY_ALLOC_FROM(64)
  TRY_ALLOC_FROM(128)
  TRY_ALLOC_FROM(256)
  TRY_ALLOC_FROM(512)

  {
    char msg[] = "Found nothing to allocate -- return NULL\n";
    eprint_str(msg);
    epoweroff();
  }
  return NULL;
}

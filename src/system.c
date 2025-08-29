#include "system.h"
#include "build_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

// Holds the value when the next timer will fire.
volatile uint64_t shadow_stime = (uint64_t)-1;
#define STIME_UNSET ((uint64_t) - 1)

void set_stimecmp(uint64_t future) {
  // TODO: This should queue what is not immediately set!
  if (future != STIME_UNSET && shadow_stime != STIME_UNSET &&
      shadow_stime < future) {
    // Do not make someone else miss their deadline.
#if DEBUG_LEVEL > DEBUG_TRACE
    {
      char msg[] = "Skipping preempting timecmp set.\n";
      eprint_str(msg);
    }
#endif

    return;
  }

#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char msg[] = "Setting timecmp.\n";
    eprint_str(msg);
  }
#endif

  shadow_stime = future;
  WRITE_FENCE();
  asm("csrw stimecmp, %0\n" : : "r"(future));
}

void unset_stimecmp() { set_stimecmp(-1); }

static uint64_t read_stime() {
  volatile uint64_t current_stime;
  asm("csrr %0, time\n" : "=r"(current_stime));
  WRITE_FENCE();
  return current_stime;
}

uint64_t get_stime() { return read_stime(); }

void sys_poweroff() {
  asm("mv a0, zero\n"
      "mv a1, zero\n"
      "mv a7, zero\n"
      "li a7, 0x53525354\n"
      "mv a6, zero\n"
      "ecall\n"
      :
      :
      : "a0", "a1", "a7", "a6");
}

bool system_time(uint64_t clockid, struct timespec *tp) {
  // Based on the goldfish RTC
  // See, e.g.,
  // https://nuttx.apache.org/docs/latest/platforms/arm/goldfish/goldfish_timer.html
  // or
  // https://android.googlesource.com/platform/external/qemu.git/+/master/docs/GOLDFISH-VIRTUAL-HARDWARE.TXT
  uint8_t *raw = (uint8_t *)0x00101000;
  uint32_t low = *(uint32_t *)raw;
  uint32_t high = *(uint32_t *)(raw + 4);

  uint64_t time = (uint64_t)high << 32 | low;

  tp->tv_sec = time / 1000000000;
  tp->tv_nsec = time % 1000000000;

  return true;
}
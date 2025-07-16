#include "os.h"
#include "build_config.h"
#include "system.h"

struct process _current;

struct kernel _kernel;

void set_deferred(uint64_t whence, at_time_deferred_t deferred, void *cookie) {
  struct kernel *kernel = &_kernel;
  kernel->deferred.cookie = cookie;
  kernel->deferred.wakeup_time = get_stime() + whence;
  kernel->deferred.deferred = deferred;

  set_stimecmp(kernel->deferred.wakeup_time);
}

void do_timer_deferreds(uint64_t now) {
  struct kernel *kernel = &_kernel;

  // TODO: Make this support a list of deferreds!
  if (kernel->deferred.wakeup_time < now) {

#if DEBUG_LEVEL > DEBUG_TRACE
    {
      char msg[] = "Doing an item of deferred work.\n";
      eprint_str(msg);
    }
#endif

    (*(kernel->deferred.deferred))(kernel->deferred.cookie);

    kernel->deferred.deferred = NULL;
    kernel->deferred.cookie = NULL;
    kernel->deferred.wakeup_time = 0;
  }
}

void yield() {
  asm("csrsi sstatus, 2\n");
  // Consider whether using wfi here is better?
  // asm("wfi\n");
  WRITE_FENCE();
  asm("csrci sstatus, 0\n");
  WRITE_FENCE();
}

void configure_dl_random() {

  uint64_t randomness = 0x0;

  if (virtio_get_randomness(&randomness, sizeof(uint64_t)) != 8) {
    char msg[] = "Error occurred configuring dl_random.\n";
    eprint_str(msg);
    epoweroff();
  }

  dl_random_bytes = randomness;
}

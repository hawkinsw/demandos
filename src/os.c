#include "os.h"
#include "build_config.h"
#include "ecall.h"
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
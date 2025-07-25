#include "build_config.h"
#include "e.h"
#include "ecall.h"
#include "io.h"
#include "runtime.h"
#include "system.h"
#include "util.h"
#include "virtio.h"
#include <asm-generic/unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "event.h"
#include "os.h"

extern uint64_t _current;
extern uint64_t _kernel;
extern uint64_t _poweroff;
extern uint64_t _gettime;

typedef uint64_t (*syscall_handler)(uint64_t, uint64_t, uint64_t, uint64_t,
                                    uint64_t, uint64_t);

syscall_handler syscall_handlers[__NR_syscalls] = {
    NULL,

};

void timer_interrupt_handler(void) {
  struct process *current = (struct process *)&_current;
  struct kernel *kernel = (struct kernel *)&_kernel;

  uint64_t stime = get_stime();

  // First (even before loggin!), unset so that we don't get confused!
  unset_stimecmp();

#if DEBUG_LEVEL > DEBUG_TRACE
{
  char msg[] = "Timer went off!\n";
  eprint_str(msg);
}
#endif

  if (kernel->deferred.wakeup_time) {
    // Second, check whether the kernel has deferred work to do!
    if (stime > kernel->deferred.wakeup_time) {
#if DEBUG_LEVEL > DEBUG_TRACE
      char msg[] = "Trying to do some deferreds!\n";
      eprint_str(msg);
#endif
      do_timer_deferreds(stime);
    } else {
      // Second (part two), make the kernel a cause of the wakeup!
      set_stimecmp(kernel->deferred.wakeup_time);
    }
  }

  // Third, check whether there is a process to wakeup!
  if (current->asleep.wakeup_time != 0) {
#if DEBUG_LEVEL > DEBUG_TRACE
    char msg[] = "There is something sleeping!\n";
    eprint_str(msg);
#endif

    // Third (part two), check whether the process needs to be woken up!
    if (stime > current->asleep.wakeup_time) {
#if DEBUG_LEVEL > DEBUG_TRACE
      char msg[] = "Time to wake up the currently sleeping process!\n";
      eprint_str(msg);
#endif
      current->asleep.should_wake = 1;
      WRITE_FENCE();
    } else {
      // Still more sleeping to do! Try to make ourselves the next interrupt
      // causer.
      set_stimecmp(current->asleep.wakeup_time);
    }
  }
}

uint64_t set_brk_s(uint64_t new_brk, uint64_t b, uint64_t c, uint64_t d,
                   uint64_t e, uint64_t f) {
  struct process *md = (struct process *)&_current;
  if (new_brk != 0) {
    md->brk = new_brk;
  }

  return md->brk;
}

uint64_t hardware_riscv(uint64_t new_brk, uint64_t b, uint64_t c, uint64_t d,
                        uint64_t e, uint64_t f) {
  return 0;
}

uint64_t set_child_tid(uint64_t tidptr, uint64_t b, uint64_t c, uint64_t d,
                       uint64_t e, uint64_t f) {
  struct process *md = (struct process *)&_current;
  md->clear_child_tid = tidptr;
  return 1;
}

uint64_t readlinkat_s(uint64_t dirfd, uint64_t _path, uint64_t _buf,
                      uint64_t _buf_size, uint64_t e, uint64_t f) {
  char actual_path[] = "/home/hawkinsw/code/boot2bpf/build/unibpform";
  size_t actual_path_length = 24;
  char *path = (char *)_path;
  char *buf = (char *)_buf;
  size_t buf_size = (size_t)_buf_size;

#if DEBUG_LEVEL > DEBUG_TRACE
  eprint_str(path);
  eprint('\n');
#endif

  memcpy(buf, actual_path, actual_path_length);

  return actual_path_length;
}

uint64_t get_random_s(uint64_t _buffer, uint64_t /* size_t */ _length,
                      uint64_t /* unsigned int */ flags, uint64_t d, uint64_t e,
                      uint64_t f) {
  void *buffer = (void *)_buffer;
  size_t length = (size_t)_length;

  // TODO: Handle flags!

  return virtio_get_randomness(buffer, length);
}

uint64_t openat_s(uint64_t _dirfd, uint64_t _pathname, uint64_t _flags,
                  uint64_t _mode, uint64_t e, uint64_t f) {
  char *pathname = (char *)_pathname;

#if DEBUG_LEVEL > DEBUG_TRACE
  char log[] = "Attempting to open a fd for ";
  eprint_str(log);
  eprint_str(pathname);
  eprint('\n');
#endif

  return open_fd(pathname);
}

uint64_t seek_s(uint64_t _fd, uint64_t _offset, uint64_t _whence, uint64_t d,
                uint64_t e, uint64_t f) {
  uint64_t fd = _fd;
  long int offset = (long int)_offset;

  if (!is_fd_open(fd)) {
    return -1;
  }

  return fds[fd].seek_handler(fd, offset, _whence);
}

uint64_t read_s(uint64_t _fd, uint64_t _buf, uint64_t _size, uint64_t d,
                uint64_t e, uint64_t f) {
  char *buf = (char *)_buf;
  size_t size = (size_t)_size;
  uint64_t fd = _fd;

  if (!is_fd_open(fd)) {
    return -1;
  }
  return fds[fd].read_handler(fd, buf, size);
}

uint64_t write_s(uint64_t _fd, uint64_t _buf, uint64_t _size, uint64_t d,
                 uint64_t e, uint64_t f) {
  char *buf = (char *)_buf;
  size_t size = (size_t)_size;
  uint64_t fd = _fd;

  if (!is_fd_open(fd)) {
    return -1;
  }
  return fds[fd].write_handler(fd, buf, size);
}

uint64_t demand_s(uint64_t _timeout, uint64_t _event_p, uint64_t c, uint64_t d,
                  uint64_t e, uint64_t f) {
  struct event *event = (struct event *)_event_p;
  uint64_t timeout = _timeout;

#if DEBUG_LEVEL > DEBUG_TRACE
  char msg[] = "Beginning to wait for a demand ...!\n";
  eprint_str(msg);
#endif

  timeout = get_stime() + timeout;
  // First, set the timeout!

  set_stimecmp(timeout);

  // Now, race the timeout for an event!

  for (;;) {
    yield();
    READ_FENCE();
    WRITE_FENCE();

    if (get_stime() > (timeout + 3e7)) {

#if DEBUG_LEVEL > DEBUG_TRACE
      {
        char msg[] = "Done waiting for a demand ... time out!!\n";
        eprint_str(msg);
      }
#endif
      return WAIT_TIMEOUT;
    }
  }

#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char msg[] = "Done waiting for a demand ... an event occurred!!\n";
    eprint_str(msg);
  }
#endif

  //set_stimecmp(get_stime() + 5 * 1e7);
  return WAIT_EVENT_OCCURRED;
}

uint64_t nanosleep_s(uint64_t _clockid, uint64_t flags, uint64_t _duration_p,
                     uint64_t _rem_p, uint64_t e, uint64_t f) {

  struct timespec *duration = (struct timespec *)_duration_p;
  // If interrupted, write remainder here!
  struct timespec *rem = (struct timespec *)_rem_p;

#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char msg[] = "In nanosleep\n";
    eprint_str(msg);
  }
#endif

  struct process *current = (struct process *)&_current;

  current->asleep.wakeup_time =
      get_stime() + (duration->tv_sec * 10e6 + duration->tv_nsec);

  set_stimecmp(current->asleep.wakeup_time);

  for (;;) {
    yield();
    READ_FENCE();
    WRITE_FENCE();
    if (current->asleep.should_wake) {
      break;
    }
    // Keep reads/writes in order before going again!
    WRITE_FENCE();
    READ_FENCE();
  }
  current->asleep.should_wake = 0;
  current->asleep.wakeup_time = 0;

  epoweroff();

  return 0;
}

uint64_t empty(uint64_t new_brk, uint64_t b, uint64_t c, uint64_t d, uint64_t e,
               uint64_t f) {
  return 0;
}

uint64_t mprotect_s(uint64_t new_brk, uint64_t b, uint64_t c, uint64_t d,
                    uint64_t e, uint64_t f) {
  return 0;
}

uint64_t clock_gettime32_s(uint64_t _clockid, uint64_t _tp, uint64_t c,
                           uint64_t d, uint64_t e, uint64_t f) {
  clockid_t clockid = (clockid_t)_clockid;
  struct timespec *tp = (struct timespec *)_tp;

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

  return 0;
}

// Taken from glibc.
struct stat {
  unsigned long st_dev;  /* Device.  */
  unsigned long st_ino;  /* File serial number.  */
  unsigned int st_mode;  /* File mode.  */
  unsigned int st_nlink; /* Link count.  */
  unsigned int st_uid;   /* User ID of the file's owner.  */
  unsigned int st_gid;   /* Group ID of the file's group. */
  unsigned long st_rdev; /* Device number, if device.  */
  unsigned long __pad1;
  long st_size;   /* Size of file, in bytes.  */
  int st_blksize; /* Optimal block size for I/O.  */
  int __pad2;
  long st_blocks; /* Number 512-byte blocks allocated. */
  long st_atime;  /* Time of last access.  */
  unsigned long st_atime_nsec;
  long st_mtime; /* Time of last modification.  */
  unsigned long st_mtime_nsec;
  long st_ctime; /* Time of last status change.  */
  unsigned long st_ctime_nsec;
  unsigned int __unused4;
  unsigned int __unused5;
};

uint64_t fstat_s(uint64_t _fd, uint64_t _statbuf, uint64_t c, uint64_t d,
                 uint64_t e, uint64_t f) {
  struct stat *statbuf = (struct stat *)_statbuf;
  int fd = (int)_fd;

#if DEBUG_LEVEL > DEBUG_TRACE
  char fd_descr[] = "Fd: ";
  eprint_str(fd_descr);
  eprint_num(fd);
  eprint('\n');
#endif

  statbuf->st_size = 0;
  statbuf->st_mode = 0x2190;
  statbuf->st_blksize = 1024;
  statbuf->st_blocks = 0;
  statbuf->st_dev = 26;
  statbuf->st_ino = 7;
  statbuf->st_nlink = 1;
  statbuf->st_uid = 1;
  return 0;
}

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;

#define NCCS 19
struct termios {
  tcflag_t c_iflag; /* input mode flags */
  tcflag_t c_oflag; /* output mode flags */
  tcflag_t c_cflag; /* control mode flags */
  tcflag_t c_lflag; /* local mode flags */
  cc_t c_line;      /* line discipline */
  cc_t c_cc[NCCS];  /* control characters */
};

uint64_t ioctl_s(uint64_t _fd, uint64_t _op, uint64_t c, uint64_t d, uint64_t e,
                 uint64_t f) {
  return 0;
}

uint64_t poweroff_s(uint64_t _fd, uint64_t _op, uint64_t c, uint64_t d,
                    uint64_t e, uint64_t f) {
  epoweroff();
  __builtin_unreachable();
}

uint64_t unimplemented(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
                       uint64_t e, uint64_t f) {
#if DEBUG_LEVEL > DEBUG_ERROR
  char unimplemented[] = "Unimplemented";
  eprint_str(unimplemented);
#endif
  sys_poweroff();
  __builtin_unreachable();
}

uint64_t software_interrupt_handler(void) {
#if DEBUG_LEVEL > DEBUG_TRACE
  char software_interrupt[] = "Software Interrupt";
  eprint_str(software_interrupt);
#endif

  epoweroff();
  return 0;
}

void configure_syscall_handlers(void) {
  for (int i = 0; i < sizeof(syscall_handlers) / sizeof(syscall_handler); i++) {
    syscall_handlers[i] = unimplemented;
  }

  syscall_handlers[258] = hardware_riscv;
  syscall_handlers[222] = empty;
  syscall_handlers[64] = write_s;
  syscall_handlers[96] = set_child_tid;
  syscall_handlers[99] = empty;      // set_robust_list
  syscall_handlers[93] = poweroff_s; // exit
  syscall_handlers[94] = poweroff_s; // exit
  syscall_handlers[214] = set_brk_s;
  syscall_handlers[261] = empty; // prlimit64.
  syscall_handlers[78] = readlinkat_s;
  syscall_handlers[278] = get_random_s;
  syscall_handlers[113] = clock_gettime32_s;
  syscall_handlers[226] = mprotect_s;
  syscall_handlers[80] = fstat_s;
  syscall_handlers[29] = ioctl_s;
  syscall_handlers[14] = sys_breakpoint;
  syscall_handlers[56] = openat_s;
  syscall_handlers[63] = read_s;
  syscall_handlers[62] = seek_s;
  syscall_handlers[115] = nanosleep_s;

  // Non-standard system call so that the runtime can ask for the next Demand.
  syscall_handlers[209] = demand_s;
}

uint64_t system_call_dispatcher(uint64_t cause, uint64_t location,
                                uint64_t value, uint64_t sp, uint64_t a7,
                                uint64_t a0, uint64_t a1, uint64_t a2,
                                uint64_t a3, uint64_t a4, uint64_t a5) {

#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char msg[] = "System call dispatcher: Cause: ";
    eprint_str(msg);
    eprint_num(a7);
    eprint(';');
    eprint_num(cause);
    eprint('\n');
  }
#endif

  uint64_t result = syscall_handlers[a7](a0, a1, a2, a3, a4, a5);

#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char msg[] = "System call dispatcher: Result: ";
    eprint_num(result);
    eprint('\n');
  }
#endif

  return result;
}

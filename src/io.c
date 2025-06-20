#include "io.h"
#include "ecall.h"
#include "virtio.h"
#include <stdbool.h>


// We currently support no more than 100 open file descriptors.
struct io_descriptor fds[100] = {};
uint64_t next_fd = 3;
uint64_t max_fd = 100;

bool is_fd_open(uint64_t fd) {
    return (fd < max_fd && fds[fd].open);
}

int stdout_handler(uint64_t fd, void *buf, size_t size) {
  struct virtio_driver *driver = find_virtio_driver(3);

  if (!driver || !driver->initialized) {
    return -1;
  }

  make_virtio_req(&driver->vring[1], (void *)buf, size, 0);

  signal_virtio_device(driver, 1);
  wait_virtio_completion(&driver->vring[1]);

  return size;
}

bool configure_stdio() {
    fds[1].write_handler = stdout_handler;
    fds[1].open = true;

    return true;
}

void configure_io() {
    if (!configure_stdio()) {
        char error[] = "There was an error initializing stdio!\n";
        eprint_str(error);
    }
}

int open_fd(char *pathname) {
    uint64_t fd = next_fd++;

    fds[fd].open = true;
    fds[fd].write_handler = stdout_handler;

    return fd;
}
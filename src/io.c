#include "io.h"
#include "blk.h"
#include "build_config.h"
#include "ecall.h"
#include "ext2.h"
#include "virtio.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "e.h"

// We currently support no more than 100 open file descriptors.
struct io_descriptor fds[100] = {};
uint64_t next_fd = 3;
uint64_t max_fd = 100;

bool is_fd_open(uint64_t fd) { return (fd < max_fd && fds[fd].open); }

int stdout_write_handler(uint64_t fd, void *buf, size_t size) {
  struct virtio_driver *driver = find_virtio_driver(3);

  if (!driver || !driver->initialized) {
    return -1;
  }

  uint32_t just_used = driver->vring[1].bookeeping.descr_next;
  vring_add_to_descr(&driver->vring[1], (void *)buf, size, 0, just_used, true);

  vring_use_avail(&driver->vring[1], just_used);
  signal_virtio_device(driver, 1);
  vring_wait_completion(&driver->vring[1]);

  return size;
}

int disk_write_handler(uint64_t fd, void *buf, size_t size) {
  struct virtio_driver *driver = find_virtio_driver(1);

  if (!driver || !driver->initialized) {
    return -1;
  }

  if (!is_fd_open(fd)) {
    return -1;
  }

  struct io_descriptor *iod = &fds[fd];

  uint64_t sector = virtio_blk_sector_from_pos(iod->pos);
  uint64_t offset = virtio_blk_sector_offset_from_pos(iod->pos);

  // First, read in the existing data ...
  uint8_t buffer[512] = {
      0,
  };
  uint8_t result = virtio_blk_read_sector_sync(driver, sector, buffer);

  if (result) {
    return result;
  }

  memcpy(((void *)buffer) + offset, buf, size);
  result = virtio_blk_write_sync(driver, sector, buffer);

#if DEBUG_LEVEL > DEBUG_TRACE
  char here[] = "Result of read operation on block device: ";
  eprint_str(here);
  eprint_num(result);
  eprint('\n');
#endif

  return size;
}

int disk_read_handler(uint64_t fd, void *buf, size_t size) {
  struct virtio_driver *driver = find_virtio_driver(1);

  if (!driver || !driver->initialized) {
    return -1;
  }

  if (!is_fd_open(fd)) {
    return -1;
  }

  struct io_descriptor *iod = &fds[fd];

  uint8_t buffer[512] = {
      0,
  };

  uint64_t sector = virtio_blk_sector_from_pos(iod->pos);
  uint64_t offset = virtio_blk_sector_offset_from_pos(iod->pos);

  uint8_t result = virtio_blk_read_sector_sync(driver, sector, buffer);

  memcpy(buf, ((void *)buffer) + offset, size);

#if DEBUG_LEVEL > DEBUG_TRACE
  char here[] = "Result of read operation on block device: ";
  eprint_str(here);
  eprint_num(result);
  eprint('\n');
#endif

  struct ext2_superblock *superb = superblock_for_ino(iod->ino);
  return read_from_ino(driver, superb, iod->ino, buf, iod->pos, size);
}

int disk_seek_handler(uint64_t fd, long int off, int whence) {
  if (!is_fd_open(fd)) {
    return -1;
  }

  struct io_descriptor *iod = &fds[fd];

  switch (whence) {
  case SEEK_CUR: {
    iod->pos += off;
    return iod->pos;
  }
  case SEEK_END: {
    // TODO: Implement
    return -1;
  }
  case SEEK_SET: {
    iod->pos = off;
    return iod->pos;
  }
  }
  return -1;
}

bool configure_stdio() {
  fds[1].write_handler = stdout_write_handler;
  fds[1].open = true;

  return true;
}

void configure_io() {
  if (!configure_stdio()) {
    char error[] = "There was an error initializing stdio!\n";
    eprint_str(error);
  }
}

static struct ext2_superblock _superblock;
static bool _superblock_mounted = false;

struct ext2_superblock *superblock_for_ino(uint32_t ino) {
  return &_superblock;
}

struct ext2_superblock *superblock_for_pathname(char *pathname) {
  return &_superblock;
}

int open_fd(char *pathname) {
  struct virtio_driver *driver = find_virtio_driver(1);
  if (!driver || !driver->initialized) {
    return -1;
  }

  uint64_t fd = next_fd++;

  fds[fd].open = true;
  fds[fd].write_handler = disk_write_handler;
  fds[fd].read_handler = disk_read_handler;
  fds[fd].seek_handler = disk_seek_handler;

  if (!_superblock_mounted) {
    eprint_str("Superblock is not mounted.\n");
    assert(_superblock_mounted);
  }

  struct ext2_superblock *superblock = superblock_for_pathname(pathname);

  bool ino_found = ino_from_path(driver, superblock, pathname, &fds[fd].ino);

  if (!ino_found) {
    eprint_str("Could not find the inode.");
    assert(_superblock_mounted);
  }

#if DEBUG_LEVEL > DEBUG_TRACE
{
  eprint_str("Found the inode (");
  eprint_num(fds[fd].ino);
  eprint('\n');
}
#endif

  return fd;
}

// TODO: Move into separate file.
// Handle assumptions.
// Handle variable block size (etc).

uint64_t io_mount_hd() {
  _superblock.s_blocks_count = 0;
  _superblock.s_inodes_count = 0;

  struct virtio_driver *driver = find_virtio_driver(1);

  if (!driver || !driver->initialized) {
    return -1;
  }

  bool superblock_read_result = read_superblock(driver, &_superblock);

  uint8_t buffer[1024] = {
      0,
  };

  uint64_t actual_block_size = 1024 << _superblock.s_log_block_size;
  uint64_t actual_block_group_sizes =
      actual_block_size * _superblock.s_blocks_per_group;

  uint32_t block_group_count =
      _superblock.s_blocks_count / _superblock.s_blocks_per_group;
  _superblock_mounted = true;

  return 0;
}

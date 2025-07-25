#include "io.h"
#include "blk.h"
#include "build_config.h"
#include "ecall.h"
#include "ext2.h"
#include "virtio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

  uint64_t sector = sector_from_pos(iod->pos);
  uint64_t offset = sector_offset_from_pos(iod->pos);

  // First, read in the existing data ...
  uint8_t buffer[512] = {
      0,
  };
  uint8_t result = virtio_blk_read_sync(driver, sector, buffer);

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

  uint64_t sector = sector_from_pos(iod->pos);
  uint64_t offset = sector_offset_from_pos(iod->pos);

  uint8_t result = virtio_blk_read_sync(driver, sector, buffer);

  memcpy(buf, ((void *)buffer) + offset, size);

#if DEBUG_LEVEL > DEBUG_TRACE
  char here[] = "Result of read operation on block device: ";
  eprint_str(here);
  eprint_num(result);
  eprint('\n');
#endif

  return size;
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

int open_fd(char *pathname) {
  uint64_t fd = next_fd++;

  fds[fd].open = true;
  fds[fd].write_handler = disk_write_handler;
  fds[fd].read_handler = disk_read_handler;
  fds[fd].seek_handler = disk_seek_handler;

  return fd;
}

// TODO: Move into separate file.
// Handle assumptions.
// Handle variable block size (etc).

uint64_t io_mount_hd() {
  struct ext2_superblock superblock = {.s_blocks_count = 0,
                                       .s_inodes_count = 0};

  struct virtio_driver *driver = find_virtio_driver(1);

  if (!driver || !driver->initialized) {
    return -1;
  }

  // TODO: Make function for reading superblock.

  uint8_t buffer[1024] = {
      0,
  };

  // The size of the superblock is 1024 -- that means that there must be 2
  // 512-byte reads to get it all.
  uint8_t result = virtio_blk_read_sync(driver, 2, buffer);
  if (result) {
    return (uint64_t)-1;
  }
  result = virtio_blk_read_sync(driver, 3, buffer + 512);
  if (result) {
    return (uint64_t)-1;
  }

  eprint_buffer("Contents of superblock", buffer, 512);

  // Copy the raw data over to the superblock.
  memcpy(&superblock, buffer, sizeof(struct ext2_superblock));

#if 1
  if (superblock.s_blocks_count == 0x400) {
    char here[] = "Read the proper number of blocks!\n";
    eprint_str(here);
  }

  if (superblock.s_inodes_count == 0x80) {
    char here[] = "Read the proper number of inodes!\n";
    eprint_str(here);
  }

  if (superblock.s_first_inode == 11) {
    char here[] = "Read the proper first inode!\n";
    eprint_str(here);
  }
#endif

  uint64_t actual_block_size = 1024 << superblock.s_log_block_size;

#if 1
  if (actual_block_size == 1024) {
    char here[] = "Read the proper block size!\n";
    eprint_str(here);
  }

  if (superblock.s_inode_size == 256) {
    char here[] = "Read the proper inode size!\n";
    eprint_str(here);
  }

  if (superblock.s_rev_level == 1) {
    char here[] = "Read the proper rev level!\n";
    eprint_str(here);
  }
#endif

  uint64_t actual_block_group_sizes =
      actual_block_size * superblock.s_blocks_per_group;

  uint32_t block_group_count =
      superblock.s_blocks_count / superblock.s_blocks_per_group;

  // Read the first block group (sector 4 = (1024 (empty space) + 1024
  // (superblock)) / 512 (sector size)) ... in other words, the sector right
  // after the superblock.
  result = virtio_blk_read_sync(driver, 4, buffer);
  if (result) {
    return (uint64_t)-1;
  }

  struct ext2_blockgroup bg;
  memcpy(&bg, buffer, sizeof(struct ext2_blockgroup));

#if 1
  if (bg.bg_block_bitmap == 0x6) {
    char here[] = "Read the proper block bitmap id!\n";
    eprint_str(here);
  }

  if (bg.bg_inode_bitmap == 0x7) {
    char here[] = "Read the proper inode bitmap id!\n";
    eprint_str(here);
  }

  if (bg.bg_inode_table == 0x8) {
    char here[] = "Read the proper inode table id!\n";
    eprint_str(here);
  }

  {
    char here[] = "Free blocks? ";
    eprint_str(here);
    eprint_num(bg.bg_free_blocks_count);
    eprint('\n');
  }

  {
    // In the first block, should be 117 (because 117 + 11 (reserved) == 128
    // (inodes per block!))
    char here[] = "Free inodes? ";
    eprint_str(here);
    eprint_num(bg.bg_free_inodes_count);
    eprint('\n');
  }
#endif

  // TODO: Make into proper unit tests.

  // Copy the raw contents of the _second_ entry in the inode table into an
  // inode.
  struct ext2_inode ino;
  inode_from_ino(driver, &ino, 2, superblock.s_inodes_per_group,
                 superblock.s_blocks_per_group, 1024);

  debug_dirent(driver, &ino, actual_block_size);

  inode_from_ino(driver, &ino, 14, superblock.s_inodes_per_group,
                 superblock.s_blocks_per_group, 1024);
  result = virtio_blk_read_sync(
      driver, sector_from_pos(ino.i_block[0] * actual_block_size), buffer);
  eprint_buffer("Contents of block 0 of blank file", buffer, 512);
  if (!strncmp("testing", (char *)buffer, strlen("testing"))) {
    eprint_str("Success!\n");
  }

  inode_from_ino(driver, &ino, 15, superblock.s_inodes_per_group,
                 superblock.s_blocks_per_group, 1024);
  result = virtio_blk_read_sync(
      driver, sector_from_pos(ino.i_block[0] * actual_block_size), buffer);
  eprint_buffer("Contents of block 0 of eve file", buffer, 512);
  if (!strncmp("adam", (char *)buffer, strlen("adam"))) {
    eprint_str("Success!\n");
  }

  epoweroff();
  return 0;
}

uint64_t sector_from_pos(uint64_t pos) { return pos / VIRTIO_BLK_SIZE; }

uint64_t sector_offset_from_pos(uint64_t pos) { return pos % VIRTIO_BLK_SIZE; }
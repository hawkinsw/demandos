#ifndef _BLK_H
#define _BLK_H

#include "virtio.h"
#include <stdint.h>
#include <unistd.h>

struct virtio_blk_req_hdr {
  uint32_t type;
  uint32_t ioprio;
  uint64_t sector;
};

struct virtio_blk_sg_hdr {
  void *addr;
  size_t len;
};

#define VIRTIO_BLK_WRITE_OP 1
#define VIRTIO_BLK_READ_OP 0

#define VIRTIO_BLK_SIZE 512

uint8_t virtio_blk_write_sync(struct virtio_driver *driver, uint64_t sector, void *addr);
uint8_t virtio_blk_read_sync(struct virtio_driver *driver, uint64_t sector, void *addr);
#endif
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

size_t virtio_blk_write_sync(struct virtio_driver *driver, uint8_t *input,
                            uint64_t offset, size_t size);

uint8_t virtio_blk_write_sector_sync(struct virtio_driver *driver,
                                    uint64_t sector, void *addr);

uint8_t virtio_blk_read_sector_sync(struct virtio_driver *driver,
                                    uint64_t sector, void *addr);
size_t virtio_blk_read_sync(struct virtio_driver *driver, uint8_t *output,
                            uint64_t offset, size_t size);

uint64_t virtio_blk_sector_from_pos(uint64_t pos);

uint64_t virtio_blk_sector_offset_from_pos(uint64_t pos);

#endif

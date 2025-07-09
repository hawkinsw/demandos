#include "blk.h"
#include "virtio.h"

#include <byteswap.h>

uint8_t virtio_blk_write_sync(struct virtio_driver *driver, uint64_t sector, void *addr) {

  uint8_t blk_io_status = 0xff;
  struct virtio_blk_req_hdr blk_write_hdr;
  struct virtio_blk_sg_hdr write_sgs[3];

  blk_write_hdr.type = VIRTIO_BLK_WRITE_OP;
  blk_write_hdr.sector = sector;

  write_sgs[0].addr = &blk_write_hdr;
  write_sgs[0].len = sizeof(struct virtio_blk_req_hdr);

  write_sgs[1].addr = addr;
  write_sgs[1].len = VIRTIO_BLK_SIZE;

  write_sgs[2].addr = &blk_io_status;
  write_sgs[2].len = sizeof(uint8_t);

  struct vring *vring = &driver->vring[0];
  uint16_t next = vring_next_descr(vring);
  uint32_t used = next;

  next = vring_add_to_descr(vring, write_sgs[0].addr, write_sgs[0].len, 0,
                              next, false);
  next = vring_add_to_descr(vring, write_sgs[1].addr, write_sgs[1].len, 0,
                              next, false);
  next = vring_add_to_descr(vring, write_sgs[2].addr, write_sgs[2].len, VRING_DESCR_WRITABLE,
                              next, true);

  vring_use_avail(vring, used);
  signal_virtio_device(driver, 0);
  vring_wait_completion(vring);

  return blk_io_status;
}

uint8_t virtio_blk_read_sync(struct virtio_driver *driver, uint64_t sector, void *addr) {

  uint8_t blk_io_status = 0xff;
  struct virtio_blk_req_hdr blk_read_hdr;
  struct virtio_blk_sg_hdr read_sgs[3];

  blk_read_hdr.type = VIRTIO_BLK_READ_OP;
  blk_read_hdr.sector = sector;

  read_sgs[0].addr = &blk_read_hdr;
  read_sgs[0].len = sizeof(struct virtio_blk_req_hdr);

  read_sgs[1].addr = addr;
  read_sgs[1].len = VIRTIO_BLK_SIZE;

  read_sgs[2].addr = &blk_io_status;
  read_sgs[2].len = sizeof(uint8_t);

  struct vring *vring = &driver->vring[0];
  uint16_t next = vring_next_descr(vring);
  uint32_t used = next;

  next = vring_add_to_descr(vring, read_sgs[0].addr, read_sgs[0].len, 0,
                              next, false);
  next = vring_add_to_descr(vring, read_sgs[1].addr, read_sgs[1].len, VRING_DESCR_WRITABLE,
                              next, false);
  next = vring_add_to_descr(vring, read_sgs[2].addr, read_sgs[2].len, VRING_DESCR_WRITABLE,
                              next, true);

  vring_use_avail(vring, used);
  signal_virtio_device(driver, 0);
  vring_wait_completion(vring);

  return blk_io_status;
}

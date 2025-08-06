#include "blk.h"
#include "io.h"
#include "virtio.h"

#include <byteswap.h>
#include <string.h>

uint8_t virtio_blk_write_sync(struct virtio_driver *driver, uint64_t sector,
                              void *addr) {

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

  next = vring_add_to_descr(vring, write_sgs[0].addr, write_sgs[0].len, 0, next,
                            false);
  next = vring_add_to_descr(vring, write_sgs[1].addr, write_sgs[1].len, 0, next,
                            false);
  next = vring_add_to_descr(vring, write_sgs[2].addr, write_sgs[2].len,
                            VRING_DESCR_WRITABLE, next, true);

  vring_use_avail(vring, used);
  signal_virtio_device(driver, 0);
  vring_wait_completion(vring);

  return blk_io_status;
}

uint8_t virtio_blk_read_sector_sync(struct virtio_driver *driver,
                                    uint64_t sector, void *addr) {

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

  next = vring_add_to_descr(vring, read_sgs[0].addr, read_sgs[0].len, 0, next,
                            false);
  next = vring_add_to_descr(vring, read_sgs[1].addr, read_sgs[1].len,
                            VRING_DESCR_WRITABLE, next, false);
  next = vring_add_to_descr(vring, read_sgs[2].addr, read_sgs[2].len,
                            VRING_DESCR_WRITABLE, next, true);

  vring_use_avail(vring, used);
  signal_virtio_device(driver, 0);
  vring_wait_completion(vring);

  return blk_io_status;
}

#define MIN(x, y) ((x < y) ? x : y)
size_t virtio_blk_read_sync(struct virtio_driver *driver, uint8_t *output,
                            uint64_t offset, size_t size) {
  size_t read_amt = 0, last_read_amt = 0;
  size_t sector_offset = virtio_blk_sector_offset_from_pos(offset);
  for (read_amt = 0; read_amt < size; read_amt += last_read_amt) {
    char blk[VIRTIO_BLK_SIZE] = {
        0,
    };
    uint8_t read_result = virtio_blk_read_sector_sync(
        driver, virtio_blk_sector_from_pos(offset + read_amt), blk);
    // If the latest read did not get what we wanted, then assume
    // that nothing came back. Tell the caller only about what we know was good.
    if (read_result) {
      return read_amt;
    }

    size_t size_of_content_in_sector = (VIRTIO_BLK_SIZE - sector_offset);

    // Copy what was read.
    // Example: Want 1118 bytes.
    // Second read: min((1118 - (512), VIRTIO_BLK_SIZE)
    // Second read: min((606        ), VIRTIO_BLK_SIZE)
    // Second read: 512
    // Last read:   min(1118 - (512 + 512)), VIRTIO_BLK_SIZE)
    // Last read:   min(1118 - (1024     )), VIRTIO_BLK_SIZE)
    // Last read:   min(94                ), VIRTIO_BLK_SIZE1)
    // Last read:   min(94                ), VIRTIO_BLK_SIZE1)
    // Last read:   94
    last_read_amt = MIN((size - read_amt), size_of_content_in_sector);
    memcpy(output + read_amt, blk + sector_offset, last_read_amt);

    sector_offset = 0;
  }

  return read_amt;
}

uint64_t virtio_blk_sector_from_pos(uint64_t pos) { return pos / VIRTIO_BLK_SIZE; }

uint64_t virtio_blk_sector_offset_from_pos(uint64_t pos) { return pos % VIRTIO_BLK_SIZE; }

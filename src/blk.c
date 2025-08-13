#include "blk.h"
#include "build_config.h"
#include "system.h"
#include "util.h"
#include "virtio.h"

#include <byteswap.h>
#include <string.h>

uint8_t virtio_blk_write_sector_sync(struct virtio_driver *driver,
                                     uint64_t sector, void *addr) {

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
  uint16_t next = vring_use_descr(vring);
  uint32_t used = next;

  next = vring_add_to_descr(vring, write_sgs[0].addr, write_sgs[0].len, 0, next,
                            false);
  next = vring_add_to_descr(vring, write_sgs[1].addr, write_sgs[1].len, 0, next,
                            false);
  next = vring_add_to_descr(vring, write_sgs[2].addr, write_sgs[2].len,
                            VRING_DESCR_WRITABLE, next, true);

  vring_post_descr(vring, used);
  signal_virtio_device(driver, 0);
  vring_wait_completion(vring);
  vring_unuse_descr(vring, used);

  WRITE_FENCE();

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

  uint16_t next = vring_use_descr(vring);
  uint32_t used = next;

  next = vring_add_to_descr(vring, read_sgs[0].addr, read_sgs[0].len, 0, next,
                            false);
  next = vring_add_to_descr(vring, read_sgs[1].addr, read_sgs[1].len,
                            VRING_DESCR_WRITABLE, next, false);
  next = vring_add_to_descr(vring, read_sgs[2].addr, read_sgs[2].len,
                            VRING_DESCR_WRITABLE, next, true);

  vring_post_descr(vring, used);
  signal_virtio_device(driver, 0);
  vring_wait_completion(vring);
  vring_unuse_descr(vring, used);

  WRITE_FENCE();

  return blk_io_status;
}

size_t virtio_blk_read_sync(struct virtio_driver *driver, uint8_t *output,
                            uint64_t offset, size_t size) {
  size_t read_amt = 0, last_read_amt = 0;
  for (read_amt = 0; read_amt < size; read_amt += last_read_amt) {

    size_t sector = virtio_blk_sector_from_pos(offset + read_amt);
    // If we are not right at the start of a sector boundary, there may
    // need to be an adjustment.
    size_t sector_offset = virtio_blk_sector_offset_from_pos(offset + read_amt);
    size_t size_of_content_in_sector = (VIRTIO_BLK_SIZE - sector_offset);
    // Determine how much to read ... either the amount of interest in this
    // sector or the remaining bytes to be read, whichever is smaller!
    last_read_amt = MIN((size - read_amt), size_of_content_in_sector);

    char blk[VIRTIO_BLK_SIZE] = {
        0,
    };

#if DEBUG_LEVEL > DEBUG_TRACE
    {
      eprint_str("(blk) Attempting to read ");
      eprint_num(last_read_amt);
      eprint_str(" bytes from sector no ");
      eprint_num(sector);
      eprint_str(" at offset ");
      eprint_num(sector_offset);
      eprint_str(".\n");
    }
#endif

    uint8_t read_result = virtio_blk_read_sector_sync(driver, sector, blk);
    // If the latest read did not get what we wanted, then assume
    // that nothing came back. Tell the caller only about what we know was good.
    if (read_result) {
      return read_amt;
    }

    memcpy(output + read_amt, blk + sector_offset, last_read_amt);
  }

  return read_amt;
}

size_t virtio_blk_write_sync(struct virtio_driver *driver, uint8_t *input,
                             uint64_t offset, size_t size) {
  size_t write_amt = 0, last_write_amt = 0;
  for (write_amt = 0; write_amt < size; write_amt += last_write_amt) {
    char blk[VIRTIO_BLK_SIZE] = {
        0,
    };

    size_t sector = virtio_blk_sector_from_pos(offset + write_amt);
    // If we are not right at the start of a sector boundary, there may
    // need to be an adjustment.
    size_t sector_offset =
        virtio_blk_sector_offset_from_pos(offset + write_amt);

    uint8_t read_result = virtio_blk_read_sector_sync(driver, sector, blk);
    // If the latest read did not get what we wanted, then assume
    // that nothing came back. Tell the caller only about what we know was good.
    if (read_result) {
      return write_amt;
    }

    size_t size_of_content_in_sector = (VIRTIO_BLK_SIZE - sector_offset);

    // Either write what's left or what can fit into this sector, whichever
    // is the smallest.
    last_write_amt = MIN((size - write_amt), size_of_content_in_sector);

    // Overwrite what was read with the appropriate user-supplied data.
    memcpy(blk + sector_offset, input + write_amt, last_write_amt);

#if DEBUG_LEVEL > DEBUG_TRACE
    {
      eprint_str("(blk) Attempting to write ");
      eprint_num(last_write_amt);
      eprint_str(" bytes to sector no ");
      eprint_num(sector);
      eprint_str(" at offset ");
      eprint_num(sector_offset);
      eprint_str(".\n");
    }
#endif

    // Try to write it all back to the disk.
    uint8_t write_result = virtio_blk_write_sector_sync(driver, sector, blk);

    // If the latest read did not get what we wanted, then assume
    // that nothing came back. Tell the caller only about what we know was good.
    if (write_result) {
      return write_amt;
    }

    sector_offset = 0;
  }

  return write_amt;
}

uint64_t virtio_blk_sector_from_pos(uint64_t pos) {
  return pos / VIRTIO_BLK_SIZE;
}

uint64_t virtio_blk_sector_offset_from_pos(uint64_t pos) {
  return pos % VIRTIO_BLK_SIZE;
}

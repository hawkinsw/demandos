#include "virtio.h"
#include "build_config.h"
#include "demandos.h"
#include "e.h"
#include "ecall.h"
#include "memory.h"
#include "system.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

struct blk_config {
  uint32_t blk_size;
};

uint16_t vring_use_descr(struct vring *vring) {
  return vring->bookeeping.descr_next;
}
uint16_t vring_unuse_descr(struct vring *vring, uint16_t goback) {
  return vring->bookeeping.descr_next = goback;
}

uint16_t vring_add_to_descr(struct vring *vring, void *buf, uint32_t size,
                            uint16_t flags, uint32_t where, bool ends_chain) {

  if (where >= vring->num) {
    eprint_str("Error: Attempting to use a descriptor entry beyond the "
               "available quantity.\n");
    epoweroff();
  }
  uint32_t next = (vring->bookeeping.descr_next + 1) % (vring->num - 1);

  vring->desc[where].addr = (uint64_t)buf;
  vring->desc[where].len = size;
  vring->desc[where].next = ends_chain ? 0 : next;
  vring->desc[where].flags = flags;
  vring->desc[where].flags = ends_chain
                                 ? vring->desc[where].flags & VRING_DESC_NO_NEXT
                                 : vring->desc[where].flags | VRING_DESC_NEXT;

  if (next >= vring->num) {
    eprint_str("Warning: Providing a next index for a descriptor that is "
               "beyond the available quantity.\n");
  }
  return vring->bookeeping.descr_next = next;
}

void vring_post_descr(struct vring *vring, uint32_t used_descr_idx) {
  vring->avail->ring[vring->bookeeping.avail_next % (vring->num - 1)] =
      used_descr_idx;
  vring->bookeeping.avail_next = (vring->bookeeping.avail_next + 1);
  WRITE_FENCE();
  vring->avail->idx = vring->bookeeping.avail_next;
  WRITE_FENCE();
}


void vring_wait_completion(struct vring *vring) {

  WRITE_FENCE();
  volatile uint16_t id = vring->used->idx;
  WRITE_FENCE();

#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char waiting[] = "(Before starting to wait for completion) Id: ";
    eprint_str(waiting);
    eprint_num(id);
    eprint('\n');
  }
#endif

  while (id == vring->bookeeping.used_last) {
    id = vring->used->idx;
    WRITE_FENCE();
  }
#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char waiting[] = "(Aftering finishing to wait for completion) Id: ";
    eprint_str(waiting);
    eprint_num(id);
    eprint('\n');
    char done_waiting[] = "Done waiting ...\n";
    eprint_str(done_waiting);
  }
#endif

  vring->bookeeping.used_last = id;
}

void signal_virtio_device(struct virtio_driver *driver, uint16_t signal) {
#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char done_waiting[] = "Signaling ...\n";
    eprint_str(done_waiting);
  }
#endif
  volatile uint16_t *signal_p = (uint16_t *)(driver->host + 16);
  *(volatile uint16_t *)signal_p = signal;
  WRITE_FENCE();
}

static inline unsigned virtq_descr_size(unsigned int qsz) {
  return sizeof(struct vring_desc_item) * qsz;
}

static inline unsigned virtq_avail_size(unsigned int qsz) {
  return sizeof(uint16_t) * 3 + sizeof(vring_avail_item) * qsz;
}

static inline unsigned virtq_used_size(unsigned int qsz) {
  return sizeof(uint16_t) * 3 + sizeof(struct vring_used_item) * qsz;
}

static inline unsigned virtq_size(unsigned int qsz) {
  return ALIGN_UP(virtq_descr_size(qsz) + virtq_avail_size(qsz), 4096) +
         ALIGN_UP(virtq_used_size(qsz), 4096);
}

void vring_init(struct vring *vring, size_t qsz) {

  size_t memsize = virtq_size(qsz);

  void *vring_shared_space = forever_alloc_aligned(memsize, 4096);

  vring->size = memsize;
  vring->bouncebufs = NULL;
  vring->desc = (struct vring_desc_item *)vring_shared_space;

  WRITE_FENCE();
  vring->avail =
      (struct vring_avail *)((uint64_t)vring->desc + virtq_descr_size(qsz));
  vring->used = (struct vring_used *)(ALIGN_UP(
      ((uint64_t)vring->avail) + virtq_avail_size(qsz), 4096));

  vring->avail->flags = 1;

  vring->bookeeping.used_avail = qsz;
  vring->bookeeping.avail_avail = qsz;
  vring->bookeeping.used_last = 0;
  vring->bookeeping.avail_next = 0;
  vring->bookeeping.descr_next = 0;
}

void virtio_device_queue_select(void *virtio_host_cfg, uint16_t q) {
  // Select the virtio q #1.
  uint16_t *virtio_q_sel_p =
      (uint16_t *)(virtio_host_cfg + VIRTIO_Q_SELECT_HOST_CFG_OFFSET);
  *virtio_q_sel_p = q;
  WRITE_FENCE();
}

uint16_t virtio_device_queue_size_read(void *virtio_host_cfg) {
  // Read the queue size.
  uint16_t *virtio_q_len_p =
      (uint16_t *)(virtio_host_cfg + VIRTIO_Q_SIZE_HOST_CFG_OFFSET); //
  uint16_t virtio_q_len = *virtio_q_len_p;
  WRITE_FENCE();
  return virtio_q_len;
}
uint16_t virtio_device_queue_desc_write(void *virtio_host_cfg,
                                        struct vring_desc_item *descr) {
  // Write the address of the virtq descr.
  uint32_t *descr_conf_addr =
      (uint32_t *)(virtio_host_cfg + VIRTIO_Q_ADDR_HOST_CFG_OFFSET);
  *descr_conf_addr = ((uint64_t)descr) >> VIRTIO_Q_ADDR_PAGE_SHIFT;
  WRITE_FENCE();
}

void virtio_device_set_status(void *virtio_host_cfg, uint8_t status) {
  uint8_t *device_status_p =
      (uint8_t *)(virtio_host_cfg + VIRTIO_DEVICE_STATUS_HOST_CFG_OFFSET);
  *device_status_p = status;
  WRITE_FENCE();
}

// If virtio_get_randomness is called early in the boot process,
// stdlib's memcpy might not yet be available.
void DEMANDOS_INTERNAL(memcpy)(uint8_t *dest, uint8_t *src, size_t amt) {
  for (size_t i = 0; i < amt; i++) {
    *dest++ = *src++;
  }
}

uint64_t virtio_get_randomness(void *buffer, uint64_t length) {
#define CHUNK_SIZE 8
  const size_t chunk_size = CHUNK_SIZE;
  uint8_t buf[CHUNK_SIZE] = {
      0x0,
  };

  struct virtio_driver *driver = find_virtio_driver(5);

  if (!driver || !driver->initialized) {
    return -1;
  }

  uint16_t next = vring_use_descr(driver->vring);

  for (size_t filled = 0; filled < length; filled += chunk_size) {
    uint32_t just_used = next;
    next = vring_add_to_descr(driver->vring, (void *)buf, chunk_size, 2, next,
                              true);

    vring_post_descr(&driver->vring[0], just_used);
    signal_virtio_device(driver, 0);
    vring_wait_completion(&driver->vring[0]);
    vring_unuse_descr(&driver->vring[0], just_used);

#if DEBUG_LEVEL > DEBUG_TRACE
    {
      char msg[] = "Randomness retrieved: ";
      eprint_str(msg);
      for (int i = 0; i < chunk_size; i++) {
        eprint_num(buf[i]);
        eprint(',');
      }
      eprint('\n');
    }
#endif

    size_t amt_to_copy = chunk_size;
    if (length - filled < chunk_size) {
      amt_to_copy = length - filled;
    }
    // Cannot use memcpy because it might not be available.

    DEMANDOS_INTERNAL(memcpy)(buffer + filled, buf, amt_to_copy);
  }
  return length;
}

#ifndef _VIRTIO_H
#define _VIRTIO_H

#include "pci.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONSOLE_DEVICE 3
#define ENTROPY_DEVICE 4

#define VRING_DESC_NEXT 1
#define VRING_DESC_NO_NEXT (~((uint32_t)VRING_DESC_NEXT))

#define VRING_DESCR_WRITABLE 2

struct vring;
struct virtio_driver;

typedef bool (*driver_init_f)(struct virtio_driver *driver, void *pci_config,
                              pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                              void *host, uint32_t pci);

struct vring_bookeeping {
  uint16_t used_last;
  uint16_t avail_next;
  uint16_t avail_avail;
  uint16_t used_avail;
  uint16_t descr_head;
  uint16_t descr_next;
};

struct virtio_driver {
  uint32_t device_id;
  bool initialized;
  driver_init_f init_f;
  void *host;
  uint32_t pci;
  void *conf;
  struct vring *vring;
};

struct vring_desc_item {
  uint64_t addr;
  uint32_t len;
  uint16_t flags;
  uint16_t next;
};

typedef uint16_t vring_avail_item;
struct vring_used_item {
  uint32_t id;
  uint32_t len;
};

struct vring_avail {
  uint16_t flags;
  uint16_t idx;
  vring_avail_item ring[];
};

struct vring_used {
  uint16_t flags;
  uint16_t idx;
  struct vring_used_item ring[];
};

struct vring {
  uint32_t num;
  size_t size;
  void *bouncebufs;
  struct vring_bookeeping bookeeping;
  struct vring_desc_item *desc;
  struct vring_avail *avail;
  struct vring_used *used;
};

uint16_t vring_next_descr(struct vring *vring);

uint16_t vring_add_to_descr(struct vring *vring, void *buf, uint32_t size,
                              uint16_t flags, uint32_t where, bool ends_chain);

void vring_use_avail(struct vring *vring, uint32_t used_descr_idx);

void vring_wait_completion(struct vring *vring);

void signal_virtio_device(struct virtio_driver *driver, uint16_t signal);

void vring_init(struct vring *, size_t size);

struct virtio_driver *find_virtio_driver(uint16_t type);

struct virtio_driver *get_virtio_desc();

#define VIRTIO_Q_SELECT_HOST_CFG_OFFSET 14
void virtio_device_queue_select(void *virtio_host_cfg, uint16_t q);

#define VIRTIO_Q_SIZE_HOST_CFG_OFFSET 12
uint16_t virtio_device_queue_size_read(void *virtio_host_cfg);

#define VIRTIO_Q_ADDR_HOST_CFG_OFFSET 8
#define VIRTIO_Q_ADDR_PAGE_SHIFT 12
uint16_t virtio_device_queue_desc_write(void *virtio_host_cfg, struct vring_desc_item *descr);

#define VIRTIO_DEVICE_STATUS_ACK 1
#define VIRTIO_DEVICE_STATUS_DRIVER_KNOWS 2
#define VIRTIO_DEVICE_STATUS_DRIVER_OK 4
#define VIRTIO_DEVICE_STATUS_HOST_CFG_OFFSET 18

void virtio_device_set_status(void *virtio_host_cfg, uint8_t status);

#define DECLARE_DRIVER(name, ...)                                              \
  struct virtio_driver __driver_##name                                         \
      __attribute__((section(".driver_inits"))) = __VA_ARGS__;

#define MAX_VIRTIO_DEVICE_ID 10
extern struct virtio_driver virtio_drivers[MAX_VIRTIO_DEVICE_ID];

uint64_t virtio_get_randomness(void *buffer, uint64_t length);

#endif

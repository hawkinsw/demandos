#include "virtio.h"
#include "build_config.h"
#include "ecall.h"
#include "pci.h"
#include "smemory.h"
#include "system.h"
#include <stdbool.h>
#include <stdint.h>

struct blk_config {
  uint32_t blk_size;
};

/*
 * Be careful about logging inside virtio operations -- if the console
 * driver is the one doing the logging, the output it generates will
 * issue an ecall for the output (and cause a nested interrupt).
 */
bool init_entropy_virtio(struct virtio_driver *driver, void *bus_base,
                         pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                         void *host, uint32_t pci);

bool init_console_virtio(struct virtio_driver *driver, void *bus_base,
                         pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                         void *host, uint32_t pci);

struct virtio_driver virtio_drivers[MAX_VIRTIO_DEVICE_ID] = {
    {},
};

struct virtio_driver *find_virtio_driver(uint16_t type) {
  if (type == 0 || type > 10) {
    return NULL;
  }
  return &virtio_drivers[type];
}

bool make_virtio_req(struct vring *vring, void *buf, uint32_t size,
                     uint16_t flags) {

  vring->desc->addr = (uint64_t)buf;
  vring->desc->len = size;
  vring->desc->flags = flags;
  vring->desc->next = 0;

  vring->avail->flags = 1;
  vring->avail->ring[0] = 0;
  WRITE_FENCE();
  vring->avail->idx++;
  WRITE_FENCE();

  return true;
}

void wait_virtio_completion(struct vring *vring) {

  volatile uint16_t id = vring->used->idx;
  WRITE_FENCE();
  while (id == vring->last_handled_used) {
    id = vring->used->idx;
    WRITE_FENCE();
  }
#if DEBUG_LEVEL > DEBUG_TRACE
  char done_waiting[] = "Done waiting ...\n";
  eprint_str(done_waiting);
#endif
  vring->last_handled_used = id;
}

void signal_virtio_device(struct virtio_driver *driver, uint16_t signal) {
  volatile uint16_t *signal_p = (uint16_t *)(driver->host + 16);
  *signal_p = signal;
}

static inline unsigned virtq_size(unsigned int qsz) {
  return ALIGN_UP(sizeof(struct vring_desc_item) * qsz +
                      sizeof(uint16_t) * (3 + qsz),
                  4096) +
         ALIGN_UP(sizeof(uint16_t) * 3 + sizeof(struct vring_used_item) * qsz,
                  4096);
}

void init_vring(struct vring *vring, size_t qsz) {

  size_t memsize = virtq_size(qsz);

  void *descr_space = salign(memsize, 4096);

  vring->size = memsize;
  vring->bouncebufs = NULL;
  vring->desc = (struct vring_desc_item *)descr_space;

  WRITE_FENCE();
  vring->avail = (struct vring_avail *)((uint64_t)vring->desc +
                                        qsz * sizeof(struct vring_desc_item));
  vring->used = (struct vring_used *)(ALIGN_UP((uint64_t)vring->avail, 4096));
}

bool init_entropy_virtio(struct virtio_driver *driver, void *device_conf,
                         pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                         void *host, uint32_t pci) {

  driver->host = host;
  driver->pci = pci;
  driver->conf = device_conf;

  // Set up the memory mapping for the IO BAR.
  uint32_t *bar0_p = (uint32_t *)(device_conf + BAR0_OFFSET);
  // Set the address for the common configuration.
  *bar0_p = pci;
  WRITE_FENCE();

  volatile uint32_t temp = *bar0_p;
  WRITE_FENCE();

  // In BAR1, mark nothing.
  uint32_t bar1_value = 0xffffffff;
  uint32_t *bar1_p = (uint32_t *)(device_conf + BAR1_OFFSET);
  *bar1_p = bar1_value;
  WRITE_FENCE();

  temp = *bar1_p;
  WRITE_FENCE();

  // Change the status of the PCI card.
  uint8_t card_status = 7;
  volatile uint8_t *card_status_p = (uint8_t *)(device_conf + 0x4);
  *card_status_p = card_status;
  WRITE_FENCE();

  // Change the status of the device.
  uint8_t device_status = 1;
  uint8_t *device_status_p = (uint8_t *)(host + 18);
  *device_status_p = device_status;
  WRITE_FENCE();

  // Select the virtio q #0.
  uint16_t virtio_q_sel = 0;
  uint16_t *virtio_q_sel_p = (uint16_t *)(host + 14);
  *virtio_q_sel_p = virtio_q_sel;
  WRITE_FENCE();

  // Read the queue size.
  uint16_t *virtio_q_len_p = (uint16_t *)(host + 12);
  uint16_t virtio_q_len = *virtio_q_len_p;
  WRITE_FENCE();

  // Allocate a single vring.
  driver->vring = smalloc(sizeof(struct vring));

  init_vring(driver->vring, 8);

  // Write the address of the virtq descr.
  uint32_t *descr_conf_addr = (uint32_t *)(host + 8);
  *descr_conf_addr = ((uint64_t)driver->vring->desc) >> 12;
  WRITE_FENCE();

  // Do final "up" status set (on the device!).
  device_status = 6;
  *device_status_p = device_status;
  WRITE_FENCE();

  driver->initialized = true;

  return true;
}

// https://docs.oasis-open.org/virtio/virtio/v1.1/cs01/virtio-v1.1-cs01.html#x1-1250008
bool init_blk_virtio(struct virtio_driver *driver, void *device_conf,
                     pci_bus_t bus, pci_dev_t dev, pci_fun_t fun, void *host,
                     uint32_t pci) {

  driver->host = host;
  driver->pci = pci;
  driver->conf = device_conf;

  // Set up the memory mapping for the IO BAR.
  uint32_t *bar0_p = (uint32_t *)(device_conf + BAR0_OFFSET);
  // Set the address for the common configuration.
  *bar0_p = pci;
  WRITE_FENCE();

  volatile uint32_t temp = *bar0_p;
  WRITE_FENCE();

  // In BAR1, mark nothing.
  // In BAR1, mark nothing.
  uint32_t bar1_value = 0xffffffff;
  uint32_t *bar1_p = (uint32_t *)(device_conf + BAR1_OFFSET);
  *bar1_p = bar1_value;
  WRITE_FENCE();

  // Change the status of the PCI card.
  uint8_t card_status = 7;
  volatile uint8_t *card_status_p = (uint8_t *)(device_conf + 0x4);
  *card_status_p = card_status;
  WRITE_FENCE();

  // Change the status of the device.
  uint8_t device_status = 1;
  uint8_t *device_status_p = (uint8_t *)(host + 18);
  *device_status_p = device_status;
  WRITE_FENCE();

  uint32_t blk_features_available = 0;
  volatile uint32_t *blk_features_available_p = (uint32_t *)(host + 0x00);
  blk_features_available = *blk_features_available_p;
  WRITE_FENCE();

  uint32_t blk_features_set = 0;
  volatile uint32_t *blk_features_set_p = (uint32_t *)(host + 0x04);
  *blk_features_set_p = blk_features_set;
  WRITE_FENCE();

  struct blk_config *blk = (struct blk_config*)(host + 20);
  WRITE_FENCE();


#if DEBUG_LEVEL > DEBUG_TRACE
  char msg[] = "Found block device with block size: ";
  eprint_str(msg);
  eprint_num(blk->blk_size);
  eprint('\n');
#endif

  // Select the virtio q #0.
  uint16_t virtio_q_sel = 0;
  uint16_t *virtio_q_sel_p = (uint16_t *)(host + 14);
  *virtio_q_sel_p = virtio_q_sel;
  WRITE_FENCE();

  // Read the queue size.
  uint16_t *virtio_q_len_p = (uint16_t *)(host + 12); //
  uint16_t virtio_q_len = *virtio_q_len_p;
  WRITE_FENCE();

  // Allocate space for 2 vrings

  driver->vring = (struct vring *)smalloc(sizeof(struct vring) * 2);
  init_vring(&driver->vring[0], 128);
  init_vring(&driver->vring[1], 128);

  // Write the address of the virtq descr.
  uint32_t *descr_conf_addr = (uint32_t *)(host + 8);
  *descr_conf_addr = ((uint64_t)driver->vring[0].desc) >> 12;
  WRITE_FENCE();

  // Select the virtio q #1.
  virtio_q_sel = 1;
  virtio_q_sel_p = (uint16_t *)(host + 14);
  *virtio_q_sel_p = virtio_q_sel;
  WRITE_FENCE();

  // Read the queue size.
  virtio_q_len_p = (uint16_t *)(host + 12); //
  virtio_q_len = *virtio_q_len_p;
  WRITE_FENCE();

  // Write the address of the virtq descr.
  *descr_conf_addr = ((uint64_t)driver->vring[1].desc) >> 12;
  WRITE_FENCE();

  // Do final "up" status set (on the device!).
  device_status = 6;
  *device_status_p = device_status;
  WRITE_FENCE();

  driver->initialized = true;

  return true;
}

// 4.1.4.8 Legacy Interfaces: A Note on PCI Device Layout ()

bool init_console_virtio(struct virtio_driver *driver, void *device_conf,
                         pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                         void *host, uint32_t pci) {

  driver->host = host;
  driver->pci = pci;
  driver->conf = device_conf;

  // Set up the memory mapping for the IO BAR.
  uint32_t *bar0_p = (uint32_t *)(device_conf + BAR0_OFFSET);
  // Set the address for the common configuration.
  *bar0_p = pci;
  WRITE_FENCE();

  volatile uint32_t temp = *bar0_p;
  WRITE_FENCE();

  // In BAR1, mark nothing.
  uint32_t bar1_value = 0xffffffff;
  uint32_t *bar1_p = (uint32_t *)(device_conf + BAR1_OFFSET);
  *bar1_p = bar1_value;
  WRITE_FENCE();

  temp = *bar1_p;
  WRITE_FENCE();

  // Change the status of the PCI card.
  uint8_t card_status = 7;
  volatile uint8_t *card_status_p = (uint8_t *)(device_conf + 0x4);
  *card_status_p = card_status;
  WRITE_FENCE();

  // Change the status of the device.
  uint8_t device_status = 1;
  uint8_t *device_status_p = (uint8_t *)(host + 18);
  *device_status_p = device_status;
  WRITE_FENCE();

  // Select the virtio q #0.
  uint16_t virtio_q_sel = 0;
  uint16_t *virtio_q_sel_p = (uint16_t *)(host + 14);
  *virtio_q_sel_p = virtio_q_sel;
  WRITE_FENCE();

  // Read the queue size.
  uint16_t *virtio_q_len_p = (uint16_t *)(host + 12); //
  uint16_t virtio_q_len = *virtio_q_len_p;
  WRITE_FENCE();

  // Allocate space for 2 vrings

  driver->vring = (struct vring *)smalloc(sizeof(struct vring) * 2);
  init_vring(&driver->vring[0], 128);
  init_vring(&driver->vring[1], 128);

  // Write the address of the virtq descr.
  uint32_t *descr_conf_addr = (uint32_t *)(host + 8);
  *descr_conf_addr = ((uint64_t)driver->vring[0].desc) >> 12;
  WRITE_FENCE();

  // Select the virtio q #1.
  virtio_q_sel = 1;
  virtio_q_sel_p = (uint16_t *)(host + 14);
  *virtio_q_sel_p = virtio_q_sel;
  WRITE_FENCE();

  // Read the queue size.
  virtio_q_len_p = (uint16_t *)(host + 12); //
  virtio_q_len = *virtio_q_len_p;
  WRITE_FENCE();

  // Write the address of the virtq descr.
  *descr_conf_addr = ((uint64_t)driver->vring[1].desc) >> 12;
  WRITE_FENCE();

  // Do final "up" status set (on the device!).
  device_status = 6;
  *device_status_p = device_status;
  WRITE_FENCE();

  driver->initialized = true;

  return true;
}

DECLARE_DRIVER(random, {
                           .device_id = 5,
                           .init_f = init_entropy_virtio,
                       });

DECLARE_DRIVER(console, {
                            .device_id = 3,
                            .init_f = init_console_virtio,
                        });

DECLARE_DRIVER(blk, {
                        .device_id = 1,
                        .init_f = init_blk_virtio,
                    });

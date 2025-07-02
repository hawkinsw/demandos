#include "build_config.h"
#include "ecall.h"
#include "pci.h"
#include "smemory.h"
#include "system.h"
#include "virtio.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

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

bool init_entropy_virtio(struct virtio_driver *driver, void *pci_device,
                         pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                         void *host_configuration_regs, uint32_t pci_configuration_regs) {

  driver->host = host_configuration_regs;
  driver->pci = pci_configuration_regs;
  driver->conf = pci_device;

  pci_device_set_bar(pci_device, 0, pci_configuration_regs);

  // In BAR1, mark nothing.
  uint32_t bar1_value = 0xffffffff;
  pci_device_set_bar(pci_device, 1, bar1_value);

  // Change the status of the PCI device.
  pci_device_set_status(pci_device, PCI_DEVICE_STATUS_UP);

  // Change the status of the Virtio device.
  virtio_device_set_status(host_configuration_regs, VIRTIO_DEVICE_STATUS_ACK);

  // Select the virtio q #0.
  virtio_device_queue_select(host_configuration_regs, 0);

  // Read the queue size.
  uint16_t virtio_q_len = virtio_device_queue_size_read(host_configuration_regs);

#if DEBUG_LEVEL > DEBUG_TRACE
  char msg2[] = "Found block device with queue size: ";
  eprint_str(msg2);
  eprint_num(virtio_q_len);
  eprint('\n');
#endif

  // Allocate a single vring.
  driver->vring = smalloc(sizeof(struct vring));

  vring_init(driver->vring, 8);

  // Write the address of the virtq descr.
  virtio_device_queue_desc_write(host_configuration_regs, driver->vring->desc);

  // Do final "up" status set (on the device!).
  virtio_device_set_status(host_configuration_regs,
                           VIRTIO_DEVICE_STATUS_DRIVER_KNOWS |
                               VIRTIO_DEVICE_STATUS_DRIVER_OK);

  driver->initialized = true;

  return true;
}

// https://docs.oasis-open.org/virtio/virtio/v1.1/cs01/virtio-v1.1-cs01.html#x1-1250008
bool init_blk_virtio(struct virtio_driver *driver, void *pci_device,
                     pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                     void *host_configuration_regs,
                     uint32_t pci_configuration_regs) {

  driver->host = host_configuration_regs;
  driver->pci = pci_configuration_regs;
  driver->conf = pci_device;

  pci_device_set_bar(pci_device, 0, pci_configuration_regs);

  // In BAR1, mark nothing.
  uint32_t bar1_value = 0xffffffff;
  pci_device_set_bar(pci_device, 1, bar1_value);

  // Change the status of the PCI device.
  pci_device_set_status(pci_device, PCI_DEVICE_STATUS_UP);

  // Change the status of the Virtio device.
  virtio_device_set_status(host_configuration_regs, VIRTIO_DEVICE_STATUS_ACK);

  uint32_t blk_features_available = 0;
  volatile uint32_t *blk_features_available_p =
      (uint32_t *)(host_configuration_regs + 0x00);
  blk_features_available = *blk_features_available_p;
  WRITE_FENCE();

  uint32_t blk_features_set = 0;
  volatile uint32_t *blk_features_set_p =
      (uint32_t *)(host_configuration_regs + 0x04);
  *blk_features_set_p = blk_features_set;
  WRITE_FENCE();

  struct blk_config *blk = (struct blk_config *)(host_configuration_regs + 20);
  WRITE_FENCE();

#if DEBUG_LEVEL > DEBUG_TRACE
  char msg[] = "Found block device with block size: ";
  eprint_str(msg);
  eprint_num(blk->blk_size);
  eprint('\n');
#endif

  // Select the virtio q #0.
  virtio_device_queue_select(host_configuration_regs, 0);

  // Read the queue size.
  uint16_t virtio_q_len = virtio_device_queue_size_read(host_configuration_regs);

#if DEBUG_LEVEL > DEBUG_TRACE
  char msg2[] = "Found block device with queue size: ";
  eprint_str(msg2);
  eprint_num(virtio_q_len);
  eprint('\n');
#endif

  driver->vring = (struct vring *)smalloc(sizeof(struct vring));
  vring_init(&driver->vring[0], 256);

  // Write the address of the virtq descr.
  virtio_device_queue_desc_write(host_configuration_regs, driver->vring[0].desc);

  // Do final "up" status set (on the device!).
  virtio_device_set_status(host_configuration_regs,
                           VIRTIO_DEVICE_STATUS_DRIVER_KNOWS |
                               VIRTIO_DEVICE_STATUS_DRIVER_OK);

  driver->initialized = true;

  return true;
}

// 4.1.4.8 Legacy Interfaces: A Note on PCI Device Layout ()

bool init_console_virtio(struct virtio_driver *driver, void *pci_device,
                         pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                         void *host_configuration_regs,
                         uint32_t pci_configuration_regs) {

  driver->host = host_configuration_regs;
  driver->pci = pci_configuration_regs;
  driver->conf = pci_device;

  // NOTE: It's possible that this value is _already_ set. Resetting
  // _may_ cause problems. Investigate further.

  // Setting BAR0 in the device's configuration tells the device
  // where all the _additional_ configuration will happen (relative)
  // to its base PCI memory space.
  pci_device_set_bar(pci_device, 0, pci_configuration_regs);

  // In BAR1, mark nothing.
  uint32_t bar1_value = 0xffffffff;
  pci_device_set_bar(pci_device, 1, bar1_value);

  pci_device_set_status(pci_device, PCI_DEVICE_STATUS_UP);

  // Change the status of the device.
  virtio_device_set_status(host_configuration_regs, VIRTIO_DEVICE_STATUS_ACK);

  // Select the virtio q #0.
  virtio_device_queue_select(host_configuration_regs, 0);

  // Read the queue size.
  uint16_t virtio_q_len = virtio_device_queue_size_read(host_configuration_regs);

#if DEBUG_LEVEL > DEBUG_TRACE
  char msg2[] = "Found console device with queue size: ";
  eprint_str(msg2);
  eprint_num(virtio_q_len);
  eprint('\n');
#endif

  // Allocate space for 2 vrings

  driver->vring = (struct vring *)smalloc(sizeof(struct vring) * 2);
  vring_init(&driver->vring[0], 128);
  vring_init(&driver->vring[1], 128);

  // Write the address of the virtq descr.
  virtio_device_queue_desc_write(host_configuration_regs, driver->vring[0].desc);

  virtio_device_queue_select(host_configuration_regs, 1);

  // Read the queue size.
  virtio_q_len = virtio_device_queue_size_read(host_configuration_regs);

  // Write the address of the virtq descr.
  virtio_device_queue_desc_write(host_configuration_regs, driver->vring[1].desc);

  // Do final "up" status set (on the device!).
  virtio_device_set_status(host_configuration_regs,
                           VIRTIO_DEVICE_STATUS_DRIVER_KNOWS |
                               VIRTIO_DEVICE_STATUS_DRIVER_OK);

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

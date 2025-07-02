#ifndef _VIRTIO_DRIVER_H
#define _VIRTIO_DRIVER_H

#include "pci.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "virtio.h"

#define CONSOLE_DEVICE 3
#define ENTROPY_DEVICE 4

typedef bool (*driver_init_f)(struct virtio_driver *driver, void *pci_config,
                              pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                              void *host, uint32_t pci);

struct virtio_driver *find_virtio_driver(uint16_t type);

#define DECLARE_DRIVER(name, ...)                                              \
  struct virtio_driver __driver_##name                                         \
      __attribute__((section(".driver_inits"))) = __VA_ARGS__;

#define MAX_VIRTIO_DEVICE_ID 10
extern struct virtio_driver virtio_drivers[MAX_VIRTIO_DEVICE_ID];

#endif
#ifndef _VIRTIO_H
#define _VIRTIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pci.h"

#define CONSOLE_DEVICE 3
#define ENTROPY_DEVICE 4

struct vring;
struct virtio_driver;

typedef bool (*driver_init_f)(struct virtio_driver *driver, void *pci_config,
                         pci_bus_t bus, pci_dev_t dev, pci_fun_t fun,
                         void *host, uint32_t pci);

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

struct vring_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
};

struct vring_used_item {
    uint32_t id;
    uint32_t len;
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
    uint16_t last_handled_used;
    struct vring_desc_item *desc;
    struct vring_avail *avail;
    struct vring_used *used;
};

bool make_virtio_req(struct vring *vring, void *buf, uint32_t size, uint16_t flags);

void wait_virtio_completion(struct vring *vring);

void signal_virtio_device(struct virtio_driver *driver, uint16_t signal);

void init_vring(struct vring *, size_t size);

struct virtio_driver *find_virtio_driver(uint16_t type);

struct virtio_driver *get_virtio_desc();

#define DECLARE_DRIVER(name, ...) \
    struct virtio_driver __driver_##name __attribute__((section(".driver_inits"))) = __VA_ARGS__;


#define MAX_VIRTIO_DEVICE_ID 10
extern struct virtio_driver virtio_drivers[MAX_VIRTIO_DEVICE_ID];

#endif
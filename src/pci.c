#include "pci.h"
#include "build_config.h"
#include "ecall.h"
#include "virtio.h"
#include <memory.h>
#include <stdint.h>
#include "system.h"


uint32_t pci_config_space_alloc() {
  static uint32_t base = 0x0;
  uint32_t result = base;

  base += 4096;

  return result;
}

void pci_scan() {
  // TODO: Make this address dynamic.
  void *pci_base = (void *)PCI_BASE;
  for (uint32_t i = 0; i < MAX_PCI_OFF; i += PCI_OFF(0, 0, 1)) {
    void *dev_ptr = (void *)(pci_base + i);
    volatile uint16_t vendor_id = *(uint16_t *)dev_ptr;
    if (vendor_id == 0x1af4) {
#if DEBUG_LEVEL > DEBUG_TRACE
      char virtio[] = "Virtio\n";
      eprint_str(virtio);
#endif
      pci_bus_t bus = PCI_BUS(i);
      pci_dev_t dev = PCI_DEV(i);
      pci_fun_t fun = PCI_FUN(i);

      uint32_t allocd = pci_config_space_alloc();

      void *host = HOST_CONF_BASE + allocd;
      uint32_t pci = PCI_CONF_BASE + allocd;

      configure_pci_device(pci_base, bus, dev, fun, host, pci);
      continue;
    } else if (vendor_id == (UINT16_MAX)) {
#if DEBUG_LEVEL > DEBUG_TRACE
      char skipping[] = "Non-existent device";
      eprint_str(skipping);
#endif
    } else {
#if DEBUG_LEVEL > DEBUG_TRACE
      char other[] = "Existent, non-virtio device";
      eprint_str(other);
#endif
    }

#if DEBUG_LEVEL > DEBUG_TRACE
    char msg_rest_start[] = " at offset (decimal) ";
    eprint_str(msg_rest_start);
    eprint_num(i);
    char msg_rest_end[] = "; skipping.\n";
    eprint_str(msg_rest_end);
#endif

  }
}
void configure_pci_device(void *bus_base, pci_bus_t bus, pci_dev_t dev,
                          pci_fun_t fun, void *host, uint32_t pci) {
  void *device_conf = (bus_base + PCI_OFF(bus, dev, fun));

  uint16_t *device_id_p = (uint16_t*)(device_conf + 2);

  volatile uint16_t device_id = *device_id_p;

  if (device_id > 0x1009) {
    device_id -= 0x1040;
  } else {
    device_id -= 0x1000;
  }
  struct virtio_driver *driver = find_virtio_driver(device_id);

  bool init_result = driver->init_f(driver, device_conf, bus, dev, fun, host, pci);
}

extern void *__drivers_start;
extern void *__drivers_end;

void load_drivers() {
  void *drivers_start = (void*)&__drivers_start;
  void *drivers_iter = drivers_start;
  void *drivers_end = (void*)&__drivers_end;

  for (void *drivers_iter = drivers_start; drivers_iter < drivers_end; drivers_iter += sizeof(struct virtio_driver)) {
    struct virtio_driver *driver = (struct virtio_driver*)drivers_iter;

    if (driver->device_id >= MAX_VIRTIO_DEVICE_ID) {
      char oops[] = "Bad driver device id\n";
      eprint_str(oops);
      continue;
    }

    virtio_drivers[driver->device_id] = *driver;
  }
}

void configure_pci() {
  // Load all drivers.
  load_drivers();

  // Now, scan the bus!
  pci_scan();
}

void pci_device_set_status(void *pci_cfg, uint8_t status) {
  // Change the status of the PCI card.
  uint8_t card_status = PCI_DEVICE_STATUS_UP;
  volatile uint8_t *card_status_p = (uint8_t *)(pci_cfg + PCI_DEVICE_STATUS_OFFSET);
  *card_status_p = card_status;
  WRITE_FENCE();
}

void pci_device_set_bar(void *pci_cfg, uint8_t which, uint32_t value) {
  uint64_t offset = BAR0_OFFSET + (which * sizeof(uint32_t));
  uint32_t *bar_p = (uint32_t *)(pci_cfg + offset);
  *bar_p = value;
  WRITE_FENCE();
}
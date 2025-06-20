#ifndef PCI_H
#define PCI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

// Note: Assume that we are on PCIe.
#define BUS_SHIFT 20
#define DEV_SHIFT 15
#define FUN_SHIFT 12

#define BUS_MASK ~(BUS_SHIFT - 1)
#define DEV_MASK ~(DEV_SHIFT - 1)
#define FUN_MASK ~(FUN_SHIFT - 1)

#define PCI_OFF(bus, dev, fun) \
  (bus << BUS_SHIFT | dev << DEV_SHIFT | fun << FUN_SHIFT)

#define pci_bus_t uint8_t
#define pci_dev_t uint8_t
#define pci_fun_t uint8_t

#define PCI_BUS(off) ((pci_bus_t)(off >> BUS_SHIFT))
#define PCI_DEV(off) ((pci_dev_t)((off >> DEV_SHIFT) & DEV_MASK))
#define PCI_FUN(off) ((pci_fun_t)((off >> FUN_SHIFT) & FUN_MASK))

// TODO: Handle recursive pci bus enumeration.
//#define MAX_PCI_BUSSES 256
#define MAX_PCI_BUSSES 2
#define MAX_PCI_DEVICES 32
#define MAX_PCI_FUNCTIONS 8

#define MAX_PCI_OFF (PCI_OFF(MAX_PCI_BUSSES, MAX_PCI_DEVICES, MAX_PCI_FUNCTIONS))

// Ranges: https://stackoverflow.com/a/37685781
#define PCI_BASE 0x30000000

#define BAR0_OFFSET 0x10
#define BAR1_OFFSET 0x14

// (See above) Ranges specify mapping between child addresses (PCI)
// and parent addresses (CPU). 
#define PCI_CONF_BASE ((uint32_t)0x0001000)
#define HOST_CONF_BASE ((void*)  0x3001000)
/*
	+------------------------------------------------------------------+ 
	|                    Host Feature Bits[0:31]                       | 
	+------------------------------------------------------------------+
	|                    Guest Feature Bits[0:31]                      |
	+------------------------------------------------------------------+
	|                    Virtqueue Address PFN                         |
	+---------------------------------+--------------------------------+
	|           Queue Select          |           Queue Size           |
	+----------------+----------------+--------------------------------+
	|   ISR Status   | Device Stat    |           Queue Notify         |
	+----------------+----------------+--------------------------------+
	|       MSI Config Vector         |         MSI Queue Vector       |
	+---------------------------------+--------------------------------+
*/

/*

Notes:
https://blogs.oracle.com/linux/post/introduction-to-VirtIO
https://www.openeuler.org/en/blog/yorifang/virtio-spec-overview.html
*/

struct pci_conf {
    /*
     * base_addr: PCI bus base address
     * conf: The base address for 
     */
    bool (*configure)(void *base_addr, void *conf, uint32_t conf_off);
};

void configure_pci();
void configure_pci_device(void *bus_base, pci_bus_t bus, pci_dev_t dev,
                          pci_fun_t fun, void *host, uint32_t pci);
#endif
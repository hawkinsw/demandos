#include <stdint.h>

// Default value is replaced through configure_dl_random (called on boot).
uint64_t dl_random_bytes = 0x12345678;

char _pgm[] = "./demandos";
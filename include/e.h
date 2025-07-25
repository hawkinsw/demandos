#ifndef _E_H
#define _E_H

#include <stdint.h>
#include <stddef.h>

void eprint(uint64_t p);

uint64_t epoweroff();

void eprint_num(uint64_t num);

void eprint_str(char *str);

void eprint_strn(char *str, size_t n);

void eprint_buffer(char *msg, uint8_t *buffer, size_t size);

#endif

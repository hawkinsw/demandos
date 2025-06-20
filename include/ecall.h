#ifndef ECALL_H
#define ECALL_H

#include <stdint.h>
#include <stddef.h>

void eprint_str(char *str);
void eprint_strn(char *str, size_t n);
void eprint(uint64_t p);
void eprint_num(uint64_t num);

#endif
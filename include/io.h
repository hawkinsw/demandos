#ifndef IO_H
#define IO_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int(*write_handler_t)(uint64_t fd, void *buf, size_t size);
typedef int(*read_handler_t)(uint64_t fd, void *buf, size_t size);

struct io_descriptor {
    bool open;
    write_handler_t write_handler;
    read_handler_t read_handler;
};
extern  struct io_descriptor fds[100];

bool is_fd_open(uint64_t fd);

int open_fd(char *pathname);

#endif
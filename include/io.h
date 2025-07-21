#ifndef IO_H
#define IO_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int(*write_handler_t)(uint64_t fd, void *buf, size_t size);
typedef int(*read_handler_t)(uint64_t fd, void *buf, size_t size);
typedef int(*seek_handler_t)(uint64_t fd, long int off, int whence);

struct io_descriptor {
    bool open;
    uint64_t pos;
    write_handler_t write_handler;
    read_handler_t read_handler;
    seek_handler_t seek_handler;
};
extern  struct io_descriptor fds[100];

bool is_fd_open(uint64_t fd);

int open_fd(char *pathname);

uint64_t io_mount_hd();

#endif

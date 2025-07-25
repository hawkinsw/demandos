#ifndef _EXT2_H
#define _EXT2_H

#include <stdint.h>
#include <stdbool.h>
#include "virtio.h"

struct __attribute__((packed)) ext2_superblock {
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_r_blocks_count;
  uint32_t s_free_blocks_count;
  uint32_t s_free_inodes_count;
  uint32_t s_first_data_block;
  uint32_t s_log_block_size;
  uint32_t s_log_frag_size;
  uint32_t s_blocks_per_group;
  uint32_t s_frags_per_group;
  uint32_t s_inodes_per_group;
  uint8_t  s_padding1[32];
  uint32_t s_rev_level;
  uint8_t  s_padding2[4];
  uint32_t s_first_inode;
  uint32_t s_inode_size;
};

struct __attribute__((packed)) ext2_blockgroup {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
};


struct __attribute__((packed)) ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint8_t  padding1[28];
    uint8_t  padding2[128];
};


struct __attribute__((packed)) ext2_dirent {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    uint8_t  name[];
};

bool inode_from_ino(struct virtio_driver *driver, struct ext2_inode *rinode,
                    uint32_t ino, uint32_t ino_per_group,
                    uint32_t blocks_per_group, uint64_t block_size);

bool bg_from_bgno(struct virtio_driver *driver, struct ext2_blockgroup *rbg,
                  uint32_t bgno);

void debug_dirent(struct virtio_driver *driver, struct ext2_inode *inode,
                  uint64_t block_size);
#endif
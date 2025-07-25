#include "ext2.h"
#include "blk.h"
#include "e.h"
#include "io.h"
#include "virtio.h"

#include <string.h>

#define INODE_TABLE_LOC(ino, inode_per_group) ((ino - 1) / inode_per_group)
#define INODE_LOC_IN_TABLE(ino, inode_per_group) ((ino - 1) % inode_per_group)

bool bg_from_bgno(struct virtio_driver *driver, struct ext2_blockgroup *rbg,
                  uint32_t bgno) {
  // Calculate the sector for the _primary_ block group containing the inode
  // table that contains this inode (got that?)!
  uint64_t bg_sector =
      sector_from_pos(1024 + 1024 + (bgno * sizeof(struct ext2_blockgroup)));
  uint64_t bg_sector_offset = sector_offset_from_pos(
      1024 + 1024 + (bgno * sizeof(struct ext2_blockgroup)));

  uint8_t buffer[512];

  // Read in the block group.
  uint8_t result = virtio_blk_read_sync(driver, bg_sector, buffer);
  memcpy(rbg, buffer + bg_sector_offset, sizeof(struct ext2_blockgroup));

  return true;
}

bool inode_from_ino(struct virtio_driver *driver, struct ext2_inode *rinode,
                    uint32_t ino, uint32_t ino_per_group,
                    uint32_t blocks_per_group, uint64_t block_size) {
  uint32_t inode_table_block_group = INODE_TABLE_LOC(ino, ino_per_group);
  uint32_t inode_table_index = INODE_LOC_IN_TABLE(ino, ino_per_group);

  char msg[] = "inode_table_block_group: ";
  eprint_str(msg);
  eprint_num(inode_table_block_group);
  eprint('\n');

  uint8_t buffer[512] = {
      0,
  };

  // Calculate the sector for the _primary_ block group containing the inode
  // table that contains this inode (got that?)!
  uint64_t inode_table_block_group_sector = sector_from_pos(
      1024 + 1024 + (inode_table_block_group * block_size * blocks_per_group));
  uint64_t inode_table_block_group_sector_offset = sector_offset_from_pos(
      1024 + 1024 + (inode_table_block_group * block_size * blocks_per_group));

  // Read in the block group.
  struct ext2_blockgroup bg;
  uint8_t result = bg_from_bgno(driver, &bg, inode_table_block_group);
  /*
  uint8_t result =
      virtio_blk_read_sync(driver, inode_table_block_group_sector, buffer);
  memcpy(&bg, buffer + inode_table_block_group_sector_offset,
         sizeof(struct ext2_blockgroup));
    */
  // Read in the inode contents.
  uint64_t inode_table_entry_position =
      bg.bg_inode_table * block_size +
      inode_table_index * /*sizeof(struct ext2_inode)*/ 256;
  uint64_t inode_table_entry_sector =
      sector_from_pos(inode_table_entry_position);
  uint64_t inode_table_entry_sector_offset =
      sector_offset_from_pos(inode_table_entry_position);

  result = virtio_blk_read_sync(driver, inode_table_entry_sector, buffer);
  memcpy(rinode, buffer + inode_table_entry_sector_offset,
         sizeof(struct ext2_inode));

  return true;
}

void debug_dirent(struct virtio_driver *driver, struct ext2_inode *inode,
                  uint64_t block_size) {

  for (size_t o = 0; o < 10; o++) {

    if (inode->i_block[o] == 0) {
      break;
    }

    // A dirent cannot cross a filesystem-block boundary. That's nice!
    char buffer[1024];
    uint64_t block0_position = inode->i_block[o] * block_size;
    uint64_t block0_sector = sector_from_pos(inode->i_block[o] * block_size);

    uint64_t result = virtio_blk_read_sync(driver, block0_sector, buffer);
    result = virtio_blk_read_sync(driver, block0_sector + 1, buffer + 512);

    // Copy the raw contents of the directory entry into a dirent.
    char de_backing[1024] = {};
    struct ext2_dirent *de = (struct ext2_dirent *)de_backing;

    size_t dirent_idx = 0;
    // TODO: Refactor to handle dynamic block sizes.
    while (dirent_idx < 1024) {

      memcpy(de, buffer + dirent_idx, sizeof(struct ext2_dirent));
      memcpy(de, buffer + dirent_idx, de->rec_len);

      // TODO: Is it possible that the inode is 0? If so, what does that mean?
      if (de->inode == 0) {
        char here[] = "Skipping\n";
        eprint_str(here);

      } else {
        char here[] = "Inode\n";
        eprint_str(here);
      }

      char filename[255] = {
          0,
      };
      memcpy(filename, de->name, de->name_len);
      eprint_str(filename);
      eprint('\n');

      dirent_idx += de->rec_len;
    }
  }
}

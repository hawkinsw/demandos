#include "ext2.h"
#include "blk.h"
#include "e.h"
#include "ecall.h"
#include "io.h"
#include "virtio.h"

#include "build_config.h"

#include <string.h>

#define INODE_TABLE_LOC(ino, inode_per_group) ((ino - 1) / inode_per_group)
#define INODE_LOC_IN_TABLE(ino, inode_per_group) ((ino - 1) % inode_per_group)

bool bg_from_bgno(struct virtio_driver *driver, struct ext2_blockgroup *rbg,
                  uint32_t bgno) {
  // Calculate the sector for the _primary_ block group containing the inode
  // table that contains this inode (got that?)!
  uint64_t bg_sector = sector_from_pos(EXT2_FIRST_OFFSET_AFTER_SUPERBLOCK +
                                       (bgno * sizeof(struct ext2_blockgroup)));
  uint64_t bg_sector_offset =
      sector_offset_from_pos(EXT2_FIRST_OFFSET_AFTER_SUPERBLOCK +
                             (bgno * sizeof(struct ext2_blockgroup)));

  uint8_t buffer[512];

  // Read in the block group.
  uint8_t result = virtio_blk_read_sector_sync(driver, bg_sector, buffer);
  memcpy(rbg, buffer + bg_sector_offset, sizeof(struct ext2_blockgroup));

  return true;
}

bool inode_from_ino(struct virtio_driver *driver,
                    struct ext2_superblock *superblock,
                    struct ext2_inode *rinode, uint32_t ino) {

  uint32_t inode_table_block_group =
      INODE_TABLE_LOC(ino, superblock->s_inodes_per_group);
  uint32_t inode_table_index =
      INODE_LOC_IN_TABLE(ino, superblock->s_inodes_per_group);

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
      1024 + 1024 +
      (inode_table_block_group * 1024 * superblock->s_blocks_per_group));
  uint64_t inode_table_block_group_sector_offset = sector_offset_from_pos(
      1024 + 1024 +
      (inode_table_block_group * 1024 * superblock->s_blocks_per_group));

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
      bg.bg_inode_table * 1024 +
      inode_table_index * /*sizeof(struct ext2_inode)*/ 256;
  uint64_t inode_table_entry_sector =
      sector_from_pos(inode_table_entry_position);
  uint64_t inode_table_entry_sector_offset =
      sector_offset_from_pos(inode_table_entry_position);

  result =
      virtio_blk_read_sector_sync(driver, inode_table_entry_sector, buffer);
  memcpy(rinode, buffer + inode_table_entry_sector_offset,
         sizeof(struct ext2_inode));

  return true;
}

void debug_dirent(struct virtio_driver *driver,
                  struct ext2_superblock *superblock,
                  struct ext2_inode *inode) {
  for (size_t o = 0; o < EXT2_MAX_DIRECT_BLOCKS_PER_INODE; o++) {
    if (inode->i_block[o] == 0) {
      break;
    }

    // A dirent cannot cross a filesystem-block boundary. So, if we read a
    // block, we know that there are a certain number of full dirents. See below
    // for information about reclen.
    uint8_t buffer[1024];
    uint64_t inode_fs_block_offset = inode->i_block[o] * 1024;

    uint64_t result =
        virtio_blk_read_sync(driver, buffer, inode_fs_block_offset, 1024);

    struct ext2_dirent *de = (struct ext2_dirent *)buffer;

    size_t dirent_idx = 0;
    // TODO: Refactor to handle dynamic block sizes.
    while (dirent_idx < 1024) {

      memcpy(de, buffer + dirent_idx, sizeof(struct ext2_dirent));
      memcpy(de, buffer + dirent_idx, de->rec_len);

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

bool inode_from_dir(struct virtio_driver *driver,
                    struct ext2_superblock *superblock, char *path,
                    struct ext2_inode *dir, struct ext2_inode *inode) {
  for (size_t o = 0; o < EXT2_MAX_DIRECT_BLOCKS_PER_INODE; o++) {
    if (dir->i_block[o] == 0) {
      break;
    }

    // A dirent cannot cross a filesystem-block boundary. So, if we read a
    // block, we know that there are a certain number of full dirents. See below
    // for information about reclen.
    uint8_t buffer[1024];
    uint64_t inode_fs_block_offset = dir->i_block[o] * 1024;

    uint64_t result =
        virtio_blk_read_sync(driver, buffer, inode_fs_block_offset, 1024);

    struct ext2_dirent *de = (struct ext2_dirent *)buffer;

    size_t dirent_idx = 0;
    // TODO: Refactor to handle dynamic block sizes.
    while (dirent_idx < 1024) {

      // First, get the part of the directory entry that we _know_ has to be there.
      memcpy(de, buffer + dirent_idx, sizeof(struct ext2_dirent));
      // Then, use that information to determine what else needs to be copied.
      memcpy(de, buffer + dirent_idx, de->rec_len);

      char filename[255] = {
          0,
      };

#if DEBUG_LEVEL > DEBUG_TRACE
      eprint_str("Determining whether ");
      memcpy(filename, de->name, de->name_len);
      eprint_str(filename);
      eprint_str(" is ");
      eprint_str(path);
      eprint_str("?\n");
#endif

      if (!strncmp(de->name, path, de->name_len)) {
        return inode_from_ino(driver, superblock, inode, de->inode);
      }
      dirent_idx += de->rec_len;
    }
  }
  return false;
}

bool inode_from_path(struct virtio_driver *driver,
                     struct ext2_superblock *superblock, char *path,
                     struct ext2_inode *inode) {
  char *strtok_ctx = NULL;

#if DEBUG_LEVEL > DEBUG_TRACE
  {
    char msg[] = "Looking for the inode for the path ";
    eprint_str(msg);
    eprint_str(path);
    eprint('\n');
  }
#endif

  // As the path is traversed, inode has the contents of the CWD.
  if (!inode_from_ino(driver, superblock, inode, 2)) {
    return false;
  }
  for (strtok_ctx = path; /* Odd for loop. */; strtok_ctx = NULL) {

    char *next = strtok(strtok_ctx, "/");

    if (next == NULL) {
      break;
    }

#if DEBUG_LEVEL > DEBUG_TRACE
    {
      char msg[] = "We found part of the path: ";
      eprint_str(msg);
      eprint_str(next);
      eprint('\n');
    }
#endif

    struct ext2_inode current_directory;
    memcpy(&current_directory, inode, sizeof(struct ext2_inode));

    // If we cannot find the current part of the path, then too bad.
    if (!inode_from_dir(driver, superblock, next, &current_directory, inode)) {
      return false;
    }
  }

  return true;
}

bool read_superblock(struct virtio_driver *driver,
                     struct ext2_superblock *superblock) {

  // We assume that the driver is valid.

  size_t result = virtio_blk_read_sync(driver, (uint8_t *)superblock, 1024,
                                       sizeof(struct ext2_superblock));
  if (result != sizeof(struct ext2_superblock)) {
    return false;
  }

  eprint_buffer("Contents of superblock", (uint8_t *)superblock,
                sizeof(struct ext2_superblock));

#if 1
  if (superblock->s_blocks_count == 0x400) {
    char here[] = "Read the proper number of blocks!\n";
    eprint_str(here);
  }

  if (superblock->s_inodes_count == 0x80) {
    char here[] = "Read the proper number of inodes!\n";
    eprint_str(here);
  }

  if (superblock->s_first_inode == 11) {
    char here[] = "Read the proper first inode!\n";
    eprint_str(here);
  }
#endif

  uint64_t actual_block_size = 1024 << superblock->s_log_block_size;
  // We only handle block sizes of 1024 now.
  if (actual_block_size != 1024) {
    char here[] = "Error: Unsupported block size for filesystem!\n";
    eprint_str(here);
    return false;
  }

  if (superblock->s_inode_size != 256) {
    char here[] = "Error: Unsupported inode size for filesystem!\n";
    eprint_str(here);
    return false;
  }

  if (superblock->s_rev_level != 1) {
    char here[] = "Error: Unsupported revision level for filesystem!\n";
    eprint_str(here);
  }

  return true;
}

#if ENABLE_TESTS
bool test_ext2_implementation(struct virtio_driver *driver,
                                  struct ext2_superblock *superblock) {

  char buffer[1024] = {
      0,
  };

  uint64_t actual_block_size = 1024 << superblock->s_log_block_size;
  uint64_t actual_block_group_sizes =
      actual_block_size * superblock->s_blocks_per_group;
  uint32_t block_group_count =
      superblock->s_blocks_count / superblock->s_blocks_per_group;

  // Read the first (primary copy of the) block group.
  uint8_t result = virtio_blk_read_sector_sync(
      driver, sector_from_pos(EXT2_FIRST_OFFSET_AFTER_SUPERBLOCK), buffer);
  if (result) {
    return (uint64_t)-1;
  }

  struct ext2_blockgroup *bg = (struct ext2_blockgroup *)buffer;

  if (bg->bg_block_bitmap == 0x6) {
    char here[] = "Read the proper block bitmap id!\n";
    eprint_str(here);
  } else {
    eprint_str("Failure to read the proper block bitmap id.\n");
    epoweroff();

  }

  if (bg->bg_inode_bitmap == 0x7) {
    char here[] = "Read the proper inode bitmap id!\n";
    eprint_str(here);
  } else {
    eprint_str("Failure to read the proper inode bitmap id.\n");
    epoweroff();

  }

  if (bg->bg_inode_table == 0x8) {
    char here[] = "Read the proper inode table id!\n";
    eprint_str(here);
  } else {
    eprint_str("Failure to read the proper inode table id.\n");
    epoweroff();
  }

  {
    char here[] = "Free blocks? ";
    eprint_str(here);
    eprint_num(bg->bg_free_blocks_count);
    eprint('\n');
  }

  {
    // In the first block, should be 117 (because 117 + 11 (reserved) == 128
    // (inodes per block!))
    char here[] = "Free inodes? ";
    eprint_str(here);
    eprint_num(bg->bg_free_inodes_count);
    eprint('\n');
  }

  // Copy the raw contents of the _second_ entry in the inode table into an
  // inode and log the contents.
  struct ext2_inode ino;
  inode_from_ino(driver, superblock, &ino, 2);
  debug_dirent(driver, superblock, &ino);

  inode_from_path(driver, superblock, "/blank", &ino);
  result = virtio_blk_read_sector_sync(
      driver, sector_from_pos(ino.i_block[0] * actual_block_size), buffer);
  eprint_buffer("Contents of block 0 of blank file", buffer, 512);
  if (!strncmp("testing", (char *)buffer, strlen("testing"))) {
    eprint_str("Success!\n");
  } else {
    eprint_str("Failure to read proper contents of /blank (as 'testing')");
    epoweroff();
  }

  inode_from_path(driver, superblock, "/eve", &ino);
  result = virtio_blk_read_sector_sync(
      driver, sector_from_pos(ino.i_block[0] * actual_block_size), buffer);
  eprint_buffer("Contents of block 0 of eve file", buffer, 512);
  if (!strncmp("adam", (char *)buffer, strlen("adam"))) {
    eprint_str("Success!\n");
  } else {
    eprint_str("Failure to read proper contents of /eve (as 'adam')");
    epoweroff();
  }

  return true;
}
#endif

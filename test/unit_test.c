#include "event.h"
#include "system.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>

uint64_t mount_hd() {
  register uint64_t syscall_no asm("a7") = 210;
  register uint64_t a0 asm("a0") = 0;
  register uint64_t a1 asm("a1") = 0;
  uint64_t result = 0;

  asm volatile("scall\n" : "+r"(a0) : "r"(syscall_no), "r"(a1) : "memory");

  result = a0;

  return result;
}

uint64_t run_internal_unit_tests() {
  register uint64_t syscall_no asm("a7") = 211;
  register uint64_t a0 asm("a0") = 0;
  register uint64_t a1 asm("a1") = 0;
  uint64_t result = 0;

  asm volatile("scall\n" : "+r"(a0) : "r"(syscall_no), "r"(a1) : "memory");

  result = a0;

  return result;
}

int main() {
  volatile uint64_t ctr = 0;
  struct event evt;

  evt.id = 0;

  printf("Preparing to run the internal unit tests.\n");

  mount_hd();

  if (run_internal_unit_tests()) {
    printf("There were errors when running the internal unit tests.\n");
    return 1;
  } else {
    printf("Internal unit tests ran successfully!\n");
  }

  printf("Preparing to run user-facing unit tests.\n");

  // Testing whether simple ext2 read works.
  printf("Attempting to read from /random4: ");
  char actual[8] = {
      0,
  };

  int fd = open("/eve", 0);
  if (fd < 0) {
    printf("Error opening /eve");
    return 1;
  }
  char expected[] = "dam\n";
  lseek(fd, 1, SEEK_SET);
  read(fd, actual, 7);
  if (strcmp(actual, expected)) {
    printf("Error reading from file (actual: %s vs expected: %s)\n", actual,
           expected);
    return 1;
  } else {
    printf("Success\n");
  }

  // Testing whether reading from a non-0 block group works.
  printf("Attempting to read from /dir9/random1: ");
  int fd2 = open("/dir9/random1", 0);
  if (fd2 < 0) {
    printf("Error opening /dir9/random1");
    return 1;
  } else {
    printf("... okay.\n");
  }
  char random1_expected[] = {0x4c, 0x5a, 0xbf, 0x19, 0x00, 0xa4, 0xd1};
  char random1_actual[8] = {
      0,
  };

#if 0
  bool random1_file_read_test_failure = false;
  for (size_t i = 0; i < 800; i++) {
    printf("Doing iteration %d\n", i);
    lseek(fd2, 0, SEEK_SET);
    read(fd2, random1_actual, 7);
    if (memcmp(random1_actual, random1_expected, 7)) {
      printf("Error reading from file (actual: ");
      for (size_t s = 0; s < 8; s++) {
        printf("0x%02x ", random1_actual[s]);
      }
      printf(" vs expected: ");
      for (size_t s = 0; s < 8; s++) {
        printf("0x%02x ", random1_expected[s]);
      }
      printf(")\n");
      random1_file_read_test_failure = true;
      break;
    }
  }
  if (!random1_file_read_test_failure) {
    printf("(random1_file_read) Success\n");
  }

  bool getrandom_test_failure = false;
  char random_buf[8] = {
      0,
  };
  for (size_t i = 0; i < 10000; i++) {
    ssize_t getrandom_result = getrandom(random_buf, 8, GRND_RANDOM);
    if (getrandom_result != 8) {
      printf("Expected to get 8 bytes of random data by only got %d\n",
             getrandom_result);
      break;
    }
  }
  if (!getrandom_test_failure) {
    printf("(getrandom) Success\n");
  }

#endif

  if (close(fd) < 0) {
    printf("There was an error closing %d\n", fd);
  } else {
    printf("(close) Success\n");
  };

  if (close(37) >= 0) {
    printf("There was no error closing an invalid file handle.\n");
  } else {
    printf("(closeinvalidhandle) Success\n");
  };

  /*
   * Setup two tests that stress the ext2 implementation and the Virtio Blk driver by
   * reading and writing random data across block and sector boundaries.
   */
  char *read_write_test_names[] = {
      "random5_across_block",
      "random5_across_sector",
      "random5_span_block",
  };

  size_t read_write_test_offsets[] = {
      0x1000 - 8, /* block boundary */
      0x1200 - 8, /* 512 sector boundary */
      0x0fff,
  };

  size_t read_write_test_lengths[] = {
      16,
      16,
      1036,
  };

  for (size_t test_index = 0; test_index < 3; test_index++) {
    bool random5_file_read_write_test_failure = false;
    char random5_file_read_write_test_data[1040] = {
        0,
    };
    char random5_file_read_write_test_readback_data[1040] = {
        0,
    };

    ssize_t getrandom_result =
        getrandom(random5_file_read_write_test_data, read_write_test_lengths[test_index], GRND_RANDOM);

    printf("(%s) Randomness to write: ", read_write_test_names[test_index]);
    for (size_t i = 0; i < read_write_test_lengths[test_index]; i++) {
      printf("%02x ", random5_file_read_write_test_data[i]);
    }
    printf(" to offset 0x%lx\n", read_write_test_offsets[test_index]);

    int random5_file_read_write_test_fd = open("/random5", 0);
    size_t actual_offset = lseek(random5_file_read_write_test_fd,
                                 read_write_test_offsets[test_index], SEEK_SET);

    if (actual_offset != read_write_test_offsets[test_index]) {
      printf("(%s) Error: Did not seek to the proper location (for write)... expected %lx "
             "and got %lx\n",
             read_write_test_names[test_index],
             read_write_test_offsets[test_index], actual_offset);
      random5_file_read_write_test_failure = true;
      break;
    }

    size_t actually_written = write(random5_file_read_write_test_fd,
                                    random5_file_read_write_test_data, read_write_test_lengths[test_index]);

    if (actually_written != read_write_test_lengths[test_index]) {
      printf("(%s) Error: Did not write the expected amount "
             "of bytes.\n",
             read_write_test_names[test_index]);
      random5_file_read_write_test_failure = true;
    }

    actual_offset = lseek(random5_file_read_write_test_fd, read_write_test_offsets[test_index],
          SEEK_SET);
    if (actual_offset != read_write_test_offsets[test_index]) {
      printf("(%s) Error: Did not seek to the proper location (for read) ... expected %lx "
             "and got %lx\n",
             read_write_test_names[test_index],
             read_write_test_offsets[test_index], actual_offset);
      random5_file_read_write_test_failure = true;
      break;
    }

    size_t actually_read = read(random5_file_read_write_test_fd,
                                random5_file_read_write_test_readback_data, read_write_test_lengths[test_index]);
    if (actually_read != read_write_test_lengths[test_index]) {
      printf("(%s) Error: Did not read the expected amount "
             "of bytes.\n",
             read_write_test_names[test_index]);
      random5_file_read_write_test_failure = true;
    }

    printf("(%s) Randomness read back: ", read_write_test_names[test_index]);
    for (size_t i = 0; i < read_write_test_lengths[test_index]; i++) {
      printf("%02x ", random5_file_read_write_test_readback_data[i]);
      if (random5_file_read_write_test_data[i] !=
          random5_file_read_write_test_readback_data[i]) {
        printf("(%s) Error: Readback mismatch at byte %lu",
               read_write_test_names[test_index], i);
        break;
        random5_file_read_write_test_failure = true;
      }
    }
    printf(" from offset 0x%lx\n", read_write_test_offsets[test_index]);

    if (!random5_file_read_write_test_failure) {
      printf("(%s) Success\n", read_write_test_names[test_index]);
    }

    close(random5_file_read_write_test_fd);
  }

  printf("End of user-facing unit tests.\n");
  return 0;
}

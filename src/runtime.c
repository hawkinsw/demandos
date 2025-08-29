#include "runtime.h"
#include "demandos.h"
#include "event.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/random.h>
#include <unistd.h>

int main() {
  volatile uint64_t ctr = 0;
  struct event evt;

  evt.id = 0;

  char buffer[8] = {
      0,
  };

  printf("Starting up the runtime\n");

  mount_hd();

  for (;;) {
    uint64_t wait_result = wait_for_event(5 * 1e7, &evt);
    if (wait_result == WAIT_TIMEOUT) {
      printf("There was a timeout!\n");
    } else if (wait_result == WAIT_EVENT_OCCURRED) {
      printf("An event occurred!\n");
      printf("Wait result? %llu (event id: %llu)\n", wait_result, evt.id);
    }
  }
}

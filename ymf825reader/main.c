#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "ymf825.h"
#include "delay.h"

#define READ_UNIT_SIZE 65536

uint8_t* read_all(int64_t* read_size) {
  uint8_t* buffer;
  size_t   buffer_size = READ_UNIT_SIZE;
  size_t   size;

  buffer = (uint8_t*)malloc(sizeof(uint8_t) * buffer_size);
  *read_size = 0;

  do {
    size = fread(buffer + *read_size, sizeof(uint8_t), READ_UNIT_SIZE, stdin);
    *read_size += size;
    if (*read_size >= buffer_size) {
      buffer_size *= 2;

      if ((buffer = (uint8_t*)realloc(buffer, buffer_size)) == NULL) {
        printf("can't allocate memory\n");
        exit(1);
      }
    }
  } while (size == READ_UNIT_SIZE);

  if (*read_size != buffer_size &&
      (buffer = (uint8_t*)realloc(buffer, *read_size)) == NULL) {
    printf("can't allocate memory\n");
    exit(1);
  }

  return buffer;
}

void sigint_handler(int signame) {
  ymf825_stop();
}

void sigusr1_handler(int signame) {
  ymf825_pause();
}

int main(int argc, const char** argv) {
  uint8_t* buffer;
  int64_t  file_size;
  uint16_t resolution;

  if (argc < 1) {
    printf("a resolution is not specified\n");
    return 1;
  }

  signal(SIGINT, sigint_handler);
  signal(SIGUSR1, sigusr1_handler);
  buffer = read_all(&file_size);
  resolution = atoi(argv[1]);

  if (resolution < 1 || resolution > 65536) {
    printf("resolution '%s' is invalid\n", argv[1]);
    exit(1);
  }

  ymf825_open();
  ymf825_play(buffer, file_size, resolution);
  ymf825_close();

  free(buffer);

  return 0;
}

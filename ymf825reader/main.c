#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

#include "ftd2xx.h"
#include "common.h"
#include "ymf825.h"
#include "delay.h"

Ymf825 ymf825;

uint8_t* read_all(const char* filepath, int64_t* file_size) {
  FILE *fp;
  uint8_t* buffer;
  struct stat st;

  if ((fp = fopen(filepath, "rb")) == NULL) {
    printf("invalid file path '%s'\n", filepath);
    exit(1);
  }

  stat(filepath, &st);
  *file_size = st.st_size;

  buffer = (uint8_t*)malloc(sizeof(uint8_t) * *file_size);
  fread(buffer, sizeof(uint8_t), *file_size, fp);
  fclose(fp);

  return buffer;
}

void sigint_handler(int signame) {
  ymf825_stop(&ymf825);
}

int main(int argc, const char** argv) {
  uint8_t* buffer;
  int64_t  file_size;
  uint16_t resolution;

  if (argc < 1) {
    printf("input file is not specified\n");
    return 1;
  }

  signal(SIGINT, sigint_handler);
  buffer = read_all(argv[1], &file_size);
  resolution = ymf825_check_header(buffer);

  printf("file size: %lld\n", file_size);
  printf("resolution: %d\n", resolution + 1);

  ymf825_create(&ymf825, DEFAULT_DEVICE, YMF825_CS_PIN, resolution);
  ymf825_play(&ymf825, buffer, file_size);
  ymf825_close(&ymf825);

  free(buffer);

  return 0;
}

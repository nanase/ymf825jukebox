#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include "ftd2xx.h"
#include "common.h"
#include "ymf825.h"

uint16_t read_uint16_t(const uint8_t* buffer) {
  uint16_t value;
  uint8_t* p = (uint8_t*)&value;
  p[0] = buffer[0];
  p[1] = buffer[1];
  return value;
}

int check_header(const uint8_t* header) {
  if (memcmp("YMF825", header, 6) != 0) {
    printf("the file is not YMF825 dump file\n");
    exit(1);
  }

  if (header[6] != 0 || header[7] != 0) {
    printf("invalid YMF825 dump file\n");
    exit(1);
  }

  return read_uint16_t(header + 8);
}

uint8_t* read_all(const char* filepath, int* file_size) {
  FILE *fp;
  uint8_t* buffer;
  struct stat st;

  if ((fp = fopen(filepath, "rb"))) == NULL) {
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

void play(Ymf825* ymf825, const uint8_t* file, int file_size) {
  uint8_t  command, address;
  size_t   length;
  uint16_t wait_time;

  int      index;
  uint16_t resolution = check_header(file);
  double   tick_time = 1.0 / (resolution + 1);

  printf("file size: %d\n", file_size);
  printf("resolution: %d\n", resolution + 1);

  for (index = 0x10; index < file_size;) {
    command = file[index++];

    switch (command) {
      case 0x00:
        //printf("[N]\n");
        break;

      case 0x01:
        //printf("[W] address: %2x, data: %2x\n", file[index + 0], file[index + 1]);
        ymf825_write(ymf825, file[index + 0], file[index + 1]);
        index += 2;
        break;

      case 0x02:
        address = file[index++];
        length = (size_t)read_uint16_t(file + index);
        index += 2;
        //printf("[B] address: %2x, length: %d\n", address, length);
        ymf825_burst_write(ymf825, address, file + index, length);
        index += length;
        break;

      case 0x80:
        //printf("[C] chip: %d\n", file[index]);
        ymf825_change_target_chip(ymf825, file[index++]);
        break;

      case 0xf0:
        //printf("[R]\n");
        ymf825_reset_hardware(ymf825);
        break;

      case 0xf8:
        //printf("[F]\n");
        ymf825_flush(ymf825);
        break;

      case 0xfd:
      case 0xfe:
        wait_time = read_uint16_t(file + index);
        index += 2;
        //printf("[W] wait: %d ms\n", (int)round((tick_time * wait_time) * 1000.0));
        msleep((int)round((tick_time * wait_time) * 1000.0));
        break;

      case 0xff:
        //printf("[O]\n");
        msleep((int)round(tick_time * 1000.0));
        break;

      default:
        printf("invalid command %2x\n", command);
        exit(1);
        break;
    }
  }
}

int main(int argc, const char** argv) {
  uint8_t* buffer;
  int      file_size;
  Ymf825   ymf825;

  if (argc < 1) {
    printf("input file is not specified\n");
    return 1;
  }

  buffer = read_all(argv[1], &file_size);
  ymf825_create(&ymf825, DEFAULT_DEVICE, YMF825_CS_PIN);
  play(&ymf825, buffer, file_size);
  ymf825_close(&ymf825);

  free(buffer);
  return 0;
}

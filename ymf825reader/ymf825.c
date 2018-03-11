#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ftd2xx.h"
#include "ymf825.h"

static uint16_t read_uint16_t(const uint8_t* buffer) {
  return *((uint16_t*)buffer);
}

// ----------------------------------------------------------------

void ymf825spi_reset_hardware() {
  spi_set_ic(false);
  msleep(2);

  spi_set_ic(true);
  msleep(2);

  spi_set_ic(false);
}

// ----------------------------------------------------------------

volatile bool request_stop = false;

void ymf825_create() {
  spi_create();
}

void ymf825_write(uint8_t pin, uint8_t address, uint8_t data) {
  if (address >= 0x80) {
    printf("invalid address %d\n", address);
    exit(1);
  }

  spi_write(pin, address, data);
}

void ymf825_write_multiple(uint8_t pin, const uint8_t* address_data, size_t length) {
  spi_write_multiple(pin, address_data, length);
}

void ymf825_burst_write(uint8_t pin, uint8_t address, const uint8_t* data, size_t length) {
  if (address >= 0x80) {
    printf("invalid address %d\n", address);
    exit(1);
  }

  spi_burst_write(pin, address, data, length);
}

void ymf825_close() {
  spi_destroy();
}

void ymf825_reset_hardware() {
  ymf825spi_reset_hardware();
}

uint16_t ymf825_check_header(const uint8_t* header) {
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

void ymf825_play(const uint8_t* file, int64_t file_size, uint16_t resolution) {
  Delay    delay;
  uint8_t  command, address;
  size_t   length;
  uint16_t wait_tick;
  int64_t  index = 0x10;
  uint8_t  pin = 0x03;

  delay_create(&delay, resolution);

  while (!request_stop && index < file_size) {
    command = file[index++];

    switch (command) {
      // Noop
      case 0x00:
        break;

      // Write
      case 0x10:
        //printf("Write\n");
        ymf825_write(pin, file[index + 0], file[index + 1]);
        index += 2;
        break;

      // Write short
      // Write and flush short
      case 0x12:
      case 0x14:
        //printf("Write and flush short\n");
        length = file[index++] * 2;
        ymf825_write_multiple(pin, file + index, length);
        index += length;
        break;

      // Write long
      // Write and flush long
      case 0x13:
      case 0x15:
        //printf("Write and flush long\n");
        length = (size_t)read_uint16_t(file + index) * 2;
        index += 2;
        ymf825_write_multiple(pin, file + index, length);
        index += length;
        break;

      // BurstWrite short
      case 0x20:
        //printf("BurstWrite short\n");
        address = file[index++];
        length = file[index++];
        ymf825_burst_write(pin, address, file + index, length);
        index += length;
        break;

      // BurstWrite long
      case 0x21:
        //printf("BurstWrite long\n");
        address = file[index++];
        length = (size_t)read_uint16_t(file + index);
        index += 2;
        ymf825_burst_write(pin, address, file + index, length);
        index += length;
        break;

      // Flush
      case 0x80:
        //printf("Flush\n");
        // do nothing
        break;

      // Change Target
      case 0x90:
        //printf("Change Target\n");
        pin = file[index++];
        break;

      // Reset Hardware
      case 0xe0:
        //printf("Reset Hardware\n");
        ymf825_reset_hardware();
        break;

      // Realtime Wait short
      // Wait short
      case 0xfc:
      case 0xfe:
        //printf("Wait short\n");
        wait_tick = file[index++] + 1;
        delay_sleep(&delay, wait_tick);
        break;

      // Realtime Wait long
      // Wait long
      case 0xfd:
      case 0xff:
        //printf("Wait long\n");
        wait_tick = read_uint16_t(file + index) + 1;
        index += 2;
        delay_sleep(&delay, wait_tick);
        break;

      default:
        printf("invalid command 0x%2x (in index 0x%2llx)\n", command, index - 1);
        exit(1);
        break;
    }
  }

  if (request_stop)
    ymf825_reset_hardware();
}

void ymf825_stop() {
  request_stop = true;
}

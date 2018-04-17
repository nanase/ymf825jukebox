#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ymf825.h"

static uint16_t read_uint16_t(const uint8_t* buffer) {
  return *((uint16_t*)buffer);
}

// ----------------------------------------------------------------

void ymf825spi_reset_hardware() {
  static struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 2000000;

  spi_set_ic(false);
  nanosleep(&ts, NULL);

  spi_set_ic(true);
  nanosleep(&ts, NULL);

  spi_set_ic(false);
}

// ----------------------------------------------------------------

volatile bool request_stop = false;
volatile bool request_pause = false;

void ymf825_open() {
  spi_open();
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
  spi_close();
}

void ymf825_reset_hardware() {
  ymf825spi_reset_hardware();
}

void ymf825_play(const uint8_t* file, int64_t file_size, uint16_t resolution) {
  Delay    delay;
  uint8_t  command, address;
  size_t   length;
  uint16_t wait_tick;
  int64_t  index = 0x00;
  uint8_t  pin = 0x03;
  bool     paused = false;
  uint8_t  i;

  delay_initialize(&delay, resolution, &pwm_decrement);

  while (!request_stop && index < file_size) {
    if (request_pause) {
      if (!paused) {
        // send note off
        for (i = 0; i < 16; i++) {
          ymf825_write(0x03, 0x0b, i);
          ymf825_write(0x03, 0x0f, i);
        }

        paused = true;
      }

      delay_sleep(&delay, 1);
      continue;
    }

    paused = false;
    command = file[index++];

    switch (command) {
      // Noop
      case 0x00:
        break;

      // Write
      case 0x10:
        ymf825_write(pin, file[index + 0], file[index + 1]);
        index += 2;
        break;

      // Write short
      // Write and flush short
      case 0x12:
      case 0x14:
        length = file[index++] * 2;
        ymf825_write_multiple(pin, file + index, length);
        index += length;
        break;

      // Write long
      // Write and flush long
      case 0x13:
      case 0x15:
        length = (size_t)read_uint16_t(file + index) * 2;
        index += 2;
        ymf825_write_multiple(pin, file + index, length);
        index += length;
        break;

      // BurstWrite short
      case 0x20:
        address = file[index++];
        length = file[index++];
        ymf825_burst_write(pin, address, file + index, length);
        index += length;
        break;

      // BurstWrite long
      case 0x21:
        address = file[index++];
        length = (size_t)read_uint16_t(file + index);
        index += 2;
        ymf825_burst_write(pin, address, file + index, length);
        index += length;
        break;

      // Flush
      case 0x80:
        // do nothing
        break;

      // Change Target
      case 0x90:
        pin = file[index++];
        break;

      // Reset Hardware
      case 0xe0:
        ymf825_reset_hardware();
        break;

      // Realtime Wait short
      // Wait short
      case 0xfc:
      case 0xfe:
        wait_tick = file[index++] + 1;
        delay_sleep(&delay, wait_tick);
        break;

      // Realtime Wait long
      // Wait long
      case 0xfd:
      case 0xff:
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

void ymf825_pause() {
  request_pause = !request_pause;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ftd2xx.h"
#include "ymf825.h"

static uint16_t read_uint16_t(const uint8_t* buffer) {
  return *((uint16_t*)buffer);
}

// ----------------------------------------------------------------

void ymf825spi_reset_hardware(Spi* spi) {
  static const uint8_t enable_buffer[3]  = { 0x82, 0xff, 0xff };
  static const uint8_t disable_buffer[3] = { 0x82, 0x00, 0xff };

  spi_queue_buffer(spi, enable_buffer, sizeof(enable_buffer));
  spi_queue_flush_command(spi);
  spi_send_buffer(spi);
  msleep(2);

  spi_queue_buffer(spi, disable_buffer, sizeof(disable_buffer));
  spi_queue_flush_command(spi);
  spi_send_buffer(spi);
  msleep(2);

  spi_queue_buffer(spi, enable_buffer, sizeof(enable_buffer));
  spi_queue_flush_command(spi);
  spi_send_buffer(spi);
}

// ----------------------------------------------------------------

volatile bool request_stop = false;

void ymf825_create(Ymf825* ymf825, int device_num, uint8_t pin, uint16_t resolution) {
  FT_HANDLE ftHandle;

  check_status(FT_Open(device_num, &ftHandle));
  printf("device %d opened\n", device_num);
  spi_create(&ymf825->spi, ftHandle, false, pin);
  delay_create(&ymf825->delay, resolution);
  spi_set_cs_target_pin(&ymf825->spi, pin);
}

void ymf825_flush(Ymf825* ymf825) {
  spi_flush(&ymf825->spi);
}

void ymf825_write(Ymf825* ymf825, uint8_t address, uint8_t data) {
  if (address >= 0x80) {
    printf("invalid address %d\n", address);
    exit(1);
  }

  spi_write(&ymf825->spi, address, data);
}

void ymf825_write_multiple(Ymf825* ymf825, const uint8_t* address_data, size_t length, bool flush) {
  spi_write_multiple(&ymf825->spi, address_data, length, flush);
}

void ymf825_burst_write(Ymf825* ymf825, uint8_t address, const uint8_t* data, size_t length) {
  if (address >= 0x80) {
    printf("invalid address %d\n", address);
    exit(1);
  }

  spi_burst_write(&ymf825->spi, address, data, length);
}

void ymf825_change_target_chip(Ymf825* ymf825, uint8_t target) {
  spi_set_cs_target_pin(&ymf825->spi, target << 3);
}

void ymf825_close(Ymf825* ymf825) {
  FT_Close(ymf825->spi.handle);
  ymf825->spi.handle = NULL;
}

void ymf825_reset_hardware(Ymf825* ymf825) {
  ymf825spi_reset_hardware(&ymf825->spi);
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

void ymf825_play(Ymf825* ymf825, const uint8_t* file, int64_t file_size) {
  uint8_t  command, address;
  size_t   length;
  uint16_t wait_tick;
  int64_t  index = 0x10;

  while (!request_stop && index < file_size) {
    command = file[index++];

    switch (command) {
      // Noop
      case 0x00:
        break;

      // Write
      case 0x10:
        ymf825_write(ymf825, file[index + 0], file[index + 1]);
        index += 2;
        break;

      // Write short
      case 0x12:
        length = file[index++] * 2;
        ymf825_write_multiple(ymf825, file + index, length, false);
        index += length;
        break;
      
      // Write long
      case 0x13:
        length = (size_t)read_uint16_t(file + index) * 2;
        index += 2;
        ymf825_write_multiple(ymf825, file + index, length, false);
        index += length;
        break;

      // Write and flush short
      case 0x14:
        length = file[index++] * 2;
        ymf825_write_multiple(ymf825, file + index, length, true);
        index += length;
        break;

      // Write and flush long
      case 0x15:
        length = (size_t)read_uint16_t(file + index) * 2;
        index += 2;
        ymf825_write_multiple(ymf825, file + index, length, true);
        index += length;
        break;

      // BurstWrite short
      case 0x20:
        address = file[index++];
        length = file[index++];
        ymf825_burst_write(ymf825, address, file + index, length);
        index += length;
        break;

      // BurstWrite long
      case 0x21:
        address = file[index++];
        length = (size_t)read_uint16_t(file + index);
        index += 2;
        ymf825_burst_write(ymf825, address, file + index, length);
        index += length;
        break;

      // Flush
      case 0x80:
        ymf825_flush(ymf825);
        break;

      // Change Target
      case 0x90:
        ymf825_change_target_chip(ymf825, file[index++]);
        break;

      // Reset Hardware
      case 0xe0:
        printf("Reset Hardware\n");
        ymf825_reset_hardware(ymf825);
        break;

      // Realtime Wait short
      // Wait short
      case 0xfc:
      case 0xfe:
        wait_tick = file[index++] + 1;
        delay_sleep(&ymf825->delay, wait_tick);
        break;

      // Realtime Wait long
      // Wait long
      case 0xfd:
      case 0xff:
        wait_tick = read_uint16_t(file + index) + 1;
        index += 2;
        delay_sleep(&ymf825->delay, wait_tick);
        break;

      default:
        printf("invalid command %2x\n", command);
        exit(1);
        break;
    }
  }

  if (request_stop)
    ymf825_reset_hardware(ymf825);
}

void ymf825_stop() {
  request_stop = true;
}

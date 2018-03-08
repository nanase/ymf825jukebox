#include <stdio.h>
#include <stdlib.h>
#include "ftd2xx.h"
#include "ymf825.h"

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

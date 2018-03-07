#include <stdio.h>
#include <stdlib.h>
#include "spi.h"

void check_status(FT_STATUS status) {
  if (status == FT_OK)
    return;

  printf("error %d\n", (int)status);
  exit(1);
}

void spi_create(Spi* spi, FT_HANDLE handle, bool cs_enable_level_high, uint8_t cs_pin) {
  spi->handle = handle;
  spi->cs_enable_level_high = cs_enable_level_high;
  spi->cs_pin = cs_pin;
  spi->cs_target_pin = 0;
  spi->write_buffer_index = 0;
  spi_initialize(spi);
}

void spi_set_cs_target_pin(Spi* spi, uint8_t pin) {
  if ((pin & 0x07) != 0) {
    printf("invalid cs target pin %d\n", pin);
    exit(1);
  }

  spi->cs_target_pin = pin;
}

void spi_flush(Spi* spi) {
  spi_queue_flush_command(spi);
  spi_send_buffer(spi);
}

void spi_write(Spi* spi, uint8_t address, uint8_t data) {
  uint8_t buffer[5] = { 0x11, 0x01, 0x00, address, data };

  if (spi->cs_target_pin == 0) {
    printf("not specified cs target pin\n");
    exit(1);
  }

  spi_queue_buffer_cs_enable(spi);
  spi_queue_buffer(spi, buffer, sizeof(buffer));
  spi_queue_buffer_cs_disable(spi);
}

void spi_burst_write(Spi* spi, uint8_t address, const uint8_t* data, size_t length) {
  uint8_t buffer[4] = { 0x11, length & 0xff, length >> 8, address };

  if (spi->cs_target_pin == 0) {
    printf("not specified cs target pin\n");
    exit(1);
  }

  spi_queue_buffer_cs_enable(spi);
  spi_queue_buffer(spi, buffer, sizeof(buffer));
  spi_queue_buffer(spi, data, length);
  spi_queue_buffer_cs_disable(spi);
}

void spi_send_buffer(Spi* spi) {
  uint32_t bytesWritten;

  if (spi->write_buffer_index == 0)
    return;

  check_status(FT_Write(spi->handle, spi->write_buffer, spi->write_buffer_index, &bytesWritten));
  spi->write_buffer_index = 0;
}

void spi_queue_buffer(Spi* spi, const uint8_t* buffer, size_t length) {
  int i;
  for (i = 0; i < length; i++)
    spi->write_buffer[spi->write_buffer_index++] = buffer[i];
}

void spi_queue_buffer_single(Spi* spi, uint8_t data) {
  spi->write_buffer[spi->write_buffer_index++] = data;
}

void spi_queue_flush_command(Spi* spi) {
  spi_queue_buffer_single(spi, 0x87);
}

void spi_queue_buffer_cs_enable(Spi* spi) {
  uint8_t buffer[3] = { 0x80, 0x00, 0xfb };
  buffer[1] = spi->cs_enable_level_high ?
      spi->cs_pin & spi->cs_target_pin :
      spi->cs_pin ^ spi->cs_target_pin;
  spi_queue_buffer(spi, buffer, sizeof(buffer));
}

void spi_queue_buffer_cs_disable(Spi* spi) {
  uint8_t buffer[3] = { 0x80, 0x00, 0xfb };
  buffer[1] = spi->cs_enable_level_high ?
      0x00 :
      spi->cs_pin;
  spi_queue_buffer(spi, buffer, sizeof(buffer));
}

void spi_initialize(Spi* spi) {
  static const uint8_t buffer[] = {
    0x86, 0x02, 0x00,  // SCK is 10MHz
    0x80, 0xf8, 0xfb,  // ADBUS: v - 1111 1000, d - 1111 1011 (0: in, 1: out)
    0x82, 0xff, 0xff,  // ACBUS: v - 1111 1111, d - 1111 1111 (0: in, 1: out)
    0x8a,
    0x85,
    0x8d
  };

  check_status(FT_Purge(spi->handle, FT_PURGE_RX | FT_PURGE_TX));
  msleep(10);

  check_status(FT_SetTimeouts(spi->handle, 1, 10));
  check_status(FT_SetLatencyTimer(spi->handle, 1));

  check_status(FT_SetBitMode(spi->handle, 0x00, 0));
  msleep(10);

  check_status(FT_SetBitMode(spi->handle, 0x00, 2));
  check_status(FT_Purge(spi->handle, FT_PURGE_RX));
  msleep(20);

  spi_queue_buffer(spi, buffer, sizeof(buffer));
  spi_send_buffer(spi);
  msleep(100);
}

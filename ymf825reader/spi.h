#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "ftd2xx.h"
#include "common.h"

#define READ_BUFFER_SIZE  16
#define WRITE_BUFFER_SIZE 1024

typedef struct {
  FT_HANDLE handle;
  bool      cs_enable_level_high;
  uint8_t   cs_pin;
  uint8_t   cs_target_pin;
  uint8_t   write_buffer[WRITE_BUFFER_SIZE];
  int32_t   write_buffer_index;
} Spi;

void check_status(FT_STATUS);
void spi_create(Spi*, FT_HANDLE, bool, uint8_t);
void spi_set_cs_target_pin(Spi*, uint8_t);
void spi_flush(Spi*);
void spi_write(Spi*, uint8_t, uint8_t);
void spi_burst_write(Spi*, uint8_t, const uint8_t*, size_t);
void spi_write_multiple(Spi*, const uint8_t*, size_t, bool);
void spi_send_buffer(Spi*);
void spi_queue_buffer(Spi*, const uint8_t*, size_t);
void spi_queue_buffer_single(Spi*, uint8_t);
void spi_queue_flush_command(Spi*);
void spi_queue_buffer_cs_enable(Spi*);
void spi_queue_buffer_cs_disable(Spi*);
void spi_initialize(Spi*);

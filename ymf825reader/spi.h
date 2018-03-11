#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <bcm2835.h>
#include "common.h"

#define READ_BUFFER_SIZE  16
#define WRITE_BUFFER_SIZE 1024

void spi_create();
void spi_write(uint8_t, uint8_t, uint8_t);
void spi_write_multiple(uint8_t, const uint8_t*, size_t);
void spi_burst_write(uint8_t, uint8_t, const uint8_t*, size_t);
void spi_set_ic(bool);
void spi_destroy();

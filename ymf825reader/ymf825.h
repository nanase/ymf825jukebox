#pragma once
#include <stdint.h>
#include "spi.h"
#include "delay.h"

void ymf825_open();
void ymf825_write(uint8_t, uint8_t, uint8_t);
void ymf825_write_multiple(uint8_t, const uint8_t*, size_t);
void ymf825_burst_write(uint8_t, uint8_t, const uint8_t*, size_t);
void ymf825_close();
void ymf825_play(const uint8_t*, int64_t, uint16_t);
void ymf825_stop();

void ymf825_reset_hardware();

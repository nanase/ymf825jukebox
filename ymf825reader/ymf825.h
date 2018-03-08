#pragma once
#include <stdint.h>
#include "spi.h"
#include "delay.h"

#define DEFAULT_DEVICE    0
#define YMF825_CS_PIN     0x18

typedef struct {
  Spi   spi;
  Delay delay;
} Ymf825;

void ymf825_create(Ymf825*, int, uint8_t, uint16_t);
void ymf825_flush(Ymf825*);
void ymf825_write(Ymf825*, uint8_t, uint8_t);
void ymf825_burst_write(Ymf825*, uint8_t, const uint8_t*, size_t);
void ymf825_change_target_chip(Ymf825*, uint8_t);
void ymf825_close(Ymf825*);
void ymf825_reset_hardware(Ymf825*);

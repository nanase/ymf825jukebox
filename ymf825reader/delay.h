#pragma once
#include <stdint.h>

// nanoseconds
#define SLEEP_PRECISION 100000

typedef struct {
  uint64_t diff_prev;
  uint64_t current_time;
  uint64_t tick_unit;
} Delay;

void delay_create(Delay*, uint16_t);
void delay_sleep(Delay*, uint16_t);

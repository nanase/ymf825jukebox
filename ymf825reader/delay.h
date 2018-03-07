#pragma once
#include <stdint.h>

#define SLEEP_PRECISION 0.0001

typedef struct {
  double diff_prev;
  double current_time;
  //int    count;
} Delay;

void delay_create(Delay*);
void delay_sleep(Delay*, double);

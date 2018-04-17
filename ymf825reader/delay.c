#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "delay.h"

static struct timespec tp;
static void (*sleep_action)();

static uint64_t get_time() {
  static struct timespec tp_get_time;
  clock_gettime(CLOCK_MONOTONIC, &tp_get_time);
  return tp_get_time.tv_sec * 1e9 + tp_get_time.tv_nsec;
}

void delay_initialize(Delay* delay, uint16_t resolution, void (*action)()) {
  delay->diff_prev = 0;
  delay->current_time = get_time();
  delay->tick_unit = (uint64_t)round(1.0e9 / resolution);

  tp.tv_sec = 0;
  tp.tv_nsec = SLEEP_PRECISION;
  sleep_action = action;
}

void delay_sleep(Delay* delay, uint32_t wait_tick) {
  uint64_t target_time = delay->current_time + wait_tick * delay->tick_unit;
  uint64_t sleep_target_time = target_time - delay->diff_prev - SLEEP_PRECISION;

  while (sleep_target_time > get_time()) {
    nanosleep(&tp, NULL);
    sleep_action();
  }

  delay->diff_prev = target_time - get_time();
  delay->current_time = target_time;
}

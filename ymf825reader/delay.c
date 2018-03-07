#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "delay.h"

static uint64_t get_time() {
  static struct timespec tp_get_time;
  //clock_gettime(CLOCK_MONOTONIC, &tp_get_time);
  clock_gettime(CLOCK_REALTIME, &tp_get_time);
  return tp_get_time.tv_sec * 1e9 + tp_get_time.tv_nsec;
}

void delay_create(Delay* delay) {
  delay->diff_prev = 0.0;
  delay->current_time = get_time() * 1.0e-9;
  delay->real_time = delay->current_time;
}

void delay_sleep(Delay* delay, double seconds) {
  static struct timespec tp;
  //tp.tv_sec = 0;
  //tp.tv_nsec = (long)(seconds * 1000000000);
  //nanosleep(&tp, NULL);
  
  double target_time = delay->current_time + seconds;
  uint64_t sleep_target_time = (uint64_t)((target_time - delay->diff_prev - SLEEP_PRECISION) * 1e9);

  tp.tv_sec = 0;
  tp.tv_nsec = SLEEP_PRECISION * 1000000000;

  while (sleep_target_time > get_time())
    nanosleep(&tp, NULL);

  delay->diff_prev = target_time - get_time() * 1.0e-9;
  delay->current_time = target_time;
}


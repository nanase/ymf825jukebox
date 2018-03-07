#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "delay.h"

double get_time() {
  static struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec + tp.tv_nsec * 1.0e-9;
}

void delay_create(Delay* delay) {
  delay->diff_prev = 0.0;
  delay->current_time = get_time();
  //delay->count = 0;
}

void delay_sleep(Delay* delay, double seconds) {
  static struct timespec tp;
  double target_time = delay->current_time + seconds - delay->diff_prev;
  tp.tv_sec = 0;
  tp.tv_nsec = SLEEP_PRECISION * 1000000000;

  while (target_time - SLEEP_PRECISION > get_time())
    nanosleep(&tp, NULL);

  delay->diff_prev = target_time - get_time();
  delay->current_time = target_time;
}


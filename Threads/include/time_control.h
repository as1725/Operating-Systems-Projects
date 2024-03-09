#ifndef TIME_H
#define TIME_H

#include <sys/time.h>
#include <stddef.h>
#include <stdio.h>
#include <signal.h>

#define TIME_SLICE 100000; // in microseconds


int get_time();
void timer_start();
void timer_stop();

#endif // TIME_H
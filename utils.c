#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "utils.h"

void ws_logger(const char * fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

char * ws_timestamp(void) {
  time_t cal_time;
  cal_time = time(NULL);
  char * timestamp = asctime(localtime(&cal_time));

  timestamp[strlen(timestamp) - 1] = '\0';

  return timestamp;
}
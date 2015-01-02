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

int ws_urlncmp(const char * first, const char * second, size_t length) {

  size_t i = 0;
  for (; i < length; ++i) {
    if(first[i] != second[i]) {
      /* Check for start of http params */
      if (second[i] == '?') {
        /* Break here because now length is irrelevant. Match found. */
        break;
      }
      /* Not a match */
      return -1;
    }
  }

  return 0;
}
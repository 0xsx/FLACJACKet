/*
Copyright (C) 2018. See AUTHORS.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "flacjacket_globals.h"
#include "logging.h"



void debug_log(const char *msg, ...) {
  #ifdef DEBUG_BUILD
    char buf[255];
    va_list args;
    va_start(args, msg);
    vsprintf(buf, msg, args);
    va_end(args);
    printf("DEBUG: %s\n", buf);
  #endif
}



void info_log(const char *msg, ...) {
  char buf[255];
  va_list args;
  va_start(args, msg);
  vsprintf(buf, msg, args);
  va_end(args);
  printf("%s\n", buf);
}



void error_log(const char *msg, ...) {
  char buf[255];
  va_list args;
  va_start(args, msg);
  vsprintf(buf, msg, args);
  va_end(args);
  fprintf(stderr, "ERROR: %s\n", buf);
}


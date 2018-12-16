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

#ifndef FLACJACKET_GLOBALS_H
#define FLACJACKET_GLOBALS_H

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
#include <unistd.h>

#include <jack/jack.h>


#include "flacjacket-config.h"



struct shared_vars_t {
  pthread_t http_thread_id;
  pthread_t sddp_thread_id;

  pthread_mutex_t encoder_lock;


  int32_t *processor_buffer;     /* Holds raw data until encoder buffer is ready. */
  size_t processor_buffer_len;
  
  int32_t *encoder_buffer;      /* Holds raw data for the FLAC encoder. */
  size_t encoder_buffer_len;
  uint64_t encoder_buffer_start_ind;
  size_t encoder_buffer_len_threshold;

  size_t num_samples_threshold;

  size_t buffer_len_max;
  size_t num_buffer_bytes;


  int http_sockfd;
  int sddp_sockfd;

  unsigned long min_allowed_ip;
  unsigned long max_allowed_ip;

  unsigned short sample_rate;
  unsigned char bit_depth;
  double out_sample_max;
  unsigned char num_channels;
  unsigned char compression_level;

  size_t max_num_connections;

  const char *uuid;
  const char *name;
  const char *server_url;

  const char **channel_names;

  
  jack_client_t *jack;
  jack_port_t *ports[8];   /* One JACK port for each possible audio channel. */

};


struct shared_vars_t g_shared;


volatile bool g_exited;



#endif /* FLACJACKET_GLOBALS_H */


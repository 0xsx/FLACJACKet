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

#include <jack/jack.h>
#include <pthread.h>
#include <uuid/uuid.h>

#include "flacjacket_globals.h"
#include "flacjacket_params.h"
#include "logging.h"
#include "server.h"



#define MAX_MAGNITUDE_8  127.0
#define MAX_MAGNITUDE_12 2047.0
#define MAX_MAGNITUDE_16 32767.0
#define MAX_MAGNITUDE_20 524287.0
#define MAX_MAGNITUDE_24 8388607.0


/* Channels used by FLAC encoder. */
const char *CHANNEL_NAMES_1[] = {"Mono"};
const char *CHANNEL_NAMES_2[] = {"Left", "Right"};
const char *CHANNEL_NAMES_3[] = {"Left", "Right", "Center"};
const char *CHANNEL_NAMES_4[] = {"Front Left", "Front Right", "Back Left", "Back Right"};
const char *CHANNEL_NAMES_5[] = {"Front Left", "Front Right", "Front Center",
                                 "Back Left", "Back Right"};
const char *CHANNEL_NAMES_6[] = {"Front Left", "Front Right", "Front Center",
                                 "LFE", "Back Left", "Back Right"};
const char *CHANNEL_NAMES_7[] = {"Front Left", "Front Right", "Front Center",
                                 "LFE", "Back Center", "Side Left", "Side Right"};
const char *CHANNEL_NAMES_8[] = {"Front Left", "Front Right", "Front Center",
                                 "LFE", "Back Left", "Back Right",
                                 "Side Left", "Side Right"};







/* SIGINT handler to set the global exit flag. */
static void signal_handler(int sig) {
  if (sig == SIGINT) {
    debug_log("Caught Control+C signal.");
    g_exited = true;
  }
}




/* Process handler for JACK audio. Reads input ports, scales the samples and
converts to integer type, then interleaves channel samples and saves them to the
buffer consumed by media threads. */
static int process_audio(jack_nframes_t nframes, void *arg) {

  jack_default_audio_sample_t *sample_buffers[8];
  int32_t scaled;
  size_t i, j;
  

  /* Get latest buffers for each input port. */
  for (j=0; j < g_shared.num_channels; ++j) {
    sample_buffers[j] = jack_port_get_buffer(g_shared.ports[j], nframes);
  }


  /* Scale and interleave samples into the processor buffer. */
  for (i=0; i < nframes; ++i) {
    for (j=0; j < g_shared.num_channels; ++j) {
      scaled = (int32_t) (g_shared.out_sample_max * (double) sample_buffers[j][i]);
      g_shared.processor_buffer[g_shared.processor_buffer_len++] = scaled;
      if (g_shared.processor_buffer_len >= g_shared.buffer_len_max) {
        g_shared.processor_buffer_len = 0;
      }
    }
  }




  /* Copy processor buffer to encoder buffer only when lock is available. */
  if (pthread_mutex_trylock(&(g_shared.encoder_lock)) == 0) {

    memcpy(&(g_shared.encoder_buffer[g_shared.encoder_buffer_len]),
           g_shared.processor_buffer,
           g_shared.processor_buffer_len * sizeof(int32_t));
    g_shared.encoder_buffer_len += g_shared.processor_buffer_len;
    g_shared.processor_buffer_len = 0;


    if (g_shared.encoder_buffer_len >= 2 * g_shared.encoder_buffer_len_threshold) {

      memmove(&(g_shared.encoder_buffer[0]),
              &(g_shared.encoder_buffer[g_shared.encoder_buffer_len_threshold]),
              (g_shared.encoder_buffer_len - g_shared.encoder_buffer_len_threshold)
              * sizeof(int32_t));

      g_shared.encoder_buffer_start_ind += g_shared.encoder_buffer_len_threshold;
      g_shared.encoder_buffer_len -= g_shared.encoder_buffer_len_threshold;
    }

    pthread_mutex_unlock(&(g_shared.encoder_lock));
    
  }


  return 0;      
}




/* If JACK shuts down, just set the exit flag and allow threads to close. */
void jack_shutdown(void *arg) {
  debug_log("Jack shutdown initiated.");
  g_exited = true;
}






int main (int argc, char *argv[]) {

  g_exited = false;

  
  /* Install signal handler to catch control+c and exit gracefully. */
  struct sigaction sigact;
  sigact.sa_handler = signal_handler;
  sigact.sa_flags = 0;
  sigemptyset(&sigact.sa_mask);
  sigaction(SIGINT, &sigact, NULL);



  /* Parse and validate JSON parameters. */

  struct fj_params_t params;


  strcpy(params.name_buffer, "FLACJACKet");
  strcpy(params.listen_hostname_buffer, "0.0.0.0");
  strcpy(params.allowed_cidr_buffer, "127.0.0.1/24");
  params.port = 4000;
  params.max_num_connections = 16;
  params.compression_level = 4;
  params.bit_depth = 16;
  params.num_channels = 8;
  params.encoder_buffer_ms = 60;



  g_shared.max_num_connections = params.max_num_connections;
  g_shared.name = params.name_buffer;
  g_shared.compression_level = params.compression_level;

  g_shared.server_url = "http://127.0.0.1:4000";



  get_allowed_address_range(params.allowed_cidr_buffer, &(g_shared.min_allowed_ip),
                            &(g_shared.max_allowed_ip));


  switch (params.bit_depth) {
    case (8):
      g_shared.out_sample_max = MAX_MAGNITUDE_8;
      break;
    case (12):
      g_shared.out_sample_max = MAX_MAGNITUDE_12;
      break;
    case (16):
      g_shared.out_sample_max = MAX_MAGNITUDE_16;
      break;
    case (20):
      g_shared.out_sample_max = MAX_MAGNITUDE_20;
      break;
    case (24):
      g_shared.out_sample_max = MAX_MAGNITUDE_24;
      break;
    default:
      error_log("Invalid bits per sample: %d.", params.bit_depth);
      exit(1);
  }
  g_shared.bit_depth = params.bit_depth;

  

  switch (params.num_channels) {
    case (1):
      g_shared.channel_names = CHANNEL_NAMES_1;
      break;
    case (2):
      g_shared.channel_names = CHANNEL_NAMES_2;
      break;
    case (3):
      g_shared.channel_names = CHANNEL_NAMES_3;
      break;
    case (4):
      g_shared.channel_names = CHANNEL_NAMES_4;
      break;
    case (5):
      g_shared.channel_names = CHANNEL_NAMES_5;
      break;
    case (6):
      g_shared.channel_names = CHANNEL_NAMES_6;
      break;
    case (7):
      g_shared.channel_names = CHANNEL_NAMES_7;
      break;
    case (8):
      g_shared.channel_names = CHANNEL_NAMES_8;
      break;
    default:
      error_log("Invalid number of channels: %d.", params.num_channels);
      exit(1);
  }
  g_shared.num_channels = params.num_channels;




  /* Generate a unique UUID for this running instance. */
  char uuid_str[37];
  uuid_t uuid;
  uuid_generate(uuid);
  uuid_unparse(uuid, &(uuid_str[0]));
  g_shared.uuid = uuid_str;



  /* Create JACK client and connect to JACK server. */
  jack_status_t status;
  g_shared.jack = jack_client_open(params.name_buffer, JackNullOption, &status, NULL);

  if (g_shared.jack == NULL) {
    error_log("JACK client not opened, status = 0x%2.0x.", status);
    if (status & JackServerFailed) {
      error_log("JACK failed to connect.");
    }
    exit(1);
  }
  if (status & JackNameNotUnique) {
    error_log("JACK client with name '%s' already exists.", params.name_buffer);
    jack_client_close(g_shared.jack);
    exit(1);
  }



  
  /* Initialize JACK callbacks, open ports, and activate the client. */
  jack_set_process_callback(g_shared.jack, process_audio, NULL);
  jack_on_shutdown(g_shared.jack, jack_shutdown, NULL);

  for (size_t i=0; i < g_shared.num_channels; ++i) {
    g_shared.ports[i] = jack_port_register(g_shared.jack,
                                           g_shared.channel_names[i],
                                           JACK_DEFAULT_AUDIO_TYPE,
                                           JackPortIsInput, 0);
    if (g_shared.ports[i] == NULL) {
      error_log("No more JACK ports available.");
      jack_client_close(g_shared.jack);
      exit(1);
    }
  }

  if (jack_activate(g_shared.jack)) {
    error_log("Cannot activate JACK client.");
    jack_client_close(g_shared.jack);
    exit(1);
  }


  g_shared.sample_rate = (size_t) jack_get_sample_rate(g_shared.jack);

  info_log("JACK client activated with sample rate %d.", g_shared.sample_rate);




  /* Allocate memory for media buffers. */
  g_shared.num_samples_threshold = (size_t) ceil((g_shared.sample_rate / 1000.0)
                                                 * params.encoder_buffer_ms);

  g_shared.encoder_buffer_len_threshold = g_shared.num_samples_threshold
                                          * g_shared.num_channels;

  g_shared.buffer_len_max = 4 * g_shared.encoder_buffer_len_threshold;
  g_shared.num_buffer_bytes = sizeof(int32_t) * g_shared.buffer_len_max;


  debug_log("Buffer size: %d bytes.", g_shared.num_buffer_bytes);


  g_shared.processor_buffer = (int32_t*) malloc(g_shared.num_buffer_bytes);
  g_shared.processor_buffer_len = 0;
  g_shared.encoder_buffer = (int32_t*) malloc(g_shared.num_buffer_bytes);
  g_shared.encoder_buffer_len = 0;
  g_shared.encoder_buffer_start_ind = 0;


  if (g_shared.processor_buffer == NULL || g_shared.encoder_buffer == NULL) {

    error_log("Cannot allocate buffer memory.");

    if (g_shared.processor_buffer != NULL) free(g_shared.processor_buffer);
    if (g_shared.encoder_buffer != NULL) free(g_shared.encoder_buffer);
    jack_client_close(g_shared.jack);

    exit(1);
  }

  


  

  /* Start DLNA server. */
  g_shared.http_sockfd = http_bind_and_listen(params.listen_hostname_buffer,
                                              params.port);
  g_shared.sddp_sockfd = sddp_bind(params.listen_hostname_buffer);

  info_log("Listening on %s:%d.", params.listen_hostname_buffer, params.port);

  


  


  /* Create and join threads to run until canceled by user. */
  pthread_mutex_init(&(g_shared.encoder_lock), NULL);

  pthread_create(&(g_shared.http_thread_id), NULL, run_http_thread, NULL);
  pthread_create(&(g_shared.sddp_thread_id), NULL, run_sddp_thread, NULL);

  pthread_join(g_shared.sddp_thread_id, NULL);
  pthread_join(g_shared.http_thread_id, NULL);




  /* Clean up. */
  pthread_mutex_destroy(&(g_shared.encoder_lock));

  close(g_shared.http_sockfd);
  close(g_shared.sddp_sockfd);

  jack_client_close(g_shared.jack);

  free(g_shared.processor_buffer);
  free(g_shared.encoder_buffer);

  sigemptyset(&sigact.sa_mask);


  info_log("Exited by user.");


  return 0;
}


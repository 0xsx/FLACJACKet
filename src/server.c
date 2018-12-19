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

#include "flacjacket_globals.h"
#include "http_sends.h"
#include "logging.h"
#include "sddp_sends.h"
#include "server.h"




/* Parses the request buffer to retrieve the URI from GET and POST methods. */
static void parse_uri(const char *buffer, const size_t buffer_len,
                      char *uri_buffer, bool *is_get, bool *is_post) {

  const char *uri_start, *c;
  size_t i;

  if (buffer_len < 6) {
    *is_get = false;
    *is_post = false;
    uri_buffer[0] = '\0';
    return;
  }

  *is_get = buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T';
  *is_post = !*is_get && buffer[0] == 'P' && buffer[1] == 'O'
             && buffer[2] == 'S' && buffer[3] == 'T';

  if (*is_get) {
    uri_start = &(buffer[4]);
  } else if (*is_post) {
    uri_start = &(buffer[5]);
  } else {
    uri_buffer[0] = '\0';
    return;
  }

  c = uri_start;
  for (i=0; i < MAX_URI_LEN; ++i) {
    if (*c == ' ' || *c == '\r' || *c == '\n') {
      break;
    }
    uri_buffer[i] = *c;
    ++c;
  }
  uri_buffer[i] = '\0';

}






void * run_media_thread(void *args) {

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 5000000L;  /* 5 ms */


  int sockfd = *((int*)args);


  char recv_buffer[RECV_SIZE];
  size_t num_received;
  bool request_beginning, encoded;
  uint64_t start_ind, end, cur_len;
  int32_t *sample_buffer;

  
  int opt = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(int));
  send_chunked_stream_response(SERVER_NAME, sockfd);


  /* Initialize the FLAC encoder. */
  FLAC__StreamEncoder *encoder;
  FLAC__bool ok = true;
  FLAC__StreamEncoderInitStatus init_status;

  encoder = FLAC__stream_encoder_new();

  if (encoder == NULL) {
    ok = false;
  }

  if (ok) {
    ok &= FLAC__stream_encoder_set_compression_level(encoder, g_shared.compression_level);
    ok &= FLAC__stream_encoder_set_channels(encoder, g_shared.num_channels);
    ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, g_shared.bit_depth);
    ok &= FLAC__stream_encoder_set_sample_rate(encoder, g_shared.sample_rate);
  }

  if (ok) {
    init_status = FLAC__stream_encoder_init_stream(encoder, send_flac_callback,
                                                   NULL, NULL, NULL, &sockfd);
    if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
      error_log("Failed to initialize encoder: %s",
                FLAC__StreamEncoderInitStatusString[init_status]);
      ok = false;
    }
  }

  if (!ok) {
    error_log("Cannot create FLAC encoder.");
    if (encoder != NULL) {
      FLAC__stream_encoder_delete(encoder);
    }
    send_error_response(SERVER_NAME, sockfd);
    close(sockfd);
    return NULL;
  }




  /* Get the start of this thread's stream as the latest possible point that is
  closest to being ready to send. */
  pthread_mutex_lock(&(g_shared.encoder_lock));
  end = g_shared.encoder_buffer_start_ind + g_shared.encoder_buffer_len;
  start_ind = end > g_shared.encoder_buffer_len_threshold
              ? end - g_shared.encoder_buffer_len_threshold
              : g_shared.encoder_buffer_start_ind;
  pthread_mutex_unlock(&(g_shared.encoder_lock));


  debug_log("Media thread started.");


  while (1) {
    if (g_exited) break;

    encoded = false;

    /* Empty the socket read buffer. */
    request_beginning = true;
    while (1) {
      num_received = recv(sockfd, recv_buffer, RECV_SIZE, 0);
      if (num_received > 0 && num_received <= RECV_SIZE && request_beginning) {
        recv_buffer[num_received-1] = '\0';
        debug_log(recv_buffer);
      }
      request_beginning = false;

      if (errno == EAGAIN || errno == EWOULDBLOCK || num_received == 0
          || num_received > RECV_SIZE) {
        break;
      }
    }



    /* Encode the stream samples. */
    if (pthread_mutex_trylock(&(g_shared.encoder_lock)) == 0) {

      end = g_shared.encoder_buffer_start_ind + g_shared.encoder_buffer_len;
      cur_len = end - start_ind;

      if (cur_len >= g_shared.encoder_buffer_len_threshold) {
        sample_buffer = &(g_shared.encoder_buffer[start_ind - g_shared.encoder_buffer_start_ind]);
        start_ind += g_shared.encoder_buffer_len_threshold;

        FLAC__stream_encoder_process_interleaved(encoder, sample_buffer,
                                                 g_shared.num_samples_threshold);

        encoded = true;
      }

      pthread_mutex_unlock(&(g_shared.encoder_lock));
    }



    if (!encoded) nanosleep(&ts, NULL);
  }


  FLAC__stream_encoder_finish(encoder);
  FLAC__stream_encoder_delete(encoder);

  close(sockfd);


  debug_log("Exiting media thread.");

  return NULL;
}






void * run_http_thread() {

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100000000L;  /* 100 ms */


  int connfd;
  socklen_t clientnamelen;
  struct sockaddr_in clientname;
  unsigned long ip;
  size_t i, j;

  char recv_buffer[RECV_SIZE];
  char uri_buffer[MAX_URI_LEN+1];  /* Length+1 to hold terminating null. */
  size_t num_received;
  bool request_beginning;
  bool is_get, is_post;


  pthread_t *media_threads = (pthread_t*) malloc(sizeof(pthread_t)
                                                 * g_shared.max_num_connections);
  int *temp_connections = (int*) malloc(sizeof(int) * g_shared.max_num_connections);
  bool *is_closed = (bool*) malloc(sizeof(bool) * g_shared.max_num_connections);
  size_t num_connections = 0;
  size_t num_threads = 0;

  while (1) {
    if (g_exited) break;

    

    /* Open connection requests and store them in the temp array. */
    clientnamelen = sizeof(struct sockaddr_in);
    connfd = accept(g_shared.http_sockfd, (struct sockaddr *)&clientname,
                    &clientnamelen);

    if (connfd < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        error_log(strerror(errno));
        g_exited = true;
      }
    }
    else {

      ip = ntohl(clientname.sin_addr.s_addr);

      if (ip < g_shared.min_allowed_ip || ip > g_shared.max_allowed_ip
          || num_connections >= g_shared.max_num_connections) {
        close(connfd);
        debug_log("Rejected connection from %s.", inet_ntoa(clientname.sin_addr));
      }
      else {
        if (fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
          error_log(strerror(errno));
          close(connfd);
          g_exited = true;
        }
        else {
          temp_connections[num_connections] = connfd;
          is_closed[num_connections] = false;
          ++num_connections;
          debug_log("Opened connection from %s.", inet_ntoa(clientname.sin_addr));
        }
        
      }

    }



    /* Service requests from open temp connections. */
    for (i=0; i < num_connections; ++i) {

      request_beginning = true;
      is_get = false;
      is_post = false;

      while (1) {
        num_received = recv(temp_connections[i], recv_buffer, RECV_SIZE, 0);



        if (num_received > 0 && num_received <= RECV_SIZE && request_beginning) {
          parse_uri(recv_buffer, num_received, uri_buffer, &is_get, &is_post);
        }
        request_beginning = false;

        if (errno == EAGAIN || errno == EWOULDBLOCK || num_received == 0
            || num_received > RECV_SIZE) {
          break;
        }
      }

      if (is_get) {

        if (strcmp(uri_buffer, "/rootDesc.xml") == 0) {
          send_root_xml_response(g_shared.uuid, g_shared.name,
                                 SERVER_NAME, temp_connections[i]);
          close(temp_connections[i]);
          is_closed[i] = true;
        }

        else if (strcmp(uri_buffer, "/ContentDir.xml") == 0) {
          send_content_dir_xml_response(SERVER_NAME, temp_connections[i]);
          close(temp_connections[i]);
          is_closed[i] = true;
        }

        else if (strcmp(uri_buffer, "/media/0.flac") == 0
                 && num_threads < g_shared.max_num_connections) {
          int sockfd = temp_connections[i];
          pthread_create(&(media_threads[num_threads]), NULL, run_media_thread, &sockfd);
          ++num_threads;
          is_closed[i] = true;
        }

        else {
          send_empty_response(SERVER_NAME, temp_connections[i]);
          close(temp_connections[i]);
          is_closed[i] = true;
        }

        
      }

      else if (is_post) {
        if (strcmp(uri_buffer, "/ctl/ContentDir") == 0) {
          send_content_response(g_shared.name, SERVER_NAME,
                                g_shared.server_url, g_shared.sample_rate,
                                g_shared.bit_depth, temp_connections[i]);
          close(temp_connections[i]);
          is_closed[i] = true;
        }
        else {
          send_empty_response(SERVER_NAME, temp_connections[i]);
          close(temp_connections[i]);
          is_closed[i] = true;
        }
      }

      else {
          send_empty_response(SERVER_NAME, temp_connections[i]);
          close(temp_connections[i]);
          is_closed[i] = true;
        }

    }



    /* Remove closed connections from temp array. */
    for (i=0; i < num_connections; ++i) {
      if (is_closed[i]) {
        for (j=i+1; j < num_connections; ++j) {
          is_closed[j-1] = is_closed[j];
          temp_connections[j-1] = temp_connections[j];
        }
        --num_connections;
      }
    }


    nanosleep(&ts, NULL);
  }



  /* Clean up. */
  for (i=0; i < num_connections; ++i) {
    close(temp_connections[i]);
  }

  for (i=0; i < num_threads; ++i) {
    pthread_join(media_threads[i], NULL);
  }

  free(temp_connections);
  free(is_closed);
  free(media_threads);

  debug_log("Exiting http thread.");



  return NULL;
}






void * run_sddp_thread() {

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100000000L;  /* 100 ms */


  char recv_buffer[RECV_SIZE];
  bool request_beginning;
  size_t num_received;



  socklen_t clientnamelen;
  struct sockaddr_in clientname;



  send_notify_multicast(g_shared.uuid, SERVER_NAME, g_shared.server_url,
                        g_shared.sddp_sockfd);



  while (1) {
    if (g_exited) break;

    request_beginning = true;

    while (1) {

      clientnamelen = sizeof(clientname);
      num_received = recvfrom(g_shared.sddp_sockfd, recv_buffer, RECV_SIZE, 0,
                              (struct sockaddr *)&clientname, &clientnamelen);
      if (num_received > 0 && num_received <= RECV_SIZE && request_beginning) {
        if (strncmp(recv_buffer, "M-SEARCH", 8) == 0) {
          debug_log("Received search from %s.", inet_ntoa(clientname.sin_addr));
          send_search_response((struct sockaddr*)&clientname, clientnamelen,
                               g_shared.uuid, SERVER_NAME,
                               g_shared.server_url, g_shared.sddp_sockfd);
        }
      }
      request_beginning = false;

      
      if (errno == EAGAIN || errno == EWOULDBLOCK || num_received == 0
          || num_received > RECV_SIZE) {
        break;
      }


    }


    nanosleep(&ts, NULL);
  }


  debug_log("Exiting sddp thread.");



  return NULL;
}











int http_bind_and_listen(const char *hostname, unsigned short port) {

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  struct sockaddr_in http_sockaddr;

  if (sockfd < 0) {
    error_log(strerror(errno));
    exit(1);
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  memset(&http_sockaddr, 0, sizeof(struct sockaddr_in));
  http_sockaddr.sin_family = AF_INET;
  http_sockaddr.sin_port = htons(port);

  if (inet_aton(hostname, &(http_sockaddr.sin_addr)) < 0) {
    error_log("Invalid hostname: %s.", hostname);
    close(sockfd);
    exit(1);
  }

  if (bind(sockfd, (struct sockaddr*)&http_sockaddr, sizeof(struct sockaddr_in)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  if (listen(sockfd, HTTP_BACKLOG) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  return sockfd;
}







int sddp_bind(const char *hostname) {

  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int opt = 1;
  struct sockaddr_in sddp_sockaddr;
  struct in_addr mc_if;



  struct ip_mreq imr;

  memset(&imr, 0, sizeof(imr));
  imr.imr_multiaddr.s_addr = inet_addr(SDDP_ADDRESS);



  if (sockfd < 0) {
    error_log(strerror(errno));
    exit(1);
  }



  if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&imr, sizeof(imr)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }



  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  opt = 1;
  if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(int)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  opt = 8;
  if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &opt, sizeof(int)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  memset(&sddp_sockaddr, 0, sizeof(struct sockaddr_in));
  sddp_sockaddr.sin_family = AF_INET;
  sddp_sockaddr.sin_port = htons(SDDP_PORT);
  sddp_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);


  if (inet_aton(hostname, &(mc_if)) < 0) {
    error_log("Invalid hostname: %s.", hostname);
    close(sockfd);
    exit(1);
  }

  if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&(mc_if),
                 sizeof(mc_if)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }

  if (bind(sockfd, (struct sockaddr*)&sddp_sockaddr, sizeof(struct sockaddr_in)) < 0) {
    error_log(strerror(errno));
    close(sockfd);
    exit(1);
  }


  return sockfd;
}







void get_allowed_address_range(const char *cidr, unsigned long *start_ip,
                               unsigned long *end_ip) {

  const char *c = cidr;
  char hostname[13];
  int mask_bits = 32;
  size_t i;

  memset(hostname, 0, 13);
  i = 0;

  while (*c != '\0') {
    if (*c == '/') {
      mask_bits = atoi(c+1);
      break;
    }
    else {
      hostname[i] = *c;
    }
    ++i;
    ++c;
  }

  unsigned long mask = ~((1 << (32 - mask_bits)) - 1);

  struct in_addr ip;

  if (inet_aton(hostname, &ip) < 0) {
    error_log("Invalid hostname: %s.", hostname);
    exit(1);
  }

  *start_ip = ntohl(ip.s_addr) & mask;
  *end_ip = (ntohl(ip.s_addr) & mask) | ~mask;
}


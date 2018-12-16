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
#include <unistd.h>

#include "logging.h"
#include "server.h"
#include "sddp_sends.h"



void send_notify_multicast(const char *uuid, const char *server_name,
                           const char *server_url, const int sockfd) {

  char time_str[32];
  char send_buffer[512];
  size_t send_len;
  time_t cur_time = time(NULL);

  struct sockaddr_in dest_addr;
  size_t addrlen = sizeof(dest_addr);


  memset(&dest_addr, 0, addrlen);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(SDDP_PORT);

  if (inet_aton(SDDP_ADDRESS, &(dest_addr.sin_addr)) < 0) {
    error_log("Could not get SDDP address.");
    exit(1);  /* Should never be reached since valid SDDP_ADDRESS is defined. */
  }


  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));

  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "NOTIFY * HTTP/1.1\r\n"
    "HOST:%s:%d\r\n"
    "CACHE-CONTROL: max-age=3600\r\n"
    "DATE: %s\r\n"
    "NT:urn:schemas-upnp-org:device:MediaServer:1\r\n"
    "USN: uuid:%s::urn:schemas-upnp-org:device:MediaServer:1\r\n"
    "NTS:ssdp:alive\r\n"
    "SERVER: %s\r\n"
    "LOCATION: %s/rootDesc.xml\r\n\r\n",
    SDDP_ADDRESS,
    SDDP_PORT,
    time_str,
    uuid,
    server_name,
    server_url);


  /* Send duplicate notifications since this is UDP. */
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 50000000L;  /* 50 ms */

  for (size_t dup=0; dup < 3; ++dup) {
    sendto(sockfd, send_buffer, send_len, 0, (struct sockaddr*)&dest_addr, addrlen);
    nanosleep(&ts, NULL);
  }
  

}





void send_search_response(const struct sockaddr *dest_addr, size_t addrlen,
                          const char *uuid, const char *server_name,
                          const char *server_url, const int sockfd) {

  char time_str[32];
  char send_buffer[512];
  size_t send_len;
  time_t cur_time = time(NULL);

  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));

  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "HTTP/1.1 200 OK\r\n"
    "CACHE-CONTROL: max-age=3600\r\n"
    "DATE: %s\r\n"
    "ST: urn:schemas-upnp-org:device:MediaServer:1\r\n"
    "USN: uuid:%s::urn:schemas-upnp-org:device:MediaServer:1\r\n"
    "EXT: \r\n"
    "SERVER: %s\r\n"
    "LOCATION: %s/rootDesc.xml\r\n"
    "Content-Length: 0\r\n\r\n",
    time_str,
    uuid,
    server_name,
    server_url);


  sendto(sockfd, send_buffer, send_len, 0, dest_addr, addrlen);

}



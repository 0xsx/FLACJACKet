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

#ifndef SERVER_H
#define SERVER_H


#define HTTP_BACKLOG 16
#define RECV_SIZE 1024
#define MAX_URI_LEN 256

#define SDDP_ADDRESS "239.255.255.250"
#define SDDP_PORT 1900

#define SERVER_NAME "FLACJACKet/" PACKAGE_VERSION


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

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <FLAC/stream_encoder.h>



/* Parses the CIDR string and returns the start ip and end ip as unsigned
long integers in host byte order. */
void get_allowed_address_range(const char *cidr, unsigned long *start_ip,
                               unsigned long *end_ip);


/* Creates a non-blocking socket, binds it to the interface of the
specified hostname, and begins listening on the specified port. */
int http_bind_and_listen(const char *hostname, unsigned short port);

/* Creates a non-blocking socket for the SDDP multicast group and binds it to the
interface of the specified hostname. */
int sddp_bind(const char *hostname);



/* Runs the thread for encoding and transferring FLAC media to the specified
client socket. */
void * run_media_thread(void *args);

/* Runs the thread for listening and responding to HTTP requests. */
void * run_http_thread();

/* Runs the thread for listening and responding to SDDP messages. */
void * run_sddp_thread();





#endif /* SERVER_H */


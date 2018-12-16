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

#ifndef HTTP_SENDS_H
#define HTTP_SENDS_H


#include "server.h"


/* Sends an empty HTTP OK response to the specified socket. */
void send_empty_response(const char *server_name, const int sockfd);

/* Sends a 500 error response to the specified socket. */
void send_error_response(const char *server_name, const int sockfd);


/* Sends the DLNA root XML response to the specified socket providing a
server description and content directory service. */
void send_root_xml_response(const char *uuid, const char *friendly_name,
                            const char *server_name, const int sockfd);


/* Sends the DLNA browse content XML response to the specified socket providing
only a single URL to the flac media file. */
void send_content_response(const char *friendly_name, const char *server_name,
                           const char *server_url, const size_t sample_rate,
                           const size_t bit_depth, const int sockfd);


/* Sends response headers to initiate a stream with chunked transfer encoding. */
void send_chunked_stream_response(const char *server_name, const int sockfd);


/* Encoder callback to send a chunk of FLAC data to the socket specified as
client data. */
FLAC__StreamEncoderWriteStatus send_flac_callback(const FLAC__StreamEncoder *encoder,
                                                  const FLAC__byte *buffer,
                                                  size_t bytes, unsigned samples,
                                                  unsigned current_frame,
                                                  void *client_data);


#endif /* HTTP_SENDS_H */

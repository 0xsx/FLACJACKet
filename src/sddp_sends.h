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

#ifndef SDDP_SENDS_H
#define SDDP_SENDS_H

#include <netinet/in.h>


/* Sends the SDDP multicast notification announcing that the service is alive,
using the specified socket. */
void send_notify_multicast(const char *uuid, const char *server_name,
                           const char *server_url, const int sockfd);


/* Sends an SDDP response to a search received from the specified destination
address, using the specified socket. */
void send_search_response(const struct sockaddr *dest_addr, size_t addrlen,
                          const char *uuid, const char *server_name,
                          const char *server_url, const int sockfd);



#endif /* SDDP_SENDS_H */

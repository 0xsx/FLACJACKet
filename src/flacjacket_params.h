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

#ifndef FLACJACKET_PARAMS_H
#define FLACJACKET_PARAMS_H


#define PARAM_STR_BUFFER_SIZE 256



struct fj_params_t {

  size_t max_num_connections;
  unsigned char bit_depth;
  unsigned char num_channels;

  size_t encoder_buffer_ms;

  unsigned short port;
  unsigned char compression_level;

  char name_buffer[PARAM_STR_BUFFER_SIZE];
  char listen_hostname_buffer[PARAM_STR_BUFFER_SIZE];
  char allowed_cidr_buffer[PARAM_STR_BUFFER_SIZE];

};



#endif /* FLACJACKET_PARAMS_H */

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

#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>


/* Prints a message to stdout only if the program is built in debug mode. */
void debug_log(const char *msg, ...);

/* Prints a message to stdout. */
void info_log(const char *msg, ...);

/* Prints an error message to stderror. */
void error_log(const char *msg, ...);


#endif /* LOGGING_H */

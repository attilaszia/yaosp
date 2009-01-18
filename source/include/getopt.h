/* yaosp C library
 *
 * Copyright (c) 2009 Kornel Csernai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _GETOPT_H_
#define _GETOPT_H_

typedef struct option {
    const char *name;
    int has_arg;
    int* flag;
    int val;
} option_t ;

#define no_argument         0
#define required_argument   1
#define optional_argument   2


extern char *optarg;
extern int optind, opterr, optopt;

int getopt( int argc, char* const * argv, const char* opstring );

int getopt_long( int argc, char* const * argv, const char* shortopts,
                 const struct option* longopts, int* longind );

int getopt_long_only( int argc, char* const * argv,
                      const char* shortopts, const struct option* longopts, int* longind );

#endif // _GETOPT_H_

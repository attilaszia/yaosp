/* Date and time handling
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
#ifndef _LIBTIME_H_
#define _LIBTIME_H_

#define EPOCH 1970 // Counting only time elapsed since 1 Jan 1970

#define STRFT_ALTFORM 0x01

typedef struct tm {
    int sec;    /* Seconds. [0-60] (1 leap second) */
    int min;    /* Minutes. [0-59] */
    int hour;   /* Hours.   [0-23] */
    int mday;   /* Day.     [1-31] */
    int mon;    /* Month.   [0-11] */
    int year;   /* Year - 1900. */
    int wday;   /* Day of week. [0-6] */
    int yday;   /* Days in year. [0-365] */
    int isdst;  /* Daylight saving [-1/0/1] */
} tm_t ;

extern const char* month_names[ 12 ];
extern const char* smonth_names[ 12 ];
extern const char* day_names[ 7 ];
extern char* sday_names[ 7 ];

/* Converts a broken-down time to UNIX timestamp */
int timestamp(tm_t* time);
/* Converts a UNIX timestamp to a broken-down time */
tm_t gettime(int* time);
/* Formats a broken-down time, uses the first at most m chars of s */
size_t strftime(char *s, size_t max, const char *format,
                       const tm_t *tm);
#endif // _LIBTIME_H_

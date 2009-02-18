/* Time management
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2008 Kornel Csernai
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

#ifndef _TIME_H_
#define _TIME_H_

#include <types.h>

#define time_t uint64_t
#define suseconds_t int

typedef struct timeval {
    time_t      tv_sec;    /* Seconds */
    suseconds_t tv_usec;   /* Microseconds */
} timeval_t;

typedef struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
} timezone_t;

typedef struct tm {
    int tm_sec;    /* Seconds. [0-60] (1 leap second) */
    int tm_min;    /* Minutes. [0-59] */
    int tm_hour;   /* Hours.   [0-23] */
    int tm_mday;   /* Day.     [1-31] */
    int tm_mon;    /* Month.   [0-11] */
    int tm_year;   /* Year [1970; ...] */
    int tm_wday;   /* Day of week. [0-6], 0=Sunday */
    int tm_yday;   /* Days in year. [0-365] */
    int tm_isdst;  /* Daylight saving [-1/0/1] */
} tm_t;

int sys_time(int* tloc);
int sys_stime(int* tptr);
int sys_get_system_time( time_t* time );
time_t time(time_t* tloc);

/* int sys_adjtimex(timex_t* txc_p); */

/* Names and short names for months and days of week used by strftime */
extern const char* month_names[ 12 ];
extern const char* smonth_names[ 12 ];
extern const char* day_names[ 7 ];
extern const char* sday_names[ 7 ];

size_t strftime(char* s, size_t max, const char* format,
                const tm_t* tm);

/* Converts a broken-down time to UNIX timestamp */
time_t mktime(tm_t* tm);

tm_t* gmtime(const time_t* timep);
tm_t* gmtime_r(const time_t* timep, tm_t* result);

#endif // _TIME_H_

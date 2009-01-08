/* Date and time handling
 *
 * Copyright (c) 2009 Kornel Csernai
 * Copyright (c) 2009 Zoltan Kovacs
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

#include <lib/string.h>
#include <lib/time.h>

#define APPEND( str ) \
        ret += strlen(str); \
        if(ret > max){ \
            return 0; \
        } \
        strcat(s, (const char*) str);

const char* month_names[ 12 ] = { "January", "February", "March",
                                  "April", "May", "June",
                                  "July", "August", "September",
                                  "October", "November", "December" };

const char* smonth_names[ 12 ] = { "Jan", "Feb", "Mar",
                                   "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep",
                                   "Oct", "Nov", "Dec" };

const char* day_names[ 7 ] = { "Sunday", "Monday",
                               "Tuesday", "Wednesday",
                               "Thursday", "Friday",
                               "Saturday" };

const char* sday_names[ 7 ] = { "Sun", "Mon", "Tue",
                                "Wed", "Thu", "Fri",
                                "Sat" };

const unsigned short int monthdays[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

const unsigned short int monthdays2[13] =
{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

uint64_t timestamp(tm_t* time) {
    return daysdiff(time->year, time->mon, time->mday) * SECONDS_PER_DAY +
          time->hour * SECONDS_PER_HOUR + time->min * SECONDS_PER_MIN + time->sec;
}

tm_t gettime( int* timeval ) {
    tm_t ret;

    /* TODO */

    ret.sec = 0;
    ret.min = 0;
    ret.hour = 0;
    ret.mday = 0;
    ret.mon = 0;
    ret.year = 0;
    ret.wday = 0;
    ret.yday = 0;
    ret.isdst = -1;

    return ret;
}

int daysdiff(int year, int month, int day){
    int i, days = 0;

    for(i = EPOCH; i<year; i++){
        if(i % 4 == 0){ /* If it is a leap year */
            days += 366;
        }else{
            days += 365;
        }
    }

    if(year % 4 == 0){
        days += monthdays2[month];
    }else{
        days += monthdays[month];
    }

    days += day - 1;

    return days;
}

int dayofweek(int year, int month, int day){

    /* The UNIX Epoch(1 Jan 1970) was a Thursday */
    return (4 + daysdiff(year, month, day)) % 7;
}

/* TODO: recursive formats are buggy, fix them */
size_t strftime(char* s, size_t max, const char* format,
                       const tm_t* tm){
    size_t ret = 0;
    int state;
    int flags; 
    char tmp[max];

    s[0] = '\0'; /* Initialize s */
    max--; /* Reserve space for '\0' at the end */
    state = flags = 0;

    for( ; *format != '\0'; format++){
        switch(state){
            case 0:
                if(*format != '%'){
                    if(ret < max){
                        s[ret++] = *format;
                    } else {
                        s[ret] = '\0';
                        return 0;
                    }
                    break;
                }
                state++;
                format++;
            case 1:
                if(*format == '%'){
                    if(ret < max){
                        s[ret++] = '%';
                    } else {
                        s[ret] = '\0';
                        return 0;
                    }
                    state = flags = 0;
                    break;
                }
                /* TODO: locale-independent, we need locales first :)
                if(*format == 'E'){
                } */
                if(*format == 'O'){
                    flags |= STRFT_ALTFORM;
                    /* TODO: use this */
                }
                /* TODO: glibc modifiers: _-0^# */
                state++;
            case 2:
                switch(*format){
                    case 'a':
                        APPEND(sday_names[tm->wday]);
                        break;
                    case 'A':
                        APPEND(day_names[tm->wday]);
                        break;
                    case 'h':
                    case 'b':
                        APPEND(smonth_names[tm->mon]);
                        break;
                    case 'B':
                        APPEND(month_names[tm->mon]);
                        break;
                    case 'c':
                        /* The preferred date and time representation for the current locale. */
                        strftime(tmp, max-ret, "%a %b %e %H:%M:%S", tm);
                        APPEND(tmp);
                        break;
                    case 'C':
                        snprintf(tmp, max, "%d", tm->year / 100);
                        APPEND(tmp);
                        break;
                    case 'd':
                        snprintf(tmp, max, "%02d", tm->mday);
                        APPEND(tmp);
                        break;
                    case 'D':
                        strftime(tmp, max-ret, "%m/%d/%y", tm);
                        APPEND(tmp);
                        break;
                    case 'e':
                        snprintf(tmp, max, "%2d", tm->mday);
                        APPEND(tmp);
                        break;
                    case 'F':
                        strftime(tmp, max-ret, "%Y-%m-%d", tm);
                        APPEND(tmp);
                        break;
                    case 'G':
/* The ISO 8601 year with century as a decimal number.  The 4-digit year corresponding to the ISO  week  number
   (see  %V).  This has the same format and value as %y, except that if the ISO week number belongs to the
   previous or next year, that year is used instead. (TZ)*/
                        break;
                    case 'g':
                        /* Like %G, but without century, that is, with a 2-digit year (00-99). (TZ) */
                        break;
                    case 'H':
                        snprintf(tmp, max, "%02d", tm->hour);
                        APPEND(tmp);
                        break;
                    case 'I':
                        snprintf(tmp, max, "%02d", tm->hour % 12);
                        APPEND(tmp);
                        break;
                    case 'j':
                        /* Day of the year, 001-366 */
                        snprintf(tmp, max, "%03d", tm->yday + 1);
                        APPEND(tmp);
                        break;
                    case 'k':
                        snprintf(tmp, max, "%2d", tm->hour);
                        APPEND(tmp);
                        break;
                    case 'l':
                        snprintf(tmp, max, "%2d", tm->hour % 12);
                        APPEND(tmp);
                        break;
                    case 'm':
                        snprintf(tmp, max, "%02d", tm->mon);
                        APPEND(tmp);
                        break;
                    case 'M':
                        snprintf(tmp, max, "%02d", tm->min);
                        APPEND(tmp);
                        break;
                    case 'n':
                        APPEND("\n");
                        break;
                    case 'p':
                        if(tm->hour >= 12){
                            APPEND("PM");
                        }else{
                            APPEND("AM");
                        }
                        break;
                    case 'P':
                        if(tm->hour >= 12){
                            APPEND("pm");
                        }else{
                            APPEND("am");
                        }
                        break;
                    case 'r':
                        strftime(tmp, max-ret, "%I:%M:%S %p", tm);
                        APPEND(tmp);
                        break;
                    case 'R':
                        strftime(tmp, max-ret, "%H:%M", tm);
                        APPEND(tmp);
                        break;
                    case 's':
                        snprintf(tmp, max, "%d", timestamp((tm_t*) tm));
                        APPEND(tmp);
                        break;
                    case 'S':
                        snprintf(tmp, max, "%02d", tm->sec);
                        APPEND(tmp);
                        break;
                    case 't':
                        APPEND("\t");
                        break;
                    case 'T':
                        strftime(tmp, max-ret, "%H:%M:%S", tm);
                        APPEND(tmp);
                        break;
                    case 'u':
                        /* Day of the week, 1-7, Monday=1 */
                        snprintf(tmp, max, "%d", 1 + (tm->wday + 6 ) % 7);
                        APPEND(tmp);
                        break;
                    case 'U':
/* The week number of the current year as a decimal number, range 00 to 53, starting with the first  Sunday  as
   the first day of week 01.*/
                        break;
                    case 'V':
/*  The  ISO  8601:1988 week number of the current year as a decimal number, range 01 to 53, where week 1 is the
    first week that has at least 4 days in the current year, and with Monday as the first day of the week.   See
    also %U and %W. */
                        break;
                    case 'w':
                        /* Day of the week, 0-6, Sunday=0 */
                        snprintf(tmp, max, "%d", tm->wday);
                        APPEND(tmp);
                        break;
                    case 'W':
/* The  week  number of the current year as a decimal number, range 00 to 53, starting with the first Monday as
   the first day of week 01. */
                        break;
                    case 'x':
                        /* The preferred date representation for the current locale without the time. */
                        break;
                    case 'X':
                        /* The preferred time representation for the current locale without the date. */
                        break;
                    case 'y':
                        /* The year as a decimal number without a century (range 00 to 99). */
                        snprintf(tmp, max, "%02d", tm->year % 100);
                        APPEND(tmp);
                        break;
                    case 'Y':
                        /* The year as a decimal number including the century. */
                        snprintf(tmp, max, "%d", tm->year);
                        APPEND(tmp);
                        break;
                    case 'z':
/* The  time-zone  as  hour  offset   from   GMT.    Required   to   emit   RFC 822-conformant   dates   (using
   "%a, %d %b %Y %H:%M:%S %z"). (GNU) */
                        break;
                    case 'Z':
                        /* The time zone or name or abbreviation. */
                        break;
                    default:
                        break;
                }
            state = flags = 0;
        }
    }

    s[ret] = '\0';

    return ret;
}

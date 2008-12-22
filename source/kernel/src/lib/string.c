/* Memory and string manipulator functions
 *
 * Copyright (c) 2008 Zoltan Kovacs, Kornel Csernai
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

#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/lib/string.h>

#ifndef ARCH_HAVE_MEMSET
void* memset( void* s, int c, size_t n ) {
    char* _src;

    _src = ( char* )s;

    while ( n-- ) {
        *_src++ = c;
    }

    return s;
}
#endif // ARCH_HAVE_MEMSET

#ifndef ARCH_HAVE_MEMSETW
void* memsetw( void* s, int c, size_t n ) {
    uint16_t* _src;

    _src = ( uint16_t* )s;

    while ( n-- ) {
        *_src++ = c;
    }

    return s;
}
#endif // ARCH_HAVE_MEMSETW

#ifndef ARCH_HAVE_MEMSETL
void* memset( void* s, int c, size_t n ) {
    uint32_t* _src;

    _src = ( uint32_t* )s;

    while ( n-- ) {
        *_src++ = c;
    }

    return s;
}
#endif // ARCH_HAVE_MEMSETL

#ifndef ARCH_HAVE_MEMMOVE
void* memmove( void* dest, const void* src, size_t n ) {
    char* _dest;
    char* _src;

    if ( dest < src ) {
        _dest = ( char* )dest;
        _src = ( char* )src;

        while ( n-- ) {
            *_dest++ = *_src++;
        }
    } else {
        _dest = ( char* )dest + n;
        _src = ( char* )src + n;

        while ( n-- ) {
            *--_dest = *--_src;
        }
    }

    return dest;
}
#endif // ARCH_HAVE_MEMMOVE

#ifndef ARCH_HAVE_MEMCPY
void* memcpy( void* d, const void* s, size_t n ) {
    char* dest;
    char* src;

    dest = ( char* )d;
    src = ( char* )s;

    while ( n-- ) {
        *dest++ = *src++;
    }

    return d;
}
#endif // ARCH_HAVE_MEMCPY

#ifndef ARCH_HAVE_STRLEN
size_t strlen( const char* str ) {
    size_t r = 0;
    for( ; *str++; r++ ) { }
    return r;
}
#endif // ARCH_HAVE_STRLEN

#ifndef ARCH_HAVE_STRCMP
int strcmp( const char* s1, const char* s2 ) {
    int result;

    while ( true ) {
        result = *s1 - *s2++;

        if ( ( result != 0 ) || ( *s1 == 0 ) ) {
            break;
        }
    }

    return result;
}
#endif // ARCH_HAVE_STRCMP

#ifndef ARCH_HAVE_STRDUP
char* strdup( const char* s ) {
    size_t len;
    char* s2;

    len = strlen( s );
    s2 = ( char* )kmalloc( len + 1 );

    if ( s2 == NULL ) {
        return s2;
    }

    memcpy( s2, s, len + 1 );

    return s2;
}
#endif // ARCH_HAVE_STRDUP

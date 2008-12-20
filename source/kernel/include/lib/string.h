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

#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_

#include <types.h>

void* memset( void* s, int c, size_t n );
void* memsetw( void* s, int c, size_t n );
void* memsetl( void* s, int c, size_t n );

void* memcpy( void* d, const void* s, size_t n );
void* memmove( void* dest, const void* src, size_t n );

size_t strlen( const char* s );
char* strdup( const char* s );

#endif // _LIB_STRING_H_

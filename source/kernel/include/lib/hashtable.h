/* Hashtable implementation
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _LIB_HASHTABLE_H_
#define _LIB_HASHTABLE_H_

#include <types.h>

typedef struct hashitem {
    struct hashitem* next;
} hashitem_t;

typedef void* key_function_t( hashitem_t* item );
typedef uint32_t hash_function_t( const void* key );
typedef bool compare_function_t( const void* key1, const void* key2 );

typedef struct hashtable {
    uint32_t size;
    hashitem_t** items;

    key_function_t* key_func;
    hash_function_t* hash_func;
    compare_function_t* compare_func;
} hashtable_t;

typedef int hashtable_iter_callback_t( hashitem_t* item, void* data );

int init_hashtable(
    hashtable_t* table,
    uint32_t size,
    key_function_t* key_func,
    hash_function_t* hash_func,
    compare_function_t* compare_func
);
void destroy_hashtable( hashtable_t* table );

int hashtable_add( hashtable_t* table, hashitem_t* item );
hashitem_t* hashtable_get( hashtable_t* table, const void* key );
int hashtable_remove( hashtable_t* table, const void* key );

int hashtable_iterate( hashtable_t* table, hashtable_iter_callback_t* callback, void* data );

/* Common hash functions */

uint32_t hash_number( uint8_t* data, size_t length );
uint32_t hash_string( uint8_t* data, size_t length );

#endif // _LIB_HASHTABLE_H_

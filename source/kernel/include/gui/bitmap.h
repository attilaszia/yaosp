/* Bitmap definitions
 *
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

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <types.h>
#include <lib/hashtable.h>

typedef int bitmap_id;

enum bitmap_flags {
    BITMAP_FREE_BUFFER = ( 1 << 0 )
};

typedef struct bitmap {
    hashitem_t hash;

    bitmap_id id;
    int ref_count;
    uint32_t width;
    uint32_t height;
    int bytes_per_line;
    color_space_t color_space;
    void* buffer;
    uint32_t flags;
} bitmap_t;

#endif /* _BITMAP_H_ */

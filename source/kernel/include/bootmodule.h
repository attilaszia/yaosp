/* Boot module management
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

#ifndef _BOOTMODULE_H_
#define _BOOTMODULE_H_

#include <types.h>
#include <multiboot.h>
#include <module.h>

#define BOOTMODULE_NAME_LENGTH 64

typedef struct bootmodule {
    char name[ BOOTMODULE_NAME_LENGTH ];
    void* address;
    size_t size;
} bootmodule_t;

int get_bootmodule_count( void );
bootmodule_t* get_bootmodule_at( int index );

module_reader_t* get_bootmodule_reader( int index );
void put_bootmodule_reader( module_reader_t* reader );

int init_bootmodules( multiboot_header_t* header );

#endif // _BOOTMODULE_H_

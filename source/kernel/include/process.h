/* Process implementation
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <semaphore.h>
#include <config.h>
#include <mm/context.h>
#include <mm/region.h>
#include <vfs/io_context.h>
#include <lib/hashtable.h>

#include <arch/atomic.h>

typedef int process_id;

typedef struct process {
    hashitem_t hash;

    process_id id;
    char* name;
    semaphore_id lock;
    atomic_t thread_count;

    memory_context_t* memory_context;
    semaphore_context_t* semaphore_context;
    io_context_t* io_context;

    region_id heap_region;

    void* loader_data;

    semaphore_id waiters;
} process_t;

typedef struct process_info {
    process_id id;
    char name[ MAX_PROCESS_NAME_LENGTH ];
    uint64_t pmem_size;
    uint64_t vmem_size;
} process_info_t;

typedef int process_iter_callback_t( process_t* process, void* data );

process_t* allocate_process( char* name );
void destroy_process( process_t* process );
int insert_process( process_t* process );
void remove_process( process_t* process );
int rename_process( process_t* process, char* new_name );

uint32_t get_process_count( void );
process_t* get_process_by_id( process_id id );
int process_table_iterate( process_iter_callback_t* callback, void* data );

process_id sys_getpid( void );
int sys_exit( int exit_code );
int sys_waitpid( process_id pid, int* status, int options );

int init_processes( void );

#endif // _PROCESS_H_

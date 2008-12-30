/* Process implementation
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

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <semaphore.h>
#include <mm/context.h>
#include <lib/hashtable.h>

typedef int process_id;

typedef struct process {
    hashitem_t hash;

    process_id id;
    char* name;

    memory_context_t* memory_context;
    semaphore_context_t* semaphore_context;
} process_t;

process_t* get_process_by_id( process_id id );

int init_processes( void );

#endif // _PROCESS_H_
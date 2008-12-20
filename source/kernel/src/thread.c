/* Thread implementation
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

#include <thread.h>
#include <config.h>
#include <errno.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <mm/pages.h>
#include <lib/hashtable.h>
#include <lib/string.h>

#include <arch/thread.h>

#define MAX_THREAD_COUNT 1000000

static int thread_id_counter = 0;
static hashtable_t thread_table;

static thread_t* allocate_thread( const char* name ) {
    int error;
    thread_t* thread;

    thread = ( thread_t* )kmalloc( sizeof( thread_t ) );

    if ( thread == NULL ) {
        return NULL;
    }

    memset( thread, 0, sizeof( thread_t ) );

    thread->name = strdup( name );

    if ( thread->name == NULL ) {
        kfree( thread );
        return NULL;
    }

    thread->kernel_stack = ( register_t* )alloc_pages( KERNEL_STACK_PAGES );

    if ( thread->kernel_stack == NULL ) {
        kfree( thread->name );
        kfree( thread );
        return NULL;
    }

    error = arch_allocate_thread( thread );

    if ( error < 0 ) {
        free_pages( thread->kernel_stack, KERNEL_STACK_PAGES );
        kfree( thread->name );
        kfree( thread );
        return NULL;
    }

    thread->state = THREAD_READY;

    return thread;
}

void kernel_thread_exit( void ) {
    /* Just for now ... */
    panic( "Kernel thread exited!" );
}

thread_id create_kernel_thread( const char* name, thread_entry_t* entry, void* arg ) {
    thread_t* thread;

    /* Allocate a new thread */

    thread = allocate_thread( name );

    if ( thread == NULL ) {
        return -ENOMEM;
    }

    /* Get an unique ID to the new thread and add to the others */

    do {
        thread->id = ( thread_id_counter++ ) % MAX_THREAD_COUNT;
    } while ( hashtable_get( &thread_table, ( const void* )thread->id ) != NULL );

    hashtable_add( &thread_table, ( hashitem_t* )thread );

    return thread->id;
}

thread_t* get_thread_by_id( thread_id id ) {
    return ( thread_t* )hashtable_get( &thread_table, ( const void* )id );
}

static void* thread_key( hashitem_t* item ) {
    thread_t* thread;

    thread = ( thread_t* )item;

    return ( void* )thread->id;
}

static uint32_t thread_hash( const void* key ) {
    return ( uint32_t )key;
}

static bool thread_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

int init_threads( void ) {
    int error;

    error = init_hashtable(
                &thread_table,
                256,
                thread_key,
                thread_hash,
                thread_compare
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

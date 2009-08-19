/* Memory context handling code
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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

#include <errno.h>
#include <macros.h>
#include <kernel.h>
#include <process.h>
#include <console.h>
#include <lock/mutex.h>
#include <mm/context.h>
#include <mm/kmalloc.h>
#include <sched/scheduler.h>
#include <lib/string.h>

#include <arch/mm/config.h>
#include <arch/mm/context.h>
#include <arch/mm/region.h>

#define REGION_STEP_SIZE 32

memory_context_t kernel_memory_context;

extern lock_id region_lock;
extern hashtable_t region_table;

int memory_context_insert_region( memory_context_t* context, memory_region_t* region ) {
    int i;
    int error;
    int region_count;

    region_count = array_get_size( &context->regions );

    /* Insert the region to the list */

    for ( i = 0; i < region_count; i++ ) {
        memory_region_t* tmp_region;

        tmp_region = ( memory_region_t* )array_get_item( &context->regions, i );

        if ( region->start < tmp_region->start ) {
            break;
        }
    }

    error = array_insert_item( &context->regions, i, ( void* )region );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int memory_context_remove_region( memory_context_t* context, memory_region_t* region ) {
    int error;

    error = array_remove_item( &context->regions, ( void* )region );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

memory_region_t* do_memory_context_get_region_for( memory_context_t* context, ptr_t address ) {
    int i;
    int region_count;
    memory_region_t* region;

    region_count = array_get_size( &context->regions );

    for ( i = 0; i < region_count; i++ ) {
        region = ( memory_region_t* )array_get_item( &context->regions, i );

        if ( ( region->start <= address ) &&
             ( address <= ( region->start + region->size - 1 ) ) ) {
            return region;
        }
    }

    return NULL;
}

memory_region_t* memory_context_get_region_for( memory_context_t* context, ptr_t address ) {
    memory_region_t* region;

    mutex_lock( region_lock );

    region = do_memory_context_get_region_for( context, address );

    mutex_unlock( region_lock );

    return region;
}

bool memory_context_find_unmapped_region( memory_context_t* context, ptr_t start, ptr_t end,
                                          uint32_t size, ptr_t* address ) {
    int region_count;

    region_count = array_get_size( &context->regions );

    if ( region_count == 0 ) {
        if ( ( start + size - 1 ) <= end ) {
            *address = start;

            return true;
        }
    } else {
        int i;
        memory_region_t* region;

        for ( i = 0; i < region_count; i++ ) {
            region = ( memory_region_t* )array_get_item( &context->regions, i );

            if ( start + size <= region->start ) {
                *address = start;

                return true;
            } else {
                start = MAX( start, region->start + region->size );
            }
        }

        if ( ( start + size - 1 ) <= end ) {
            *address = start;

            return true;
        }
    }

    return false;
}

bool memory_context_can_resize_region( memory_context_t* context, memory_region_t* region, uint32_t new_size ) {
    int index;
    memory_region_t* tmp_region;

    index = array_index_of( &context->regions, ( void* )region );

    if ( index == -1 ) {
        kprintf(
            WARNING,
            "memory_context_can_resize_region(): Region not found in the memory context!\n"
        );

        return false;
    }

    if ( index == array_get_size( &context->regions ) - 1 ) {
        ptr_t end_address;

        if ( region->flags & REGION_KERNEL ) {
            end_address = LAST_KERNEL_ADDRESS;
        } else {
            end_address = LAST_USER_ADDRESS;
        }

        return ( ( region->start + new_size - 1 ) <= end_address );
    }

    tmp_region = ( memory_region_t* )array_get_item( &context->regions, index + 1 );

    return ( ( region->start + new_size - 1 ) < tmp_region->start );
}

memory_context_t* memory_context_clone( memory_context_t* old_context, process_t* new_process ) {
    int i;
    int error;
    int region_count;
    memory_context_t* new_context;

    /* Allocate a new memory context */

    new_context = ( memory_context_t* )kmalloc( sizeof( memory_context_t ) );

    if ( new_context == NULL ) {
        return NULL;
    }

    error = memory_context_init( new_context );

    if ( error < 0 ) {
        kfree( new_context );
        return NULL;
    }

    new_context->process = new_process;

    /* Initialize the architecture dependent part of the memory context */

    error = arch_init_memory_context( new_context );

    if ( error < 0 ) {
        kfree( new_context );
        return NULL;
    }

    mutex_lock( region_lock );

    arch_clone_memory_context( old_context, new_context );

    /* Go throught regions and clone each one */

    region_count = array_get_size( &old_context->regions );

    for ( i = 0; i < region_count; i++ ) {
        memory_region_t* region;
        memory_region_t* new_region;

        region = ( memory_region_t* )array_get_item( &old_context->regions, i );

        /* Don't clone kernel regions */

        if ( region->flags & REGION_KERNEL ) {
            ASSERT( region->file == NULL );

            continue;
        }

        new_region = allocate_region( region->name );

        if ( new_region == NULL ) {
            goto error;
        }

        new_region->flags = region->flags;
        new_region->alloc_method = region->alloc_method;
        new_region->start = region->start;
        new_region->size = region->size;
        new_region->context = new_context;

        new_region->file = region->file;
        new_region->file_offset = region->file_offset;
        new_region->file_size = region->file_size;

        /* Increase the reference count of the file because the new region will use it as well */

        if ( new_region->file != NULL ) {
            atomic_inc( &new_region->file->ref_count );
        }

        error = arch_clone_memory_region( old_context, region, new_context, new_region );

        if ( error < 0 ) {
            goto error;
        }

        error = region_insert( new_context, new_region );

        if ( error < 0 ) {
            goto error;
        }
    }

    mutex_unlock( region_lock );

    /* Clone the vmem_size of the old process as well ;) */

    scheduler_lock();
    new_process->vmem_size = old_context->process->vmem_size;
    scheduler_unlock();

    return new_context;

error:
    /* TODO: cleanup! */

    mutex_unlock( region_lock );

    return NULL;
}

int memory_context_delete_regions( memory_context_t* context ) {
    int i;
    int region_count;

    mutex_lock( region_lock );

    /* Delete all memory regions */

    region_count = array_get_size( &context->regions );

    for ( i = 0; i < region_count; i++ ) {
        memory_region_t* region;

        region = ( memory_region_t* )array_get_item( &context->regions, i );

        ASSERT( ( region->flags & REGION_KERNEL ) == 0 );

        arch_delete_region_pages( context, region );
        hashtable_remove( &region_table, ( const void* )&region->id );
        destroy_region( region );
    }

    /* Set the new size of memory regions in the context */

    array_make_empty( &context->regions );

    /* Update vmem statistics */

    scheduler_lock();
    context->process->vmem_size = 0;
    scheduler_unlock();

    mutex_unlock( region_lock );

    return 0;
}

void memory_context_destroy( memory_context_t* context ) {
    arch_destroy_memory_context( context );

    array_destroy( &context->regions );
    kfree( context );
}

#ifdef ENABLE_DEBUGGER
int memory_context_translate_address( memory_context_t* context, ptr_t linear, ptr_t* physical ) {
    if ( linear < FIRST_USER_ADDRESS ) {
        *physical = linear;

        return 0;
    }

    return arch_memory_context_translate_address( context, linear, physical );
}
#endif /* ENABLE_DEBUGGER */

void memory_context_dump( memory_context_t* context ) {
    int i;
    int region_count;

    kprintf( INFO, "Memory context dump:\n" );

    region_count = array_get_size( &context->regions );

    for ( i = 0; i < region_count; i++ ) {
        memory_region_t* region;

        region = ( memory_region_t* )array_get_item( &context->regions, i );

        memory_region_dump( region, i );
    }
}

int memory_context_init( memory_context_t* context ) {
    int error;

    memset(
        context,
        0,
        sizeof( memory_context_t )
    );

    error = array_init( &context->regions );

    if ( error < 0 ) {
        return error;
    }

    array_set_realloc_size( &context->regions, 32 );

    return 0;
}

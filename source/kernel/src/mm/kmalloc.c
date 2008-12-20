/* Memory allocator
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

#include <types.h>
#include <errno.h>
#include <console.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <mm/pages.h>
#include <lib/string.h>

#include <arch/spinlock.h>
#include <arch/mm/config.h>

static spinlock_t kmalloc_lock = INIT_SPINLOCK;
static kmalloc_block_t* root = NULL;

static kmalloc_block_t* __kmalloc_create_block( uint32_t pages ) {
    kmalloc_block_t* block;
    kmalloc_chunk_t* chunk;

    block = ( kmalloc_block_t* )alloc_pages( pages );

    if ( block == NULL ) {
        return NULL;
    }

    block->magic = KMALLOC_BLOCK_MAGIC;
    block->pages = KMALLOC_ROOT_SIZE;
    block->next = NULL;
    block->first_chunk = ( kmalloc_chunk_t* )( block + 1 );

    chunk = block->first_chunk;

    chunk->magic = KMALLOC_CHUNK_MAGIC;
    chunk->type = CHUNK_FREE;
    chunk->block = block;
    chunk->size = pages * PAGE_SIZE - sizeof( kmalloc_block_t ) - sizeof( kmalloc_chunk_t );
    chunk->prev = NULL;
    chunk->next = NULL;

    block->biggest_free = chunk->size;

    return block;
}

static void* __kmalloc_from_block( kmalloc_block_t* block, uint32_t size ) {
    void* p;
    uint32_t remaining_size;
    kmalloc_chunk_t* chunk;

    chunk = block->first_chunk;

    while ( ( chunk != NULL ) &&
            ( ( chunk->type != CHUNK_FREE ) ||
              ( chunk->size < size ) ) ) {
        chunk = chunk->next;
    }

    if ( chunk == NULL ) {
        return NULL;
    }

    remaining_size = chunk->size - size - sizeof( kmalloc_chunk_t );

    chunk->type = CHUNK_ALLOCATED;
    p = ( void* )( chunk + 1 );

    if ( remaining_size > ( sizeof( kmalloc_chunk_t ) + 4 ) ) {
        kmalloc_chunk_t* new_chunk;

        chunk->size = size;

        new_chunk = ( kmalloc_chunk_t* )( ( uint8_t* )chunk + sizeof( kmalloc_chunk_t ) + size );

        new_chunk->magic = KMALLOC_CHUNK_MAGIC;
        new_chunk->type = CHUNK_FREE;
        new_chunk->size = remaining_size - sizeof( kmalloc_chunk_t );
        new_chunk->block = chunk->block;

        /* link it to the current block */

        new_chunk->prev = chunk;
        new_chunk->next = chunk->next;
        chunk->next = new_chunk;

        if ( new_chunk->next != NULL ) {
            new_chunk->next->prev = new_chunk;
        }
    }

    /* recalculate the biggest free chunk in this block */

    chunk = block->first_chunk;
    block->biggest_free = 0;

    while ( chunk != NULL ) {
        if ( ( chunk->type == CHUNK_FREE ) &&
             ( chunk->size > block->biggest_free ) ) {
            block->biggest_free = chunk->size;
        }

        chunk = chunk->next;
    }

    return p;
}

void* kmalloc( uint32_t size ) {
    void* p;
    uint32_t min_size;
    uint32_t min_pages;
    kmalloc_block_t* block;

    spinlock_disable( &kmalloc_lock );

    block = root;

    while ( block != NULL ) {
        if ( block->biggest_free >= size ) {
            goto block_found;
        }

        block = block->next;
    }

    /* create a new block */

    min_size = size + sizeof( kmalloc_block_t ) + sizeof( kmalloc_chunk_t );
    min_pages = PAGE_ALIGN( min_size ) / PAGE_SIZE;

    if ( min_pages < KMALLOC_BLOCK_SIZE ) {
        min_pages = KMALLOC_BLOCK_SIZE;
    }

    block = __kmalloc_create_block( KMALLOC_BLOCK_SIZE );

    if ( block == NULL ) {
        return NULL;
    }

    /* link the new block to the list */

    block->next = root;
    root = block;

    /* allocate the required memory from the new block */

block_found:
    p = __kmalloc_from_block( block, size );

    spinunlock_enable( &kmalloc_lock );

    return p;
}

void kfree( void* p ) {
    kmalloc_chunk_t* chunk;

    spinlock_disable( &kmalloc_lock );

    chunk = ( kmalloc_chunk_t* )( ( uint8_t* )p - sizeof( kmalloc_chunk_t ) );

    if ( chunk->magic != KMALLOC_CHUNK_MAGIC ) {
        panic( "kfree(): Tried to free an invalid memory region!\n" );
        goto out;
    }

    if ( chunk->type != CHUNK_ALLOCATED ) {
        panic( "kfree(): Tried to free a non-allocated memory region!\n" );
        goto out;
    }

    /* make the current chunk free */

    chunk->type = CHUNK_FREE;

    /* merge with the previous chunk if it is free */

    if ( ( chunk->prev != NULL ) &&
         ( chunk->prev->type == CHUNK_FREE ) ) {
        kmalloc_chunk_t* prev_chunk = chunk->prev;

        chunk->magic = 0;

        prev_chunk->size += chunk->size;
        prev_chunk->size += sizeof( kmalloc_chunk_t );

        prev_chunk->next = chunk->next;
        chunk = prev_chunk;
    }

    /* merge with the next chunk if it is free */

    if ( ( chunk->next != NULL ) &&
         ( chunk->next->type == CHUNK_FREE ) ) {
        kmalloc_chunk_t* next_chunk = chunk->next;

        next_chunk->magic = 0;

        chunk->size += next_chunk->size;
        chunk->size += sizeof( kmalloc_chunk_t );

        chunk->next = next_chunk->next;
    }

    /* update the biggest free chunk size in the current block */

    if ( chunk->size > chunk->block->biggest_free ) {
        chunk->block->biggest_free = chunk->size;
    }

out:
    spinunlock_enable( &kmalloc_lock );
}

int init_kmalloc( void ) {
    root = __kmalloc_create_block( KMALLOC_ROOT_SIZE );

    if ( root == NULL ) {
        return -ENOMEM;
    }

    return 0;
}
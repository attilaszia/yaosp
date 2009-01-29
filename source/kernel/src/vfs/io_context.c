/* I/O context
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

#include <macros.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <vfs/io_context.h>
#include <lib/string.h>

file_t* create_file( void ) {
    file_t* file;

    file = ( file_t* )kmalloc( sizeof( file_t ) );

    if ( file == NULL ) {
        return NULL;
    }

    memset( file, 0, sizeof( file_t ) );

    atomic_set( &file->ref_count, 0 );

    return file;
}

void delete_file( file_t* file ) {
    if ( file->inode != NULL ) {
        put_inode( file->inode );
        file->inode = NULL;
    }

    kfree( file );
}

int io_context_insert_file( io_context_t* io_context, file_t* file ) {
    atomic_set( &file->ref_count, 1 );

    LOCK( io_context->lock );

    do {
        file->fd = io_context->fd_counter++;

        if ( io_context->fd_counter < 0 ) {
            io_context->fd_counter = 3;
        }
    } while ( hashtable_get( &io_context->file_table, ( const void* )file->fd ) != NULL );

    hashtable_add( &io_context->file_table, ( hashitem_t* )file );

    UNLOCK( io_context->lock );

    return 0;
}

int io_context_insert_file_with_fd( io_context_t* io_context, file_t* file, int fd ) {
    int error;

    atomic_set( &file->ref_count, 1 );

    LOCK( io_context->lock );

    if ( hashtable_get( &io_context->file_table, ( const void* )fd ) != NULL ) {
        error = -EEXIST;
    } else {
        error = 0;
        file->fd = fd;

        hashtable_add( &io_context->file_table, ( hashitem_t* )file );
    }

    UNLOCK( io_context->lock );

    return error;
}

file_t* io_context_get_file( io_context_t* io_context, int fd ) {
    file_t* file;

    LOCK( io_context->lock );

    file = ( file_t* )hashtable_get( &io_context->file_table, ( const void* )fd );

    if ( file != NULL ) {
        atomic_inc( &file->ref_count );
    }

    UNLOCK( io_context->lock );

    return file;
}

void io_context_put_file( io_context_t* io_context, file_t* file ) {
    bool do_delete = false;

    ASSERT( file != NULL );

    LOCK( io_context->lock );

    ASSERT( atomic_get( &file->ref_count ) > 0 );

    if ( atomic_dec_and_test( &file->ref_count ) ) {
        hashtable_remove( &io_context->file_table, ( const void* )file->fd );
        do_delete = true;
    }

    UNLOCK( io_context->lock );

    if ( do_delete ) {
        delete_file( file );
    }
}

int io_context_file_clone_iterator( hashitem_t* item, void* data ) {
    int error;
    file_t* old_file;
    file_t* new_file;

    old_file = ( file_t* )item;

    new_file = create_file();

    if ( new_file == NULL ) {
        return 0;
    }

    new_file->type = old_file->type;
    new_file->inode = old_file->inode;
    new_file->cookie = old_file->cookie;

    atomic_inc( &new_file->inode->ref_count );

    error = io_context_insert_file_with_fd( ( io_context_t* )data, new_file, old_file->fd );

    if ( error < 0 ) {
        delete_file( new_file );
    }

    return 0;
}

io_context_t* io_context_clone( io_context_t* old_io_context ) {
    int error;
    io_context_t* new_io_context;

    new_io_context = ( io_context_t* )kmalloc( sizeof( io_context_t ) );

    if ( new_io_context == NULL ) {
        return NULL;
    }

    error = init_io_context( new_io_context );

    if ( error < 0 ) {
        kfree( new_io_context );
        return NULL;
    }

    LOCK( old_io_context->lock );

    /* Clone root and current working directory */

    new_io_context->root_directory = old_io_context->root_directory;
    atomic_inc( &new_io_context->root_directory->ref_count );

    new_io_context->current_directory = old_io_context->current_directory;
    atomic_inc( &new_io_context->current_directory->ref_count );

    /* Clone file handles */

    hashtable_iterate(
        &old_io_context->file_table,
        io_context_file_clone_iterator,
        ( void* )new_io_context
    );

    UNLOCK( old_io_context->lock );

    return new_io_context;
}

static void* file_key( hashitem_t* item ) {
    file_t* file;

    file = ( file_t* )item;

    return ( void* )file->fd;
}

static uint32_t file_hash( const void* key ) {
    return hash_number( ( uint8_t* )&key, sizeof( int ) );
}

static bool file_compare( const void* key1, const void* key2 ) {
    return key1 == key2;
}

int init_io_context( io_context_t* io_context ) {
    int error;

    /* Initialize the file table */

    error = init_hashtable(
        &io_context->file_table,
        32,
        file_key,
        file_hash,
        file_compare
    );

    io_context->fd_counter = 3;

    /* Create the I/O context lock */

    io_context->lock = create_semaphore( "I/O context lock", SEMAPHORE_BINARY, 0, 1 );

    if ( io_context->lock < 0 ) {
        destroy_hashtable( &io_context->file_table );
        return io_context->lock;
    }

    return 0;
}

void destroy_io_context( io_context_t* io_context ) {
    file_t* file;

    /* Delete all files and the hashtable */

    while ( ( file = ( file_t* )hashtable_get_first_item( &io_context->file_table ) ) != NULL ) {
        hashtable_remove( &io_context->file_table, ( const void* )file->fd );
        delete_file( file );
    }

    destroy_hashtable( &io_context->file_table );

    /* Put the inodes in the I/O context */

    put_inode( io_context->root_directory );
    put_inode( io_context->current_directory );

    /* Delete the lock of the context */

    delete_semaphore( io_context->lock );
    kfree( io_context );
}

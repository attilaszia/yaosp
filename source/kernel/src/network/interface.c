/* Network interface handling
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

#include <kernel.h>
#include <semaphore.h>
#include <errno.h>
#include <console.h>
#include <ioctl.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <network/interface.h>
#include <lib/string.h>

static uint32_t interface_counter = 0;
static hashtable_t interface_table;
static semaphore_id interface_lock;

static net_interface_t* alloc_network_interface( void ) {
    net_interface_t* interface;

    interface = ( net_interface_t* )kmalloc( sizeof( net_interface_t ) );

    if ( interface == NULL ) {
        goto error1;
    }

    memset( interface, 0, sizeof( net_interface_t ) );

    interface->input_queue = create_packet_queue();

    if ( interface->input_queue == NULL ) {
        kfree( interface );
        goto error2;
    }

    interface->device = -1;

    return interface;

error2:
    kfree( interface );

error1:
    return NULL;
}

static int insert_network_interface( net_interface_t* interface ) {
    LOCK( interface_lock );

    atomic_set( &interface->ref_count, 1 );

    do {
        snprintf( interface->name, sizeof( interface->name ), "eth%u", interface_counter );
        interface_counter++;
    } while ( hashtable_get( &interface_table, ( const void* )interface->name ) != NULL );

    hashtable_add( &interface_table, ( hashitem_t* )interface );

    UNLOCK( interface_lock );

    return 0;
}

static int start_network_interface( net_interface_t* interface ) {
    int error;

    error = ioctl( interface->device, IOCTL_NET_SET_IN_QUEUE, ( void* )interface->input_queue );

    if ( error < 0 ) {
        return error;
    }

    error = ioctl( interface->device, IOCTL_NET_START, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int create_network_interface( int device ) {
    net_interface_t* interface;

    interface = alloc_network_interface();

    if ( interface == NULL ) {
        return -ENOMEM;
    }

    interface->device = device;

    insert_network_interface( interface );
    start_network_interface( interface );

    kprintf( "Created network interface: %s\n", interface->name );

    return 0;
}

__init int create_network_interfaces( void ) {
    int dir;
    int device;
    dirent_t entry;
    char path[ 128 ];

    dir = open( "/device/network", O_RDONLY );

    if ( dir < 0 ) {
        return dir;
    }

    while ( getdents( dir, &entry, sizeof( dirent_t ) ) == 1 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        snprintf( path, sizeof( path ), "/device/network/%s", entry.name );

        device = open( path, O_RDWR );

        if ( device < 0 ) {
            continue;
        }

        create_network_interface( device );
    }

    close( dir );

    return 0;
}

static void* net_if_key( hashitem_t* item ) {
    net_interface_t* interface;

    interface = ( net_interface_t* )item;

    return ( void* )interface->name;
}

static uint32_t net_if_hash( const void* key ) {
    return hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

static bool net_if_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

__init int init_network_interfaces( void ) {
    int error;

    /* Initialize network interface table */

    error = init_hashtable(
        &interface_table,
        32,
        net_if_key,
        net_if_hash,
        net_if_compare
    );

    if ( error < 0 ) {
        return error;
    }

    interface_lock = create_semaphore( "network if lock", SEMAPHORE_BINARY, 0, 1 );

    if ( interface_lock < 0 ) {
        destroy_hashtable( &interface_table );
        return interface_lock;
    }

    return 0;
}

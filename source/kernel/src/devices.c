/* Device management
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

#include <devices.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

static hashtable_t bus_table;
static semaphore_id bus_table_lock = -1;

int register_bus_driver( const char* name, void* bus ) {
    int error = 0;
    bus_driver_t* driver;

    driver = ( bus_driver_t* )kmalloc( sizeof( bus_driver_t ) );

    if ( driver == NULL ) {
        return -ENOMEM;
    }

    driver->name = strdup( name );

    if ( driver->name == NULL ) {
        kfree( driver );
        return -ENOMEM;
    }

    driver->bus = bus;

    LOCK( bus_table_lock );

    if ( hashtable_get( &bus_table, ( const void* )name ) != NULL ) {
        error = -EINVAL;
    }

    if ( error == 0 ) {
        hashtable_add( &bus_table, ( hashitem_t* )driver );
    }

    UNLOCK( bus_table_lock );

    if ( error < 0 ) {
        kfree( driver );
    }

    return error;
}

int unregister_bus_driver( const char* name ) {
    bus_driver_t* driver;

    LOCK( bus_table_lock );

    driver = ( bus_driver_t* )hashtable_get( &bus_table, ( const void* )name );

    if ( driver != NULL ) {
        hashtable_remove( &bus_table, ( const void* )name );
    }

    UNLOCK( bus_table_lock );

    if ( driver == NULL ) {
        return -EINVAL;
    }

    kfree( driver->name );
    kfree( driver );

    return 0;
}

void* get_bus_driver( const char* name ) {
    void* bus;
    bus_driver_t* driver;

    LOCK( bus_table_lock );

    driver = ( bus_driver_t* )hashtable_get( &bus_table, ( const void* )name );

    if ( driver == NULL ) {
        bus = NULL;
    } else {
        bus = driver->bus;
    }

    UNLOCK( bus_table_lock );

    return bus;
}

static void* bus_driver_key( hashitem_t* item ) {
    bus_driver_t* driver;

    driver = ( bus_driver_t* )item;

    return ( void* )driver->name;
}

static uint32_t bus_driver_hash( const void* key ) {
    return hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

static bool bus_driver_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

__init int init_devices( void ) {
    int error;

    /* Initialize bus driver table */

    error = init_hashtable(
        &bus_table,
        32,
        bus_driver_key,
        bus_driver_hash,
        bus_driver_compare
    );

    if ( error < 0 ) {
        return error;
    }

    /* Create the bus driver table lock */

    bus_table_lock = create_semaphore( "bus_table_lock", SEMAPHORE_BINARY, 0, 1 );

    if ( bus_table_lock < 0 ) {
        return bus_table_lock;
    }

    return 0;
}

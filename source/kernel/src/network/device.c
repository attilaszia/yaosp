/* Network device handling
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <config.h>
#include <lib/string.h>

#ifdef ENABLE_NETWORK

#include <mm/kmalloc.h>
#include <network/device.h>

net_device_t* net_device_create( size_t priv_size ) {
    net_device_t* device;

    device = ( net_device_t* )kmalloc( sizeof( net_device_t ) + priv_size );

    if ( device == NULL ) {
        return NULL;
    }

    memset( device, 0, sizeof( net_device_t ) );

    device->private = ( void* )( device + 1 );

    return device;
}

int net_device_free( net_device_t* device ) {
    kfree( device );

    return 0;
}

int net_device_register( net_device_t* device ) {
    return 0;
}

int net_device_running( net_device_t* device ) {
    return 0;
}

int net_device_carrier_ok( net_device_t* device ) {
    return ( atomic_get( &device->flags ) & NETDEV_CARRIER_ON );
}

int net_device_carrier_on( net_device_t* device ) {
    atomic_or( &device->flags, NETDEV_CARRIER_ON );
    return 0;
}

int net_device_carrier_off( net_device_t* device ) {
    atomic_and( &device->flags, ~NETDEV_CARRIER_ON );
    return 0;
}

void* net_device_get_private( net_device_t* device ) {
    return device->private;
}

#endif /* ENABLE_NETWORK */

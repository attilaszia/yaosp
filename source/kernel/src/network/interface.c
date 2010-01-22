/* Network interface handling
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#ifdef ENABLE_NETWORK

#include <kernel.h>
#include <errno.h>
#include <console.h>
#include <ioctl.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <network/interface.h>
#include <network/ethernet.h>
#include <network/device.h>
#include <network/arp.h>
#include <network/ipv4.h>
#include <network/route.h>
#include <lib/string.h>

lock_id interface_mutex;
hashtable_t interface_table;

static uint32_t interface_counter = 0;

static net_interface_t* alloc_network_interface( void ) {
    net_interface_t* interface;

    interface = ( net_interface_t* )kmalloc( sizeof( net_interface_t ) );

    if ( interface == NULL ) {
        goto error1;
    }

    memset( interface, 0, sizeof( net_interface_t ) );

    packet_queue_init( &interface->input_queue );

    interface->device = -1;

    return interface;

 error1:
    return NULL;
}

static int insert_network_interface( net_interface_t* interface ) {
    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );

    atomic_set( &interface->ref_count, 1 );

    do {
        snprintf( interface->name, sizeof( interface->name ), "eth%u", interface_counter );
        interface_counter++;
    } while ( hashtable_get( &interface_table, ( const void* )interface->name ) != NULL );

    hashtable_add( &interface_table, ( hashitem_t* )interface );

    mutex_unlock( interface_mutex );

    return 0;
}

int ethernet_send_packet( net_interface_t* interface, uint8_t* hw_address, uint16_t protocol, packet_t* packet ) {
    int error;
    ethernet_header_t* eth_header;

    eth_header = ( ethernet_header_t* )packet->data;

    /* Build the ethernet header */

    eth_header->proto = htonw( protocol );

    memcpy( eth_header->dest, hw_address, ETH_ADDR_LEN );
    memcpy( eth_header->src, interface->hw_address, ETH_ADDR_LEN );

    /* Send the packet */

    error = pwrite( interface->device, packet->data, packet->size, 0 );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int network_rx_thread( void* data ) {
    packet_t* packet;
    net_interface_t* interface;
    ethernet_header_t* eth_header;

    interface = ( net_interface_t* )data;

    while ( 1 ) {
        int if_up;

        packet = packet_queue_pop_head( &interface->input_queue, INFINITE_TIMEOUT );

        if ( packet == NULL ) {
            continue;
        }

        mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );
        if_up = ( interface->flags & IFF_UP );
        mutex_unlock( interface_mutex );

        if ( !if_up ) {
            delete_packet( packet );
            continue;
        }

        eth_header = ( ethernet_header_t* )packet->data;
        packet->network_data = ( uint8_t* )( eth_header + 1 );

        switch ( ntohw( eth_header->proto ) ) {
            case ETH_P_IP :
                ipv4_input( packet );
                break;

            case ETH_P_ARP :
                arp_input( interface, packet );
                break;

            default :
                kprintf( WARNING, "net: unknown protocol: %x\n", ntohw( eth_header->proto ) );
                break;
        }
    }

    return 0;
}

static int get_interface_count( void ) {
    int count;

    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );
    count = hashtable_get_item_count( &interface_table );
    mutex_unlock( interface_mutex );

    return count;
}

typedef struct if_list_data {
    int cur_index;
    struct ifreq* conf_table;
} if_list_data_t;

static int get_if_list_helper( hashitem_t* item, void* _data ) {
    if_list_data_t* data;
    net_interface_t* interface;
    struct ifreq* req;

    data = ( if_list_data_t* )_data;
    interface = ( net_interface_t* )item;

    req = &data->conf_table[ data->cur_index++ ];

    /* Interface name */

    strncpy( req->ifr_ifrn.ifrn_name, interface->name, IFNAMSIZ );
    req->ifr_ifrn.ifrn_name[ IFNAMSIZ - 1 ] = 0;

    return 0;
}

static int get_interface_list( struct ifconf* list ) {
    if_list_data_t data;

    data.cur_index = 0;
    data.conf_table = list->ifc_ifcu.ifcu_req;

    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );
    hashtable_iterate( &interface_table, get_if_list_helper, &data );
    mutex_unlock( interface_mutex );

    return 0;
}

static int get_interface_parameter( int param, struct ifreq* req ) {
    int ret;
    net_interface_t* interface;
    struct sockaddr_in* addr;

    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );

    interface = ( net_interface_t* )hashtable_get( &interface_table, ( const void* )req->ifr_ifrn.ifrn_name );

    if ( interface == NULL ) {
        ret = -EINVAL;
        goto out;
    }

    switch ( param ) {
        case SIOCGIFADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_addr;
            memcpy( &addr->sin_addr, interface->ip_address, IPV4_ADDR_LEN );
            break;

        case SIOCGIFNETMASK :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_netmask;
            memcpy( &addr->sin_addr, interface->netmask, IPV4_ADDR_LEN );
            break;

        case SIOCGIFBRDADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_broadaddr;
            memcpy( &addr->sin_addr, interface->broadcast, IPV4_ADDR_LEN );
            break;

        case SIOCGIFHWADDR :
            memcpy( req->ifr_ifru.ifru_hwaddr.sa_data, interface->hw_address, ETH_ADDR_LEN );
            break;

        case SIOCGIFFLAGS :
            req->ifr_ifru.ifru_flags = interface->flags;
            break;

        case SIOCGIFMTU :
            req->ifr_ifru.ifru_mtu = interface->mtu;
            break;

        default :
            kprintf( ERROR, "get_interface_parameter(): invalid request: %d\n", param );
            break;
    }

    ret = 0;

 out:
    mutex_unlock( interface_mutex );

    return ret;
}

static int set_interface_parameter( int param, struct ifreq* req ) {
    int ret;
    net_interface_t* interface;
    struct sockaddr_in* addr;

    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );

    interface = ( net_interface_t* )hashtable_get( &interface_table, ( const void* )req->ifr_ifrn.ifrn_name );

    if ( interface == NULL ) {
        ret = -EINVAL;
        goto out;
    }

    switch ( param ) {
        case SIOCSIFADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_addr;
            memcpy( interface->ip_address, &addr->sin_addr, IPV4_ADDR_LEN );
            break;

        case SIOCSIFNETMASK :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_netmask;
            memcpy( interface->netmask, &addr->sin_addr, IPV4_ADDR_LEN );
            break;

        case SIOCSIFBRDADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_broadaddr;
            memcpy( interface->broadcast, &addr->sin_addr, IPV4_ADDR_LEN );
            break;

        case SIOCSIFFLAGS :
            interface->flags = req->ifr_ifru.ifru_flags;
            break;

        default :
            kprintf( ERROR, "set_interface_parameter(): invalid request: %d\n", param );
            break;
    }

    ret = 0;

 out:
    mutex_unlock( interface_mutex );

    return ret;
}

static int start_network_interface( net_interface_t* interface ) {
    int error;

    interface->rx_thread = create_kernel_thread(
        "network_rx",
        PRIORITY_NORMAL,
        network_rx_thread,
        ( void* )interface,
        0
    );

    if ( interface->rx_thread < 0 ) {
        return -1;
    }

    thread_wake_up( interface->rx_thread );

    error = ioctl( interface->device, IOCTL_NET_GET_HW_ADDRESS, ( void* )interface->hw_address );

    if ( error < 0 ) {
        return error;
    }

    error = ioctl( interface->device, IOCTL_NET_SET_IN_QUEUE, ( void* )&interface->input_queue );

    if ( error < 0 ) {
        return error;
    }

    error = ioctl( interface->device, IOCTL_NET_START, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int create_network_interface( int device ) {
    net_interface_t* interface;

    interface = alloc_network_interface();

    if ( interface == NULL ) {
        return -ENOMEM;
    }

    interface->mtu = 1500; /* todo */
    interface->device = device;

    memset( interface->ip_address, 0, IPV4_ADDR_LEN );
    memset( interface->netmask, 255, IPV4_ADDR_LEN );
    memset( interface->broadcast, 255, IPV4_ADDR_LEN );

    arp_interface_init( interface ); /* todo: error check */

    insert_network_interface( interface );
    start_network_interface( interface );

    kprintf( INFO, "Created network interface: %s\n", interface->name );

    return 0;
}

int network_interface_ioctl( int command, void* buffer, bool from_kernel ) {
    int error;

    switch ( command ) {
        case SIOCGIFCOUNT :
            error = get_interface_count();
            break;

        case SIOCGIFCONF :
            error = get_interface_list( ( struct ifconf* )buffer );
            break;

        case SIOCGIFADDR :
        case SIOCGIFNETMASK :
        case SIOCGIFHWADDR :
        case SIOCGIFMTU :
        case SIOCGIFFLAGS :
        case SIOCGIFBRDADDR :
            error = get_interface_parameter( command, ( struct ifreq* )buffer );
            break;

        case SIOCSIFADDR :
        case SIOCSIFNETMASK :
        case SIOCSIFBRDADDR :
        case SIOCSIFFLAGS :
            error = set_interface_parameter( command, ( struct ifreq* )buffer );
            break;

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static void* net_if_key( hashitem_t* item ) {
    net_interface_t* interface;

    interface = ( net_interface_t* )item;

    return ( void* )interface->name;
}

__init int init_network_interfaces( void ) {
    int error;

    /* Initialize network interface table */

    error = init_hashtable(
        &interface_table,
        32,
        net_if_key,
        hash_str,
        compare_str
    );

    if ( error < 0 ) {
        goto error1;
    }

    interface_mutex = mutex_create( "network interface mutex", MUTEX_NONE );

    if ( interface_mutex < 0 ) {
        error = interface_mutex;
        goto error2;
    }

    return 0;

 error2:
    destroy_hashtable( &interface_table );

 error1:
    return error;
}

#endif /* ENABLE_NETWORK */

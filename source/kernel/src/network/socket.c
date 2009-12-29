/* Socket handling
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

#include <config.h>

#ifdef ENABLE_NETWORK

#include <errno.h>
#include <smp.h>
#include <console.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <lock/mutex.h>
#include <vfs/vfs.h>
#include <network/socket.h>
#include <network/interface.h>
#include <network/device.h>
#include <network/tcp.h>
#include <network/udp.h>

static lock_id socket_mutex;
static ino_t socket_inode_counter = 0;
static hashtable_t socket_inode_table;

static mount_point_t* socket_mount_point;

static int socket_read_inode( void* fs_cookie, ino_t inode_number, void** node ) {
    socket_t* socket;

    mutex_lock( socket_mutex, LOCK_IGNORE_SIGNAL );
    socket = ( socket_t* )hashtable_get( &socket_inode_table, ( const void* )&inode_number );
    mutex_unlock( socket_mutex );

    *node = ( void* )socket;

    if ( socket == NULL ) {
        return -ENOINO;
    }

    return 0;
}

static int socket_write_inode( void* fs_cookie, void* node ) {
    socket_t* socket;

    socket = ( socket_t* )node;

    mutex_lock( socket_mutex, LOCK_IGNORE_SIGNAL );
    hashtable_remove( &socket_inode_table, ( const void* )&socket->inode_number );
    mutex_unlock( socket_mutex );

    switch ( socket->type ) {
        case SOCK_STREAM :
            put_tcp_endpoint( ( tcp_socket_t* )socket->data );
            break;
    }

    kfree( socket );

    return 0;
}

static int socket_close( void* fs_cookie, void* node, void* file_cookie ) {
    socket_t* socket;

    socket = ( socket_t* )node;

    if ( socket->operations->close != NULL ) {
        socket->operations->close( socket );
    }

    return 0;
}

static int socket_read( void* fs_cookie, void* node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    int error;
    socket_t* socket;
    struct msghdr msg;
    struct iovec iov;

    socket = ( socket_t* )node;

    if ( socket->operations->recvmsg == NULL ) {
        error = -ENOSYS;
        goto out;
    }

    iov.iov_base = buffer;
    iov.iov_len = size;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    error = socket->operations->recvmsg( socket, &msg, 0 );

 out:
    return error;
}

static int socket_write( void* fs_cookie, void* node, void* file_cookie, const void* buffer, off_t pos, size_t size ) {
    int error;
    socket_t* socket;
    struct msghdr msg;
    struct iovec iov;

    socket = ( socket_t* )node;

    if ( socket->operations->sendmsg == NULL ) {
        error = -ENOSYS;
        goto out;
    }

    iov.iov_base = ( void* )buffer;
    iov.iov_len = size;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    error = socket->operations->sendmsg( socket, &msg, 0 );

 out:
    return error;
}

static int socket_ioctl( void* fs_cookie, void* node, void* file_cookie, int command, void* buffer, bool from_kernel ) {
    return network_interface_ioctl( command, buffer, from_kernel );
}

static int socket_read_stat( void* fs_cookie, void* node, struct stat* stat ) {
    socket_t* socket;

    socket = ( socket_t* )node;

    stat->st_dev = ( dev_t )fs_cookie;
    stat->st_ino = socket->inode_number;

    return 0;
}

static int socket_set_flags( void* fs_cookie, void* node, void* file_cookie, int flags ) {
    int error;
    socket_t* socket;

    socket = ( socket_t* )node;

    if ( socket->operations->add_select_request != NULL ) {
        error = socket->operations->set_flags(
            socket,
            flags
        );
    } else {
        error = -ENOSYS;
    }

    return error;
}

static int socket_add_select_request( void* fs_cookie, void* node, void* file_cookie, struct select_request* request ) {
    int error;
    socket_t* socket;

    socket = ( socket_t* )node;

    if ( socket->operations->add_select_request != NULL ) {
        error = socket->operations->add_select_request(
            socket,
            request
        );
    } else {
        error = -ENOSYS;
    }

    return error;
}

static int socket_remove_select_request( void* fs_cookie, void* node, void* file_cookie, struct select_request* request ) {
    int error;
    socket_t* socket;

    socket = ( socket_t* )node;

    if ( socket->operations->remove_select_request != NULL ) {
        error = socket->operations->remove_select_request(
            socket,
            request
        );
    } else {
        error = -ENOSYS;
    }

    return error;
}

filesystem_calls_t socket_calls = {
    .probe = NULL,
    .mount = NULL,
    .unmount = NULL,
    .read_inode = socket_read_inode,
    .write_inode = socket_write_inode,
    .lookup_inode = NULL,
    .open = NULL,
    .close = socket_close,
    .free_cookie = NULL,
    .read = socket_read,
    .write = socket_write,
    .ioctl = socket_ioctl,
    .read_stat = socket_read_stat,
    .write_stat = NULL,
    .read_directory = NULL,
    .rewind_directory = NULL,
    .create = NULL,
    .unlink = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .isatty = NULL,
    .symlink = NULL,
    .readlink = NULL,
    .set_flags = socket_set_flags,
    .add_select_request = socket_add_select_request,
    .remove_select_request = socket_remove_select_request
};

static int socket_get( bool kernel, int sockfd, file_t** _file, socket_t** _socket ) {
    int error;
    file_t* file;
    socket_t* socket;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    file = io_context_get_file( io_context, sockfd );

    if ( file == NULL ) {
        error = -EBADF;
        goto error1;
    }

    socket = ( socket_t* )hashtable_get( &socket_inode_table, ( const void* )&file->inode->inode_number );

    if ( socket == NULL ) {
        error = -EINVAL;
        goto error2;
    }

    *_file = file;
    *_socket = socket;

    return 0;

 error2:
    io_context_put_file( io_context, file );

 error1:
    return error;
}

static int socket_put( bool kernel, file_t* file, socket_t* socket ) {
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    io_context_put_file( io_context, file );

    return 0;
}

static int do_socket( bool kernel, int family, int type, int protocol ) {
    int error;
    file_t* file;
    socket_t* socket;
    io_context_t* io_context;

    if ( family != AF_INET ) {
        return -EINVAL;
    }

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    socket = ( socket_t* )kmalloc( sizeof( socket_t ) );

    if ( socket == NULL ) {
        goto error1;
    }

    switch ( type ) {
        case SOCK_STREAM :
            error = tcp_create_socket( socket );
            break;

        case SOCK_DGRAM :
            error = udp_create_socket( socket );
            break;

        default :
            error = -EINVAL;
            break;
    }

    if ( error < 0 ) {
        goto error2;
    }

    socket->family = family;
    socket->type = type;

    file = create_file();

    if ( file == NULL ) {
        goto error2;
    }

    mutex_lock( socket_mutex, LOCK_IGNORE_SIGNAL );

    do {
        socket->inode_number = socket_inode_counter++;
    } while ( hashtable_get( &socket_inode_table, ( const void* )&socket->inode_number ) != NULL );

    error = hashtable_add( &socket_inode_table, ( hashitem_t* )socket );

    mutex_unlock( socket_mutex );

    if ( error < 0 ) {
        goto error3;
    }

    file->inode = get_inode( socket_mount_point, socket->inode_number );

    if ( file->inode == NULL ) {
        goto error4;
    }

    error = io_context_insert_file( io_context, file, 3 );

    if ( error < 0 ) {
        goto error4;
    }

    return error;

 error4:
    mutex_lock( socket_mutex, LOCK_IGNORE_SIGNAL );
    hashtable_remove( &socket_inode_table, ( const void* )&socket->inode_number );
    mutex_unlock( socket_mutex );

 error3:
    delete_file( file );

 error2:
    kfree( socket );

 error1:
    return -ENOMEM;
}

int sys_socket( int family, int type, int protocol ) {
    return do_socket( false, family, type, protocol );
}

int do_connect( bool kernel, int fd, struct sockaddr* address, socklen_t addrlen ) {
    int error;
    file_t* file;
    socket_t* socket;
    struct sockaddr_in* in_address;

    error = socket_get( kernel, fd, &file, &socket );

    if ( error < 0 ) {
        goto error1;
    }

    in_address = ( struct sockaddr_in* )address;

    memcpy( socket->dest_address, &in_address->sin_addr.s_addr, IPV4_ADDR_LEN );
    socket->dest_port = ntohw( in_address->sin_port );

    if ( socket->operations->connect == NULL ) {
        error = -ENOSYS;
    } else {
        error = socket->operations->connect(
            socket,
            address,
            addrlen
        );
    }

    socket_put( kernel, file, socket );

 error1:
    return error;
}

int sys_connect( int fd, struct sockaddr* address, socklen_t addrlen ) {
    return do_connect( false, fd, address, addrlen );
}

static int do_bind( bool kernel, int sockfd, struct sockaddr* addr, socklen_t addrlen ) {
    int error;
    file_t* file;
    socket_t* socket;

    error = socket_get( kernel, sockfd, &file, &socket );

    if ( error < 0 ) {
        goto error1;
    }

    if ( socket->operations->bind == NULL ) {
        error = -ENOSYS;
    } else {
        error = socket->operations->bind(
            socket,
            addr,
            addrlen
        );
    }

    socket_put( kernel, file, socket );

 error1:
    return error;
}

int sys_bind( int sockfd, struct sockaddr* addr, socklen_t addrlen ) {
    return do_bind( false, sockfd, addr, addrlen );
}

int sys_listen( int sockfd, int backlog ) {
    DEBUG_LOG( "%s()\n", __FUNCTION__ );
    return -ENOSYS;
}

int sys_accept( int sockfd, struct sockaddr* addr, socklen_t* addrlen ) {
    DEBUG_LOG( "%s()\n", __FUNCTION__ );
    return -ENOSYS;
}

static int do_getsockopt( bool kernel, int sockfd, int level, int optname, void* optval, socklen_t* optlen ) {
    int error;
    file_t* file;
    socket_t* socket;

    error = socket_get( kernel, sockfd, &file, &socket );

    if ( error < 0 ) {
        goto error1;
    }

    switch ( level ) {
        case SOL_SOCKET :
            switch ( optname ) {
                default :
                    if ( socket->operations->getsockopt != NULL ) {
                        error = socket->operations->getsockopt( socket, level, optname, optval, optlen );
                    } else {
                        error = -EINVAL;
                    }

                    break;
            }

            break;

        default :
            kprintf( WARNING, "do_getsockopt(): called on level = %d.\n", level );
            error = -EINVAL;
            break;
    }

    socket_put( kernel, file, socket );

 error1:
    return error;
}

int sys_getsockopt( int s, int level, int optname, void* optval, socklen_t* optlen ) {
    DEBUG_LOG( "%s() level=%d, optname=%d\n", __FUNCTION__, level, optname );
    return do_getsockopt( false, s, level, optname, optval, optlen );
}

static int do_setsockopt( bool kernel, int sockfd, int level, int optname, void* optval, socklen_t optlen ) {
    int error;
    file_t* file;
    socket_t* socket;

    error = socket_get( kernel, sockfd, &file, &socket );

    if ( error < 0 ) {
        goto error1;
    }

    switch ( level ) {
        case SOL_SOCKET :
            switch ( optname ) {
                default :
                    if ( socket->operations->setsockopt != NULL ) {
                        error = socket->operations->setsockopt( socket, level, optname, optval, optlen );
                    } else {
                        error = -EINVAL;
                    }

                    break;
            }

            break;

        default :
            kprintf( WARNING, "do_setsockopt(): called on level = %d.\n", level );
            error = -EINVAL;
            break;
    }

    socket_put( kernel, file, socket );

 error1:
    return error;
}

int sys_setsockopt( int s, int level, int optname, void* optval, socklen_t optlen ) {
    DEBUG_LOG( "%s() level=%d, optname=%d\n", __FUNCTION__, level, optname );
    return do_setsockopt( false, s, level, optname, optval, optlen );
}

int sys_getsockname( int s, struct sockaddr* name, socklen_t* namelen ) {
    DEBUG_LOG( "%s()\n", __FUNCTION__ );
    return -ENOSYS;
}

int sys_getpeername( int s, struct sockaddr* name, socklen_t* namelen ) {
    DEBUG_LOG( "%s()\n", __FUNCTION__ );
    return -ENOSYS;
}

static int do_recvmsg( bool kernel, int sockfd, struct msghdr* msg, int flags ) {
    int error;
    file_t* file;
    socket_t* socket;

    error = socket_get( kernel, sockfd, &file, &socket );

    if ( error < 0 ) {
        goto error1;
    }

    if ( socket->operations->recvmsg == NULL ) {
        error = -ENOSYS;
    } else {
        error = socket->operations->recvmsg(
            socket,
            msg,
            flags
        );
    }

    socket_put( kernel, file, socket );

 error1:
    return error;
}

int sys_recvmsg( int fd, struct msghdr* msg, int flags ) {
    return do_recvmsg( false, fd, msg, flags );
}

static int do_sendmsg( bool kernel, int sockfd, struct msghdr* msg, int flags ) {
    int error;
    file_t* file;
    socket_t* socket;

    error = socket_get( kernel, sockfd, &file, &socket );

    if ( error < 0 ) {
        goto error1;
    }

    if ( socket->operations->sendmsg == NULL ) {
        error = -ENOSYS;
    } else {
        error = socket->operations->sendmsg(
            socket,
            msg,
            flags
        );
    }

    socket_put( kernel, file, socket );

 error1:
    return error;
}

int sys_sendmsg( int fd, struct msghdr* msg, int flags ) {
    return do_sendmsg( false, fd, msg, flags );
}

static void* socket_key( hashitem_t* item ) {
    socket_t* socket;

    socket = ( socket_t* )item;

    return ( void* )&socket->inode_number;
}

__init int init_socket( void ) {
    int error;

    socket_mutex = mutex_create( "socket table mutex", MUTEX_NONE );

    if ( socket_mutex < 0 ) {
        error = socket_mutex;
        goto error1;
    }

    error = init_hashtable(
        &socket_inode_table, 64,
        socket_key, hash_int64,
        compare_int64
    );

    if ( error < 0 ) {
        goto error2;
    }

    socket_mount_point = create_mount_point(
        &socket_calls,
        64, 16, 32, 0
    );

    if ( socket_mount_point == NULL ) {
        error = -ENOMEM;
        goto error3;
    }

    error = insert_mount_point( socket_mount_point );

    if ( error < 0 ) {
        goto error4;
    }

    return 0;

 error4:
    delete_mount_point( socket_mount_point );

 error3:
    destroy_hashtable( &socket_inode_table );

 error2:
    mutex_destroy( socket_mutex );

 error1:
    return error;
}

#endif /* ENABLE_NETWORK */

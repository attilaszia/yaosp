/* GUI server
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <yaosp/thread.h>
#include <yaosp/debug.h>

#include <window.h>
#include <windowdecorator.h>
#include <windowmanager.h>

#define MAX_WINDOW_BUFSIZE 8192

typedef struct r_buf_header {
    ipc_port_id reply_port;
} __attribute__(( packed )) r_buf_header_t;

extern window_decorator_t* window_decorator;

static int window_do_render( window_t* window, uint8_t* buffer, int size ) {
    r_buf_header_t* header;

    header = ( r_buf_header_t* )buffer;

    /* Tell the window that rendering is done */

    send_ipc_message( header->reply_port, 0, NULL, 0 );

    return 0;
}

static int window_thread( void* arg ) {
    int error;
    uint32_t code;
    void* buffer;
    window_t* window;

    window = ( window_t* )arg;

    buffer = malloc( MAX_WINDOW_BUFSIZE );

    if ( buffer == NULL ) {
        return -ENOMEM;
    }

    while ( 1 ) {
        error = recv_ipc_message( window->server_port, &code, buffer, MAX_WINDOW_BUFSIZE, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            dbprintf( "window_thread(): Failed to receive message: %d\n", error );
            break;
        }

        switch ( code ) {
            case MSG_RENDER_COMMANDS :
                window_do_render( window, ( uint8_t* )buffer, error );
                break;

            case MSG_SHOW_WINDOW :
                wm_register_window( window );
                break;

            default :
                dbprintf( "window_thread(): Received unknown message: %x\n", code );
                break;
        }
    }

    free( buffer );

    return 0;
}

int handle_create_window( msg_create_win_t* request ) {
    int error;
    int width;
    int height;
    window_t* window;
    thread_id thread;
    msg_create_win_reply_t reply;

    window = ( window_t* )malloc( sizeof( window_t ) );

    if ( window == NULL ) {
        goto error1;
    }

    window->title = strdup( ( const char* )( request + 1 ) );

    if ( window->title == NULL ) {
        goto error2;
    }

    window->server_port = create_ipc_port();

    if ( window->server_port < 0 ) {
        goto error3;
    }

    error = init_region( &window->visible_regions );

    if ( error < 0 ) {
        goto error4;
    }

    window->client_port = request->client_port;
    window->flags = request->flags;
    window->is_moving = 0;
    window->mouse_on_decorator = 0;

    rect_init(
        &window->screen_rect,
        request->position.x,
        request->position.y,
        request->position.x + request->size.x + window_decorator->border_size.x - 1,
        request->position.y + request->size.y + window_decorator->border_size.y - 1
    );
    rect_init(
        &window->client_rect,
        request->position.x + window_decorator->lefttop_offset.x,
        request->position.y + window_decorator->lefttop_offset.y,
        request->position.x + window_decorator->lefttop_offset.x + request->size.x - 1,
        request->position.y + window_decorator->lefttop_offset.y + request->size.y - 1
    );

    rect_bounds( &window->screen_rect, &width, &height );

    window->bitmap = create_bitmap( width, height, CS_RGB32 );

    if ( window->bitmap == NULL ) {
        goto error5;
    }

    error = window_decorator->initialize( window );

    if ( error < 0 ) {
        goto error6;
    }

    window_decorator->calculate_regions( window );

    thread = create_thread(
        "window",
        PRIORITY_DISPLAY,
        window_thread,
        window,
        0
    );

    if ( thread < 0 ) {
        goto error7;
    }

    wake_up_thread( thread );

    reply.server_port = window->server_port;

    goto out;

error7:
    window_decorator->destroy( window );

error6:
    put_bitmap( window->bitmap );

error5:
    destroy_region( &window->visible_regions );

error4:
    /* TODO: Delete server port */

error3:
    free( window->title );

error2:
    free( window );

error1:
    reply.server_port = -1;

out:
    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_win_reply_t ) );

    return 0;
}

int window_mouse_entered( window_t* window, point_t* mouse_position ) {
    point_t window_position;

    point_sub_xy_n( &window_position, mouse_position, window->screen_rect.left, window->screen_rect.top );

    window->mouse_on_decorator = window_decorator->border_has_position( window, mouse_position );

    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_entered( window, mouse_position );
    } else {
        /* TODO: Tell the window, the mouse entered */
    }

    return 0;
}

int window_mouse_exited( window_t* window ) {
    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_exited( window );
    } else {
        /* TODO: Tell the window, the mouse is exited */
    }

    return 0;
}

int window_mouse_moved( window_t* window, point_t* mouse_position ) {
    int on_decorator;

    on_decorator = window_decorator->border_has_position( window, mouse_position );

    if ( on_decorator ) {
        if ( window->mouse_on_decorator ) {
            window_decorator->mouse_moved( window, mouse_position );
        } else {
            window_decorator->mouse_entered( window, mouse_position );
        }
    } else {
        if ( window->mouse_on_decorator ) {
            /* TODO: Tell the window, the mouse is exited */

            window_decorator->mouse_exited( window );
        } else {
            /* TODO: Tell the window, the mouse is moved */
        }
    }

    window->mouse_on_decorator = on_decorator;

    return 0;
}

int window_mouse_pressed( window_t* window, int button ) {
    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_pressed( window, button );
    }

    return 0;
}

int window_mouse_released( window_t* window, int button ) {
    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_released( window, button );
    }

    return 0;
}

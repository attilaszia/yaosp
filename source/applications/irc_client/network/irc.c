/* IRC client
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <yaosp/debug.h>

#include "../core/event.h"
#include "../core/eventmanager.h"
#include "../ui/view.h"
#include "../ui/ui.h"
#include "irc.h"

char* my_nick;

static int s;
static event_t irc_read;

static size_t input_size = 0;
static char* input_buffer = NULL;

static void parse_line1(const char* line){
    char tmp[256];
    char* cmd = NULL;
    char* param1 = NULL;
    char* param2 = NULL;

    cmd = strchr( line, ' ' );
    if(cmd != NULL){ /* found command */
        *cmd++ = '\0';

        param1 = strchr( cmd, ' ' );

        if(param1 != NULL){ /* found param1 */
            *param1++ = '\0';

            /* Look for a one-parameter command */
            if(strcmp(cmd, "whatever") == 0){
//                irc_handle_whatever(param1);
            }else{ /* Command not found */
                param2 = strchr( param1, ' ' );

                if(param2 != NULL){ /* found param2 */
                *param2++ = '\0';

                /* Look for a two-parameter command */
                if(strcmp(cmd, "PRIVMSG") == 0){
                    irc_handle_privmsg(line, param1, param2 + 1);
                }
                    
                }
            }
        }
    }

    snprintf(tmp, 256, "line1 sender='%s' cmd='%s', param1='%s', param2='%s'", line, cmd, param1, param2);
    ui_debug_message( tmp );

}

static void parse_line2(const char* line){
    char tmp[256];
    char* params;

    params = strchr( line, ' ' );
    if(params != NULL){
        *params++ = '\0';
    }

    snprintf(tmp, 256, "line2 cmd='%s' params='%s'", line, params);
    ui_debug_message( tmp );
}


static void irc_handle_line( char* line ) {
    char tmp[ 256 ];

    snprintf(tmp, 256, "<< %s", line);
    ui_debug_message( tmp );

    if(line[0] == 0) {
        return;
    } else if(line[0] == ':') {
        parse_line1(++line);
    } else {
        parse_line2(line);
    }
    return;
}


int irc_handle_privmsg( const char* sender, const char* chan, const char* msg){

    char buf[ 256 ];
    view_t* channel;
    struct client _sender;
    int error;

    char timestamp[ 128 ];
    struct tm* tmval;
    time_t now;
    char* timestamp_format = "%D %T"; /* TODO: make global variable */

    error = parse_client(sender, &_sender);

    if(error < 0){
        return error;
    }

    channel = ui_get_channel( chan );

    if ( channel == NULL ) {
        return -1;
    }

    /* Create timestamp */
    time( &now );

    if( now != (time_t) -1 ){
        tmval = ( struct tm* )malloc( sizeof( struct tm ) );
        gmtime_r( &now, tmval );

        if ( tmval != NULL ) {
            strftime( ( char* )timestamp, 128, timestamp_format, tmval );
        }
    free( tmval );
    }

    snprintf( buf, sizeof( buf ), "%s <%s> %s", timestamp, _sender.nick, msg );

    view_add_text( channel, buf );

    return 0;
}

static int irc_handle_incoming( event_t* event ) {
    int size;
    char buffer[ 512 ];

    size = read( s, buffer, sizeof( buffer ) - 1 );

    if ( size > 0 ) {
        char* tmp;
        char* start;
        size_t length;

        buffer[ size ] = 0;

        tmp = ( char* )malloc( input_size + size + 1 );

        if ( tmp != NULL ) {
            if ( input_size > 0 ) {
                memcpy( tmp, input_buffer, input_size );
            }

            if ( input_buffer != NULL ) {
                free( input_buffer );
            }

            input_buffer = tmp;
        }

        memcpy( input_buffer + input_size, buffer, size );

        input_size += size;
        tmp[ input_size ] = 0;

        start = input_buffer;
        tmp = strstr( start, "\r\n" );

        while ( tmp != NULL ) {
            *tmp = 0;

            irc_handle_line( start );

            start = tmp + 2;
            tmp = strstr( start, "\r\n" );
        }

        if ( start > input_buffer ) {
            length = strlen( start );

            if ( start == 0 ) {
                free( input_buffer );
            } else {
                memmove( input_buffer, start, length );
                input_buffer = ( char* )realloc( input_buffer, length );
            }

            input_size = length;
        }
    }

    return 0;
}

int irc_join_channel( const char* channel ) {
    char buf[ 128 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "JOIN %s\r\n", channel );

    irc_write( s, buf, length );

    return 0;
}

int irc_part_channel( const char* channel, const char* message ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "PART %s :%s\r\n", channel, message );

    irc_write( s, buf, length );

    return 0;
}

int irc_send_privmsg( const char* channel, const char* message ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "PRIVMSG %s :%s\r\n", channel, message );

    irc_write( s, buf, length );

    return 0;
}

int irc_raw_command( const char* command ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "%s\r\n", command );

    irc_write( s, buf, length );

    return 0;
}

int init_irc( void ) {
    int error;
    char buffer[ 256 ];

    struct sockaddr_in address;

    s = socket( AF_INET, SOCK_STREAM, 0 );

    if ( s < 0 ) {
        return s;
    }

    address.sin_family = AF_INET;
    inet_aton( "157.181.1.129", &address.sin_addr ); /* elte.irc.hu */
    address.sin_port = htons( 6667 );

    error = connect( s, ( struct sockaddr* )&address, sizeof( struct sockaddr_in ) );

    if ( error < 0 ) {
        return error;
    }

    /* TODO: parameterize */
    snprintf( buffer, sizeof( buffer ), "NICK %s\r\nUSER %s SERVER \"elte.irc.hu\" :yaOSp IRC client\r\n", my_nick, my_nick );
    irc_write( s, buffer, strlen( buffer ) );

    /* TODO: handle "nickname already in use" */

    irc_read.fd = s;
    irc_read.events[ EVENT_READ ].interested = 1;
    irc_read.events[ EVENT_READ ].callback = irc_handle_incoming;
    irc_read.events[ EVENT_WRITE ].interested = 0;
    irc_read.events[ EVENT_EXCEPT ].interested = 0;

    event_manager_add_event( &irc_read );

    return 0;
}

int irc_quit_server( const char* reason ) {
    char buf[ 256 ];
    size_t length;

    if ( reason == NULL ) {
        length = snprintf( buf, sizeof( buf ), "QUIT :Leaving...\r\n" );
    } else {
        length = snprintf( buf, sizeof( buf ), "QUIT :%s\r\n", reason );
    }

    irc_write( s, buf, length );

    return 0;
}

ssize_t irc_write(int fd, const void *buf, size_t count) {
    char tmp[256];

    /* NOTE: size of buf unknown, '\0' is delimiter */
    snprintf(tmp, 256, ">> %s", (char*) buf);
    return write(fd, buf, count);
}

int parse_client(const char* str, client_t* sender){
    char* tmp;
    
    tmp = strchr(str, '!');
    if(tmp != NULL){
        strncpy(sender->nick, str, tmp - str);
        sender->ident[0] = '\0';
        sender->host[0] = '\0';
    }

    return 0;
}

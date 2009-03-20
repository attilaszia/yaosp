/* IRC client
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

#include <unistd.h>
#include <ncurses.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "view.h"
#include "channel_view.h"
#include "../core/event.h"
#include "../core/eventmanager.h"
#include "../network/irc.h"

#define MAX_INPUT_SIZE 255

static WINDOW* screen;
static WINDOW* win_title;
static WINDOW* win_main;
static WINDOW* win_status;
static WINDOW* win_input;

static event_t stdin_event;

static int cur_input_size = 0;
static char input_line[ MAX_INPUT_SIZE + 1 ];

int screen_w = 0;
int screen_h = 0;
view_t* active_view;

static array_t channel_list;

int ui_activate_view( view_t* view );

static int get_terminal_size( int* width, int* height ) {
    int error;
    struct winsize size;

    error = ioctl( STDIN_FILENO, TIOCGWINSZ, &size );

    if ( error >= 0 ) {
        *width = size.ws_col;
        *height = size.ws_row;
    }

    return error;
}

view_t* ui_get_channel( const char* chan_name ) {
    int i;
    int count;
    view_t* view;
    channel_data_t* channel;

    count = array_get_size( &channel_list );

    for ( i = 0; i < count; i++ ) {
        view = ( view_t* )array_get_item( &channel_list, i );
        channel = ( channel_data_t* )view->data;

        if ( strcmp( channel->name, chan_name ) == 0 ) {
            return view;
        }
    }

    return NULL;
}

int ui_handle_command( const char* command, const char* params ) {
    if ( strcmp( command, "/quit" ) == 0 ) {
        event_manager_quit();
    } else if ( strcmp( command, "/join" ) == 0 ) {
        if ( params != NULL ) {
            view_t* view;

            view = create_channel_view( params );
            array_add_item( &channel_list, ( void* )view );

            irc_join_channel( params );

            ui_activate_view( view );
        }
    } else if ( strcmp( command, "/privmsg" ) == 0 ) {
        if ( params != NULL ) {
            char* msg = strchr( params, ' ' );

            if ( msg != NULL ) {
                *msg++ = 0;

                irc_send_privmsg( params, msg );
            }
        }
    }

    return 0;
}

static int ui_stdin_event( event_t* event ) {
    int c;

    while ( ( c = getch() ) != EOF ) {
        switch ( c ) {
            case '\n' : {
                if ( cur_input_size == 0 ) {
                    break;
                }

                if ( input_line[ 0 ] == '/' ) {
                    char* params;

                    params = strchr( input_line, ' ' );

                    if ( params != NULL ) {
                        *params++ = 0;
                    }

                    if ( active_view->operations->handle_command != NULL ) {
                        active_view->operations->handle_command( active_view, input_line, params );
                    } else {
                        ui_handle_command( input_line, params );
                    }
                } else {
                    if ( active_view->operations->handle_text != NULL ) {
                        active_view->operations->handle_text( active_view, input_line );
                    }
                }

                cur_input_size = 0;
                input_line[ 0 ] = 0;

                break;
            }

            case '\b' :
                if ( cur_input_size > 0 ) {
                    input_line[ --cur_input_size ] = 0;
                }

                break;

            default :
                if ( cur_input_size < MAX_INPUT_SIZE ) {
                    input_line[ cur_input_size++ ] = c;
                    input_line[ cur_input_size ] = 0;
                }

                break;
        }

        wclear( win_input );
        mvwprintw( win_input, 0, 0, "%s", input_line );
        wrefresh( win_input );
    }

    return 0;
}

void ui_draw_view( view_t* view ) {
    int i;
    int start_line;
    char* line;

    start_line = array_get_size( &view->lines ) - ( screen_h - 3 );

    if ( start_line < 0 ) {
        start_line = 0;
    }

    wclear( win_main );

    for ( i = start_line; i < array_get_size( &view->lines ); i++ ) {
        line = ( char* )array_get_item( &view->lines, i );

        mvwprintw( win_main, i - start_line, 0, "%s", line );
    }

    wrefresh( win_main );
}

int ui_activate_view( view_t* view ) {
    int x;
    size_t length;
    const char* title;

    active_view = view;

    /* Update the title bar */

    wclear( win_title );

    title = view->operations->get_title( view );
    length = strlen( title );

    x = ( screen_w - length ) / 2;

    if ( x < 0 ) {
        x = 0;
    }

    mvwprintw( win_title, 0, x, "%s", title );
    wrefresh( win_title );

    /* Update the main view */

    ui_draw_view( view );

    return 0;
}

int init_ui( void ) {
    int error;

    /* Initialize channel list */

    error = init_array( &channel_list );

    if ( error < 0 ) {
        return error;
    }

    /* Initialize ncurses */

    screen = initscr();
    noecho();
    cbreak();
    nodelay( screen, TRUE );
    refresh();

    /* Get the terminal size */

    get_terminal_size( &screen_w, &screen_h );

    /* Create the windows */

    win_title = newwin( 1, screen_w, 0, 0 );
    win_main = newwin( screen_h - 3, screen_w, 1, 0 );
    win_status = newwin( 1, screen_w, screen_h - 2, 0 );
    win_input = newwin( 1, screen_w, screen_h - 1, 0 );

    refresh();

    /* Initialize our views */

    init_server_view();
    ui_activate_view( &server_view );

    /* Register an event for stdin */

    stdin_event.fd = STDIN_FILENO;
    stdin_event.events[ EVENT_READ ].interested = 1;
    stdin_event.events[ EVENT_READ ].callback = ui_stdin_event;
    stdin_event.events[ EVENT_WRITE ].interested = 0;
    stdin_event.events[ EVENT_EXCEPT ].interested = 0;

    event_manager_add_event( &stdin_event );

    return 0;
}

int destroy_ui( void ) {
    return 0;
}

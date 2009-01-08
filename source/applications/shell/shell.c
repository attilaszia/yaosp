/* Shell application
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

#include <stdio.h>
#include <unistd.h>

#include <yaosp/debug.h>

static char line[ 255 ];

static void get_line( void ) {
    int c;
    int pos = 0;
    int done = 0;

    while ( !done ) {
        c = fgetc( stdin );

        switch ( c ) {
            case '\n' :
                done = 1;
                break;

            case '\b' :
                if ( pos > 0 ) {
                    pos--;
                }

                break;

            default :
                if ( pos < 254 ) {
                    line[ pos++ ] = c;
                }

                break;
        }
    }

    line[ pos ] = 0;
}

int main( int argc, char** argv ) {
    while ( 1 ) {
        fputs( "> ", stdout );
        get_line();
    }

    return 0;
}

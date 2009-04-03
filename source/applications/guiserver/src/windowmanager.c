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

#include <yutil/array.h>

#include <windowmanager.h>
#include <graphicsdriver.h>
#include <windowdecorator.h>

static array_t window_stack;

window_decorator_t* window_decorator = &default_decorator;

int wm_register_window( window_t* window ) {
    int i;
    int j;
    int size;
    int error;
    rect_t winrect;
    point_t lefttop;
    window_t* tmp;
    window_t* tmp2;

    /* Insert the new window to the window stack */

    error = array_insert_item( &window_stack, 0, window );

    if ( error < 0 ) {
        return error;
    }

    /* Regenerate visible regions of other windows */

    size = array_get_size( &window_stack );

    for ( i = 1; i < size; i++ ) {
        tmp = ( window_t* )array_get_item( &window_stack, i );

        region_clear( &tmp->visible_regions );
        region_add( &tmp->visible_regions, &tmp->screen_rect );

        for ( j = i - 1; j >= 0; j-- ) {
            tmp2 = ( window_t* )array_get_item( &window_stack, j );

            region_exclude( &tmp->visible_regions, &tmp2->screen_rect );
        }
    }

    window_decorator->update_border( window );

    rect_lefttop( &window->screen_rect, &lefttop );

    memcpy( &winrect, &window->screen_rect, sizeof( rect_t ) );
    rect_sub_point( &winrect, &lefttop );

    graphics_driver->blit_bitmap(
        screen_bitmap,
        &lefttop,
        window->bitmap,
        &winrect,
        DM_COPY
    );

    return 0;
}

int init_windowmanager( void ) {
    int error;

    error = init_array( &window_stack );

    if ( error < 0 ) {
        return error;
    }

    array_set_realloc_size( &window_stack, 32 );

    return 0;
}

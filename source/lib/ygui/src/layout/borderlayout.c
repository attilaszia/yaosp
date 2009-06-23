/* yaosp GUI library
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
#include <sys/param.h>

#include <ygui/layout/borderlayout.h>

#include "../internal.h"

enum {
    W_PAGE_START,
    W_PAGE_END,
    W_LINE_START,
    W_LINE_END,
    W_CENTER,
    W_COUNT
};

static void borderlayout_do_page_start( widget_t* widget, point_t* panel_position,
                                        point_t* panel_size, point_t* center_size ) {
    point_t widget_size;
    point_t widget_position;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        MIN( panel_size->x, maximum_size.x ),
        preferred_size.y
    );

    point_init(
        &widget_position,
        panel_position->x + ( panel_size->x - widget_size.x ) / 2,
        panel_position->y
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_size->y -= widget_size.y;
}

static void borderlayout_do_page_end( widget_t* widget, point_t* panel_position,
                                        point_t* panel_size, point_t* center_size ) {
    point_t widget_size;
    point_t widget_position;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        MIN( panel_size->x, maximum_size.x ),
        preferred_size.y
    );

    point_init(
        &widget_position,
        panel_position->x + ( panel_size->x - widget_size.x ) / 2,
        panel_position->y + panel_size->y - ( widget_size.y + 1 )
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_size->y -= widget_size.y;
}

static int borderlayout_do_layout( widget_t* widget ) {
    int i;
    int count;
    point_t center_size;
    point_t panel_size;
    point_t panel_position;
    widget_t* widget_table[ W_COUNT ] = { NULL, NULL, NULL, NULL, NULL };

    widget_get_size( widget, &panel_size );
    widget_get_position( widget, &panel_position );

    count = array_get_size( &widget->children );

    for ( i = 0; i < count; i++ ) {
        widget_wrapper_t* wrapper;

        wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, i );

        switch ( ( int )wrapper->data ) {
            case ( int )BRD_PAGE_START :
                widget_table[ W_PAGE_START ] = wrapper->widget;
                break;

            case ( int )BRD_PAGE_END :
                widget_table[ W_PAGE_END ] = wrapper->widget;
                break;

            case ( int )BRD_LINE_START :
                widget_table[ W_LINE_START ] = wrapper->widget;
                break;

            case ( int )BRD_LINE_END :
                widget_table[ W_LINE_END ] = wrapper->widget;
                break;

            case ( int )BRD_CENTER :
                widget_table[ W_CENTER ] = wrapper->widget;
                break;
        }
    }

    memcpy( &center_size, &panel_size, sizeof( point_t ) );

    if ( widget_table[ W_PAGE_START ] != NULL ) {
        borderlayout_do_page_start( widget_table[ W_PAGE_START ], &panel_position, &panel_size, &center_size );
    }

    if ( widget_table[ W_PAGE_END ] != NULL ) {
        borderlayout_do_page_end( widget_table[ W_PAGE_END ], &panel_position, &panel_size, &center_size );
    }

    return 0;
}

static layout_operations_t borderlayout_ops = {
    .do_layout = borderlayout_do_layout
};

layout_t* create_border_layout( void ) {
    layout_t* layout;

    layout = create_layout( &borderlayout_ops );

    if ( layout == NULL ) {
        goto error1;
    }

    return layout;

 error1:
    return NULL;
}

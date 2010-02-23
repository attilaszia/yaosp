/* yaosp GUI library
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

#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>

#include <ygui/panel.h>
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

typedef struct borderlayout {
    int spacing;
} borderlayout_t;

static void borderlayout_do_page_start( borderlayout_t* borderlayout, widget_t* widget, point_t* panel_size,
                                        point_t* center_position, point_t* center_size ) {
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
        ( panel_size->x - widget_size.x ) / 2,
        0
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_position->y = widget_size.y + borderlayout->spacing;
    center_size->y -= ( widget_size.y + borderlayout->spacing );
}

static void borderlayout_do_page_end( borderlayout_t* borderlayout, widget_t* widget,
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
        ( panel_size->x - widget_size.x ) / 2,
        panel_size->y - widget_size.y
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_size->y -= ( widget_size.y + borderlayout->spacing );
}

static void borderlayout_do_line_start( borderlayout_t* borderlayout, widget_t* widget, point_t* panel_size,
                                        point_t* center_position, point_t* center_size ) {
    point_t widget_position;
    point_t widget_size;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        preferred_size.x,
        MIN( center_size->y, maximum_size.y )
    );

    point_init(
        &widget_position,
        0,
        center_position->y + ( center_size->y - widget_size.y ) / 2
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_position->x += widget_size.x + borderlayout->spacing;
    center_size->x -= ( widget_size.x + borderlayout->spacing );
}

static void borderlayout_do_line_end( borderlayout_t* borderlayout, widget_t* widget, point_t* panel_size,
                                      point_t* center_position, point_t* center_size ) {
    point_t widget_position;
    point_t widget_size;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        preferred_size.x,
        MIN( center_size->y, maximum_size.y )
    );

    point_init(
        &widget_position,
        panel_size->x - widget_size.x,
        center_position->y + ( center_size->y - widget_size.y ) / 2
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_size->x -= ( widget_size.x + borderlayout->spacing );
}

static void borderlayout_do_center( widget_t* widget, point_t* panel_size,
                                    point_t* center_position, point_t* center_size ) {
    point_t tmp;
    point_t max_size;

    if ( center_size->y <= 0 ) {
        return;
    }

    widget_get_maximum_size( widget, &max_size );
    point_min( &max_size, center_size );

    point_sub_n( &tmp, center_size, &max_size );
    point_div( &tmp, 2 );
    point_add( center_position, &tmp );

    widget_set_position_and_size(
        widget,
        center_position,
        &max_size
    );
}

static int borderlayout_fill_widget_table( widget_t* widget, widget_t** table ) {
    int i;
    int count;

    count = array_get_size( &widget->children );

    for ( i = 0; i < count; i++ ) {
        widget_wrapper_t* wrapper;

        wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, i );

        switch ( ( int )wrapper->data ) {
            case ( int )BRD_PAGE_START :
                table[ W_PAGE_START ] = wrapper->widget;
                break;

            case ( int )BRD_PAGE_END :
                table[ W_PAGE_END ] = wrapper->widget;
                break;

            case ( int )BRD_LINE_START :
                table[ W_LINE_START ] = wrapper->widget;
                break;

            case ( int )BRD_LINE_END :
                table[ W_LINE_END ] = wrapper->widget;
                break;

            case ( int )BRD_CENTER :
                table[ W_CENTER ] = wrapper->widget;
                break;
        }
    }

    return 0;
}

static int borderlayout_do_layout( widget_t* widget ) {
    rect_t tmp;
    point_t center_size;
    point_t center_position;
    point_t panel_size;
    layout_t* layout;
    borderlayout_t* borderlayout;
    widget_t* widget_table[ W_COUNT ] = { NULL, NULL, NULL, NULL, NULL };

    layout = panel_get_layout(widget);
    borderlayout = (borderlayout_t*)layout_get_data(layout);

    widget_get_bounds( widget, &tmp );
    panel_size.x = rect_width( &tmp );
    panel_size.y = rect_height( &tmp );

    borderlayout_fill_widget_table( widget, widget_table );

    point_init( &center_position, 0, 0 );
    point_copy( &center_size, &panel_size );

    /* Page start widget */

    if ( widget_table[W_PAGE_START] != NULL ) {
        borderlayout_do_page_start(
            borderlayout, widget_table[W_PAGE_START], &panel_size,
            &center_position, &center_size
        );
    }

    /* Page end widget */

    if ( widget_table[W_PAGE_END] != NULL ) {
        borderlayout_do_page_end(
            borderlayout, widget_table[W_PAGE_END],
            &panel_size, &center_size
        );
    }

    /* Line start widget */

    if ( widget_table[W_LINE_START] != NULL ) {
        borderlayout_do_line_start(
            borderlayout, widget_table[W_LINE_START], &panel_size,
            &center_position, &center_size
        );
    }

    /* Line end widget */

    if ( widget_table[W_LINE_END] != NULL ) {
        borderlayout_do_line_end(
            borderlayout, widget_table[W_LINE_END], &panel_size,
            &center_position, &center_size
        );
    }

    /* Center widget */

    if ( widget_table[W_CENTER] != NULL ) {
        borderlayout_do_center(
            widget_table[W_CENTER], &panel_size,
            &center_position, &center_size
        );
    }

    layout_dec_ref(layout);

    return 0;
}

static int borderlayout_get_preferred_size( widget_t* widget, point_t* size ) {
    int i;
    layout_t* layout;
    borderlayout_t* borderlayout;
    point_t pref_size[ W_COUNT ];
    widget_t* widget_table[ W_COUNT ] = { NULL, NULL, NULL, NULL, NULL };

    layout = panel_get_layout(widget);
    borderlayout = (borderlayout_t*)layout_get_data(layout);

    borderlayout_fill_widget_table( widget, widget_table );

    for ( i = 0; i < W_COUNT; i++ ) {
        if ( widget_table[i] == NULL ) {
            point_init( &pref_size[i], 0, 0 );
        } else {
            widget_get_preferred_size( widget_table[i], &pref_size[i] );
        }
    }

    size->x = pref_size[ W_LINE_START ].x +
              pref_size[ W_CENTER ].x +
              pref_size[ W_LINE_END ].x;

    /* Add X spacing */

    if ( widget_table[W_CENTER] == NULL ) {
        if ( widget_table[W_LINE_START] != NULL &&
             widget_table[W_LINE_END] != NULL ) {
            size->x += borderlayout->spacing;
        }
    } else {
        if ( widget_table[W_LINE_START] != NULL ) {
            size->x += borderlayout->spacing;
        }

        if ( widget_table[W_LINE_END] != NULL ) {
            size->x += borderlayout->spacing;
        }
    }

    size->y = pref_size[ W_PAGE_START ].y +
              pref_size[ W_CENTER ].y +
              pref_size[ W_PAGE_END ].y;
    size->y = MAX( size->y, pref_size[ W_PAGE_START ].y +
                            pref_size[ W_LINE_START ].y +
                            pref_size[ W_PAGE_END ].y );
    size->y = MAX( size->y, pref_size[ W_PAGE_START ].y +
                            pref_size[ W_LINE_END ].y +
                            pref_size[ W_PAGE_END ].y );

    /* Add Y spacing */

    int has_mid_line = ( widget_table[W_LINE_START] != NULL ||
                         widget_table[W_CENTER] != NULL ||
                         widget_table[W_LINE_END] != NULL );

    if ( has_mid_line ) {
        if ( widget_table[W_PAGE_START] != NULL ) {
            size->y += borderlayout->spacing;
        }

        if ( widget_table[W_PAGE_END] != NULL ) {
            size->y += borderlayout->spacing;
        }
    } else {
        if ( widget_table[W_PAGE_START] != NULL &&
             widget_table[W_PAGE_END] != NULL ) {
            size->y += borderlayout->spacing;
        }
    }

    layout_dec_ref(layout);

    return 0;
}

static layout_operations_t borderlayout_ops = {
    .do_layout = borderlayout_do_layout,
    .get_preferred_size = borderlayout_get_preferred_size
};

layout_t* create_borderlayout( void ) {
    layout_t* layout;
    borderlayout_t* borderlayout;

    layout = create_layout( L_BORDER, &borderlayout_ops, sizeof(borderlayout_t) );

    if ( layout == NULL ) {
        return NULL;
    }

    borderlayout = ( borderlayout_t* )layout_get_data( layout );

    borderlayout->spacing = 0;

    return layout;
}

int borderlayout_set_spacing( layout_t* layout, int spacing ) {
    borderlayout_t* borderlayout;

    if ( layout_get_type( layout ) != L_BORDER ) {
        return -EINVAL;
    }

    borderlayout = ( borderlayout_t* )layout_get_data( layout );
    borderlayout->spacing = spacing;

    return 0;
}

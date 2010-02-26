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

#include <ygui/panel.h>

typedef struct panel {
    layout_t* layout;
    color_t background;
} panel_t;

static int panel_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    panel_t* panel;

    panel = widget_get_data( widget );
    widget_get_bounds( widget, &bounds );

    gc_set_pen_color( gc, &panel->background );
    gc_fill_rect( gc, &bounds );

    return 0;
}

static int panel_get_preferred_size( widget_t* widget, point_t* size ) {
    panel_t* panel;

    panel = ( panel_t* )widget_get_data( widget );

    if ( ( panel->layout == NULL ) ||
         ( panel->layout->ops->get_preferred_size == NULL ) ) {
        point_init( size, 0, 0 );
    } else {
        panel->layout->ops->get_preferred_size( widget, size );
    }

    return 0;
}

static int panel_do_validate( widget_t* widget ) {
    panel_t* panel;

    panel = ( panel_t* )widget_get_data( widget );

    if ( panel->layout != NULL ) {
        panel->layout->ops->do_layout( widget );
    }

    return 0;
}

static int panel_destroy( widget_t* widget ) {
    panel_t* panel;

    panel = ( panel_t* )widget_get_data( widget );

    if ( panel->layout != NULL ) {
        layout_dec_ref( panel->layout );
    }

    free( panel );

    return 0;
}

static widget_operations_t panel_ops = {
    .paint = panel_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = panel_get_preferred_size,
    .get_maximum_size = NULL,
    .get_viewport = NULL,
    .do_validate = panel_do_validate,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL,
    .destroy = panel_destroy
};

layout_t* panel_get_layout( widget_t* widget ) {
    panel_t* panel;
    layout_t* layout;

    if ( ( widget == NULL ) ||
         ( widget_get_id(widget) != W_PANEL ) ) {
        return NULL;
    }

    panel = ( panel_t* )widget_get_data( widget );

    layout = panel->layout;

    if ( layout != NULL ) {
        layout_inc_ref(layout);
    }

    return layout;
}

int panel_set_layout( widget_t* widget, layout_t* layout ) {
    panel_t* panel;

    if ( ( widget == NULL ) ||
         ( layout == NULL ) ||
         ( widget_get_id(widget) != W_PANEL ) ) {
        return -EINVAL;
    }

    panel = ( panel_t* )widget_get_data( widget );

    if ( panel->layout != NULL ) {
        layout_dec_ref( panel->layout );
    }

    panel->layout = layout;

    if ( panel->layout != NULL ) {
        layout_inc_ref( panel->layout );
    }

    return 0;
}

int panel_set_background_color( widget_t* widget, color_t* color ) {
    panel_t* panel;

    if ( ( widget == NULL ) ||
         ( color == NULL ) ) {
        return -EINVAL;
    }

    if ( widget_get_id( widget ) != W_PANEL ) {
        return -EINVAL;
    }

    panel = ( panel_t* )widget_get_data( widget );

    color_copy( &panel->background, color );

    return 0;
}

widget_t* create_panel( void ) {
    return create_panel_with_layout(NULL);
}

widget_t* create_panel_with_layout( layout_t* layout ) {
    panel_t* panel;
    widget_t* widget;

    panel = ( panel_t* )malloc( sizeof( panel_t ) );

    if ( panel == NULL ) {
        goto error1;
    }

    widget = create_widget( W_PANEL, WIDGET_NONE, &panel_ops, ( void* )panel );

    if ( widget == NULL ) {
        goto error2;
    }

    panel->layout = layout;

    if ( panel->layout != NULL ) {
        layout_inc_ref(panel->layout);
    }

    color_init( &panel->background, 216, 216, 216, 255 );

    return widget;

 error2:
    free( panel );

 error1:
    return NULL;
}

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
#include <errno.h>

#include <ygui/panel.h>
#include <ygui/layout/layout.h>

static color_t panel_bg = { 0x11, 0x22, 0x33, 0xFF };

typedef struct panel {
    layout_t* layout;
} panel_t;

static int panel_paint( widget_t* widget ) {
    rect_t bounds;

    widget_get_bounds( widget, &bounds );

    widget_set_pen_color( widget, &panel_bg );
    widget_fill_rect( widget, &bounds );

    return 0;
}

static widget_operations_t panel_ops = {
    .paint = panel_paint
};

int panel_set_layout( widget_t* widget, layout_t* layout ) {
    panel_t* panel;

    if ( widget_get_id( widget ) != W_PANEL ) {
        return -EINVAL;
    }

    panel = ( panel_t* )widget_get_data( widget );

    if ( panel->layout != NULL ) {
        layout_dec_ref( panel->layout );
    }

    panel->layout = layout;

    layout_inc_ref( panel->layout );

    return 0;
}

widget_t* create_panel( void ) {
    panel_t* panel;
    widget_t* widget;

    panel = ( panel_t* )malloc( sizeof( panel_t ) );

    if ( panel == NULL ) {
        goto error1;
    }

    widget = create_widget( W_PANEL, &panel_ops, ( void* )panel );

    if ( widget == NULL ) {
        goto error2;
    }

    panel->layout = NULL;

    return widget;

error2:
    free( panel );

error1:
    return NULL;
}
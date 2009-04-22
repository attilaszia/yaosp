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
#include <string.h>

#include <ygui/label.h>
#include <ygui/font.h>
#include <ygui/yconstants.h>

typedef struct label {
    char* text;
    font_t* font;
    h_alignment_t h_align;
    v_alignment_t v_align;
} label_t;

static int label_paint( widget_t* widget ) {
    rect_t bounds;
    label_t* label;
    point_t text_position;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 216, 216, 216, 0xFF };

    label = ( label_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    widget_set_pen_color( widget, &bg_color );
    widget_fill_rect( widget, &bounds );

    switch ( label->h_align ) {
        case H_ALIGN_LEFT :
            text_position.x = 0;
            break;

        case H_ALIGN_CENTER : {
            int text_width;

            text_width = font_get_string_width( label->font, label->text, -1 );

            text_position.x = ( rect_width( &bounds ) - text_width ) / 2;

            if ( text_position.x < 0 ) {
                text_position.x = 0;
            }

            break;
        }

        case H_ALIGN_RIGHT : {
            int text_width;

            text_width = font_get_string_width( label->font, label->text, -1 );

            text_position.x = bounds.right - text_width + 1;

            if ( text_position.x < 0 ) {
                text_position.x = 0;
            }

            break;
        }
    }

    switch ( label->v_align ) {
        case V_ALIGN_TOP :
            text_position.y = font_get_ascender( label->font );
            break;

        case V_ALIGN_CENTER :
            text_position.y = bounds.bottom -
                ( ( rect_height( &bounds ) - ( font_get_ascender( label->font ) - font_get_descender( label->font ) ) ) / 2 - font_get_descender( label->font ) );
            break;

        case V_ALIGN_BOTTOM :
            text_position.y = bounds.bottom - ( font_get_line_gap( label->font ) - font_get_descender( label->font ) );
            break;
    }

    widget_set_pen_color( widget, &fg_color );
    widget_set_font( widget, label->font );
    widget_draw_text( widget, &text_position, label->text, -1 );

    return 0;
}

static widget_operations_t label_ops = {
    .paint = label_paint
};

widget_t* create_label( const char* text ) {
    label_t* label;
    widget_t* widget;
    font_properties_t properties;

    label = ( label_t* )malloc( sizeof( label_t ) );

    if ( label == NULL ) {
        goto error1;
    }

    label->text = strdup( text );

    if ( label->text == NULL ) {
        goto error2;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    label->font = create_font( "DejaVu Sans", "Book", &properties );

    if ( label->font == NULL ) {
        goto error3;
    }

    widget = create_widget( W_LABEL, &label_ops, label );

    if ( widget == NULL ) {
        goto error4;
    }

    label->h_align = H_ALIGN_CENTER;
    label->v_align = V_ALIGN_CENTER;

    return widget;

error4:
    /* TODO: free the font */

error3:
    free( label->text );

error2:
    free( label );

error1:
    return NULL;
}
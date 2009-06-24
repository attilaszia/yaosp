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
#include <string.h>
#include <assert.h>

#include <ygui/widget.h>
#include <ygui/protocol.h>

#include "internal.h"

int widget_add( widget_t* parent, widget_t* child, void* data ) {
    widget_wrapper_t* wrapper;

    wrapper = ( widget_wrapper_t* )malloc( sizeof( widget_wrapper_t ) );

    if ( wrapper == NULL ) {
        return -ENOMEM;
    }

    widget_inc_ref( child );

    wrapper->widget = child;
    wrapper->data = data;

    array_add_item( &parent->children, ( void* )wrapper );

    if ( parent->window != NULL ) {
        widget_set_window( child, parent->window );
    }

    return 0;
}

int widget_get_id( widget_t* widget ) {
    return widget->id;
}

void* widget_get_data( widget_t* widget ) {
    return widget->data;
}

int widget_get_child_count( widget_t* widget ) {
    return array_get_size( &widget->children );
}

widget_t* widget_get_child( widget_t* widget, int index ) {
    widget_wrapper_t* wrapper;

    if ( ( index < 0 ) ||
         ( index >= array_get_size( &widget->children ) ) ) {
        return NULL;
    }

    wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, index );

    return wrapper->widget;
}

int widget_get_bounds( widget_t* widget, rect_t* bounds ) {
    rect_init(
        bounds,
        0,
        0,
        widget->full_size.x - 1,
        widget->full_size.y - 1
    );

    return 0;
}

int widget_get_minimum_size( widget_t* widget, point_t* size ) {
    if ( widget->ops->get_minimum_size == NULL ) {
        point_init(
            size,
            0,
            0
        );
    } else {
        widget->ops->get_minimum_size( widget, size );
    }

    return 0;
}

int widget_get_preferred_size( widget_t* widget, point_t* size ) {
    if ( widget->is_pref_size_set ) {
        point_copy( size, &widget->preferred_size );
    } else if ( widget->ops->get_preferred_size != NULL ) {
        widget->ops->get_preferred_size( widget, size );
    } else {
        point_init(
            size,
            0,
            0
        );
    }

    return 0;
}

int widget_get_maximum_size( widget_t* widget, point_t* size ) {
    if ( widget->ops->get_maximum_size == NULL ) {
        point_init(
            size,
            INT_MAX,
            INT_MAX
        );
    } else {
        widget->ops->get_maximum_size( widget, size );
    }

    return 0;
}

int widget_set_window( widget_t* widget, struct window* window ) {
    int i;
    int size;
    widget_t* child;

    widget->window = window;

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        child = ( ( widget_wrapper_t* )array_get_item( &widget->children, i ) )->widget;

        widget_set_window( child, window );
    }

    return 0;
}

int widget_set_position_and_size( widget_t* widget, point_t* position, point_t* size ) {
    point_copy( &widget->position, position );
    point_copy( &widget->full_size, size );
    point_copy( &widget->visible_size, size );

    widget_invalidate( widget, 0 );

    return 0;
}

int widget_set_preferred_size( widget_t* widget, point_t* size ) {
    point_copy( &widget->preferred_size, size );
    widget->is_pref_size_set = 1;

    return 0;
}

int widget_inc_ref( widget_t* widget ) {
    assert( widget->ref_count > 0 );

    widget->ref_count++;

    return 0;
}

int widget_dec_ref( widget_t* widget ) {
    assert( widget->ref_count > 0 );

    if ( --widget->ref_count == 0 ) {
        /* TODO: free the widget! */
    }

    return 0;
}

int widget_paint( widget_t* widget, gc_t* gc ) {
    int i;
    int size;
    rect_t res_area;
    widget_t* child;

    /* Calculate the restricted area of the current widget */

    rect_init(
        &res_area,
        0,
        0,
        widget->visible_size.x - 1,
        widget->visible_size.y - 1
    );
    rect_add_point( &res_area, &gc->lefttop );
    rect_add_point( &res_area, &widget->scroll_offset );
    rect_and( &res_area, gc_current_restricted_area( gc ) );

    /* If the restricted area is not valid, then we can make sure that
       none of the child widgets will be visible, so painting can be
       terminated here! */

    if ( !rect_is_valid( &res_area ) ) {
        return 0;
    }

    /* Push the restricted area */

    gc_push_restricted_area( gc, &res_area );

    /* Repaint the widget if it has a valid
       paint method and the widget is invalid */

    if ( !widget->is_valid ) {
        /* Validate the widget */

        if ( widget->ops->do_validate != NULL ) {
            widget->ops->do_validate( widget );
        }

        /* Paint the widget */

        if ( widget->ops->paint != NULL ) {
            gc_push_translate_checkpoint( gc );

            widget->ops->paint( widget, gc );

            gc_rollback_translate( gc );
        }

        /* The widget is valid now :) */

        widget->is_valid = 1;
    }

    /* Call paint on the children */

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        child = ( ( widget_wrapper_t* )array_get_item( &widget->children, i ) )->widget;

        gc_push_translate_checkpoint( gc );
        gc_translate( gc, &child->position );

        widget_paint( child, gc );

        gc_rollback_translate( gc );
    }

    /* Pop the restricted area */

    gc_pop_restricted_area( gc );

    return 0;
}

int widget_invalidate( widget_t* widget, int notify_window ) {
    int i;
    int size;
    widget_t* tmp;

    widget->is_valid = 0;

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        tmp = ( ( widget_wrapper_t* )array_get_item( &widget->children, i ) )->widget;

        widget_invalidate( tmp, 0 );
    }

    if ( ( notify_window ) &&
         ( widget->window != NULL ) ) {
        send_ipc_message( widget->window->client_port, MSG_WIDGET_INVALIDATED, NULL, 0 );
    }

    return 0;
}

int widget_key_pressed( widget_t* widget, int key ) {
    if ( widget->ops->key_pressed == NULL ) {
        return 0;
    }

    return widget->ops->key_pressed( widget, key );
}

int widget_key_released( widget_t* widget, int key ) {
    if ( widget->ops->key_released == NULL ) {
        return 0;
    }

    return widget->ops->key_released( widget, key );
}

int widget_mouse_entered( widget_t* widget, point_t* position ) {
    if ( widget->ops->mouse_entered == NULL ) {
        return 0;
    }

    return widget->ops->mouse_entered( widget, position );
}

int widget_mouse_exited( widget_t* widget ) {
    if ( widget->ops->mouse_exited == NULL ) {
        return 0;
    }

    return widget->ops->mouse_exited( widget );
}

int widget_mouse_moved( widget_t* widget, point_t* position ) {
    if ( widget->ops->mouse_moved == NULL ) {
        return 0;
    }

    return widget->ops->mouse_moved( widget, position );
}

int widget_mouse_pressed( widget_t* widget, point_t* position, int mouse_button ) {
    if ( widget->ops->mouse_pressed == NULL ) {
        return 0;
    }

    return widget->ops->mouse_pressed( widget, position, mouse_button );
}

int widget_mouse_released( widget_t* widget, int mouse_button ) {
    if ( widget->ops->mouse_released == NULL ) {
        return 0;
    }

    return widget->ops->mouse_released( widget, mouse_button );
}

static int widget_find_event_handler( widget_t* widget, const char* name, int* pos ) {
    int first;
    int last;
    int mid;
    int result;
    event_entry_t* tmp;

    first = 0;
    last = array_get_size( &widget->event_handlers ) - 1;

    while ( first <= last ) {
        mid = ( first + last ) / 2;
        tmp = ( event_entry_t* )array_get_item( &widget->event_handlers, mid );

        result = strcmp( name, tmp->name );

        if ( result < 0 ) {
            last = mid - 1;
        } else if ( result > 0 ) {
            first = mid + 1;
        } else {
            if ( pos != NULL ) {
                *pos = mid;
            }

            return 0;
        }
    }

    if ( pos != NULL ) {
        *pos = first;
    }

    return -ENOENT;
}

int widget_connect_event_handler( widget_t* widget, const char* event_name, event_callback_t* callback, void* data ) {
    int pos;
    int error;
    event_entry_t* entry;

    error = widget_find_event_handler( widget, event_name, &pos );

    if ( error < 0 ) {
        return error;
    }

    entry = ( event_entry_t* )array_get_item( &widget->event_handlers, pos );

    entry->callback = callback;
    entry->data = data;

    return 0;
}

int widget_signal_event_handler( widget_t* widget, int event_handler ) {
    event_entry_t* entry;

    if ( ( event_handler < 0 ) ||
         ( event_handler >= array_get_size( &widget->event_handlers ) ) ) {
        return -EINVAL;
    }

    entry = ( event_entry_t* )array_get_item( &widget->event_handlers, event_handler );

    if ( entry->callback != NULL ) {
        entry->callback( widget, entry->data );
    }

    return 0;
}

widget_t* create_widget( int id, widget_operations_t* ops, void* data ) {
    int error;
    widget_t* widget;

    widget = ( widget_t* )malloc( sizeof( widget_t ) );

    if ( widget == NULL ) {
        goto error1;
    }

    memset( widget, 0, sizeof( widget_t ) );

    error = init_array( &widget->children );

    if ( error < 0 ) {
        goto error2;
    }

    error = init_array( &widget->event_handlers );

    if ( error < 0 ) {
        goto error3;
    }

    array_set_realloc_size( &widget->children, 8 );

    /* Add widget related events */

    event_type_t widget_events[] = {
        { "preferred-size-changed", &widget->event_ids[ E_PREF_SIZE_CHANGED ] }
    };

    error = widget_add_events( widget, widget_events, widget->event_ids, E_WIDGET_COUNT );

    if ( error < 0 ) {
        goto error4;
    }

    widget->id = id;
    widget->data = data;
    widget->ref_count = 1;

    widget->ops = ops;
    widget->window = NULL;
    widget->is_valid = 0;

    return widget;

 error4:
    destroy_array( &widget->event_handlers );

 error3:
    destroy_array( &widget->children );

 error2:
    free( widget );

 error1:
    return NULL;
}

int widget_add_events( widget_t* widget, event_type_t* event_types, int* event_indexes, int event_count ) {
    int i;
    int pos;
    int size;
    int error;
    event_type_t* type;
    event_entry_t* entry;

    /* Insert the event types */

    for ( i = 0, type = event_types; i < event_count; i++, type++ ) {
        entry = ( event_entry_t* )malloc( sizeof( event_entry_t ) );

        if ( entry == NULL ) {
            return -ENOMEM;
        }

        entry->name = type->name;
        entry->event_id = type->event_id;
        entry->callback = NULL;

        error = widget_find_event_handler( widget, type->name, &pos );

        if ( error == 0 ) {
            return -EEXIST;
        }

        error = array_insert_item( &widget->event_handlers, pos, ( void* )entry );

        if ( error < 0 ) {
            return error;
        }
    }

    /* Calculate event handler indexes */

    size = array_get_size( &widget->event_handlers );

    for ( i = 0; i < size; i++ ) {
        entry = ( event_entry_t* )array_get_item( &widget->event_handlers, i );

        *entry->event_id = i;
    }

    return 0;
}

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

#ifndef _YGUI_INTERNAL_H_
#define _YGUI_INTERNAL_H_

#include <ygui/window.h>

typedef struct widget_wrapper {
    widget_t* widget;
    void* data;
} widget_wrapper_t;

int initialize_render_buffer( window_t* window );
int allocate_render_packet( window_t* window, size_t size, void** buffer );
int flush_render_buffer( window_t* window );

#endif /* _YGUI_INTERNAL_H_ */

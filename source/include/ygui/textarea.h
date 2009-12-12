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

#ifndef _YGUI_TEXTAREA_H_
#define _YGUI_TEXTAREA_H_

#include <ygui/widget.h>

#include <yutil/array.h>

widget_t* create_textarea( void );

int textarea_get_line_count( widget_t* widget );
char* textarea_get_line( widget_t* widget, int index );

int textarea_add_lines( widget_t* widget, array_t* lines );
int textarea_set_lines( widget_t* widget, array_t* lines );

#endif /* _YGUI_TEXTAREA_H_ */

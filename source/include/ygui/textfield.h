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

#ifndef _YAOSP_TEXTFIELD_H_
#define _YAOSP_TEXTFIELD_H_

#include <ygui/widget.h>

#define IS_TEXTFIELD(w) (widget_get_id(w) == W_TEXTFIELD)
#define TEXTFIELD(w) ( (textfield_t*)widget_get_data(w) )

char* textfield_get_text( widget_t* widget );

int textfield_set_text( widget_t* widget, char* text );

widget_t* create_textfield( void );

#endif /* _YAOSP_TEXTFIELD_H_ */

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

/*
 * This file described the protocol between the guiserver and the applications.
 */

#ifndef _YGUI_PROTOCOL_H_
#define _YGUI_PROTOCOL_H_

#include <yaosp/ipc.h>
#include <yaosp/region.h>

#include <ygui/point.h>
#include <ygui/rect.h>
#include <ygui/font.h>
#include <ygui/yconstants.h>

enum {
    MSG_CREATE_APPLICATION = 1,
    MSG_CREATE_WINDOW,
    MSG_WINDOW_SHOW,
    MSG_WINDOW_DO_SHOW,
    MSG_WINDOW_HIDE,
    MSG_WINDOW_DO_HIDE,
    MSG_WINDOW_DO_RESIZE,
    MSG_WINDOW_RESIZED,
    MSG_WINDOW_DO_MOVE,
    MSG_WINDOW_MOVED,
    MSG_WINDOW_ACTIVATED,
    MSG_WINDOW_DEACTIVATED,
    MSG_RENDER_COMMANDS,
    MSG_CREATE_FONT,
    MSG_GET_STRING_WIDTH,
    MSG_WIDGET_INVALIDATED,
    MSG_KEY_PRESSED,
    MSG_KEY_RELEASED,
    MSG_MOUSE_ENTERED,
    MSG_MOUSE_EXITED,
    MSG_MOUSE_MOVED,
    MSG_MOUSE_PRESSED,
    MSG_MOUSE_RELEASED,
    MSG_GET_DESKTOP_SIZE,
    MSG_WINDOW_CALLBACK,
    MSG_CREATE_BITMAP
};

/* MSG_CREATE_APPLICATION */

typedef struct msg_create_app {
    ipc_port_id reply_port;
    ipc_port_id client_port;
} msg_create_app_t;

typedef struct msg_create_app_reply {
    ipc_port_id server_port;
} msg_create_app_reply_t;

/* MSG_CREATE_WINDOW */

/* NOTE: The title of the window is added to the message after this structure */
typedef struct msg_create_win {
    ipc_port_id reply_port;
    ipc_port_id client_port;
    point_t position;
    point_t size;
    int flags;
} msg_create_win_t;

typedef struct msg_create_win_reply {
    ipc_port_id server_port;
} msg_create_win_reply_t;

/* MSG_CREATE_FONT */

/* NOTE: The family and style name of the font is added to the payload, after this structure */
typedef struct msg_create_font {
    ipc_port_id reply_port;
    font_properties_t properties;
} msg_create_font_t;

typedef struct msg_create_font_reply {
    int handle;
    int ascender;
    int descender;
    int line_gap;
} msg_create_font_reply_t;

/* MSG_GET_STRING_WIDTH */

typedef struct msg_get_str_width {
    ipc_port_id reply_port;
    int font_handle;
    int length;
} msg_get_str_width_t;

typedef struct msg_get_str_width_reply {
    int width;
} msg_get_str_width_reply_t;

/* MSG_KEY_* */

typedef struct msg_key_pressed {
    int key;
} msg_key_pressed_t;

typedef struct msg_key_released {
    int key;
} msg_key_released_t;

/* MSG_MOUSE_* */

typedef struct msg_mouse_entered {
    point_t mouse_position;
} msg_mouse_entered_t;

typedef struct msg_mouse_moved {
    point_t mouse_position;
} msg_mouse_moved_t;

typedef struct msg_mouse_pressed {
    point_t mouse_position;
    int button;
} msg_mouse_pressed_t;

typedef struct msg_mouse_released {
    int button;
} msg_mouse_released_t;

/* MSG_DESKTOP_GET_SIZE */

typedef struct msg_desk_get_size {
    ipc_port_id reply_port;
    int desktop;
} msg_desk_get_size_t;

typedef struct msg_desk_get_size_reply {
    point_t size;
} msg_desk_get_size_reply_t;

typedef struct msg_window_callback {
    void* callback;
    void* data;
} msg_window_callback_t;

/* MSG_CREATE_BITMAP */

typedef struct msg_create_bitmap {
    ipc_port_id reply_port;
    int width;
    int height;
    color_space_t color_space;
} msg_create_bitmap_t;

typedef struct msg_create_bmp_reply {
    int id;
    region_id bitmap_region;
} msg_create_bmp_reply_t;

/* MSG_WINDOW_DO_RESIZE */

typedef struct msg_win_do_resize {
    ipc_port_id reply_port;
    point_t size;
} msg_win_do_resize_t;

typedef struct msg_win_resized {
    point_t size;
} msg_win_resized_t;

/* MSG_WINDOW_DO_MOVE */

typedef struct msg_win_do_move {
    ipc_port_id reply_port;
    point_t position;
} msg_win_do_move_t;

typedef struct msg_win_moved {
    point_t position;
} msg_win_moved_t;

#endif /* _YGUI_PROTOCOL_H_ */

/* IRC client
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

#ifndef _CORE_EVENTMANAGER_H_
#define _CORE_EVENTMANAGER_H_

#include "event.h"

int event_init( event_t* event );

int event_manager_add_event( event_t* event );
int event_manager_remove_event( event_t* event );

int event_manager_mainloop( void );
int event_manager_quit( void );

int init_event_manager( void );

#endif /* _CORE_EVENTMANAGER_H_ */

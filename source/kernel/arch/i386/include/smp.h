/* Symmetric multi-processing
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

#ifndef _ARCH_SMP_H_
#define _ARCH_SMP_H_

#include <config.h>

#define rmb() __asm__ __volatile__( "" : : : "memory" )
#define wmb() __asm__ __volatile__( "lock ; addl $0, 0(%%esp)" : : : "memory" )

#ifdef ENABLE_SMP

extern volatile uint32_t tlb_invalidate_mask;

void processor_activated( void );

void flush_tlb_global( void );

int arch_boot_processors( void );
#else
#include <arch/cpu.h>
#define flush_tlb_global flush_tlb
#endif /* ENABLE_SMP */

#endif /* _ARCH_SMP_H_ */

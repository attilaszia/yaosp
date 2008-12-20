/* i386 architecture specific processor definitions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _ARCH_CPU_H_
#define _ARCH_CPU_H_

#define EFLAG_IF ( 1 << 9 )  /* Interrupt flag */
#define EFLAG_ID ( 1 << 21 ) /* ID flag */

#ifndef __ASSEMBLER__

#include <types.h>
#include <config.h>

typedef struct i386_cpu {
    int family;
    int model;
    int features;
} i386_cpu_t;

typedef struct i386_feature {
    int feature;
    const char* name;
} i386_feature_t;

enum {
    CPU_FEATURE_MMX = ( 1 << 0 ),
    CPU_FEATURE_SSE = ( 1 << 1 ),
    CPU_FEATURE_APIC = ( 1 << 2 ),
    CPU_FEATURE_MTRR = ( 1 << 3 ),
    CPU_FEATURE_SYSCALL = ( 1 << 4 ),
    CPU_FEATURE_TSC = ( 1 << 5 )
};

extern i386_cpu_t arch_processor_table[ MAX_CPU_COUNT ];
extern i386_feature_t i386_features[];

/**
 * Sets the value of the CR3 register in the processor.
 *
 * @param cr3 The value to set the cr3 register to
 */
void set_cr3( uint32_t cr3 );

/**
 * This will execute an endless loops that halts the processor.
 */
void halt_loop( void );

/**
 * Returns the current eflags value from the processor.
 *
 * @return The current eflags
 */
register_t get_eflags( void );
/**
 * Sets the eflags value to the processor.
 *
 * @param eflags The eflags value to set
 */
void set_eflags( register_t eflags );

/**
 * This is used during the initialization of the kernel to
 * detect the processor type and features.
 *
 * @return On success 0 is returned
 */
int detect_cpu( void );

#endif // __ASSELBLER__

#endif // _ARCH_CPU_H_
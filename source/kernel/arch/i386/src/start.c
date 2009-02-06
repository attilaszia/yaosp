/* C entry point of the i386 architecture
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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

#include <console.h>
#include <multiboot.h>
#include <kernel.h>
#include <bootmodule.h>
#include <version.h>
#include <macros.h>
#include <mm/pages.h>
#include <mm/kmalloc.h>
#include <mm/region.h>

#include <arch/screen.h>
#include <arch/gdt.h>
#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/pit.h>
#include <arch/elf32.h>
#include <arch/io.h>
#include <arch/mp.h>
#include <arch/apic.h>
#include <arch/mm/config.h>
#include <arch/mm/paging.h>

extern int __kernel_end;

static int arch_init_page_allocator( multiboot_header_t* header ) {
    int error;
    int module_count;
    uint32_t memory_size;
    uint32_t mmap_length;
    ptr_t first_free_address;
    multiboot_mmap_entry_t* mmap_entry;

    /* Calculate the address of the first usable memory page */

    first_free_address = ( ptr_t )&__kernel_end;

    ASSERT( ( first_free_address % PAGE_SIZE ) == 0 );

    module_count = get_bootmodule_count();

    if ( module_count > 0 ) {
        int j;
        ptr_t module_end;
        bootmodule_t* module;

        for ( j = 0; j < module_count; j++ ) {
            module = get_bootmodule_at( j );
            module_end = PAGE_ALIGN( ( ptr_t )module->address + module->size );

            if ( module_end > first_free_address ) {
                first_free_address = module_end;
            }
        }
    }

    /* Initialize the page allocator */

    memory_size = header->memory_upper * 1024 + 1024 * 1024;

    error = init_page_allocator( first_free_address, memory_size );

    if ( error < 0 ) {
        return error;
    }

    /* Register memory types */

    register_memory_type( MEM_COMMON, 0x100000, header->memory_upper * 1024 );
    register_memory_type( MEM_LOW, 0x0, 1024 * 1024 );

    init_page_allocator_late();

    /* Reserve the first page of the memory (realmode IVT) */

    reserve_memory_pages( 0x0, PAGE_SIZE );

    /* Reserve kernel memory pages */

    reserve_memory_pages( 0x100000, ( ptr_t )&__kernel_end - 0x100000 );

    /* Reserve bootmodule memory pages */

    if ( module_count > 0 ) {
        int j;
        bootmodule_t* module;

        for ( j = 0; j < module_count; j++ ) {
            module = get_bootmodule_at( j );

            reserve_memory_pages( ( ptr_t )module->address, PAGE_ALIGN( module->size ) );
        }
    }

    /* Reserve not usable memory pages reported by GRUB */

    mmap_length = 0;
    mmap_entry = ( multiboot_mmap_entry_t* )header->memory_map_address;

    for ( ; mmap_length < header->memory_map_length;
            mmap_length += ( mmap_entry->size + 4 ),
            mmap_entry = ( multiboot_mmap_entry_t* )( ( uint8_t* )mmap_entry + mmap_entry->size + 4 ) ) {
        if ( mmap_entry->type != 1 ) {
            uint64_t real_base;
            uint64_t real_length;

            real_base = mmap_entry->base & PAGE_MASK;
            real_length = PAGE_ALIGN( ( mmap_entry->base & ~PAGE_MASK ) + mmap_entry->length );

            /* Make sure the region is inside the available physical memory */

            if ( real_base >= memory_size ) {
                continue;
            }

            if ( real_length > ( memory_size - real_base ) ) {
                real_length = memory_size - real_base;
            }

            /* Reserve the region */

            reserve_memory_pages( real_base, real_length );
        }
    }

    return 0;
}

void arch_start( multiboot_header_t* header ) {
    int error;

    /* Initialize the screen */

    init_screen();

    kprintf(
        "Booting yaOSp %d.%d.%d built on %s %s.\n",
        KERNEL_MAJOR_VERSION,
        KERNEL_MINOR_VERSION,
        KERNEL_RELEASE_VERSION,
        build_date,
        build_time
    );

    /* Setup our own Global Descriptor Table */

    kprintf( "Initializing GDT ... " );
    init_gdt();
    kprintf( "done\n" );

    /* Initialize CPU features */

    error = detect_cpu();

    if ( error < 0 ) {
        kprintf( "Failed to detect CPU: %d\n", error );
        return;
    }

    /* Initialize interrupts */

    kprintf( "Initializing interrupts ... " );
    init_interrupts();
    kprintf( "done\n" );

    /* Calibrate the boot CPU speed */

    cpu_calibrate_speed();

    /* Initializing bootmodules */

    kprintf( "Initializing bootmodules ... " );
    init_bootmodules( header );
    kprintf( "done\n" );

    if ( get_bootmodule_count() > 0 ) {
        kprintf( "Loaded %d module(s)\n", get_bootmodule_count() );
    }

    /* Initialize page allocator */

    kprintf( "Initializing page allocator ... " );

    error = arch_init_page_allocator( header );

    if ( error < 0 ) {
        kprintf( "failed (error=%d)\n", error );
        return;
    }

    kprintf( "done\n" );
    kprintf( "Free memory: %u Kb\n", get_free_page_count() * PAGE_SIZE / 1024 );

    /* Initialize kmalloc */

    kprintf( "Initializing kmalloc ... " );

    error = init_kmalloc();

    if ( error < 0 ) {
        kprintf( "failed (error=%d)\n", error );
        return;
    }

    kprintf( "done\n" );

    /* Initialize memory region manager */

    kprintf( "Initializing region manager ... " );
    preinit_regions();
    kprintf( "done\n" );

    /* Initialize paging */

    kprintf( "Initializing paging ... " );

    error = init_paging();

    if ( error < 0 ) {
        kprintf( "failed (error=%d)\n", error );
        return;
    }

    kprintf( "done\n" );

    /* Call the architecture independent entry
       point of the kernel */

    kernel_main();
}

int arch_late_init( void ) {
    init_mp();
    init_apic();
    init_pit();
    init_apic_timer();
    init_system_time();
    init_elf32_module_loader();
    init_elf32_application_loader();

    return 0;
}

void arch_reboot( void ) {
    int i;

    disable_interrupts();

    /* Flush keyboard */
    for ( ; ( ( i = inb( 0x64 ) ) & 0x01 ) != 0 && ( i & 0x02 ) != 0 ; ) ;

    /* CPU RESET */
    outb( 0xFE, 0x64 );

    halt_loop();
}

void arch_shutdown( void ) {
    kprintf( "Shutdown complete!\n" );
    disable_interrupts();
    halt_loop();
}

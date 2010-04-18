/* 32bit ELF application loader
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#include <loader.h>
#include <errno.h>
#include <console.h>
#include <smp.h>
#include <config.h>
#include <kernel.h>
#include <macros.h>
#include <linker/elf32.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/gdt.h>
#include <arch/linker/elf32.h>
#include <arch/mm/paging.h>

static bool elf32_application_check( binary_loader_t* loader ) {
    int error;
    elf32_image_info_t info;

    if ( elf32_init_image_info( &info, 0x40000000 ) != 0 ) {
        return false;
    }

    error = elf32_load_and_validate_header( &info, loader, ELF_TYPE_EXEC );

    elf32_destroy_image_info( &info );

    return ( error == 0 );
}

static int elf32_application_load( binary_loader_t* loader ) {
    int error;
    elf32_context_t* app_context;

    app_context = ( elf32_context_t* )kmalloc( sizeof( elf32_context_t ) );

    if ( app_context == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    elf32_context_init( app_context, elf32_relocate_i386 );

    error = elf32_image_load( &app_context->main, loader, 0x40000000, ELF_APPLICATION );

    if ( error != 0 ) {
        goto error2;
    }

    error = elf32_relocate_i386( app_context, &app_context->main );

    if ( error != 0 ) {
        goto error3;
    }

    current_process()->loader_data = ( void* )app_context;

    return 0;

 error3:
    /* todo */

 error2:
    kfree( app_context );

 error1:
    return error;
}

int elf32_application_execute( void ) {
    thread_t* thread;
    registers_t* regs;
    elf32_context_t* app_context;

    /* Change the registers on the stack pushed by the syscall
       entry to return to the userspace. */

    thread = current_thread();
    app_context = ( elf32_context_t* )thread->process->loader_data;

    regs = ( registers_t* )( thread->syscall_stack );

    regs->eax = 0;
    regs->ebx = 0;
    regs->ecx = 0;
    regs->edx = 0;
    regs->esi = 0;
    regs->edi = 0;
    regs->ebp = 0;
    regs->cs = USER_CS | 3;
    regs->ds = USER_DS | 3;
    regs->es = USER_DS | 3;
    regs->fs = USER_DS | 3;
    regs->eip = app_context->main.info.header.entry;
    regs->esp = ( register_t )thread->user_stack_end;
    regs->ss = USER_DS | 3;

    return 0;
}

static int elf32_application_get_symbol_info( thread_t* thread, ptr_t address, symbol_info_t* info ) {
    elf32_context_t* app_context;

    app_context = ( elf32_context_t* )thread->process->loader_data;

    return elf32_get_symbol_info( &app_context->main.info, address, info );
}

static int elf32_application_destroy( void* data ) {
    /*elf_application_t* app;

    app = ( elf_application_t* )data;

    if ( app->text_region != NULL ) {
        memory_region_put( app->text_region );
    }

    if ( app->data_region != NULL ) {
        memory_region_put( app->data_region );
    }

    elf32_destroy_image_info( &app->image_info );
    kfree( app );*/

    // todo!

    return 0;
}

static application_loader_t elf32_application_loader = {
    .name = "ELF32",
    .check = elf32_application_check,
    .load = elf32_application_load,
    .execute = elf32_application_execute,
    .get_symbol_info = elf32_application_get_symbol_info,
    .destroy = elf32_application_destroy
};

__init int init_elf32_application_loader( void ) {
    register_application_loader( &elf32_application_loader );

    return 0;
}

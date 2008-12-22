/* 32bit ELF format handling
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

#include <module.h>
#include <console.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <mm/region.h>
#include <lib/string.h>

#include <arch/elf32.h>
#include <arch/mm/config.h>

static bool elf32_module_check( void* data, size_t size ) {
    elf_header_t* header;

    if ( size < sizeof( elf_header_t ) ) {
        return false;
    }

    header = ( elf_header_t* )data;

    if ( ( header->ident[ ID_MAGIC0 ] != ELF32_MAGIC0 ) ||
         ( header->ident[ ID_MAGIC1 ] != ELF32_MAGIC1 ) ||
         ( header->ident[ ID_MAGIC2 ] != ELF32_MAGIC2 ) ||
         ( header->ident[ ID_MAGIC3 ] != ELF32_MAGIC3 ) ) {
        return false;
    }

    if ( header->ident[ ID_CLASS ] != ELF_CLASS_32 ) {
        return false;
    }

    /* TODO: check other stuffs, such as endian and machine */

    return true;
}

static int elf32_parse_section_headers( void* data, elf_module_t* elf_module ) {
    uint32_t i;
    elf_section_header_t* dynsym_section;
    elf_section_header_t* section_header;

    dynsym_section = NULL;

    /* Loop through the section headers and save those ones we're
       interested in */

    for ( i = 0; i < elf_module->section_count; i++ ) {
        section_header = &elf_module->sections[ i ];

        switch ( section_header->type ) {
            case SECTION_DYNSYM :
                dynsym_section = section_header;
                break;
        }
    }

    /* Handle dynsym section */

    if ( dynsym_section != NULL ) {
        elf_symbol_t* symbols;
        elf_section_header_t* string_section;

        string_section = &elf_module->sections[ dynsym_section->link ];

        /* Load the string section */

        elf_module->strings = ( char* )kmalloc( string_section->size );

        if ( elf_module->strings == NULL ) {
            return -ENOMEM;
        }

        memcpy(
            elf_module->strings,
            ( char* )data + string_section->offset,
            string_section->size
        );

        /* Load symbols */

        symbols = ( elf_symbol_t* )kmalloc( dynsym_section->size );

        if ( symbols == NULL ) {
            kfree( elf_module->strings );
            elf_module->strings = NULL;
            return -ENOMEM;
        }

        memcpy(
            symbols,
            ( char* )data + dynsym_section->offset,
            dynsym_section->size
        );

        /* Build our own symbol list */

        elf_module->symbol_count = dynsym_section->size / dynsym_section->entsize;

        elf_module->symbols = ( my_elf_symbol_t* )kmalloc(
            sizeof( my_elf_symbol_t ) * elf_module->symbol_count
        );

        if ( elf_module->symbols == NULL ) {
            kfree( elf_module->strings );
            kfree( symbols );
            elf_module->strings = NULL;
            return -ENOMEM;
        }

        kprintf( "Symbol count: %d\n", elf_module->symbol_count );

        for ( i = 0; i < elf_module->symbol_count; i++ ) {
            my_elf_symbol_t* my_symbol;
            elf_symbol_t* elf_symbol;

            my_symbol = &elf_module->symbols[ i ];
            elf_symbol = &symbols[ i ];

            my_symbol->name = elf_module->strings + elf_symbol->name;
            my_symbol->address = elf_symbol->value;

            kprintf( "Symbol %d: %s (%x)\n", i, my_symbol->name, my_symbol->address );
        }
    }

    return 0;
}

static int elf32_module_map( void* data, elf_module_t* elf_module ) {
    uint32_t i;
    elf_section_header_t* section_header;

    bool text_found = false;
    uint32_t text_start;
    uint32_t text_end;
    uint32_t text_size;
    uint32_t text_offset;
    void* text_address;

    bool data_found = false;
    uint32_t data_start;
    uint32_t data_end;
    uint32_t data_size;
    uint32_t data_offset;
    void* data_address;

    uint32_t bss_end;
    uint32_t data_size_with_bss;

    for ( i = 0; i < elf_module->section_count; i++ ) {
        section_header = &elf_module->sections[ i ];

        /* Check if the current section occupies memory during execution */

        if ( section_header->flags & SF_ALLOC ) {
            if ( section_header->flags & SF_WRITE ) {
                if ( !data_found ) {
                    data_found = true;
                    data_start = section_header->address;
                    data_offset = section_header->offset;
                }

                switch ( section_header->type ) {
                    case SECTION_NOBITS :
                        bss_end = section_header->address + section_header->size - 1;
                        break;

                    default :
                        data_end = section_header->address + section_header->size - 1;
                        bss_end = data_end;
                        break;
                }
            } else {
                if ( !text_found ) {
                    text_found = true;
                    text_start = section_header->address;
                    text_offset = section_header->offset;
                }

                text_end = section_header->address + section_header->size - 1;
            }
        }
    }

    if ( ( !text_found ) || ( !data_found ) ) {
        return -EINVAL;
    }

    text_offset -= ( text_start & ~PAGE_MASK );
    text_start &= PAGE_MASK;
    text_size = text_end - text_start + 1;

    data_offset -= ( data_start & ~PAGE_MASK );
    data_start &= PAGE_MASK;
    data_size = data_end - data_start + 1;
    data_size_with_bss = bss_end - data_start + 1;

    kprintf( "text_start=0x%x text_size=0x%x\n", text_start, text_size );
    kprintf( "data_start=0x%x data_size=0x%x\n", data_start, data_size );

    elf_module->text_region = create_region(
        "ro",
        PAGE_ALIGN( text_size ),
        REGION_READ | REGION_KERNEL,
        ALLOC_PAGES,
        &text_address
    );

    kprintf( "text_region=%d text_address=0x%x\n", elf_module->text_region, text_address );

    if ( elf_module->text_region < 0 ) {
        return elf_module->text_region;
    }

    elf_module->data_region = create_region(
        "rw",
        PAGE_ALIGN( data_size_with_bss ),
        REGION_READ | REGION_WRITE | REGION_KERNEL,
        ALLOC_PAGES,
        &data_address
    );

    kprintf( "data_region=%d data_address=0x%x\n", elf_module->data_region, data_address );

    if ( elf_module->data_region < 0 ) {
        return elf_module->data_region;
    }

    /* Copy text and data in */

    kprintf( "text_offset=%d data_offset=%d\n", text_offset, data_offset );

    memcpy( text_address, ( char* )data + text_offset, text_size );
    memcpy( data_address, ( char* )data + data_offset, data_size );
    memset( ( char* )data_address + data_size, 0, data_size_with_bss - data_size );

    return 0;
}

static module_t* elf32_module_load( void* data, size_t size ) {
    int error;
    module_t* module;
    elf_module_t* elf_module;
    elf_header_t* header;

    elf_module = ( elf_module_t* )kmalloc( sizeof( elf_module_t ) );

    if ( elf_module == NULL ) {
        return NULL;
    }

    memset( elf_module, 0, sizeof( elf_module_t ) );

    header = ( elf_header_t* )data;

    if ( header->shentsize != sizeof( elf_section_header_t ) ) {
        kprintf( "ELF32: Invalid section header size!\n" );
        kfree( elf_module );
        return NULL;
    }

    /* Load section headers from the ELF file */

    elf_module->section_count = header->shnum;

    elf_module->sections = ( elf_section_header_t* )kmalloc(
        sizeof( elf_section_header_t ) * elf_module->section_count
    );

    if ( elf_module->sections == NULL ) {
        kfree( elf_module );
        return NULL;
    }

    memcpy(
        elf_module->sections,
        ( char* )data + header->shoff,
        sizeof( elf_section_header_t ) * elf_module->section_count
    );

    /* Parse section headers */

    error = elf32_parse_section_headers( data, elf_module );

    if ( error < 0 ) {
        kfree( elf_module->sections );
        kfree( elf_module );
        return NULL;
    }

    /* Map the ELF image to the kernel memory */

    error = elf32_module_map( data, elf_module );

    if ( error < 0 ) {
        /* TODO: free other stuffs */
        kfree( elf_module->sections );
        kfree( elf_module );
        return NULL;
    }

    module = create_module();

    if ( module == NULL ) {
        return NULL;
    }

    module->loader_data = ( void* )elf_module;

    return module;
}

static int elf32_module_free( module_t* module ) {
    /* TODO */

    return 0;
}

static bool elf32_module_get_symbol( module_t* module, const char* symbol_name, ptr_t* symbol_addr ) {
    uint32_t i;
    elf_module_t* elf_module;

    elf_module = ( elf_module_t* )module->loader_data;

    for ( i = 0; i < elf_module->symbol_count; i++ ) {
        if ( strcmp( elf_module->symbols[ i ].name, symbol_name ) == 0 ) {
            *symbol_addr = elf_module->symbols[ i ].address;
            return true;
        }
    }

    return false;
}

static module_loader_t elf32_module_loader = {
    .name = "ELF32",
    .check = elf32_module_check,
    .load = elf32_module_load,
    .free = elf32_module_free,
    .get_symbol = elf32_module_get_symbol
};

int init_elf32_loader( void ) {
    set_module_loader( &elf32_module_loader );

    return 0;
}

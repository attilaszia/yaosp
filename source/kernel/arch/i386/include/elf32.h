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

#ifndef _ARCH_ELF32_H_
#define _ARCH_ELF32_H_

#include <types.h>

enum {
    ID_MAGIC0 = 0,
    ID_MAGIC1,
    ID_MAGIC2,
    ID_MAGIC3,
    ID_CLASS,
    ID_DATA,
    ID_VERSION,
    ID_SIZE = 16
};

enum {
    ELF32_MAGIC0 = 0x7F,
    ELF32_MAGIC1 = 'E',
    ELF32_MAGIC2 = 'L',
    ELF32_MAGIC3 = 'F'
};

enum {
    ELF_CLASS_32 = 1
};

enum {
    SECTION_NULL = 0,
    SECTION_PROGBITS,
    SECTION_SYMTAB,
    SECTION_STRTAB,
    SECTION_RELA,
    SECTION_HASH,
    SECTION_DYNAMIC,
    SECTION_NOTE,
    SECTION_NOBITS,
    SECTION_REL,
    SECTION_SHLIB,
    SECTION_DYNSYM
};

typedef struct elf_header {
    unsigned char ident[ ID_SIZE ];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__(( packed )) elf_header_t;

typedef struct elf_section_header {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t address;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} __attribute__(( packed )) elf_section_header_t;

typedef struct elf_symbol {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
} __attribute__(( packed )) elf_symbol_t;

typedef struct my_elf_symbol {
    char* name;
    uint32_t address;
} my_elf_symbol_t;

typedef struct elf_module {
    uint32_t section_count;
    elf_section_header_t* sections;

    char* strings;

    uint32_t symbol_count;
    my_elf_symbol_t* symbols;
} elf_module_t;

int init_elf32_loader( void );

#endif // _ARCH_ELF32_H_

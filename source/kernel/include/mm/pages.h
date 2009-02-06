/* Memory page allocator
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#ifndef _MM_PAGES_H_
#define _MM_PAGES_H_

#include <multiboot.h>

#include <arch/atomic.h>

enum memory_type {
    MEM_COMMON,
    MEM_ARCH_FIRST
};

typedef struct memory_type_desc {
    bool free;
    ptr_t start;
    ptr_t size;
    ptr_t free_pages;
} memory_type_desc_t;

/**
 * @struct page
 *
 * This structure represents a page of the physical memory.
 * An array of this structure is stored after the kernel to
 * manage the status (allocated/free) of all physical memory
 * pages.
 */
typedef struct page {
    atomic_t ref_count;
} page_t;

/**
 * @struct memory_info
 *
 * This structure is used to pass memory informations to
 * userspace applications.
 */
typedef struct memory_info {
    uint32_t free_page_count;
    uint32_t total_page_count;
} memory_info_t;

/**
 * Allocates a number of physical memory pages.
 *
 * @param count The number of memory pages to allocate
 * @return In the case of failure NULL is returned, otherwise
 *         the start address of the allocated memory region
 */
void* alloc_pages( uint32_t count, int mem_type );

/**
 * Frees a previously allocated set of memory pages.
 *
 * @param address The start address of the memory region to free
 * @param count The number of the pages to free
 */
void free_pages( void* address, uint32_t count );

/**
 * Returns the number of free pages.
 *
 * @return The number of the free physical memory pages
 */
uint32_t get_free_page_count( void );

/**
 * Returns the total number of available physical memory pages
 *
 * @return The total number of memory pages
 */
uint32_t get_total_page_count( void );

int sys_get_memory_info( memory_info_t* info );

int reserve_memory_pages( ptr_t start, ptr_t size );
int register_memory_type( int mem_type, ptr_t start, ptr_t size );
int init_page_allocator( ptr_t page_map_address, uint64_t _memory_size );
int init_page_allocator_late( void );

#endif // _MM_PAGES_H_

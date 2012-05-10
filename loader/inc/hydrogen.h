/**
 * Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <stdint.h>

/**
 * Hydrogen ABI (Version 1)
 *
 * The ABI for both, the info tables that Hydrogen passes to the kernel, as
 * well as the Hydrogen header in the loaded kernel that configures how the
 * kernel should be loaded. See SPECIFICATION.mdown for a more detailed
 * description.
 * 
 * This file can be included in the kernel in order to access the generated
 * info tables easily. In order to prevent potential name clashes, the name of
 * each structure and macro in this file begins with the prefix "hy_".
 *
 * The values and definitions in this file are subject to change in further
 * versions of Hydrogen, potentially breaking backward compatibility.
 */

//-----------------------------------------------------------------------------
// Common
//-----------------------------------------------------------------------------

/**
 * The magic number for both the Hydrogen info table and the Hydrogen header ("HYDR").
 */
#define HY_MAGIC                0x52445948

//-----------------------------------------------------------------------------
// Info Table - Memory Structure
//-----------------------------------------------------------------------------

#define HY_INFO_ROOT            ((hy_info_root_t *) 0x10B000)
#define HY_INFO_CPU             ((hy_info_cpu_t *) 0x10C000)
#define HY_INFO_IOAPIC          ((hy_info_ioapic_t *) 0x10D000)
#define HY_INFO_MMAP            ((hy_info_mmap_t *) 0x10E000)
#define HY_INFO_MODULE          ((hy_info_module_t *) 0x10F000)
#define HY_INFO_STRING          ((char *) 0x110000)

//-----------------------------------------------------------------------------
// Info Table - Flags
//-----------------------------------------------------------------------------

/** CPU Flag: Set when the CPU entry represents an enabled and present CPU. */
#define HY_INFO_CPU_FLAG_PRESENT        (1 << 0)

/** CPU Flag: Set when the CPU entry represents the bootstrap processor. */
#define HY_INFO_CPU_FLAG_BSP            (1 << 1)

/** Root Flag: The system has a 8259 PIC. */
#define HY_INFO_FLAG_PCAT_COMPAT        (1 << 0)

/** IRQ Flag: The IRQ's interrupt line is active low (default: active high). */
#define HY_INFO_IRQ_FLAG_ACTIVE_LOW     (1 << 0)

/** IRQG Flag:The IRQ's interrupt line is level triggered (default: edge). */
#define HY_INFO_IRQ_FLAG_LEVEL          (1 << 1)

//-----------------------------------------------------------------------------
// Info Table - Structures
//-----------------------------------------------------------------------------

/**
 * Hydrogen root info table that contains general information about the system.
 */
typedef struct hy_info_root {
    
    uint32_t magic;             //< a magic number (HY_MAGIC)
    uint32_t flags;             //< flags
    
    uint64_t lapic_paddr;       //< physical address of the LAPIC MMIO window
    uint64_t rsdp_paddr;        //< physical address of the RSDP (ACPI)
    
    uint64_t idt_paddr;         //< physical address of the IDT
    uint64_t gdt_paddr;         //< physical address of the GDT
    uint64_t tss_paddr;         //< physical address of the TSS entries
    
    uint64_t free_paddr;        //< physical address of the first free to use byte
    
    uint32_t irq_gsi[16];       //< map of ISR IRQ numbers to GSI numbers
    uint8_t irq_flags[16];      //< flags regarding the IRQs
    
    uint16_t cpu_count_active;  //< number of active CPUs in the system
    uint16_t cpu_count;         //< number of entries in the CPU table
    uint16_t ioapic_count;      //< number of IO APICs
    uint16_t mmap_count;        //< number of entries in the memory map
    uint16_t module_count;      //< number of modules
    
} __attribute__((packed)) hy_info_root_t;

/**
 * An entry in the CPU info table which represents a single CPU in the system.
 * 
 * Without the HY_INFO_CPU_PRESENT flag being set, the CPU entry can be ignored.
 * 
 * Length: 8 bytes.
 */
typedef struct hy_info_cpu {
    uint8_t apic_id;            //< apic id of the CPU's LAPIC
    uint8_t acpi_id;            //< acpi id of the CPU
    uint16_t flags;             //< CPU flags
    uint32_t lapic_timer_freq;  //< lapic timer ticks per second
} __attribute__((packed)) hy_info_cpu_t;

/**
 * An entry in the IO APIC info table which represents a single IO APIC that
 * is installed into the system and that covers a given interval of GSIs.
 * 
 * Length: 16 bytes.
 */
typedef struct hy_info_ioapic {
    uint8_t apic_id;            //< apic id of the IO APIC
    uint8_t version;			//< version of the IO APIC
    uint32_t gsi_base;          //< lowest GSI covered by this IO APIC
    uint16_t gsi_count;         //< number of GSIs covered by this IO APIC
    uint64_t mmio_paddr;        //< physical address of IO APIC's MMIO window
} __attribute__((packed)) hy_info_ioapic_t;

/**
 * An entry in the memory map, indicating whether a region is free to use as
 * normal memory or is allocated by another device.
 * 
 * Length: 32 bytes.
 */
typedef struct hy_info_mmap {
    uint64_t address;           //< physical address the region begins on
    uint64_t length;            //< length of the region in bytes
    uint64_t available;         //< one if available, zero otherwise
    uint64_t padding;
} __attribute__((packed)) hy_info_mmap_t;

/**
 * An entry in the module list which represents a module loaded into memory.
 * 
 * Length: 16 bytes.
 */
typedef struct hy_info_module {
    uint16_t name;              //< offset of the name in the string table
    uint64_t address;           //< physical address of the module
    uint32_t length;            //< length of the module in bytes
    uint16_t padding;
} __attribute__((packed)) hy_info_module_t;

//-----------------------------------------------------------------------------
// Kernel Header - Symbol Names
//-----------------------------------------------------------------------------

/** The name of the symbol that points to the kernel header. */
#define HY_HEADER_SYMNAME   "hydrogen_header"

//-----------------------------------------------------------------------------
// Kernel Header - Flags
//-----------------------------------------------------------------------------

/** Root Flag: Instead of lowest priority delivery, route all GSIs to the BSP. */
#define HY_HEADER_FLAG_IOAPIC_BSP    (1 << 0)

/** IRQ Flag: The IRQ should be masked when the kernel is entered. */
#define HY_HEADER_IRQ_FLAG_MASK      (1 << 0)

//-----------------------------------------------------------------------------
// Kernel Header - Structures
//-----------------------------------------------------------------------------

/**
 * An entry in the IRQ array of the root header structure.
 *
 * Enables the kernel to configure masks and vectors for each IRQ individually.
 */
typedef struct hy_header_irq {
    uint8_t flags;              //< IRQ flags
    uint8_t vector;             //< IRQ vector
} __attribute__((packed)) hy_header_irq_t;

/**
 * The root structure of the kernel header.
 *
 * Enables the kernel to configure various memory mappings and entry points and
 * contains the IRQ array.
 */
typedef struct hy_header_root {
    uint32_t magic;             //< magic value
    uint32_t flags;             //< flags

    uint64_t stack_vaddr;       //< virtual address for the stack (or null)
    uint64_t info_vaddr;        //< virtual address for the info tables (or null)

    uint64_t ap_entry;          //< entry point for APs (or null)
    uint64_t syscall_entry;     //< entry point for syscalls (or null)
    uint64_t isr_entry_table;   //< ISR entry table pointer (or null)

    hy_header_irq_t irqs[16];   //< IRQ configuration
} __attribute__((packed)) hy_header_root_t;

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

#include <acpi.h>
#include <elf64.h>
#include <gdt.h>
#include <heap.h>
#include <hydrogen.h>
#include <idt.h>
#include <info.h>
#include <ioapic.h>
#include <kernel.h>
#include <lapic.h>
#include <main.h>
#include <multiboot.h>
#include <pic.h>
#include <screen.h>
#include <smp.h>
#include <stdint.h>
#include <syscall.h>

volatile uint8_t main_entry_barrier = 1;

void main_bsp(void)
{
    // Print header
    screen_write("Hydrogen v0.2b - http://github.com/farok/H2", 0, 0);
    screen_write("Copyright (c) 2012 by Lukas Heidemann", 0, 1);
    screen_write("-------------------------------------------------", 0, 2);

    // Load the IDT
    idt_load(idt_address, IDT_LENGTH);
    idt_setup_loader();

    // Initialize Hydrogen info tables and parse the multiboot tables
    info_init();
    multiboot_parse();

    // Setup the heap
    heap_init();

    // Now parse the ACPI tables and analyze the IO APICs
    acpi_parse();
    ioapic_analyze();

    // Find, check and load the kernel binary
    kernel_find();
    kernel_check();
    elf64_load(kernel_binary);
    kernel_analyze();

    // Initialize interrupt controllers
    lapic_detect();
    lapic_setup();
    ioapic_setup_loader();
    pic_setup();

    // Calibrate the LAPIC timer
    lapic_timer_calibrate();

    // Boot APs
    info_cpu[lapic_id()].flags |= HY_INFO_CPU_FLAG_BSP;
    smp_setup();

    // Setup IDT and IOAPIC according to kernel header
    idt_setup_kernel();
    ioapic_setup_kernel();

    // Setup fast syscall support
    syscall_init();

    // Setup mapping
    kernel_map_info();
    kernel_map_stack();
    kernel_map_idt();
    kernel_map_gdt();

    // Set free address
    info_root->free_paddr = (heap_top + 0xFFF) & ~0xFFF;

    // Lower main entry barrier and jump to the kernel entry point
    main_entry_barrier = 0;
    kernel_enter_bsp();
}

void main_ap(void)
{
    // Load the IDT
    idt_load((uintptr_t) &idt_data, IDT_LENGTH);

    // Enable LAPIC and calibrate the timer
    lapic_setup();
    lapic_timer_calibrate();

    // Setup stack mapping
    kernel_map_stack();

    // Setup fast syscall support
    syscall_init();

    // Signal complete AP startup
    ++smp_ready_count;

    // Wait for main entry barrier, then enter the kernel (or halt)
    while (main_entry_barrier == 1);
    kernel_enter_ap();
}

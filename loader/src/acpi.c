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
#include <hydrogen.h>
#include <info.h>
#include <screen.h>
#include <stdint.h>
#include <string.h>

static void acpi_parse_madt_lapic(acpi_madt_lapic_t *entry)
{
    hy_info_cpu_t *cpu = &info_cpu[entry->apic_id];

    cpu->acpi_id = entry->acpi_id;
    cpu->apic_id = entry->apic_id;
    cpu->flags = HY_INFO_CPU_FLAG_PRESENT;

    ++info_root->cpu_count_active;

    size_t new_count = entry->apic_id + 1;
    if (info_root->cpu_count < new_count) {
        info_root->cpu_count = new_count;
    }
}

static void acpi_parse_madt_ioapic(acpi_madt_ioapic_t *entry)
{
    hy_info_ioapic_t *ioapic = &info_ioapic[info_root->ioapic_count++];

    ioapic->apic_id = entry->apic_id;
    ioapic->mmio_paddr = entry->mmio_addr;
    ioapic->gsi_base = entry->gsi_base;
}

static void acpi_parse_madt_iso(acpi_madt_iso_t *iso)
{
    if (iso->bus != 1 || iso->irq >= 16) // ISA IRQs
        return;

    uint8_t irq = iso->irq;
    uint16_t polarity = ((iso->flags >> ACPI_MADT_ISO_POLARITY_OFFSET) & 0b11);
    uint16_t trigger = ((iso->flags >> ACPI_MADT_ISO_TRIGGER_OFFSET) & 0b11);

    info_root->irq_gsi[irq] = iso->gsi;

    if (polarity == ACPI_MADT_ISO_POLARITY_LOW) {
        info_root->irq_flags[irq] |= HY_INFO_IRQ_FLAG_ACTIVE_LOW;
    }

    if (trigger == ACPI_MADT_ISO_TRIGGER_LEVEL) {
        info_root->irq_flags[irq] |= HY_INFO_IRQ_FLAG_LEVEL;
    }
}

static void acpi_parse_madt(acpi_madt_t *madt)
{
    info_root->lapic_paddr = madt->lapic_paddr;

    if ((madt->flags & ACPI_MADT_PCAT_COMPAT) != 0) {
        info_root->flags |= HY_INFO_FLAG_PCAT_COMPAT;
    }

    acpi_madt_entry_t *entry = (acpi_madt_entry_t *) ((uintptr_t) madt + sizeof (acpi_madt_t));
    size_t size_left = madt->header.length - sizeof (acpi_madt_t);

    while (size_left > 0) {
        size_left -= entry->length;

        switch (entry->type) {
        case ACPI_MADT_TYPE_LAPIC:
            acpi_parse_madt_lapic((acpi_madt_lapic_t *) entry);
            break;

        case ACPI_MADT_TYPE_IOAPIC:
            acpi_parse_madt_ioapic((acpi_madt_ioapic_t *) entry);
            break;

        case ACPI_MADT_TYPE_ISO:
            acpi_parse_madt_iso((acpi_madt_iso_t *) entry);
            break;
        }

        entry = (acpi_madt_entry_t *) ((uintptr_t) entry + entry->length);
    }
}

static void acpi_parse_table(acpi_sdt_header_t *table)
{
    if (!acpi_check(table, table->length))
        return;

    if (memcmp(&table->signature, "MADT", 4) ||
            memcmp(&table->signature, "APIC", 4)) {

        acpi_parse_madt((acpi_madt_t *) table);
    }
}

static void acpi_parse_xsdt(acpi_sdt_header_t *xsdt)
{
    if (!acpi_check(xsdt, xsdt->length)) {
        SCREEN_PANIC("ACPI: XSDT is invalid.");
    }

    size_t count = (xsdt->length - sizeof (acpi_sdt_header_t)) / 8;
    uint64_t *entry = (uint64_t *) ((uintptr_t) xsdt + sizeof (acpi_sdt_header_t));
    size_t i;

    for (i = 0; i < count; ++i) {
        acpi_parse_table((acpi_sdt_header_t *) entry[i]);
    }
}

static void acpi_parse_rsdt(acpi_sdt_header_t *rsdt)
{
    if (!acpi_check(rsdt, rsdt->length)) {
        SCREEN_PANIC("ACPI: RSDT is invalid.");
    }

    size_t count = (rsdt->length - sizeof (acpi_sdt_header_t)) / 4;
    uint32_t *entry = (uint32_t *) ((uintptr_t) rsdt + sizeof (acpi_sdt_header_t));
    size_t i;

    for (i = 0; i < count; ++i) {
        acpi_parse_table((acpi_sdt_header_t *) (uintptr_t) entry[i]);
    }
}

static void acpi_parse_rsdp(acpi_rsdp_t *rsdp)
{
    if (rsdp->revision > 0) {
        acpi_parse_xsdt((acpi_sdt_header_t *) rsdp->xsdt_addr);
    } else {
        acpi_parse_rsdt((acpi_sdt_header_t *) (uintptr_t) rsdp->rsdt_addr);
    }
}

void acpi_parse(void)
{
    acpi_rsdp_t *rsdp = acpi_find_rsdp(0xE0000, 0x100000 - 0xE0000);

    if (0 == rsdp) {
        SCREEN_PANIC("ACPI: Could not find RSDP.");
    }

    info_root->rsdp_paddr = (uintptr_t) rsdp;
    acpi_parse_rsdp(rsdp);

    if (0 == info_root->cpu_count) {
        SCREEN_PANIC("No CPU information in ACPI tables.");
    }

    if (0 == info_root->ioapic_count) {
        SCREEN_PANIC("No I/O APIC found in ACPI tables.");
    }
}

acpi_rsdp_t *acpi_find_rsdp(uintptr_t begin, size_t length)
{
    // Search on 16 byte boundary
    uintptr_t begin_aligned = (begin + 15) & ~15;
    length -= begin_aligned - begin;
    begin = begin_aligned;

    // Search for table
    size_t offset;

    for (offset = 0; offset < length; offset += 16) {
        // Check signature
        uintptr_t address = begin + offset;
        uint64_t *signature = (uint64_t *) address;

        if (!memcmp(signature, "RSD PTR ", 8))
            continue;

        // Validate version 1.0 checksum
        // (version 1.0 part is 8 bytes long)
        acpi_rsdp_t *rsdp = (acpi_rsdp_t *) address;

        if (!acpi_check(rsdp, 20))
            continue;

        // Is version 2.0 or later?
        if (rsdp->revision >= 1) {
            // Validate version 2.0 checksum
            if (!acpi_check(rsdp, rsdp->length))
                continue;
        }

        // RSDP found
        return rsdp;
    }

    // No RSDP found
    return 0;
}

bool acpi_check(void *table, size_t length)
{
    uint8_t *bytes = (uint8_t *) table;
    uint8_t sum = 0;
    size_t i;

    for (i = 0; i < length; ++i) {
        sum += bytes[i];
    }

    return (0 == sum);
}
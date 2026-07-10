// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hygon CSV support
 *
 * This file is shared between decompression boot code and running
 * linux kernel.
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#include <linux/psp-hygon.h>

#include <asm/e820/types.h>

static u64 csv3_boot_sc_page_a __initdata = -1ul;
static u64 csv3_boot_sc_page_b __initdata = -1ul;
static u32 early_page_idx __initdata;

/**
 * csv3_scan_secure_call_pages - try to find the secure call pages.
 * @boot_params:	boot parameters where e820_table resides.
 *
 * The secure call pages are reserved by BIOS. We scan all the reserved pages
 * to check the CSV3 secure call guid bytes.
 */
void __init csv3_scan_secure_call_pages(struct boot_params *boot_params)
{
	struct boot_e820_entry *entry;
	struct csv3_secure_call_cmd *sc_page;
	u64 offset;
	u64 addr;
	u8 i;
	u8 table_num;
	int count = 0;

	if (!boot_params)
		return;

	if (csv3_boot_sc_page_a != -1ul && csv3_boot_sc_page_b != -1ul)
		return;

	table_num = min_t(u8, boot_params->e820_entries,
			  E820_MAX_ENTRIES_ZEROPAGE);
	entry = &boot_params->e820_table[0];
	for (i = 0; i < table_num; i++) {
		if (entry[i].type != E820_TYPE_RESERVED)
			continue;

		addr = entry[i].addr & PAGE_MASK;
		for (offset = 0; offset < entry[i].size; offset += PAGE_SIZE) {
			sc_page = (void *)(addr + offset);
			if (sc_page->guid_64[0] == CSV3_SECURE_CALL_GUID_LOW &&
			    sc_page->guid_64[1] == CSV3_SECURE_CALL_GUID_HIGH) {
				if (count == 0)
					csv3_boot_sc_page_a = addr + offset;
				else if (count == 1)
					csv3_boot_sc_page_b = addr + offset;
				count++;
			}
			if (count >= 2)
				return;
		}
	}
}

/**
 * csv3_early_secure_call_ident_map - issue early secure call command at the
 *			stage where identity page table is created.
 * @base_address:	Start address of the specified memory range.
 * @num_pages:		number of the specific pages.
 * @cmd_type:		Secure call cmd type.
 */
void __init csv3_early_secure_call_ident_map(u64 base_address, u64 num_pages,
					     enum csv3_secure_command_type cmd_type)
{
	struct csv3_secure_call_cmd *page_rd;
	struct csv3_secure_call_cmd *page_wr;
	u32 cmd_ack;

	if (csv3_boot_sc_page_a == -1ul || csv3_boot_sc_page_b == -1ul)
		return;

	/* identity mapping at the stage. */
	page_rd = (void *)(early_page_idx ? csv3_boot_sc_page_a : csv3_boot_sc_page_b);
	page_wr = (void *)(early_page_idx ? csv3_boot_sc_page_b : csv3_boot_sc_page_a);

	while (1) {
		page_wr->cmd_type = (u32)cmd_type;
		page_wr->nums = 1;
		page_wr->entry[0].base_address = base_address;
		page_wr->entry[0].size = num_pages << PAGE_SHIFT;

		/*
		 * Write command in page_wr must be done before retrieve cmd
		 * ack from page_rd, and it is ensured by the mb below.
		 */
		mb();

		cmd_ack = page_rd->cmd_type;
		if (cmd_ack != cmd_type)
			break;
	}
	early_page_idx ^= 1;
}

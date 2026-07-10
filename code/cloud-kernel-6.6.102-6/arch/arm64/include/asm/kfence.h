/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arm64 KFENCE support.
 *
 * Copyright (C) 2020, Google LLC.
 */

#ifndef __ASM_KFENCE_H
#define __ASM_KFENCE_H

#include <linux/kfence.h>

#include <asm/set_memory.h>

#ifdef CONFIG_KFENCE
static inline bool arch_kfence_init_pool(struct kfence_pool_area *kpa)
{
	unsigned long addr = (unsigned long)kpa->addr;

	if (!can_set_block_and_cont_map())
		return false;

	/*
	 * If the allocated range is block and contiguous mapping, split it
	 * to pte level before re-initializing kfence pages.
	 */
	split_linear_mapping_after_init(addr, kpa->pool_size, PAGE_KERNEL);

	return true;
}

static inline bool kfence_protect_page(unsigned long addr, bool protect)
{
	set_memory_valid(addr, 1, !protect);

	return true;
}

static inline bool arch_kfence_free_pool(unsigned long addr) { return false; }

extern bool kfence_early_init;
static inline bool arm64_kfence_can_set_direct_map(void)
{
	return !kfence_early_init;
}
#else /* CONFIG_KFENCE */
static inline bool arm64_kfence_can_set_direct_map(void) { return false; }
#endif /* CONFIG_KFENCE */

#endif /* __ASM_KFENCE_H */

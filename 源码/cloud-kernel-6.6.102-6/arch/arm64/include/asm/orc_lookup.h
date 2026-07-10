/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */
#ifndef _ORC_LOOKUP_H
#define _ORC_LOOKUP_H

/*
 * This is a lookup table for speeding up access to the .orc_unwind table.
 * Given an input address offset, the corresponding lookup table entry
 * specifies a subset of the .orc_unwind table to search.
 *
 * Each block represents the end of the previous range and the start of the
 * next range.  An extra block is added to give the last range an end.
 *
 * The block size should be a power of 2 to avoid a costly 'div' instruction.
 *
 * A block size of 256 was chosen because it roughly doubles unwinder
 * performance while only adding ~5% to the ORC data footprint.
 */
#define LOOKUP_BLOCK_ORDER	8
#define LOOKUP_BLOCK_SIZE	(1 << LOOKUP_BLOCK_ORDER)

#ifndef LINKER_SCRIPT

#include <asm-generic/sections.h>

extern unsigned int orc_lookup[];
extern unsigned int orc_lookup_end[];

#define LOOKUP_START_IP		(unsigned long)_stext
#define LOOKUP_STOP_IP		(unsigned long)_etext

#endif /* LINKER_SCRIPT */

#ifndef __ASSEMBLY__

#ifdef CONFIG_UNWINDER_ORC
void orc_lookup_init(void);
void orc_lookup_module_init(struct module *mod,
			    void *orc_ip, size_t orc_ip_size,
			    void *orc, size_t orc_size);
#else
static inline void orc_lookup_init(void) {}
static inline
void orc_lookup_module_init(struct module *mod,
			    void *orc_ip, size_t orc_ip_size,
			    void *orc, size_t orc_size)
{
}
#endif

struct orc_entry *arch_orc_find(unsigned long ip);

#define orc_warn(fmt, ...) \
	printk_deferred_once(KERN_WARNING "WARNING: " fmt, ##__VA_ARGS__)

#define orc_warn_current(args...)					\
({									\
	static bool dumped_before;					\
	if (state->task == current && !state->error) {			\
		orc_warn(args);						\
		if (unwind_debug && !dumped_before) {			\
			dumped_before = true;				\
			unwind_dump(state);				\
		}							\
	}								\
})

struct orc_entry *orc_find(unsigned long ip);

extern bool orc_init;
extern int __start_orc_unwind_ip[];
extern int __stop_orc_unwind_ip[];
extern struct orc_entry __start_orc_unwind[];
extern struct orc_entry __stop_orc_unwind[];

#endif /* __ASSEMBLY__ */

#endif /* _ORC_LOOKUP_H */

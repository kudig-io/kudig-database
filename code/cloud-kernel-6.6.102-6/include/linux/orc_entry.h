/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#ifndef _ORC_ENTRY_H
#define _ORC_ENTRY_H

#ifndef __ASSEMBLY__
#include <asm/byteorder.h>

/*
 * This struct is more or less a vastly simplified version of the DWARF Call
 * Frame Information standard.  It contains only the necessary parts of DWARF
 * CFI, simplified for ease of access by the in-kernel unwinder.  It tells the
 * unwinder how to find the previous SP and BP (and sometimes entry regs) on
 * the stack for a given code address.  Each instance of the struct corresponds
 * to one or more code locations.
 */
struct orc_entry {
	s16		sp_offset;
	s16		fp_offset;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned	sp_reg:4;
	unsigned	fp_reg:4;
	unsigned	type:4;
	unsigned	signal:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
	unsigned	fp_reg:4;
	unsigned	sp_reg:4;
	unsigned	unused:3;
	unsigned	signal:1;
	unsigned	type:4;
#endif
} __packed;

#endif /* __ASSEMBLY__ */

#endif /* _ORC_ENTRY_H */

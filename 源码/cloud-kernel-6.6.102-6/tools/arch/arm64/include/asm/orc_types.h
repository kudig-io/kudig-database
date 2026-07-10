/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Author: Madhavan T. Venkataraman (madvenka@linux.microsoft.com)
 *
 * Copyright (C) 2022 Microsoft Corporation
 */

#ifndef _ORC_TYPES_H
#define _ORC_TYPES_H

#include <linux/types.h>
#include <linux/compiler.h>

/*
 * The ORC_REG_* registers are base registers which are used to find other
 * registers on the stack.
 *
 * ORC_REG_PREV_SP, also known as DWARF Call Frame Address (CFA), is the
 * address of the previous frame: the caller's SP before it called the current
 * function.
 *
 * ORC_REG_UNDEFINED means the corresponding register's value didn't change in
 * the current frame.
 *
 * We only use base registers SP and FP -- which the previous SP is based on --
 * and PREV_SP and UNDEFINED -- which the previous FP is based on.
 */
#define ORC_REG_UNDEFINED		0
#define ORC_REG_PREV_SP			1
#define ORC_REG_SP			2
#define ORC_REG_FP			3
#define ORC_REG_MAX			4

#define ORC_TYPE_UNDEFINED		0
#define ORC_TYPE_END_OF_STACK		1
#define ORC_TYPE_CALL			2
#define ORC_TYPE_REGS			3
#define ORC_TYPE_REGS_PARTIAL		4

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
	s16             sp_offset;
	s16             fp_offset;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned        sp_reg:4;
	unsigned        fp_reg:4;
	unsigned        type:4;
	unsigned        signal:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
	unsigned        fp_reg:4;
	unsigned        sp_reg:4;
	unsigned        unused:3;
	unsigned        signal:1;
	unsigned        type:4;
#endif
} __packed;

#endif /* __ASSEMBLY__ */

#endif /* _ORC_TYPES_H */

/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_ARM64_UNWIND_HINTS_H
#define _ASM_ARM64_UNWIND_HINTS_H

#include <linux/objtool_types.h>

#include "orc_types.h"

#ifdef CONFIG_STACK_VALIDATION

#ifndef __ASSEMBLY__

#define UNWIND_HINT(type, sp_reg, sp_offset, signal)		\
	"987: \n\t"						\
	".pushsection .discard.unwind_hints\n\t"		\
	/* struct unwind_hint */				\
	".long 987b - .\n\t"					\
	".short " __stringify(sp_offset) "\n\t"			\
	".byte " __stringify(sp_reg) "\n\t"			\
	".byte " __stringify(type) "\n\t"			\
	".byte " __stringify(signal) "\n\t"			\
	".balign 4 \n\t"					\
	".popsection\n\t"

#else /* __ASSEMBLY__ */

/*
 * In asm, there are two kinds of code: normal C-type callable functions and
 * the rest.  The normal callable functions can be called by other code, and
 * don't do anything unusual with the stack.  Such normal callable functions
 * are annotated with the ENTRY/ENDPROC macros.  Most asm code falls in this
 * category.  In this case, no special debugging annotations are needed because
 * objtool can automatically generate the ORC data for the ORC unwinder to read
 * at runtime.
 *
 * Anything which doesn't fall into the above category, such as syscall and
 * interrupt handlers, tends to not be called directly by other functions, and
 * often does unusual non-C-function-type things with the stack pointer.  Such
 * code needs to be annotated such that objtool can understand it.  The
 * following CFI hint macros are for this type of code.
 *
 * These macros provide hints to objtool about the state of the stack at each
 * instruction.  Objtool starts from the hints and follows the code flow,
 * making automatic CFI adjustments when it sees pushes and pops, filling out
 * the debuginfo as necessary.  It will also warn if it sees any
 * inconsistencies.
 */
.macro UNWIND_HINT type:req sp_reg=0 sp_offset=0 signal=0
.Lhere_\@:
	.pushsection .discard.unwind_hints
		/* struct unwind_hint */
		.long .Lhere_\@ - .
		.short \sp_offset
		.byte \sp_reg
		.byte \type
		.byte \signal
		.balign 4
	.popsection
.endm

#endif /* __ASSEMBLY__ */

#else /* !CONFIG_STACK_VALIDATION */

#ifndef __ASSEMBLY__

#define UNWIND_HINT(type, sp_reg, sp_offset, signal) "\n\t"
#else
.macro UNWIND_HINT type:req sp_reg=0 sp_offset=0 signal=0
.endm
#endif

#endif /* CONFIG_STACK_VALIDATION */
#ifdef __ASSEMBLY__

.macro UNWIND_HINT_FTRACE, offset
	.set sp_reg, ORC_REG_SP
	.set sp_offset, \offset
	.set type, UNWIND_HINT_TYPE_FTRACE
	UNWIND_HINT type=type sp_reg=sp_reg sp_offset=sp_offset
.endm

.macro UNWIND_HINT_REGS, offset
	.set sp_reg, ORC_REG_SP
	.set sp_offset, \offset
	.set type, UNWIND_HINT_TYPE_REGS
	UNWIND_HINT type=type sp_reg=sp_reg sp_offset=sp_offset
.endm

.macro UNWIND_HINT_IRQ, offset
	.set sp_reg, ORC_REG_SP
	.set sp_offset, \offset
	.set type, UNWIND_HINT_TYPE_IRQ_STACK
	UNWIND_HINT type=type sp_reg=sp_reg sp_offset=sp_offset
.endm

#endif /* __ASSEMBLY__ */

#endif /* _ASM_ARM64_UNWIND_HINTS_H */

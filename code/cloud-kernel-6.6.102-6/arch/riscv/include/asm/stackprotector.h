/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_RISCV_STACKPROTECTOR_H
#define _ASM_RISCV_STACKPROTECTOR_H

#ifdef CONFIG_STACKPROTECTOR_PER_TASK
#include <generated/stackguard.h>
#endif

extern unsigned long __stack_chk_guard;

/*
 * Initialize the stackprotector canary value.
 *
 * NOTE: this must only be called from functions that never return,
 * and it must always be inlined.
 */
static __always_inline void boot_init_stack_canary(void)
{
	unsigned long canary = get_random_canary();

	current->stack_canary = canary;
	if (!IS_ENABLED(CONFIG_STACKPROTECTOR_PER_TASK)) {
		__stack_chk_guard = current->stack_canary;
	} else {
		/*
		 * Per-task mode with global fallback: all tasks share a global canary,
		 * due to offset overflow.
		 * Per-task TLS mode: canary accessed via tp register offset,
		 * no need to set __stack_chk_guard.
		 */
#if defined(CONFIG_STACKPROTECTOR_GUARD_GLOBAL)
		__stack_chk_guard = current->stack_canary;
#endif
	}
}
#endif /* _ASM_RISCV_STACKPROTECTOR_H */

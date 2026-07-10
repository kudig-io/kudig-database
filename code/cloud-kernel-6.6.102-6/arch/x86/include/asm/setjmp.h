/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SETJMP_H
#define _ASM_X86_SETJMP_H

#include <linux/types.h>

struct label_t {
#ifdef CONFIG_X86_32
	/* ABI (ebx, esp, ebp, esi, edi) and eip. */
	u32 regs[6];
#else
	/* ABI (rbx, rsp, rbp, r12-r15) and rip. */
	u64 regs[8];
#endif
};

int setjmp(struct label_t *label);
void longjmp(struct label_t *label, int val);

#endif /* _ASM_X86_SETJMP_H */

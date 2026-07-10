/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_ARM64_SETJMP_H
#define _ASM_ARM64_SETJMP_H

#include <linux/types.h>

struct label_t {
	/* ABI x19 .. x30 (lr), sp */
	u64 regs[13];
};

int setjmp(struct label_t *label);
void longjmp(struct label_t *label, int val);

#endif /* _ASM_ARM64_SETJMP_H */

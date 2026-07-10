/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015-2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#ifndef _OBJTOOL_ORC_H
#define _OBJTOOL_ORC_H

#include <asm/orc_types.h>

int init_orc_entry(struct orc_entry *orc, struct cfi_state *cfi,
		   struct instruction *insn);
void orc_print_dump(struct elf *dummy_elf, struct orc_entry *orc, int i);
int write_orc_entry(struct elf *elf, struct section *orc_sec,
		    struct section *ip_sec, unsigned int idx,
		    struct section *insn_sec, unsigned long insn_off,
		    struct orc_entry *o);
const char *orc_type_name(unsigned int type);
void orc_print_reg(unsigned int reg, int offset);
void orc_print_sp(void);
void orc_print_fp(void);

#endif /* _OBJTOOL_ORC_H */

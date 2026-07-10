// SPDX-License-Identifier: GPL-2.0-only
/*
 * Author: Madhavan T. Venkataraman (madvenka@linux.microsoft.com)
 *
 * Copyright (C) 2022 Microsoft Corporation
 */
#include <string.h>

#include <linux/objtool_types.h>

#include <objtool/insn.h>
#include <objtool/endianness.h>
#include <arch/orc.h>
#include <asm/orc_types.h>

void arch_init_orc_entry(struct orc_entry *entry)
{
	entry->fp_reg = ORC_REG_UNDEFINED;
	entry->type   = UNWIND_HINT_TYPE_CALL;
}

int init_orc_entry(struct orc_entry *orc, struct cfi_state *cfi,
		   struct instruction *insn)
{
	struct cfi_reg *fp = &cfi->regs[CFI_FP];

	memset(orc, 0, sizeof(*orc));

	orc->sp_reg = ORC_REG_SP;
	orc->fp_reg = ORC_REG_PREV_SP;
	orc->type = UNWIND_HINT_TYPE_CALL;

	if (!cfi || cfi->cfa.base == CFI_UNDEFINED ||
	    (cfi->type == UNWIND_HINT_TYPE_CALL && !fp->offset)) {
		/*
		 * The frame pointer has not been set up. This instruction is
		 * unreliable from an unwind perspective.
		 */
		return 0;
	}

	orc->sp_offset = cfi->cfa.offset;
	orc->fp_offset = fp->offset;
	orc->type = cfi->type;
	orc->signal = cfi->end;

	return 0;
}

int write_orc_entry(struct elf *elf, struct section *orc_sec,
		    struct section *ip_sec, unsigned int idx,
		    struct section *insn_sec, unsigned long insn_off,
		    struct orc_entry *o)
{
	struct orc_entry *orc;

	/* populate ORC data */
	orc = (struct orc_entry *)orc_sec->data->d_buf + idx;
	memcpy(orc, o, sizeof(*orc));
	orc->sp_offset = bswap_if_needed(elf, orc->sp_offset);
	orc->fp_offset = bswap_if_needed(elf, orc->fp_offset);

	/* populate reloc for ip */
	if (!elf_init_reloc_text_sym(elf, ip_sec, idx * sizeof(int), idx,
				     insn_sec, insn_off))
		return -1;

	return 0;
}

static const char *reg_name(unsigned int reg)
{
	switch (reg) {
	case ORC_REG_PREV_SP:
		return "cfa";
	case ORC_REG_FP:
		return "x29";
	case ORC_REG_SP:
		return "sp";
	default:
		return "?";
	}
}

const char *orc_type_name(unsigned int type)
{
	switch (type) {
	case UNWIND_HINT_TYPE_CALL:
		return "call";
	case UNWIND_HINT_TYPE_REGS:
		return "regs";
	case UNWIND_HINT_TYPE_IRQ_STACK:
		return "irqstack";
	default:
		return "?";
	}
}

void orc_print_dump(struct elf *dummy_elf, struct orc_entry *orc, int i)
{
	printf("type:%s", orc_type_name(orc[i].type));

	printf(" sp:");

	orc_print_reg(orc[i].sp_reg, bswap_if_needed(dummy_elf, orc[i].sp_offset));

	printf(" fp:");

	orc_print_reg(orc[i].fp_reg, bswap_if_needed(dummy_elf, orc[i].fp_offset));

	printf(" signal:%d\n", orc[i].signal);
}

void orc_print_reg(unsigned int reg, int offset)
{
	if (reg == ORC_REG_UNDEFINED)
		printf("(und)");
	else
		printf("%s%+d", reg_name(reg), offset);
}

void orc_print_sp(void)
{
	printf(" cfa:");
}

void orc_print_fp(void)
{
	printf(" x29:");
}

bool orc_ignore_section(struct section *sec)
{
	return !strcmp(sec->name, ".head.text");
}

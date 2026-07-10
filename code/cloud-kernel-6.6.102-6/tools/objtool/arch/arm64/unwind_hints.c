// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015-2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */
#include <asm/unwind_hints.h>

#include <objtool/builtin.h>
#include <objtool/endianness.h>
#include <objtool/insn.h>
#include <objtool/warn.h>
#include <arch/cfi.h>

int read_unwind_hints(struct objtool_file *file)
{
	struct cfi_state cfi = init_cfi;
	struct section *sec;
	struct unwind_hint *hint;
	struct instruction *insn;
	struct reloc *reloc;
	u8 sp_reg, type;
	int i;

	sec = find_section_by_name(file->elf, ".discard.unwind_hints");
	if (!sec)
		return 0;

	if (!sec->rsec) {
		WARN("missing .rela.discard.unwind_hints section");
		return -1;
	}

	if (sec->sh.sh_size % sizeof(struct unwind_hint)) {
		WARN("struct unwind_hint size mismatch");
		return -1;
	}

	file->hints = true;

	for (i = 0; i < sec->sh.sh_size / sizeof(struct unwind_hint); i++) {
		hint = (struct unwind_hint *)sec->data->d_buf + i;

		sp_reg = bswap_if_needed(file->elf, hint->sp_reg);
		type = bswap_if_needed(file->elf, hint->type);

		reloc = find_reloc_by_dest(file->elf, sec, i * sizeof(*hint));
		if (!reloc) {
			WARN("can't find reloc for unwind_hints[%d]", i);
			return -1;
		}

		insn = find_insn(file, reloc->sym->sec, reloc_addend(reloc));
		if (!insn) {
			WARN("can't find insn for unwind_hints[%d]", i);
			return -1;
		}

		insn->hint = true;

		if (type == UNWIND_HINT_TYPE_UNDEFINED) {
			insn->cfi = &force_undefined_cfi;
			continue;
		}

		if (type == UNWIND_HINT_TYPE_SAVE) {
			insn->hint = false;
			insn->save = true;
			continue;
		}

		if (type == UNWIND_HINT_TYPE_RESTORE) {
			insn->restore = true;
			continue;
		}

		if (type == UNWIND_HINT_TYPE_REGS_PARTIAL) {
			struct symbol *sym = find_symbol_by_offset(insn->sec, insn->offset);

			if (sym && sym->bind == STB_GLOBAL) {
				if (opts.ibt && insn->type != INSN_ENDBR && !insn->noendbr) {
					WARN_INSN(insn, "UNWIND_HINT_IRET_REGS without ENDBR");
				}
			}
		}

		if (type == UNWIND_HINT_TYPE_FUNC) {
			insn->cfi = &func_cfi;
			continue;
		}

		if (insn->cfi)
			cfi = *(insn->cfi);

		if (arch_decode_hint_reg(sp_reg, &cfi.cfa.base)) {
			WARN_INSN(insn, "unsupported unwind_hint sp base reg %d", sp_reg);
			return -1;
		}

		cfi.cfa.offset = bswap_if_needed(file->elf, hint->sp_offset);
		cfi.type = type;
		cfi.signal = hint->signal;

		insn->cfi = cfi_hash_find_or_add(&cfi);
	}

	return 0;
}

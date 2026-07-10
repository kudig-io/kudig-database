// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015-2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#include <string.h>

#include <objtool/builtin.h>
#include <objtool/insn.h>
#include <objtool/warn.h>
#include <arch/cfi.h>

struct instruction *find_insn(struct objtool_file *file,
			      struct section *sec, unsigned long offset)
{
	struct instruction *insn;

	hash_for_each_possible(file->insn_hash, insn, hash, sec_offset_hash(sec, offset)) {
		if (insn->sec == sec && insn->offset == offset)
			return insn;
	}

	return NULL;
}

struct instruction *next_insn_same_sec(struct objtool_file *file,
				       struct instruction *insn)
{
	if (insn->idx == INSN_CHUNK_MAX)
		return find_insn(file, insn->sec, insn->offset + insn->len);

	insn++;
	if (!insn->len)
		return NULL;

	return insn;
}

struct instruction *next_insn_same_func(struct objtool_file *file,
					struct instruction *insn)
{
	struct instruction *next = next_insn_same_sec(file, insn);
	struct symbol *func = insn_func(insn);

	if (!func)
		return NULL;

	if (next && insn_func(next) == func)
		return next;

	/* Check if we're already in the subfunction: */
	if (func == func->cfunc)
		return NULL;

	/* Move to the subfunction: */
	return find_insn(file, func->cfunc->sec, func->cfunc->offset);
}

struct instruction *prev_insn_same_sec(struct objtool_file *file,
					      struct instruction *insn)
{
	if (insn->idx == 0) {
		if (insn->prev_len)
			return find_insn(file, insn->sec, insn->offset - insn->prev_len);
		return NULL;
	}

	return insn - 1;
}

struct instruction *prev_insn_same_sym(struct objtool_file *file,
					      struct instruction *insn)
{
	struct instruction *prev = prev_insn_same_sec(file, insn);

	if (prev && insn_func(prev) == insn_func(insn))
		return prev;

	return NULL;
}

void init_insn_state(struct objtool_file *file, struct insn_state *state,
			    struct section *sec)
{
	memset(state, 0, sizeof(*state));
	init_cfi_state(&state->cfi);

	/*
	 * We need the full vmlinux for noinstr validation, otherwise we can
	 * not correctly determine insn_call_dest(insn)->sec (external symbols
	 * do not have a section).
	 */
	if (opts.link && opts.noinstr && sec)
		state->noinstr = sec->noinstr;
}

struct instruction *find_last_insn(struct objtool_file *file,
				   struct section *sec)
{
	struct instruction *insn = NULL;
	unsigned int offset;
	unsigned int end = (sec->sh.sh_size > 10) ? sec->sh.sh_size - 10 : 0;

	for (offset = sec->sh.sh_size - 1; offset >= end && !insn; offset--)
		insn = find_insn(file, sec, offset);

	return insn;
}

struct reloc *insn_reloc(struct objtool_file *file, struct instruction *insn)
{
	struct reloc *reloc;

	if (insn->no_reloc)
		return NULL;

	if (!file)
		return NULL;

	reloc = find_reloc_by_dest_range(file->elf, insn->sec,
					 insn->offset, insn->len);
	if (!reloc) {
		insn->no_reloc = 1;
		return NULL;
	}

	return reloc;
}

bool is_first_func_insn(struct objtool_file *file,
			struct instruction *insn, struct symbol *sym)
{
	if (insn->offset == sym->offset)
		return true;

	/* Allow direct CALL/JMP past ENDBR */
	if (opts.ibt) {
		struct instruction *prev = prev_insn_same_sym(file, insn);

		if (prev && prev->type == INSN_ENDBR &&
		    insn->offset == sym->offset + prev->len)
			return true;
	}

	return false;
}

bool insn_cfi_match(struct instruction *insn, struct cfi_state *cfi2,
		    bool print)
{
	struct cfi_state *cfi1 = insn->cfi;
	int i;

	if (!cfi1) {
		WARN("CFI missing");
		return false;
	}

	if (memcmp(&cfi1->cfa, &cfi2->cfa, sizeof(cfi1->cfa))) {
		if (print) {
			WARN_INSN(insn, "stack state mismatch: cfa1=%d%+d cfa2=%d%+d",
				  cfi1->cfa.base, cfi1->cfa.offset,
				  cfi2->cfa.base, cfi2->cfa.offset);
		}
	} else if (memcmp(&cfi1->regs, &cfi2->regs, sizeof(cfi1->regs))) {
		for (i = 0; i < CFI_NUM_REGS; i++) {
			if (!memcmp(&cfi1->regs[i], &cfi2->regs[i],
				    sizeof(struct cfi_reg)))
				continue;
			if (print) {
				WARN_INSN(insn, "stack state mismatch: reg1[%d]=%d%+d reg2[%d]=%d%+d",
					  i, cfi1->regs[i].base, cfi1->regs[i].offset,
					  i, cfi2->regs[i].base, cfi2->regs[i].offset);
			}
			break;
		}

	} else if (cfi1->type != cfi2->type) {
		if (print) {
			WARN_INSN(insn, "stack state mismatch: type1=%d type2=%d",
				  cfi1->type, cfi2->type);
		}

	} else if (cfi1->drap != cfi2->drap ||
		   (cfi1->drap && cfi1->drap_reg != cfi2->drap_reg) ||
		   (cfi1->drap && cfi1->drap_offset != cfi2->drap_offset)) {
		if (print) {
			WARN_INSN(insn, "stack state mismatch: drap1=%d(%d,%d) drap2=%d(%d,%d)",
				  cfi1->drap, cfi1->drap_reg, cfi1->drap_offset,
				  cfi2->drap, cfi2->drap_reg, cfi2->drap_offset);
		}

	} else
		return true;

	return false;
}

/*
 * This is a hack for Clang. Clang is aggressive about removing section
 * symbols and then some. If we cannot find something to relocate an
 * instruction against, we must not generate CFI for it or the ORC
 * generation will fail later.
 */
bool insn_can_reloc(struct instruction *insn)
{
	struct section *insn_sec = insn->sec;
	unsigned long insn_off = insn->offset;

	if (insn_sec->sym ||
	    find_symbol_containing(insn_sec, insn_off) ||
	    find_symbol_containing(insn_sec, insn_off - 1)) {
		/* See elf_add_reloc_to_insn(). */
		return true;
	}
	return false;
}

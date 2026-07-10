// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015-2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/mman.h>

#include <objtool/objtool.h>
#include <objtool/builtin.h>
#include <objtool/insn.h>
#include <arch/cfi.h>

/*
 * Find the destination instructions for all jumps.
 */
static void add_jump_destinations(struct objtool_file *file)
{
	struct instruction *insn;
	struct reloc *reloc;
	struct section *dest_sec;
	unsigned long dest_off;

	for_each_insn(file, insn) {
		if (insn->type != INSN_CALL &&
		    insn->type != INSN_JUMP_CONDITIONAL &&
		    insn->type != INSN_JUMP_UNCONDITIONAL) {
			continue;
		}

		reloc = insn_reloc(file, insn);
		if (!reloc) {
			dest_sec = insn->sec;
			dest_off = arch_jump_destination(insn);
		} else if (reloc->sym->type == STT_SECTION) {
			dest_sec = reloc->sym->sec;
			dest_off = arch_dest_reloc_offset(reloc_addend(reloc));
		} else if (reloc->sym->sec->idx) {
			dest_sec = reloc->sym->sec;
			dest_off = reloc->sym->sym.st_value +
				   arch_dest_reloc_offset(reloc_addend(reloc));
		} else {
			/* non-func asm code jumping to another file */
			continue;
		}

		insn->jump_dest = find_insn(file, dest_sec, dest_off);
	}
}

static bool update_cfi_state(struct cfi_state *cfi, struct stack_op *op)
{
	struct cfi_reg *cfa = &cfi->cfa;
	struct cfi_reg *fp_reg = &cfi->regs[CFI_FP];
	struct cfi_reg *fp_val = &cfi->vals[CFI_FP];
	struct cfi_reg *ra_val = &cfi->vals[CFI_RA];
	enum op_src_type src_type = op->src.type;
	enum op_dest_type dest_type = op->dest.type;
	unsigned char dest_reg = op->dest.reg;
	int offset;

	if (src_type == OP_SRC_ADD && dest_type == OP_DEST_REG) {

		if (op->src.reg == CFI_SP) {
			if (op->dest.reg == CFI_SP) {
				cfa->offset -= op->src.offset;
			} else {
				if (fp_reg->offset) {
					/* FP is already set. */
					return false;
				}
				fp_reg->offset = -cfa->offset + op->src.offset;
				if (fp_reg->offset != fp_val->offset) {
					/*
					 * FP does not match the location
					 * where FP is stored on stack.
					 */
					return false;
				}
			}
		} else {
			if (op->dest.reg == CFI_SP) {
				cfa->offset =
					-(fp_reg->offset + op->src.offset);
			} else {
				/* Setting the FP from itself is unreliable. */
				return false;
			}
		}
		/*
		 * When the stack pointer is restored in the frame pointer
		 * epilog, forget where the FP and RA were stored.
		 */
		if (cfa->offset < -fp_val->offset)
			fp_val->offset = 0;
		if (cfa->offset < -ra_val->offset)
			ra_val->offset = 0;
		goto out;
	}

	if (src_type == OP_SRC_REG_INDIRECT && dest_type == OP_DEST_REG) {
		offset = -cfa->offset + op->src.offset;
		if (dest_reg == CFI_FP) {
			if (!fp_val->offset || fp_val->offset != offset) {
				/*
				 * Loading the FP from a different place than
				 * where it is stored.
				 */
				return false;
			}
			if (!ra_val->offset ||
			    (ra_val->offset - fp_val->offset) != 8) {
				/* FP and RA must be adjacent in a frame. */
				return false;
			}
			fp_reg->offset = 0;
		}
		goto out;
	}

	if (src_type == OP_SRC_REG && dest_type == OP_DEST_REG_INDIRECT) {
		offset = -cfa->offset + op->dest.offset;
		if (dest_reg == CFI_FP) {
			/* Record where the FP is stored on the stack. */
			fp_val->offset = offset;
		} else {
			/* Record where the RA is stored on the stack. */
			if (fp_val->offset && (offset - fp_val->offset) == 8)
				ra_val->offset = offset;
		}
		goto out;
	}
	return false;
out:
	if (cfa->offset < 0 || fp_reg->offset > 0 ||
	    fp_val->offset > 0 || ra_val->offset > 0) {
		/* Unexpected SP and FP offset values. */
		return false;
	}
	return true;
}

static bool do_stack_ops(struct instruction *insn, struct insn_state *state)
{
	struct stack_op *op;

	for (op = insn->stack_ops; op; op = op->next) {
		if (!update_cfi_state(&state->cfi, op))
			return false;
	}
	return true;
}

static bool validate_branch(struct objtool_file *file, struct section *sec,
			    struct symbol *func, struct instruction *insn,
			    struct insn_state *state)
{
	struct symbol *insn_func = insn->sym;
	struct instruction *dest;
	struct cfi_state save_cfi;
	struct cfi_reg *cfa;
	struct cfi_reg *regs;
	unsigned long start, end;

	for (; insn; insn = next_insn_same_sec(file, insn)) {

		if (insn->sym != insn_func)
			return true;

		if (insn->cfi)
			return insn_cfi_match(insn, &state->cfi, false);

		insn->cfi = cfi_hash_find_or_add(&state->cfi);
		dest = insn->jump_dest;

		if (!do_stack_ops(insn, state))
			return false;

		switch (insn->type) {
		case INSN_BUG:
			return true;

		case INSN_UNRELIABLE:
			return false;

		case INSN_RETURN:
			cfa = &state->cfi.cfa;
			regs = state->cfi.regs;
			if (cfa->offset || regs[CFI_FP].offset) {
				/* SP and FP offsets should be 0 on return. */
				return false;
			}
			return true;

		case INSN_CALL:
		case INSN_CALL_DYNAMIC:
			start = func->offset;
			end = start + func->len;
			/* Treat intra-function calls as jumps. */
			if (!dest || dest->sec != sec ||
			    dest->offset <= start || dest->offset >= end) {
				break;
			}

		case INSN_JUMP_UNCONDITIONAL:
		case INSN_JUMP_CONDITIONAL:
		case INSN_JUMP_DYNAMIC:
			if (dest) {
				save_cfi = state->cfi;
				if (!validate_branch(file, sec, func, dest,
						     state)) {
					return false;
				}
				state->cfi = save_cfi;
			}
			if (insn->type == INSN_JUMP_UNCONDITIONAL ||
			    insn->type == INSN_JUMP_DYNAMIC) {
				return true;
			}
			break;

		default:
			break;
		}
	}
	return true;
}

static bool walk_reachable(struct objtool_file *file, struct section *sec,
			   struct symbol *func)
{
	struct instruction *insn = find_insn(file, sec, func->offset);
	struct insn_state state;

	func_for_each_insn(file, func, insn) {

		if (insn->offset != func->offset &&
		    (insn->type != INSN_START || insn->cfi)) {
			continue;
		}

		init_insn_state(file, &state, sec);
		set_func_state(&state.cfi);

		if (!validate_branch(file, sec, func, insn, &state))
			return false;
	}
	return true;
}

static void remove_cfi(struct objtool_file *file, struct symbol *func)
{
	struct instruction *insn;

	func_for_each_insn(file, func, insn) {
		insn->cfi = NULL;
	}
}

/*
 * Instructions that were not visited by walk_reachable() would not have a
 * CFI. Try to initialize their CFI. For instance, there could be a table of
 * unconditional branches like for a switch statement. Or, code can be patched
 * by the kernel at runtime. After patching, some of the previously unreachable
 * code may become reachable.
 *
 * This follows the same pattern as the DWARF info generated by the compiler.
 */
static bool walk_unreachable(struct objtool_file *file, struct section *sec,
			     struct symbol *func)
{
	struct instruction *insn, *prev;
	struct insn_state state;

	func_for_each_insn(file, func, insn) {

		if (insn->cfi)
			continue;

		prev = prev_insn_same_sec(file, insn);
		if (!prev || prev->sym != insn->sym || !prev->cfi)
			continue;

		if (prev->type != INSN_JUMP_UNCONDITIONAL &&
		    prev->type != INSN_JUMP_DYNAMIC &&
		    prev->type != INSN_BUG) {
			continue;
		}

		/* Propagate the CFI. */
		state.cfi = *prev->cfi;
		if (!validate_branch(file, sec, func, insn, &state))
			return false;
	}
	return true;
}

static void walk_section(struct objtool_file *file, struct section *sec)
{
	struct symbol *func;

	list_for_each_entry(func, &sec->symbol_list, list) {

		if (func->type != STT_FUNC || !func->len ||
		    func->pfunc != func || func->alias != func) {
			/* No CFI generated for this function. */
			continue;
		}

		if (!walk_reachable(file, sec, func) ||
		    !walk_unreachable(file, sec, func)) {
			remove_cfi(file, func);
			continue;
		}
	}
}

static void walk_sections(struct objtool_file *file)
{
	struct section *sec;

	for_each_sec(file, sec) {
		if (sec->sh.sh_flags & SHF_EXECINSTR)
			walk_section(file, sec);
	}
}

int check(struct objtool_file *file)
{
	int ret;

	if (!opts.stackval)
		return 1;

	arch_initial_func_cfi_state(&initial_func_cfi);

	if (!cfi_hash_alloc(1UL << (file->elf->symbol_bits - 3)))
		return -1;

	ret = decode_instructions(file);
	if (ret)
		return ret;

	add_jump_destinations(file);

	walk_sections(file);

	ret = read_unwind_hints(file);
	if (ret)
		return ret;

	if (opts.orc)
		ret = orc_create(file);

	return ret;
}

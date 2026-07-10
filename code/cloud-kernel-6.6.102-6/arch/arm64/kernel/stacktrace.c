// SPDX-License-Identifier: GPL-2.0-only
/*
 * Stack tracing support
 *
 * Copyright (C) 2012 ARM Ltd.
 */
#include <linux/kernel.h>
#include <asm/unwind_hints.h>
#include <asm/orc_lookup.h>
#include <linux/efi.h>
#include <linux/export.h>
#include <linux/ftrace.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/stacktrace.h>

#include <asm/efi.h>
#include <asm/irq.h>
#include <asm/stack_pointer.h>
#include <asm/stacktrace.h>

static inline bool unwind_completed(struct unwind_state *state)
{
	if (state->fp == (unsigned long)task_pt_regs(state->task)->stackframe) {
		/* Final frame; nothing to unwind */
		return true;
	}
	return false;
}

#ifdef CONFIG_FRAME_POINTER_VALIDATION

static void unwind_check_reliable(struct unwind_state *state)
{
	unsigned long pc, fp;
	struct orc_entry *orc;
	bool adjust_pc = false;

	if (unwind_completed(state))
		return;

	/*
	 * If a previous frame was unreliable, the CFA cannot be reliably
	 * computed anymore.
	 */
	if (!state->reliable)
		return;

	pc = state->pc;

	/* Don't let modules unload while we're reading their ORC data. */
	preempt_disable();

	orc = orc_find(pc);
	if (!orc || (!orc->fp_offset && orc->type == UNWIND_HINT_TYPE_CALL)) {
		/*
		 * If the final instruction in a function happens to be a call
		 * instruction, the return address would fall outside of the
		 * function. That could be the case here. This can happen, for
		 * instance, if the called function is a "noreturn" function.
		 * The compiler can optimize away the instructions after the
		 * call. So, adjust the PC so it falls inside the function and
		 * retry.
		 *
		 * We only do this if the current and the previous frames
		 * are call frames and not hint frames.
		 */
		if (state->unwind_type == UNWIND_HINT_TYPE_CALL) {
			pc -= 4;
			adjust_pc = true;
			orc = orc_find(pc);
		}
	}
	if (!orc) {
		state->reliable = false;
		goto out;
	}
	state->unwind_type = orc->type;

	if (!state->cfa) {
		/* Set up the initial CFA and return. */
		state->cfa = state->fp - orc->fp_offset;
		goto out;
	}

	/* Compute the next CFA and FP. */
	switch (orc->type) {
	case UNWIND_HINT_TYPE_CALL:
		/* Normal call */
		state->cfa += orc->sp_offset;
		fp = state->cfa + orc->fp_offset;
		break;

	case UNWIND_HINT_TYPE_REGS:
		/*
		 * pt_regs hint: The frame pointer points to either the
		 * synthetic frame within pt_regs or to the place where
		 * x29 and x30 are saved in the register save area in
		 * pt_regs.
		 */
		state->cfa += orc->sp_offset;
		fp = state->cfa + offsetof(struct pt_regs, stackframe) -
		     sizeof(struct pt_regs);
		if (state->fp != fp) {
			fp = state->cfa + offsetof(struct pt_regs, regs[29]) -
			     sizeof(struct pt_regs);
		}
		break;

	case UNWIND_HINT_TYPE_IRQ_STACK:
		/* Hint to unwind from the IRQ stack to the task stack. */
		state->cfa = state->fp + orc->sp_offset;
		fp = state->fp;
		break;

	default:
		fp = 0;
		break;
	}

	/* Validate the actual FP with the computed one. */
	if (state->fp != fp)
		state->reliable = false;
out:
	if (state->reliable && adjust_pc)
		state->pc = pc;
	preempt_enable();
}

#else /* !CONFIG_FRAME_POINTER_VALIDATION */

static void unwind_check_reliable(struct unwind_state *state)
{
}

#endif /* CONFIG_FRAME_POINTER_VALIDATION */

/*
 * Start an unwind from a pt_regs.
 *
 * The unwind will begin at the PC within the regs.
 *
 * The regs must be on a stack currently owned by the calling task.
 */
static __always_inline void
unwind_init_from_regs(struct unwind_state *state,
		      struct pt_regs *regs)
{
	unwind_init_common(state, current);

	state->fp = regs->regs[29];
	state->pc = regs->pc;
}

/*
 * Start an unwind from a caller.
 *
 * The unwind will begin at the caller of whichever function this is inlined
 * into.
 *
 * The function which invokes this must be noinline.
 */
static __always_inline void
unwind_init_from_caller(struct unwind_state *state)
{
	unwind_init_common(state, current);

	state->fp = (unsigned long)__builtin_frame_address(1);
	state->pc = (unsigned long)__builtin_return_address(0);
}

/*
 * Start an unwind from a blocked task.
 *
 * The unwind will begin at the blocked tasks saved PC (i.e. the caller of
 * cpu_switch_to()).
 *
 * The caller should ensure the task is blocked in cpu_switch_to() for the
 * duration of the unwind, or the unwind will be bogus. It is never valid to
 * call this for the current task.
 */
static __always_inline void
unwind_init_from_task(struct unwind_state *state,
		      struct task_struct *task)
{
	unwind_init_common(state, task);

	state->fp = thread_saved_fp(task);
	state->pc = thread_saved_pc(task);
}

static __always_inline int
unwind_recover_return_address(struct unwind_state *state)
{
#ifdef CONFIG_FUNCTION_GRAPH_TRACER
	if (state->task->ret_stack &&
	    (state->pc == (unsigned long)return_to_handler)) {
		unsigned long orig_pc;
		orig_pc = ftrace_graph_ret_addr(state->task, NULL, state->pc,
						(void *)state->fp);
		if (WARN_ON_ONCE(state->pc == orig_pc))
			return -EINVAL;
		state->pc = orig_pc;
	}
#endif /* CONFIG_FUNCTION_GRAPH_TRACER */

#ifdef CONFIG_KRETPROBES
	if (is_kretprobe_trampoline(state->pc)) {
		state->pc = kretprobe_find_ret_addr(state->task,
						    (void *)state->fp,
						    &state->kr_cur);
	}
#endif /* CONFIG_KRETPROBES */

	return 0;
}

/*
 * Unwind from one frame record (A) to the next frame record (B).
 *
 * We terminate early if the location of B indicates a malformed chain of frame
 * records (e.g. a cycle), determined based on the location and fp value of A
 * and the location (but not the fp value) of B.
 */
static __always_inline int
unwind_next(struct unwind_state *state)
{
	int err;

	if (unwind_completed(state))
		return -ENOENT;

	err = unwind_next_frame_record(state);
	if (err)
		return err;

	state->pc = ptrauth_strip_kernel_insn_pac(state->pc);

	return unwind_recover_return_address(state);
}

static __always_inline int
unwind(struct unwind_state *state, bool need_reliable,
       stack_trace_consume_fn consume_entry, void *cookie)
{
	int ret = unwind_recover_return_address(state);

	if (ret)
		return ret;

	while (1) {
		if (need_reliable && !state->reliable)
			return -EINVAL;

		if (!consume_entry(cookie, state->pc))
			break;
		ret = unwind_next(state);
		if (need_reliable && !ret)
			unwind_check_reliable(state);
		if (ret < 0)
			break;
	}
	return ret;
}

/*
 * Per-cpu stacks are only accessible when unwinding the current task in a
 * non-preemptible context.
 */
#define STACKINFO_CPU(name)					\
	({							\
		((task == current) && !preemptible())		\
			? stackinfo_get_##name()		\
			: stackinfo_get_unknown();		\
	})

/*
 * SDEI stacks are only accessible when unwinding the current task in an NMI
 * context.
 */
#define STACKINFO_SDEI(name)					\
	({							\
		((task == current) && in_nmi())			\
			? stackinfo_get_sdei_##name()		\
			: stackinfo_get_unknown();		\
	})

#define STACKINFO_EFI						\
	({							\
		((task == current) && current_in_efi())		\
			? stackinfo_get_efi()			\
			: stackinfo_get_unknown();		\
	})

noinline noinstr void arch_stack_walk(stack_trace_consume_fn consume_entry,
			      void *cookie, struct task_struct *task,
			      struct pt_regs *regs)
{
	struct stack_info stacks[] = {
		stackinfo_get_task(task),
		STACKINFO_CPU(irq),
#if defined(CONFIG_VMAP_STACK)
		STACKINFO_CPU(overflow),
#endif
#if defined(CONFIG_VMAP_STACK) && defined(CONFIG_ARM_SDE_INTERFACE)
		STACKINFO_SDEI(normal),
		STACKINFO_SDEI(critical),
#endif
#ifdef CONFIG_EFI
		STACKINFO_EFI,
#endif
	};
	struct unwind_state state = {
		.stacks = stacks,
		.nr_stacks = ARRAY_SIZE(stacks),
	};

	if (regs) {
		if (task != current)
			return;
		unwind_init_from_regs(&state, regs);
	} else if (task == current) {
		unwind_init_from_caller(&state);
	} else {
		unwind_init_from_task(&state, task);
	}

	unwind(&state, false, consume_entry, cookie);
}

noinline notrace int arch_stack_walk_reliable(
				stack_trace_consume_fn consume_entry,
				void *cookie, struct task_struct *task)
{
	struct stack_info stacks[] = {
		stackinfo_get_task(task),
		STACKINFO_CPU(irq),
#if defined(CONFIG_VMAP_STACK)
		STACKINFO_CPU(overflow),
#endif
#if defined(CONFIG_VMAP_STACK) && defined(CONFIG_ARM_SDE_INTERFACE)
		STACKINFO_SDEI(normal),
		STACKINFO_SDEI(critical),
#endif
#ifdef CONFIG_EFI
		STACKINFO_EFI,
#endif
	};
	struct unwind_state state = {
		.stacks = stacks,
		.nr_stacks = ARRAY_SIZE(stacks),
	};
	int ret;

	if (task == current)
		unwind_init_from_caller(&state);
	else
		unwind_init_from_task(&state, task);
	unwind_check_reliable(&state);

	ret = unwind(&state, true, consume_entry, cookie);

	return ret == -ENOENT ? 0 : -EINVAL;
}

static bool dump_backtrace_entry(void *arg, unsigned long where)
{
	char *loglvl = arg;
	printk("%s %pSb\n", loglvl, (void *)where);
	return true;
}

void dump_backtrace(struct pt_regs *regs, struct task_struct *tsk,
		    const char *loglvl)
{
	pr_debug("%s(regs = %p tsk = %p)\n", __func__, regs, tsk);

	if (regs && user_mode(regs))
		return;

	if (!tsk)
		tsk = current;

	if (!try_get_task_stack(tsk))
		return;

	printk("%sCall trace:\n", loglvl);
	arch_stack_walk(dump_backtrace_entry, (void *)loglvl, tsk, regs);

	put_task_stack(tsk);
}

void show_stack(struct task_struct *tsk, unsigned long *sp, const char *loglvl)
{
	dump_backtrace(NULL, tsk, loglvl);
	barrier();
}

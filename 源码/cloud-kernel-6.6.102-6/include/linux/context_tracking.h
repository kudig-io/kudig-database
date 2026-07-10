/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_CONTEXT_TRACKING_H
#define _LINUX_CONTEXT_TRACKING_H

#include <linux/sched.h>
#include <linux/vtime.h>
#include <linux/context_tracking_state.h>
#include <linux/instrumentation.h>

#include <asm/ptrace.h>


#ifdef CONFIG_CONTEXT_TRACKING_USER
extern void ct_cpu_track_user(int cpu);

/* Called with interrupts disabled.  */
extern void __ct_user_enter(enum ctx_state state);
extern void __ct_user_exit(enum ctx_state state);
extern void __sys_tracking_enter(enum sys_state state);
extern void __sys_tracking_exit(enum sys_state state);

extern void sys_tracking_enter(enum sys_state state);
extern void sys_tracking_exit(enum sys_state state);
extern void ct_user_enter(enum ctx_state state);
extern void ct_user_exit(enum ctx_state state);

extern void user_enter_callable(void);
extern void user_exit_callable(void);

extern bool is_sys_aware_enabled(void);

static inline void user_enter(void)
{
	if (context_tracking_enabled())
		ct_user_enter(CONTEXT_USER);
	if (is_sys_aware_enabled())
		sys_tracking_enter(ST_USER);
}
static inline void user_exit(void)
{
	if (context_tracking_enabled())
		ct_user_exit(CONTEXT_USER);
	if (is_sys_aware_enabled())
		sys_tracking_exit(ST_USER);
}

/* Called with interrupts disabled.  */
static __always_inline void user_enter_irqoff(void)
{
	if (context_tracking_enabled())
		__ct_user_enter(CONTEXT_USER);
	if (is_sys_aware_enabled())
		__sys_tracking_enter(ST_USER);
}
static __always_inline void user_exit_irqoff(void)
{
	if (context_tracking_enabled())
		__ct_user_exit(CONTEXT_USER);
	if (is_sys_aware_enabled())
		__sys_tracking_exit(ST_USER);
}

static inline enum ctx_state exception_enter(void)
{
	enum ctx_state prev_ctx;

	if (IS_ENABLED(CONFIG_HAVE_CONTEXT_TRACKING_USER_OFFSTACK) ||
	    !context_tracking_enabled())
		return 0;

	prev_ctx = __ct_state();
	if (prev_ctx != CONTEXT_KERNEL)
		ct_user_exit(prev_ctx);

	return prev_ctx;
}

static inline enum sys_state st_exception_enter(void)
{
	enum sys_state prev_sys;

	if (!is_sys_aware_enabled())
		return 0;

	prev_sys = this_cpu_read(sys_tracking.state);
	if (prev_sys != ST_KERNEL)
		sys_tracking_exit(prev_sys);

	return prev_sys;
}

static inline void exception_exit(enum ctx_state prev_ctx)
{
	if (!IS_ENABLED(CONFIG_HAVE_CONTEXT_TRACKING_USER_OFFSTACK) &&
	    context_tracking_enabled()) {
		if (prev_ctx != CONTEXT_KERNEL)
			ct_user_enter(prev_ctx);
	}
}

static inline void st_exception_exit(enum sys_state prev_sys)
{
	if (is_sys_aware_enabled()) {
		if (prev_sys != ST_KERNEL)
			sys_tracking_enter(prev_sys);
	}
}

static __always_inline enum sys_state sys_state(void)
{
	return is_sys_aware_enabled() ?
		this_cpu_read(sys_tracking.state) : ST_DISABLED;
}

static __always_inline bool context_tracking_guest_enter(void)
{
	if (context_tracking_enabled())
		__ct_user_enter(CONTEXT_GUEST);
	if (is_sys_aware_enabled())
		__sys_tracking_enter(ST_GUEST);

	return context_tracking_enabled_this_cpu();
}

static __always_inline void context_tracking_guest_exit(void)
{
	if (context_tracking_enabled())
		__ct_user_exit(CONTEXT_GUEST);
	if (is_sys_aware_enabled())
		__sys_tracking_exit(ST_GUEST);
}

#define CT_WARN_ON(cond) WARN_ON(context_tracking_enabled() && (cond))

#else
static inline void user_enter(void) { }
static inline void user_exit(void) { }
static inline void user_enter_irqoff(void) { }
static inline void user_exit_irqoff(void) { }
static inline int exception_enter(void) { return 0; }
static inline enum sys_state st_exception_enter(void) { return 0; }
static inline void exception_exit(enum ctx_state prev_ctx) { }
static inline void st_exception_exit(enum sys_state prev_sys) { }
static inline int ct_state(void) { return -1; }
static inline int __ct_state(void) { return -1; }
static inline enum sys_state sys_state(void) { return ST_DISABLED; }
static __always_inline bool context_tracking_guest_enter(void) { return false; }
static __always_inline void context_tracking_guest_exit(void) { }
#define CT_WARN_ON(cond) do { } while (0)
#endif /* !CONFIG_CONTEXT_TRACKING_USER */

#ifdef CONFIG_CONTEXT_TRACKING_USER_FORCE
extern void context_tracking_init(void);
#else
static inline void context_tracking_init(void) { }
#endif /* CONFIG_CONTEXT_TRACKING_USER_FORCE */

#ifdef CONFIG_CONTEXT_TRACKING_IDLE
extern void ct_idle_enter(void);
extern void ct_idle_exit(void);

/*
 * Is the current CPU in an extended quiescent state?
 *
 * No ordering, as we are sampling CPU-local information.
 */
static __always_inline bool rcu_dynticks_curr_cpu_in_eqs(void)
{
	return !(raw_atomic_read(this_cpu_ptr(&context_tracking.state)) & RCU_DYNTICKS_IDX);
}

/*
 * Increment the current CPU's context_tracking structure's ->state field
 * with ordering.  Return the new value.
 */
static __always_inline unsigned long ct_state_inc(int incby)
{
	return raw_atomic_add_return(incby, this_cpu_ptr(&context_tracking.state));
}

static __always_inline bool warn_rcu_enter(void)
{
	bool ret = false;

	/*
	 * Horrible hack to shut up recursive RCU isn't watching fail since
	 * lots of the actual reporting also relies on RCU.
	 */
	preempt_disable_notrace();
	if (rcu_dynticks_curr_cpu_in_eqs()) {
		ret = true;
		ct_state_inc(RCU_DYNTICKS_IDX);
	}

	return ret;
}

static __always_inline void warn_rcu_exit(bool rcu)
{
	if (rcu)
		ct_state_inc(RCU_DYNTICKS_IDX);
	preempt_enable_notrace();
}

#else
static inline void ct_idle_enter(void) { }
static inline void ct_idle_exit(void) { }

static __always_inline bool warn_rcu_enter(void) { return false; }
static __always_inline void warn_rcu_exit(bool rcu) { }
#endif /* !CONFIG_CONTEXT_TRACKING_IDLE */

#endif

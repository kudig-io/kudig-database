/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_ARM64_PARAVIRT_H
#define _ASM_ARM64_PARAVIRT_H

#ifdef CONFIG_PARAVIRT
#include <linux/static_call_types.h>

struct static_key;
extern struct static_key paravirt_steal_enabled;
extern struct static_key paravirt_steal_rq_enabled;

u64 dummy_steal_clock(int cpu);

DECLARE_STATIC_CALL(pv_steal_clock, dummy_steal_clock);

static inline u64 paravirt_steal_clock(int cpu)
{
	return static_call(pv_steal_clock)(cpu);
}

int __init pv_time_init(void);

#ifdef CONFIG_PARAVIRT_SCHED
int __init pv_sched_init(void);

__visible bool __native_vcpu_is_preempted(int cpu);
DECLARE_STATIC_CALL(pv_vcpu_preempted, __native_vcpu_is_preempted);

static inline bool pv_vcpu_is_preempted(int cpu)
{
	return static_call(pv_vcpu_preempted)(cpu);
}

#if defined(CONFIG_SMP) && defined(CONFIG_PARAVIRT_SPINLOCKS)
void __init pv_qspinlock_init(void);
bool pv_is_native_spin_unlock(void);

void dummy_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);
DECLARE_STATIC_CALL(pv_qspinlock_queued_spin_lock_slowpath,
		    dummy_queued_spin_lock_slowpath);
static inline void pv_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val)
{
	return static_call(pv_qspinlock_queued_spin_lock_slowpath)(lock, val);
}

void dummy_queued_spin_unlock(struct qspinlock *lock);
DECLARE_STATIC_CALL(pv_qspinlock_queued_spin_unlock, dummy_queued_spin_unlock);
static inline void pv_queued_spin_unlock(struct qspinlock *lock)
{
	return static_call(pv_qspinlock_queued_spin_unlock)(lock);
}

void dummy_wait(u8 *ptr, u8 val);
DECLARE_STATIC_CALL(pv_qspinlock_wait, dummy_wait);
static inline void pv_wait(u8 *ptr, u8 val)
{
	return static_call(pv_qspinlock_wait)(ptr, val);
}

void dummy_kick(int cpu);
DECLARE_STATIC_CALL(pv_qspinlock_kick, dummy_kick);
static inline void pv_kick(int cpu)
{
	return static_call(pv_qspinlock_kick)(cpu);
}
#else

#define pv_qspinlock_init() do {} while (0)

#endif /* SMP && PARAVIRT_SPINLOCKS */

#endif /* CONFIG_PARAVIRT_SCHED */

#else

#define pv_time_init() do {} while (0)
#define pv_sched_init() do {} while (0)

#endif // CONFIG_PARAVIRT

#endif

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_SYSCTL_H
#define _LINUX_SCHED_SYSCTL_H

#include <linux/types.h>

#ifdef CONFIG_DETECT_HUNG_TASK
/* used for hung_task and block/ */
extern unsigned long sysctl_hung_task_timeout_secs;
#else
/* Avoid need for ifdefs elsewhere in the code */
enum { sysctl_hung_task_timeout_secs = 0 };
#endif

enum sched_tunable_scaling {
	SCHED_TUNABLESCALING_NONE,
	SCHED_TUNABLESCALING_LOG,
	SCHED_TUNABLESCALING_LINEAR,
	SCHED_TUNABLESCALING_END,
};

#define NUMA_BALANCING_DISABLED		0x0
#define NUMA_BALANCING_NORMAL		0x1
#define NUMA_BALANCING_MEMORY_TIERING	0x2

#ifdef CONFIG_NUMA_BALANCING
extern int sysctl_numa_balancing_mode;
#else
#define sysctl_numa_balancing_mode	0
#endif

#ifdef CONFIG_SCHED_ACPU
extern unsigned int sysctl_sched_acpu_enabled;
extern int sched_acpu_enable_handler(struct ctl_table *table, int write,
				     void __user *buffer, size_t *lenp,
				     loff_t *ppos);
#endif
#ifdef CONFIG_GROUP_BALANCER
extern unsigned int sysctl_sched_group_balancer_enabled;
extern int sched_group_balancer_enable_handler(struct ctl_table *table, int write,
					       void __user *buffer, size_t *lenp,
					       loff_t *ppos);
#endif
#ifdef CONFIG_SMP
extern unsigned int sysctl_sched_push_expellee_interval;
#endif
extern unsigned int sysctl_sched_idle_shares_relax;
extern unsigned int sysctl_sched_idle_shares_value;
#endif /* _LINUX_SCHED_SYSCTL_H */

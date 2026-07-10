// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Housekeeping management. Manage the targets for routine code that can run on
 *  any CPU: unbound workqueues, timers, kthreads and any offloadable work.
 *
 * Copyright (C) 2017 Red Hat, Inc., Frederic Weisbecker
 * Copyright (C) 2017-2018 SUSE, Frederic Weisbecker
 *
 */

enum hk_flags {
	HK_FLAG_TIMER		= BIT(HK_TYPE_TIMER),
	HK_FLAG_RCU		= BIT(HK_TYPE_RCU),
	HK_FLAG_MISC		= BIT(HK_TYPE_MISC),
	HK_FLAG_SCHED		= BIT(HK_TYPE_SCHED),
	HK_FLAG_TICK		= BIT(HK_TYPE_TICK),
	HK_FLAG_DOMAIN		= BIT(HK_TYPE_DOMAIN),
	HK_FLAG_WQ		= BIT(HK_TYPE_WQ),
	HK_FLAG_MANAGED_IRQ	= BIT(HK_TYPE_MANAGED_IRQ),
	HK_FLAG_KTHREAD		= BIT(HK_TYPE_KTHREAD),
};

DEFINE_STATIC_KEY_FALSE(housekeeping_overridden);
EXPORT_SYMBOL_GPL(housekeeping_overridden);

struct housekeeping {
	cpumask_var_t cpumasks[HK_TYPE_MAX];
	unsigned long flags;
};

static struct housekeeping housekeeping;

bool housekeeping_enabled(enum hk_type type)
{
	return !!(housekeeping.flags & BIT(type));
}
EXPORT_SYMBOL_GPL(housekeeping_enabled);

int housekeeping_any_cpu(enum hk_type type)
{
	int cpu;

	if (static_branch_unlikely(&housekeeping_overridden)) {
		if (housekeeping.flags & BIT(type)) {
			cpu = sched_numa_find_closest(housekeeping.cpumasks[type], smp_processor_id());
			if (cpu < nr_cpu_ids)
				return cpu;

			return cpumask_any_and(housekeeping.cpumasks[type], cpu_online_mask);
		}
	}
	return smp_processor_id();
}
EXPORT_SYMBOL_GPL(housekeeping_any_cpu);

#ifdef CONFIG_CGROUP_SCHED
/*
 * dyn_allowed  -- allowed CPUs for wild tasks.
 *
 * dyn_isolated -- isolated CPUs for wild tasks.
 *
 * dyn_possible -- possible CPUs for dynamical isolation.
 */
static cpumask_var_t dyn_allowed;
static cpumask_var_t dyn_isolated;
static cpumask_var_t dyn_possible;

static bool dyn_isolcpus_ready;

DEFINE_STATIC_KEY_FALSE(dyn_isolcpus_enabled);
EXPORT_SYMBOL_GPL(dyn_isolcpus_enabled);
#endif

const struct cpumask *housekeeping_cpumask(enum hk_type type)
{
#ifdef CONFIG_CGROUP_SCHED
	if (static_branch_unlikely(&dyn_isolcpus_enabled))
		if (BIT(type) & HK_FLAG_DOMAIN)
			return dyn_allowed;
#endif

	if (static_branch_unlikely(&housekeeping_overridden))
		if (housekeeping.flags & BIT(type))
			return housekeeping.cpumasks[type];
	return cpu_possible_mask;
}
EXPORT_SYMBOL_GPL(housekeeping_cpumask);

void housekeeping_affine(struct task_struct *t, enum hk_type type)
{
	if (static_branch_unlikely(&housekeeping_overridden))
		if (housekeeping.flags & BIT(type))
			set_cpus_allowed_ptr(t, housekeeping.cpumasks[type]);
}
EXPORT_SYMBOL_GPL(housekeeping_affine);

bool housekeeping_test_cpu(int cpu, enum hk_type type)
{
#ifdef CONFIG_CGROUP_SCHED
	if (static_branch_unlikely(&dyn_isolcpus_enabled))
		if (BIT(type) & HK_FLAG_DOMAIN)
			return cpumask_test_cpu(cpu, dyn_allowed);
#endif

	if (static_branch_unlikely(&housekeeping_overridden))
		if (housekeeping.flags & BIT(type))
			return cpumask_test_cpu(cpu, housekeeping.cpumasks[type]);
	return true;
}
EXPORT_SYMBOL_GPL(housekeeping_test_cpu);

#ifdef CONFIG_CGROUP_SCHED
static inline void free_dyn_masks(void)
{
	free_cpumask_var(dyn_allowed);
	free_cpumask_var(dyn_isolated);
	free_cpumask_var(dyn_possible);
}
#endif

void __init housekeeping_init(void)
{
	enum hk_type type;

#ifdef CONFIG_CGROUP_SCHED
	if (zalloc_cpumask_var(&dyn_allowed, GFP_KERNEL) &&
	    zalloc_cpumask_var(&dyn_isolated, GFP_KERNEL) &&
	    zalloc_cpumask_var(&dyn_possible, GFP_KERNEL)) {
		cpumask_copy(dyn_allowed, cpu_possible_mask);
		cpumask_copy(dyn_possible, cpu_possible_mask);
		dyn_isolcpus_ready = true;
	} else
		free_dyn_masks();
#endif

	if (!housekeeping.flags)
		return;

	static_branch_enable(&housekeeping_overridden);

	if (housekeeping.flags & HK_FLAG_TICK)
		sched_tick_offload_init();

	for_each_set_bit(type, &housekeeping.flags, HK_TYPE_MAX) {
		/* We need at least one CPU to handle housekeeping work */
		WARN_ON_ONCE(cpumask_empty(housekeeping.cpumasks[type]));
	}
#ifdef CONFIG_CGROUP_SCHED
	if (dyn_isolcpus_ready && (housekeeping.flags & HK_FLAG_DOMAIN) &&
	    type < HK_TYPE_MAX) {
		cpumask_copy(dyn_allowed, housekeeping.cpumasks[type]);
		cpumask_copy(dyn_possible, housekeeping.cpumasks[type]);
	}
#endif
}

static void __init housekeeping_setup_type(enum hk_type type,
					   cpumask_var_t housekeeping_staging)
{

	alloc_bootmem_cpumask_var(&housekeeping.cpumasks[type]);
	cpumask_copy(housekeeping.cpumasks[type],
		     housekeeping_staging);
}

static int __init housekeeping_setup(char *str, unsigned long flags)
{
	cpumask_var_t non_housekeeping_mask, housekeeping_staging;
	unsigned int first_cpu;
	int err = 0;

	if ((flags & HK_FLAG_TICK) && !(housekeeping.flags & HK_FLAG_TICK)) {
		if (!IS_ENABLED(CONFIG_NO_HZ_FULL)) {
			pr_warn("Housekeeping: nohz unsupported."
				" Build with CONFIG_NO_HZ_FULL\n");
			return 0;
		}
	}

	alloc_bootmem_cpumask_var(&non_housekeeping_mask);
	if (cpulist_parse(str, non_housekeeping_mask) < 0) {
		pr_warn("Housekeeping: nohz_full= or isolcpus= incorrect CPU range\n");
		goto free_non_housekeeping_mask;
	}

	alloc_bootmem_cpumask_var(&housekeeping_staging);
	cpumask_andnot(housekeeping_staging,
		       cpu_possible_mask, non_housekeeping_mask);

	first_cpu = cpumask_first_and(cpu_present_mask, housekeeping_staging);
	if (first_cpu >= nr_cpu_ids || first_cpu >= setup_max_cpus) {
		__cpumask_set_cpu(smp_processor_id(), housekeeping_staging);
		__cpumask_clear_cpu(smp_processor_id(), non_housekeeping_mask);
		if (!housekeeping.flags) {
			pr_warn("Housekeeping: must include one present CPU, "
				"using boot CPU:%d\n", smp_processor_id());
		}
	}

	if (cpumask_empty(non_housekeeping_mask))
		goto free_housekeeping_staging;

	if (!housekeeping.flags) {
		/* First setup call ("nohz_full=" or "isolcpus=") */
		enum hk_type type;

		for_each_set_bit(type, &flags, HK_TYPE_MAX)
			housekeeping_setup_type(type, housekeeping_staging);
	} else {
		/* Second setup call ("nohz_full=" after "isolcpus=" or the reverse) */
		enum hk_type type;
		unsigned long iter_flags = flags & housekeeping.flags;

		for_each_set_bit(type, &iter_flags, HK_TYPE_MAX) {
			if (!cpumask_equal(housekeeping_staging,
					   housekeeping.cpumasks[type])) {
				pr_warn("Housekeeping: nohz_full= must match isolcpus=\n");
				goto free_housekeeping_staging;
			}
		}

		iter_flags = flags & ~housekeeping.flags;

		for_each_set_bit(type, &iter_flags, HK_TYPE_MAX)
			housekeeping_setup_type(type, housekeeping_staging);
	}

	if ((flags & HK_FLAG_TICK) && !(housekeeping.flags & HK_FLAG_TICK))
		tick_nohz_full_setup(non_housekeeping_mask);

	housekeeping.flags |= flags;
	err = 1;

free_housekeeping_staging:
	free_bootmem_cpumask_var(housekeeping_staging);
free_non_housekeeping_mask:
	free_bootmem_cpumask_var(non_housekeeping_mask);

	return err;
}

static int __init housekeeping_nohz_full_setup(char *str)
{
	unsigned long flags;

	flags = HK_FLAG_TICK | HK_FLAG_WQ | HK_FLAG_TIMER | HK_FLAG_RCU |
		HK_FLAG_MISC | HK_FLAG_KTHREAD;

	return housekeeping_setup(str, flags);
}
__setup("nohz_full=", housekeeping_nohz_full_setup);

static int __init housekeeping_isolcpus_setup(char *str)
{
	unsigned long flags = 0;
	bool illegal = false;
	char *par;
	int len;

	while (isalpha(*str)) {
		if (!strncmp(str, "nohz,", 5)) {
			str += 5;
			flags |= HK_FLAG_TICK;
			continue;
		}

		if (!strncmp(str, "domain,", 7)) {
			str += 7;
			flags |= HK_FLAG_DOMAIN;
			continue;
		}

		if (!strncmp(str, "managed_irq,", 12)) {
			str += 12;
			flags |= HK_FLAG_MANAGED_IRQ;
			continue;
		}

		/*
		 * Skip unknown sub-parameter and validate that it is not
		 * containing an invalid character.
		 */
		for (par = str, len = 0; *str && *str != ','; str++, len++) {
			if (!isalpha(*str) && *str != '_')
				illegal = true;
		}

		if (illegal) {
			pr_warn("isolcpus: Invalid flag %.*s\n", len, par);
			return 0;
		}

		pr_info("isolcpus: Skipped unknown flag %.*s\n", len, par);
		str++;
	}

	/* Default behaviour for isolcpus without flags */
	if (!flags)
		flags |= HK_FLAG_DOMAIN;

	return housekeeping_setup(str, flags);
}
__setup("isolcpus=", housekeeping_isolcpus_setup);

#ifdef CONFIG_CGROUP_SCHED
static int dyn_isolcpus_show(struct seq_file *s, void *p)
{
	seq_printf(s, "%*pbl\n", cpumask_pr_args(dyn_isolated));

	return 0;
}

static int dyn_isolcpus_open(struct inode *inode, struct file *file)
{
	return single_open(file, dyn_isolcpus_show, NULL);
}

void wilds_cpus_allowed(struct cpumask *pmask)
{
	if (static_branch_unlikely(&dyn_isolcpus_enabled))
		cpumask_and(pmask, pmask, dyn_allowed);
}

/*
 * Update the cpumask of all wild tasks (non-kthread tasks whose
 * cpus_ptr matches @old_allowed) to @new_allowed.
 *
 * set_cpus_allowed_ptr() may sleep, so we cannot call it under
 * tasklist_lock or any spinlock.  Instead we walk the cpu cgroup
 * hierarchy under cgroup_mutex (a mutex, sleepable) and use
 * css_task_iter to enumerate tasks in each cgroup — the same
 * pattern used by cpuset's update_tasks_cpumask().
 */
void update_wilds_cpumask(cpumask_var_t new_allowed, cpumask_var_t old_allowed)
{
	struct cgroup_subsys_state *pos;
	struct css_task_iter it;
	struct task_struct *task;

	cgroup_lock();
	css_for_each_descendant_pre(pos, &root_task_group.css) {
		css_task_iter_start(pos, 0, &it);
		while ((task = css_task_iter_next(&it))) {
			/* Percpu kthreads are ignored */
			if ((task->flags & PF_KTHREAD) && kthread_is_per_cpu(task))
				continue;

			if (!cpumask_equal(task->cpus_ptr, old_allowed))
				continue;

			set_cpus_allowed_ptr(task, new_allowed);
		}
		css_task_iter_end(&it);
	}
	cgroup_unlock();
}

static DEFINE_MUTEX(dyn_isolcpus_mutex);

static ssize_t write_dyn_isolcpus(struct file *file, const char __user *buf,
					size_t count, loff_t *ppos)
{
	int ret = count;
	cpumask_var_t isolated;
	cpumask_var_t new_allowed;
	cpumask_var_t old_allowed;

	mutex_lock(&dyn_isolcpus_mutex);

	if (!zalloc_cpumask_var(&isolated, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto out;
	}

	if (!zalloc_cpumask_var(&new_allowed, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto free_isolated;
	}

	if (!zalloc_cpumask_var(&old_allowed, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto free_new_allowed;
	}

	if (cpumask_parselist_user(buf, count, isolated)) {
		ret = -EINVAL;
		goto free_all;
	}

	if (!cpumask_subset(isolated, dyn_possible)) {
		ret = -EINVAL;
		goto free_all;
	}

	/* At least reserve one for wild tasks to run */
	cpumask_andnot(new_allowed, dyn_possible, isolated);
	if (!cpumask_intersects(new_allowed, cpu_online_mask)) {
		ret = -EINVAL;
		goto free_all;
	}

	cpumask_copy(old_allowed, dyn_allowed);
	cpumask_copy(dyn_allowed, new_allowed);
	cpumask_copy(dyn_isolated, isolated);

	if (cpumask_empty(dyn_isolated))
		static_branch_disable(&dyn_isolcpus_enabled);
	else
		static_branch_enable(&dyn_isolcpus_enabled);

	update_wilds_cpumask(new_allowed, old_allowed);

	rebuild_sched_domains();
	update_group_balancer_root_cpumask();
	workqueue_set_unbound_cpumask(new_allowed);

free_all:
	free_cpumask_var(old_allowed);
free_new_allowed:
	free_cpumask_var(new_allowed);
free_isolated:
	free_cpumask_var(isolated);
out:
	mutex_unlock(&dyn_isolcpus_mutex);

	return ret;
}

static const struct proc_ops proc_dyn_isolcpus_operations = {
	.proc_open		= dyn_isolcpus_open,
	.proc_read		= seq_read,
	.proc_write		= write_dyn_isolcpus,
	.proc_lseek		= noop_llseek,
	.proc_release		= single_release,
};

static int __init dyn_isolcpus_init(void)
{
	if (dyn_isolcpus_ready &&
	    !proc_create("dyn_isolcpus", 0200, NULL,
				&proc_dyn_isolcpus_operations)) {
		dyn_isolcpus_ready = false;
		free_dyn_masks();
	}

	if (!dyn_isolcpus_ready)
		pr_err("Initialize Dynamical Isolation Failed\n");

	return 0;
}
early_initcall(dyn_isolcpus_init);
#endif

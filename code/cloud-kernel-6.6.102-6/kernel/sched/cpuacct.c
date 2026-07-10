// SPDX-License-Identifier: GPL-2.0

/*
 * CPU accounting code for task groups.
 *
 * Based on the work by Paul Menage (menage@google.com) and Balbir Singh
 * (balbir@in.ibm.com).
 */

#include <linux/ck_kabi.h>

/* Time spent by the tasks of the CPU accounting group executing in ... */
enum cpuacct_stat_index {
	CPUACCT_STAT_USER,	/* ... user mode */
	CPUACCT_STAT_SYSTEM,	/* ... kernel mode */

	CPUACCT_STAT_NSTATS,
};

static const char * const cpuacct_stat_desc[] = {
	[CPUACCT_STAT_USER] = "user",
	[CPUACCT_STAT_SYSTEM] = "system",
};

struct cpuacct_prev_cputime {
	struct prev_cputime prev_cputime1; /* utime and stime */
	struct prev_cputime prev_cputime2; /* user and nice */
} ____cacheline_aligned;

/* track CPU usage of a group of tasks and its child groups */
struct cpuacct {
	struct cgroup_subsys_state	css;
	/* cpuusage holds pointer to a u64-type object on every CPU */
	u64 __percpu	*cpuusage;
	struct cpuacct_prev_cputime __percpu *prev_cputime;
	struct kernel_cpustat __percpu	*cpustat;

	CK_KABI_RESERVE(1)
	CK_KABI_RESERVE(2)
	CK_KABI_RESERVE(3)
	CK_KABI_RESERVE(4)
};

static inline struct cpuacct *cgroup_ca(struct cgroup *cgrp)
{
	return container_of(global_cgroup_css(cgrp, cpuacct_cgrp_id),
				struct cpuacct, css);
}

static inline struct cpuacct *css_ca(struct cgroup_subsys_state *css)
{
	return css ? container_of(css, struct cpuacct, css) : NULL;
}

/* Return CPU accounting group to which this task belongs */
static inline struct cpuacct *task_ca(struct task_struct *tsk)
{
	return css_ca(task_css(tsk, cpuacct_cgrp_id));
}

static inline struct cpuacct *parent_ca(struct cpuacct *ca)
{
	return css_ca(ca->css.parent);
}

static DEFINE_PER_CPU(u64, root_cpuacct_cpuusage);
static DEFINE_PER_CPU(struct cpuacct_prev_cputime, root_cpuacct_prev_cputime);

static inline struct task_group *task_tg(struct task_struct *tsk)
{
	return css_tg(task_css(tsk, cpu_cgrp_id));
}

static struct cpuacct root_cpuacct = {
	.cpustat	= &kernel_cpustat,
	.prev_cputime	= &root_cpuacct_prev_cputime,
	.cpuusage	= &root_cpuacct_cpuusage,
};

#ifdef CONFIG_SCHED_SLI

void task_cpu_increase_nr_migrations(struct task_struct *tsk)
{
	struct task_group *tg;

	rcu_read_lock();
	tg = task_tg(tsk);
	if (tg)
		this_cpu_ptr(tg->alistats)->nr_migrations++;
	rcu_read_unlock();
}

#endif

#ifdef CONFIG_SCHED_SLI
static DEFINE_SPINLOCK(sli_tg_lock);
LIST_HEAD(sli_tg_list);

void tg_enable_sli(struct task_group *tg, bool val)
{
	spin_lock(&sli_tg_lock);
	if (val && !READ_ONCE(tg->sli_enabled))
		list_add_tail_rcu(&tg->sli_list, &sli_tg_list);
	else if (!val && READ_ONCE(tg->sli_enabled))
		list_del_rcu(&tg->sli_list);
	WRITE_ONCE(tg->sli_enabled, val);
	spin_unlock(&sli_tg_lock);
}

void create_rich_container_reaper(struct task_struct *tsk)
{
	struct task_group *tg, *parent_tg;

	if (thread_group_leader(tsk)) {
		rcu_read_lock();
		tg = task_tg(tsk);
		if (!tg || !in_rich_container(tsk, RC_LOADAVG)) {
			rcu_read_unlock();
			return;
		}

		tg_enable_sli(tg, true);
		parent_tg = tg->parent;
		if (parent_tg && parent_tg != &root_task_group)
			tg_enable_sli(parent_tg, true);
		rcu_read_unlock();
	}
}

int enable_sli_write(struct cgroup_subsys_state *css,
		struct cftype *cft, u64 val)
{
	struct task_group *tg;

	tg = cgroup_tg(css->cgroup);
	if (unlikely(!tg)) {
		WARN_ONCE(1, "cgroup \"cpu,cpuacct\" are not bound together");
		return -EOPNOTSUPP;
	}

	tg_enable_sli(tg, !!val);

	return 0;
}

u64 enable_sli_read(struct cgroup_subsys_state *css, struct cftype *cft)
{
	struct task_group *tg;

	tg = cgroup_tg(css->cgroup);
	if (unlikely(!tg)) {
		WARN_ONCE(1, "cgroup \"cpu,cpuacct\" are not bound together");
		return -EOPNOTSUPP;
	}

	return READ_ONCE(tg->sli_enabled);
}
#endif

/* Create a new CPU accounting group */
static struct cgroup_subsys_state *
cpuacct_css_alloc(struct cgroup_subsys_state *parent_css)
{
	struct cpuacct *ca;
	int i;

	if (!parent_css)
		return &root_cpuacct.css;

	ca = kzalloc(sizeof(*ca), GFP_KERNEL);
	if (!ca)
		goto out;

	ca->cpuusage = alloc_percpu(u64);
	if (!ca->cpuusage)
		goto out_free_ca;

	ca->cpustat = alloc_percpu(struct kernel_cpustat);
	if (!ca->cpustat)
		goto out_free_cpuusage;

	ca->prev_cputime = alloc_percpu(struct cpuacct_prev_cputime);
	if (!ca->prev_cputime)
		goto out_free_cpustat;

	for_each_possible_cpu(i) {
		prev_cputime_init(
			&per_cpu_ptr(ca->prev_cputime, i)->prev_cputime1);
		prev_cputime_init(
			&per_cpu_ptr(ca->prev_cputime, i)->prev_cputime2);
	}

	return &ca->css;

out_free_cpustat:
	free_percpu(ca->cpustat);
out_free_cpuusage:
	free_percpu(ca->cpuusage);
out_free_ca:
	kfree(ca);
out:
	return ERR_PTR(-ENOMEM);
}

/* Destroy an existing CPU accounting group */
static void cpuacct_css_free(struct cgroup_subsys_state *css)
{
	struct cpuacct *ca = css_ca(css);

	free_percpu(ca->prev_cputime);
	free_percpu(ca->cpustat);
	free_percpu(ca->cpuusage);
	kfree(ca);
}

static u64 cpuacct_cpuusage_read(struct cpuacct *ca, int cpu,
				 enum cpuacct_stat_index index)
{
	u64 *cpuusage = per_cpu_ptr(ca->cpuusage, cpu);
	u64 *cpustat = per_cpu_ptr(ca->cpustat, cpu)->cpustat;
	u64 data;

	/*
	 * We allow index == CPUACCT_STAT_NSTATS here to read
	 * the sum of usages.
	 */
	if (WARN_ON_ONCE(index > CPUACCT_STAT_NSTATS))
		return 0;

#ifndef CONFIG_64BIT
	/*
	 * Take rq->lock to make 64-bit read safe on 32-bit platforms.
	 */
	raw_spin_rq_lock_irq(cpu_rq(cpu));
#endif

	switch (index) {
	case CPUACCT_STAT_USER:
		data = cpustat[CPUTIME_USER] + cpustat[CPUTIME_NICE];
		break;
	case CPUACCT_STAT_SYSTEM:
		data = cpustat[CPUTIME_SYSTEM] + cpustat[CPUTIME_IRQ] +
			cpustat[CPUTIME_SOFTIRQ];
		break;
	case CPUACCT_STAT_NSTATS:
		data = *cpuusage;
		break;
	}

#ifndef CONFIG_64BIT
	raw_spin_rq_unlock_irq(cpu_rq(cpu));
#endif

	return data;
}

static void cpuacct_cpuusage_write(struct cpuacct *ca, int cpu)
{
	u64 *cpuusage = per_cpu_ptr(ca->cpuusage, cpu);
	u64 *cpustat = per_cpu_ptr(ca->cpustat, cpu)->cpustat;

	/* Don't allow to reset global kernel_cpustat */
	if (ca == &root_cpuacct)
		return;

#ifndef CONFIG_64BIT
	/*
	 * Take rq->lock to make 64-bit write safe on 32-bit platforms.
	 */
	raw_spin_rq_lock_irq(cpu_rq(cpu));
#endif
	*cpuusage = 0;
	cpustat[CPUTIME_USER] = cpustat[CPUTIME_NICE] = 0;
	cpustat[CPUTIME_SYSTEM] = cpustat[CPUTIME_IRQ] = 0;
	cpustat[CPUTIME_SOFTIRQ] = 0;

#ifndef CONFIG_64BIT
	raw_spin_rq_unlock_irq(cpu_rq(cpu));
#endif
}

/* Return total CPU usage (in nanoseconds) of a group */
static u64 __cpuusage_read(struct cgroup_subsys_state *css,
			   enum cpuacct_stat_index index)
{
	struct cpuacct *ca = css_ca(css);
	u64 totalcpuusage = 0;
	int i;

	for_each_possible_cpu(i)
		totalcpuusage += cpuacct_cpuusage_read(ca, i, index);

	return totalcpuusage;
}

static u64 cpuusage_user_read(struct cgroup_subsys_state *css,
			      struct cftype *cft)
{
	return __cpuusage_read(css, CPUACCT_STAT_USER);
}

static u64 cpuusage_sys_read(struct cgroup_subsys_state *css,
			     struct cftype *cft)
{
	return __cpuusage_read(css, CPUACCT_STAT_SYSTEM);
}

static u64 cpuusage_read(struct cgroup_subsys_state *css, struct cftype *cft)
{
	return __cpuusage_read(css, CPUACCT_STAT_NSTATS);
}

static int cpuusage_write(struct cgroup_subsys_state *css, struct cftype *cft,
			  u64 val)
{
	struct cpuacct *ca = css_ca(css);
	int cpu;

	/*
	 * Only allow '0' here to do a reset.
	 */
	if (val)
		return -EINVAL;

	for_each_possible_cpu(cpu)
		cpuacct_cpuusage_write(ca, cpu);

	return 0;
}

static int __cpuacct_percpu_seq_show(struct seq_file *m,
				     enum cpuacct_stat_index index)
{
	struct cpuacct *ca = css_ca(seq_css(m));
	u64 percpu;
	int i;

	for_each_possible_cpu(i) {
		percpu = cpuacct_cpuusage_read(ca, i, index);
		seq_printf(m, "%llu ", (unsigned long long) percpu);
	}
	seq_printf(m, "\n");
	return 0;
}

static int cpuacct_percpu_user_seq_show(struct seq_file *m, void *V)
{
	return __cpuacct_percpu_seq_show(m, CPUACCT_STAT_USER);
}

static int cpuacct_percpu_sys_seq_show(struct seq_file *m, void *V)
{
	return __cpuacct_percpu_seq_show(m, CPUACCT_STAT_SYSTEM);
}

static int cpuacct_percpu_seq_show(struct seq_file *m, void *V)
{
	return __cpuacct_percpu_seq_show(m, CPUACCT_STAT_NSTATS);
}

static int cpuacct_all_seq_show(struct seq_file *m, void *V)
{
	struct cpuacct *ca = css_ca(seq_css(m));
	int index;
	int cpu;

	seq_puts(m, "cpu");
	for (index = 0; index < CPUACCT_STAT_NSTATS; index++)
		seq_printf(m, " %s", cpuacct_stat_desc[index]);
	seq_puts(m, "\n");

	for_each_possible_cpu(cpu) {
		seq_printf(m, "%d", cpu);
		for (index = 0; index < CPUACCT_STAT_NSTATS; index++)
			seq_printf(m, " %llu",
				   cpuacct_cpuusage_read(ca, cpu, index));
		seq_puts(m, "\n");
	}
	return 0;
}

static int cpuacct_stats_show(struct seq_file *sf, void *v)
{
	struct cpuacct *ca = css_ca(seq_css(sf));
	struct task_cputime cputime;
	u64 val[CPUACCT_STAT_NSTATS];
	int cpu;
	int stat;

	memset(&cputime, 0, sizeof(cputime));
	for_each_possible_cpu(cpu) {
		u64 *cpustat = per_cpu_ptr(ca->cpustat, cpu)->cpustat;

		cputime.utime += cpustat[CPUTIME_USER];
		cputime.utime += cpustat[CPUTIME_NICE];
		cputime.stime += cpustat[CPUTIME_SYSTEM];
		cputime.stime += cpustat[CPUTIME_IRQ];
		cputime.stime += cpustat[CPUTIME_SOFTIRQ];

		cputime.sum_exec_runtime += *per_cpu_ptr(ca->cpuusage, cpu);
	}

	cputime_adjust(&cputime, &seq_css(sf)->cgroup->prev_cputime,
		&val[CPUACCT_STAT_USER], &val[CPUACCT_STAT_SYSTEM]);

	for (stat = 0; stat < CPUACCT_STAT_NSTATS; stat++) {
		seq_printf(sf, "%s %llu\n", cpuacct_stat_desc[stat],
			nsec_to_clock_t(val[stat]));
	}

	return 0;
}

#ifdef CONFIG_SCHED_SLI
void __get_cgroup_avenrun(struct task_group *tg, unsigned long *loads,
		unsigned long offset, int shift, bool running)
{
	unsigned long *avenrun;

	if (running)
		avenrun = tg->avenrun_r;
	else
		avenrun = tg->avenrun;

	loads[0] = (avenrun[0] + offset) << shift;
	loads[1] = (avenrun[1] + offset) << shift;
	loads[2] = (avenrun[2] + offset) << shift;
}

static inline unsigned long nr_uninterruptible(void)
{
	unsigned long i, sum = 0;

	for_each_possible_cpu(i)
		sum += cpu_rq(i)->nr_uninterruptible;

	/*
	 * Since we read the counters lockless, it might be slightly
	 * inaccurate. Do not allow it to go below zero though:
	 */
	if (unlikely((long)sum < 0))
		sum = 0;

	return sum;
}

#ifdef CONFIG_CFS_BANDWIDTH
static inline bool tg_cfs_throttled(struct task_group *tg, int cpu)
{
	return tg->cfs_rq[cpu]->throttle_count;
}
#else
static inline bool tg_cfs_throttled(struct task_group *tg, int cpu)
{
	return false;
}
#endif

#ifdef CONFIG_RT_GROUP_SCHED
static inline bool tg_rt_throttled(struct task_group *tg, int cpu)
{
	return tg->rt_rq[cpu]->rt_throttled && !tg->rt_rq[cpu]->rt_nr_boosted;
}
#endif

unsigned long tg_running(struct task_group *tg, int cpu)
{
	unsigned long nr_running = 0;
	/* Make sure it is only called for non-root cpuacct */
	if (tg == &root_task_group)
		return 0;

	rcu_read_lock();
	if (!tg_cfs_throttled(tg, cpu))
		nr_running += tg->cfs_rq[cpu]->h_nr_runnable;
#ifdef CONFIG_RT_GROUP_SCHED
	/*
	 * tg->rt_rq is only allocated when rt_group_sched is runtime-enabled.
	 * With rt_group_sched=off (default when CONFIG_RT_GROUP_SCHED_DEFAULT_
	 * DISABLED=y), alloc_rt_sched_group() short-circuits and leaves
	 * tg->rt_rq NULL for non-root task_groups, so we must guard the access.
	 */
	if (rt_group_sched_enabled() && tg->rt_rq && tg->rt_rq[cpu]) {
		if (!tg_rt_throttled(tg, cpu))
			nr_running += tg->rt_rq[cpu]->rt_nr_running;
	}
#endif
	/* SCHED_DEADLINE doesn't support cgroup yet */

	rcu_read_unlock();
	return nr_running;
}

unsigned long tg_uninterruptible(struct task_group *tg, int cpu)
{
	unsigned long nr = 0;

	/* Make sure it is only called for non-root cpuacct */
	if (tg == &root_task_group)
		return nr;

	rcu_read_lock();

	nr = tg->cfs_rq[cpu]->nr_uninterruptible;
#ifdef CONFIG_RT_GROUP_SCHED
	/* See comment in tg_running(): tg->rt_rq may be NULL at runtime. */
	if (rt_group_sched_enabled() && tg->rt_rq && tg->rt_rq[cpu])
		nr += tg->rt_rq[cpu]->nr_uninterruptible;
#endif

	rcu_read_unlock();
	return nr;
}

void cgroup_idle_start(struct sched_entity *se)
{
	unsigned long flags;
	u64 clock;

	if (!schedstat_enabled())
		return;

	clock = __rq_clock_broken(se->cfs_rq->rq);

	write_seqcount_begin(&se->idle_seqcount);
	__schedstat_set(se->cg_idle_start, clock);
	write_seqcount_end(&se->idle_seqcount);

	spin_lock_irqsave(&se->iowait_lock, flags);
	if (schedstat_val(se->cg_nr_iowait))
		__schedstat_set(se->cg_iowait_start, clock);
	spin_unlock_irqrestore(&se->iowait_lock, flags);
}

void cgroup_idle_end(struct sched_entity *se)
{
	unsigned long flags;
	u64 clock;
	u64 idle_start, iowait_start;

	if (!schedstat_enabled())
		return;

	clock = __rq_clock_broken(se->cfs_rq->rq);

	write_seqcount_begin(&se->idle_seqcount);
	idle_start = schedstat_val(se->cg_idle_start);
	__schedstat_add(se->cg_idle_sum, clock - idle_start);
	__schedstat_set(se->cg_idle_start, 0);
	write_seqcount_end(&se->idle_seqcount);

	spin_lock_irqsave(&se->iowait_lock, flags);
	if (schedstat_val(se->cg_nr_iowait)) {
		iowait_start = schedstat_val(se->cg_iowait_start);
		__schedstat_add(se->cg_iowait_sum, clock - iowait_start);
		__schedstat_set(se->cg_iowait_start, 0);
	}
	spin_unlock_irqrestore(&se->iowait_lock, flags);
}

void cpuacct_cpuset_changed(struct cgroup *cgrp, struct cpumask *deleted,
		struct cpumask *added)
{
	struct rq_flags rf;
	struct rq *rq;
	struct task_group *tg;
	struct sched_entity *se;
	int cpu;

	if (!schedstat_enabled())
		return;

	rcu_read_lock();
	tg = cgroup_tg(cgrp);

	if (!tg) {
		rcu_read_unlock();
		return;
	}

	if (added) {
		/* Mark newly added cpus as newly-idle */
		for_each_cpu(cpu, added) {
			rq = cpu_rq(cpu);
			se = tg->se[cpu];
			rq_lock_irqsave(rq, &rf);
			cgroup_idle_start(se);
			rq_unlock_irqrestore(rq, &rf);
			__schedstat_add(se->cg_ineffective_sum,
				__rq_clock_broken(rq) -
					se->cg_ineffective_start);
			__schedstat_set(se->cg_ineffective_start, 0);
		}
	}

	if (deleted) {
		/* Mark ineffective_cpus as idle-invalid */
		for_each_cpu(cpu, deleted) {
			rq = cpu_rq(cpu);
			se = tg->se[cpu];
			rq_lock_irqsave(rq, &rf);
			cgroup_idle_end(se);
			rq_unlock_irqrestore(rq, &rf);
			/* Use __rq_clock_broken to avoid warning */
			__schedstat_set(se->cg_ineffective_start,
				__rq_clock_broken(rq));
		}
	}

	rcu_read_unlock();
}

static void task_group_calc_load(struct task_group *tg)
{
	long active = 0, active_r = 0, nr_r;
	int cpu;

	if (tg != &root_task_group) {
		for_each_possible_cpu(cpu) {
			nr_r = tg_running(tg, cpu);
			active   += nr_r;
			active_r += nr_r;
			active += tg_uninterruptible(tg, cpu);
		}
		active = active > 0 ? active * FIXED_1 : 0;
		tg->avenrun[0] = calc_load(tg->avenrun[0], EXP_1, active);
		tg->avenrun[1] = calc_load(tg->avenrun[1], EXP_5, active);
		tg->avenrun[2] = calc_load(tg->avenrun[2], EXP_15, active);

		active_r = active_r > 0 ? active_r * FIXED_1 : 0;
		tg->avenrun_r[0] = calc_load(tg->avenrun_r[0],
				EXP_1, active_r);
		tg->avenrun_r[1] = calc_load(tg->avenrun_r[1],
				EXP_5, active_r);
		tg->avenrun_r[2] = calc_load(tg->avenrun_r[2],
				EXP_15, active_r);
	} else {
		tg->avenrun[0] = avenrun[0];
		tg->avenrun[1] = avenrun[1];
		tg->avenrun[2] = avenrun[2];

		tg->avenrun_r[0] = avenrun_r[0];
		tg->avenrun_r[1] = avenrun_r[1];
		tg->avenrun_r[2] = avenrun_r[2];
	}
}

/*
 * We walk cpuacct whose SLI is enabled to perform per-cgroup load calculation
 * the overhead is acceptable if SLI is not enabled for most of the cgroups.
 */
void calc_cgroup_load(void)
{
	struct task_group *tg;

	rcu_read_lock();
	list_for_each_entry_rcu(tg, &sli_tg_list, sli_list)
		task_group_calc_load(tg);
	rcu_read_unlock();
}

void __cpuacct_get_usage(struct cgroup_subsys_state *css, int cpu,
					struct cpuacct_usage_result *res)
{
	struct cpuacct *ca;
	struct kernel_cpustat *kcpustat;
	u64 *cpuusage;
	struct cpuacct_prev_cputime *prev_cputime;
	struct task_cputime cputime;
	u64 tick_user, tick_nice, tick_sys;
	u64 left, right;

	ca = cgroup_ca(css->cgroup);
	if (!ca)
		return;

	kcpustat = per_cpu_ptr(ca->cpustat, cpu);

	cpuusage = per_cpu_ptr(ca->cpuusage, cpu);

	prev_cputime = per_cpu_ptr(ca->prev_cputime, cpu);
	tick_user = kcpustat->cpustat[CPUTIME_USER];
	tick_nice = kcpustat->cpustat[CPUTIME_NICE];
	tick_sys = kcpustat->cpustat[CPUTIME_SYSTEM];

	/* Calculate system run time */
	cputime.sum_exec_runtime = *cpuusage;
	cputime.utime = tick_user + tick_nice;
	cputime.stime = tick_sys;
	cputime_adjust(&cputime, &prev_cputime->prev_cputime1, &left, &right);
	res->system = right;

	/* Calculate user and nice run time */
	cputime.sum_exec_runtime = left; /* user + nice */
	cputime.utime = tick_user;
	cputime.stime = tick_nice;
	cputime_adjust(&cputime, &prev_cputime->prev_cputime2, &left, &right);
	res->user = left;
	res->nice = right;

	res->irq = kcpustat->cpustat[CPUTIME_IRQ];
	res->softirq = kcpustat->cpustat[CPUTIME_SOFTIRQ];

	res->guest = kcpustat->cpustat[CPUTIME_GUEST];
	res->guest_nice = kcpustat->cpustat[CPUTIME_GUEST_NICE];
}

static int cpuacct_proc_stats_show(struct seq_file *sf, void *v)
{
	struct cgroup_subsys_state *css = seq_css(sf);
	struct cpuacct *ca = css_ca(css);
	struct cgroup *cgrp = css->cgroup;
	struct task_group *tg;
	u64 user, nice, system, idle, iowait, irq, softirq, steal, guest;
	u64 nr_migrations = 0;
	struct cpu_alistats *alistats;
	unsigned long load, avnrun[3], avnrun_r[3];
	unsigned long nr_run = 0, nr_uninter = 0;
	int cpu;

	user = nice = system = idle = iowait =
		irq = softirq = steal = guest = 0;

	tg = cgroup_tg(cgrp);
	if (unlikely(!tg)) {
		WARN_ONCE(1, "cgroup \"cpu,cpuacct\" are not bound together");
		return -EOPNOTSUPP;
	}

	if (ca != &root_cpuacct) {
		struct cpuacct_usage_result res;

		for_each_possible_cpu(cpu) {
			if (!housekeeping_cpu(cpu, HK_TYPE_DOMAIN))
				continue;

			rcu_read_lock();
			__cgroup_get_usage_result(css, cpu, &res);
			rcu_read_unlock();

			user += res.user;
			nice += res.nice;
			system += res.system;
			irq += res.irq;
			softirq += res.softirq;
			steal += res.steal;
			guest += res.guest;
			guest += res.guest_nice;
			iowait += res.iowait;
			idle += res.idle;

			alistats = per_cpu_ptr(tg->alistats, cpu);
			nr_migrations += alistats->nr_migrations;
			nr_run += tg_running(tg, cpu);
			nr_uninter += tg_uninterruptible(tg, cpu);
		}

		__get_cgroup_avenrun(tg, avnrun, FIXED_1/200, 0, false);
		__get_cgroup_avenrun(tg, avnrun_r, FIXED_1/200, 0, true);
	} else {
		struct kernel_cpustat *kcpustat;

		for_each_possible_cpu(cpu) {
			kcpustat = per_cpu_ptr(ca->cpustat, cpu);
			user += kcpustat->cpustat[CPUTIME_USER];
			nice += kcpustat->cpustat[CPUTIME_NICE];
			system += kcpustat->cpustat[CPUTIME_SYSTEM];
			irq += kcpustat->cpustat[CPUTIME_IRQ];
			softirq += kcpustat->cpustat[CPUTIME_SOFTIRQ];
			guest += kcpustat->cpustat[CPUTIME_GUEST];
			guest += kcpustat->cpustat[CPUTIME_GUEST_NICE];
			idle += get_idle_time(kcpustat, cpu);
			iowait += get_iowait_time(kcpustat, cpu);
			steal += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
			alistats = per_cpu_ptr(tg->alistats, cpu);
			nr_migrations += alistats->nr_migrations;
		}

		nr_run = nr_running();
		nr_uninter = nr_uninterruptible();

		get_avenrun(avnrun, FIXED_1/200, 0);
		get_avenrun_r(avnrun_r, FIXED_1/200, 0);
	}

	seq_printf(sf, "user %lld\n", nsec_to_clock_t(user));
	seq_printf(sf, "nice %lld\n", nsec_to_clock_t(nice));
	seq_printf(sf, "system %lld\n", nsec_to_clock_t(system));
	seq_printf(sf, "idle %lld\n", nsec_to_clock_t(idle));
	seq_printf(sf, "iowait %lld\n", nsec_to_clock_t(iowait));
	seq_printf(sf, "irq %lld\n", nsec_to_clock_t(irq));
	seq_printf(sf, "softirq %lld\n", nsec_to_clock_t(softirq));
	seq_printf(sf, "steal %lld\n", nsec_to_clock_t(steal));
	seq_printf(sf, "guest %lld\n", nsec_to_clock_t(guest));

	load = LOAD_INT(avnrun[0]) * 100 + LOAD_FRAC(avnrun[0]);
	seq_printf(sf, "load average(1min) %lld\n", (u64)load);
	load = LOAD_INT(avnrun[1]) * 100 + LOAD_FRAC(avnrun[1]);
	seq_printf(sf, "load average(5min) %lld\n", (u64)load);
	load = LOAD_INT(avnrun[2]) * 100 + LOAD_FRAC(avnrun[2]);
	seq_printf(sf, "load average(15min) %lld\n", (u64)load);

	seq_printf(sf, "nr_running %lld\n", (u64)nr_run);
	if ((long) nr_uninter < 0)
		nr_uninter = 0;
	seq_printf(sf, "nr_uninterruptible %lld\n", (u64)nr_uninter);
	seq_printf(sf, "nr_migrations %lld\n", (u64)nr_migrations);

	load = LOAD_INT(avnrun_r[0]) * 100 + LOAD_FRAC(avnrun_r[0]);
	seq_printf(sf, "running load average(1min) %lld\n", (u64)load);
	load = LOAD_INT(avnrun_r[1]) * 100 + LOAD_FRAC(avnrun_r[1]);
	seq_printf(sf, "running load average(5min) %lld\n", (u64)load);
	load = LOAD_INT(avnrun_r[2]) * 100 + LOAD_FRAC(avnrun_r[2]);
	seq_printf(sf, "running load average(15min) %lld\n", (u64)load);

	return 0;
}

static int cpuacct_sched_cfs_show(struct seq_file *sf, void *v)
{
	struct cgroup *cgrp = seq_css(sf)->cgroup;
	struct task_group *tg = cgroup_tg(cgrp);
	struct sched_entity *se;
	struct sched_statistics *stats;
	int cpu;
	u64 wait_max = 0, wait_sum = 0, wait_sum_other = 0, exec_sum = 0;

	if (!schedstat_enabled())
		goto out_show;

	rcu_read_lock();
	tg = cgroup_tg(cgrp);
	if (unlikely(!tg)) {
		WARN_ONCE(1, "cgroup \"cpu,cpuacct\" are not bound together");
		goto rcu_unlock_show;
	}

	for_each_online_cpu(cpu) {
		se = tg->se[cpu];
		if (!se)
			continue;
		stats = __schedstats_from_se(se);
		exec_sum += schedstat_val(se->sum_exec_runtime);
		wait_sum_other +=
			schedstat_val(stats->parent_wait_contrib);
		wait_sum += schedstat_val(stats->wait_sum);
		wait_max = max(wait_max, schedstat_val(stats->wait_max));
	}
rcu_unlock_show:
	rcu_read_unlock();
out_show:
	/* [Serve time] [On CPU time] [Queue other time] [Queue sibling time] [Queue max time] */
	seq_printf(sf, "%lld %lld %lld %lld %lld\n",
			exec_sum + wait_sum, exec_sum, wait_sum_other,
			wait_sum - wait_sum_other, wait_max);

	return 0;
}
#endif

static struct cftype files[] = {
	{
		.name = "usage",
		.read_u64 = cpuusage_read,
		.write_u64 = cpuusage_write,
	},
	{
		.name = "usage_user",
		.read_u64 = cpuusage_user_read,
	},
	{
		.name = "usage_sys",
		.read_u64 = cpuusage_sys_read,
	},
	{
		.name = "usage_percpu",
		.seq_show = cpuacct_percpu_seq_show,
	},
	{
		.name = "usage_percpu_user",
		.seq_show = cpuacct_percpu_user_seq_show,
	},
	{
		.name = "usage_percpu_sys",
		.seq_show = cpuacct_percpu_sys_seq_show,
	},
	{
		.name = "usage_all",
		.seq_show = cpuacct_all_seq_show,
	},
	{
		.name = "stat",
		.seq_show = cpuacct_stats_show,
	},
#ifdef CONFIG_SCHED_SLI
	{
		.name = "proc_stat",
		.seq_show = cpuacct_proc_stats_show,
	},
	{
		.name = "enable_sli",
		.read_u64 = enable_sli_read,
		.write_u64 = enable_sli_write
	},
	{
		.name = "sched_cfs_statistics",
		.seq_show = cpuacct_sched_cfs_show,
	},
	{
		.name = "wait_latency",
		.private = SCHED_LAT_WAIT,
		.write_u64 = sched_lat_stat_write,
		.seq_show = sched_lat_stat_show
	},
	{
		.name = "cgroup_wait_latency",
		.private = SCHED_LAT_CGROUP_WAIT,
		.write_u64 = sched_lat_stat_write,
		.seq_show = sched_lat_stat_show
	},
	{
		.name = "block_latency",
		.private = SCHED_LAT_BLOCK,
		.write_u64 = sched_lat_stat_write,
		.seq_show = sched_lat_stat_show
	},
	{
		.name = "ioblock_latency",
		.private = SCHED_LAT_IOBLOCK,
		.write_u64 = sched_lat_stat_write,
		.seq_show = sched_lat_stat_show
	},
#endif
	{ }	/* terminate */
};

/*
 * charge this task's execution time to its accounting group.
 *
 * called with rq->lock held.
 */
void cpuacct_charge(struct task_struct *tsk, u64 cputime)
{
	unsigned int cpu = task_cpu(tsk);
	struct cpuacct *ca;

	lockdep_assert_rq_held(cpu_rq(cpu));

	for (ca = task_ca(tsk); ca; ca = parent_ca(ca))
		*per_cpu_ptr(ca->cpuusage, cpu) += cputime;
}

/*
 * Add user/system time to cpuacct.
 *
 * Note: it's the caller that updates the account of the root cgroup.
 */
void cpuacct_account_field(struct task_struct *tsk, int index, u64 val)
{
	struct cpuacct *ca;

	for (ca = task_ca(tsk); ca != &root_cpuacct; ca = parent_ca(ca))
		__this_cpu_add(ca->cpustat->cpustat[index], val);
}

static void cpuacct_cgroup_attach(struct cgroup_taskset *tset)
{
	struct task_struct *task;
	struct cgroup_subsys_state *css;

	cgroup_taskset_for_each(task, css, tset)
		if (task->pid && is_child_reaper(task_pid(task)))
			create_rich_container_reaper(task);
}

struct cgroup_subsys cpuacct_cgrp_subsys = {
	.css_alloc	= cpuacct_css_alloc,
	.css_free	= cpuacct_css_free,
	.attach		= cpuacct_cgroup_attach,
	.legacy_cftypes	= files,
	.early_init	= true,
};

#ifdef CONFIG_SCHED_SLI
static DEFINE_STATIC_KEY_FALSE(async_load_calc);

bool async_load_calc_enabled(void)
{
	return static_branch_likely(&async_load_calc);
}

static int async_load_calc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", async_load_calc_enabled());
	return 0;
}

static int async_load_calc_open(struct inode *inode, struct file *file)
{
	return single_open(file, async_load_calc_show, NULL);
}

static void async_calc_cgroup_load(unsigned long update_version)
{
	int cnt;
	struct task_group *tg;

again:
	cnt = 1;
	rcu_read_lock();
	list_for_each_entry_rcu(tg, &sli_tg_list, sli_list) {
		/*
		 * Need per ca check since after break the list
		 * could have been changed, otherwise the loop
		 * will be endless.
		 */
		if (update_version == tg->load_update_version)
			continue;

		task_group_calc_load(tg);
		tg->load_update_version = update_version;

		/* Take a break for every 100 ca */
		if (cnt++ >= 100) {
			rcu_read_unlock();
			cond_resched();
			goto again;
		}
	}
	rcu_read_unlock();
}

int load_calc_func(void *unsed)
{
	unsigned long next_update = jiffies + LOAD_FREQ;
	unsigned long update_version = 0;

	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ/5);
		set_current_state(TASK_RUNNING);

		if (time_before(jiffies, next_update + 10))
			continue;

		async_calc_cgroup_load(update_version);
		next_update += LOAD_FREQ;
		update_version++;
	}

	return 0;
}

static struct task_struct *load_calc_p;

static int mod_async_load_calc(bool enable)
{
	if (enable == async_load_calc_enabled())
		return 0;

	if (enable) {
		load_calc_p = kthread_create(load_calc_func, NULL, "load_calc");
		if (!load_calc_p)
			return -ENOMEM;

		wake_up_process(load_calc_p);
		static_branch_enable(&async_load_calc);
	} else {
		kthread_stop(load_calc_p);
		load_calc_p = NULL;

		static_branch_disable(&async_load_calc);
	}

	return 0;
}

static DEFINE_MUTEX(load_calc_mutex);

static ssize_t async_load_calc_write(struct file *file,
		const char __user *ubuf,
		size_t count, loff_t *ppos)
{
	char val = -1;
	int ret = 0;

	if (count < 1 || *ppos) {
		ret = -EINVAL;
		goto out;
	}

	if (copy_from_user(&val, ubuf, 1)) {
		ret = -EFAULT;
		goto out;
	}

	mutex_lock(&load_calc_mutex);

	switch (val) {
	case '0':
		ret = mod_async_load_calc(false);
		break;
	case '1':
		ret = mod_async_load_calc(true);
		break;
	default:
		ret = -EINVAL;
	}

	mutex_unlock(&load_calc_mutex);
out:
	return ret ? ret : count;
}

static const struct proc_ops async_load_calc_opt = {
	.proc_open	= async_load_calc_open,
	.proc_read	= seq_read,
	.proc_write	= async_load_calc_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int __init async_load_calc_init(void)
{
	if (!proc_create("async_load_calc", 0600, NULL,
				&async_load_calc_opt)) {
		pr_err("Failed to register async_load_calc interface\n");
		return 0;
	}

	if (mod_async_load_calc(true))
		pr_err("Failed to enable async_load_calc\n");

	return 0;
}
late_initcall_sync(async_load_calc_init);
#endif

#ifdef CONFIG_RICH_CONTAINER

/* 0 - cpu quota; 1 - cpuset.cpus; 2 - cpu.shares */
int sysctl_rich_container_cpuinfo_source;
/* when cpu.shares */
unsigned int sysctl_rich_container_cpuinfo_sharesbase = 1024;

void rich_container_get_cpus(struct task_struct *tsk, struct cpumask *pmask)
{
	struct task_group *tg;
	int i, cpus;

	/* cfs quota source */
	if (sysctl_rich_container_cpuinfo_source == 0) {
		long quota, period;

		rcu_read_lock();
		tg = task_tg(tsk);
		if (sysctl_rich_container_source == 2 && tg->parent)
			tg = tg->parent;
		quota = tg_get_cfs_quota(tg);
		period = tg_get_cfs_period(tg);
		rcu_read_unlock();

		if (quota == -1) {
			/* Fallback to use cpuset.cpus if quota not set */
			goto cpuset_source;
		} else {
			/* period can't be 0 */
			cpus = (quota + period - 1) / period;
			cpus = clamp(cpus, 1, (int)num_online_cpus());
			cpumask_clear(pmask);
			for (i = 0; i < cpus; i++)
				cpumask_set_cpu(i, pmask);
		}

		return;
	}

	/* cpu.shares source */
	if (sysctl_rich_container_cpuinfo_source == 2) {
		unsigned long shares;

		rcu_read_lock();
		tg = task_tg(tsk);
		if (sysctl_rich_container_source == 2 && tg->parent)
			tg = tg->parent;
		shares = scale_load_down(tg->shares);
		rcu_read_unlock();

		/* sysctl_rich_container_cpuinfo_sharesbase can't be 0 */
		cpus = (shares + sysctl_rich_container_cpuinfo_sharesbase - 1) /
			sysctl_rich_container_cpuinfo_sharesbase;
		cpus = clamp(cpus, 1, (int)num_online_cpus());
		cpumask_clear(pmask);
		for (i = 0; i < cpus; i++)
			cpumask_set_cpu(i, pmask);

		return;
	}

cpuset_source:
	/* cpuset.cpus source */
	rich_container_get_cpuset_cpus(pmask);
}

bool child_task_group(struct task_struct *tsk)
{
	struct task_group *tg = task_tg(tsk);

	if (tg && tg != &root_task_group)
		return true;

	return false;
}


bool check_rich_container(unsigned int cpu, unsigned int *index,
		bool *rich_container, unsigned int *total)
{
	struct cpumask cpuset_allowed;
	struct task_struct __maybe_unused *scenario;
	bool in_rich;
	int i, id = 0;

	rcu_read_lock();
	in_rich = in_rich_container(current, RC_CPUINFO);
	rcu_read_unlock();
	if (!in_rich)
		return false;

	*rich_container = true;

	read_lock(&tasklist_lock);
	scenario = rich_container_get_scenario();
	get_task_struct(scenario);
	read_unlock(&tasklist_lock);
	rich_container_get_cpus(scenario, &cpuset_allowed);
	put_task_struct(scenario);

	*total = cpumask_weight(&cpuset_allowed);
	if (cpumask_test_cpu(cpu, &cpuset_allowed)) {
		for_each_cpu(i, &cpuset_allowed) {
			if (i == cpu)
				break;
			id++;
		}
		*index = id;
		return false;
	}

	/* Hide this cpu in the container */
	return true;
}

void rich_container_source(enum rich_container_source *from)
{
	if (sysctl_rich_container_source == 1)
		*from = RICH_CONTAINER_REAPER;
	else if (sysctl_rich_container_source == 0)
		*from = RICH_CONTAINER_CURRENT;
	else
		*from = RICH_CONTAINER_PARENT_CGROUP;
}

void rich_container_get_usage(enum rich_container_source from,
		struct task_struct *reaper, int cpu,
		struct cpuacct_usage_result *res)
{
	struct cgroup_subsys_state *css;

	rcu_read_lock();
	/* To avoid iterating css for every cpu */
	if (likely(from == RICH_CONTAINER_REAPER)) {
		css = task_css(reaper, cpu_cgrp_id);
	} else if (from == RICH_CONTAINER_CURRENT) {
		css = task_css(current, cpu_cgrp_id);
	} else if (from == RICH_CONTAINER_PARENT_CGROUP) {
		css = task_css(current, cpu_cgrp_id)->parent;
		if (!css)
			css = task_css(current, cpu_cgrp_id);
	}

	__cgroup_get_usage_result(css, cpu, res);
	rcu_read_unlock();
}

unsigned long rich_container_get_running(enum rich_container_source from,
		struct task_struct *reaper, int cpu)
{
	struct task_group *tg;
	unsigned long nr;
	rcu_read_lock();
	/* To avoid iterating css for every cpu */
	if (likely(from == RICH_CONTAINER_REAPER)) {
		tg = task_tg(reaper);
	} else if (from == RICH_CONTAINER_CURRENT) {
		tg = task_tg(current);
	} else if (from == RICH_CONTAINER_PARENT_CGROUP) {
		tg = task_tg(current);
		if (tg->parent)
			tg = tg->parent;
	}

	nr = tg_running(tg, cpu);
	rcu_read_unlock();

	return nr;
}

void rich_container_get_avenrun(enum rich_container_source from,
		struct task_struct *reaper, unsigned long *loads,
		unsigned long offset, int shift, bool running)
{
	struct task_group *tg;

	rcu_read_lock();
	/* To avoid iterating css for every cpu */
	if (likely(from == RICH_CONTAINER_REAPER)) {
		tg = task_tg(reaper);
	} else if (from == RICH_CONTAINER_CURRENT) {
		tg = task_tg(current);
	} else if (from == RICH_CONTAINER_PARENT_CGROUP) {
		tg = task_tg(current);
		if (tg->parent)
			tg = tg->parent;
	}

	__get_cgroup_avenrun(tg, loads, offset, shift, running);
	rcu_read_unlock();
}

#endif

// SPDX-License-Identifier: GPL-2.0
/*
 * Group Balancer
 *
 * Group Balancer sched domains define and build
 * Copyright (C) 2024 Alibaba Group, Inc., Cruz Zhao <CruzZhao@linux.alibaba.com>
 */
#include "sched.h"
#include "autogroup.h"
#include <linux/log2.h>
#include <linux/fs_context.h>
#include <linux/sched/isolation.h>

struct gb_lb_env {
	int					src_cpu;
	struct group_balancer_sched_domain	*src;
	struct group_balancer_sched_domain	*dst;
	struct group_balancer_sched_domain	*gb_sd;
	long					imbalance;
	unsigned long				nr_balance_failed;
	enum migration_type			migration_type;
	struct rb_root				task_groups;

	CK_KABI_RESERVE(1)
	CK_KABI_RESERVE(2)
	CK_KABI_RESERVE(3)
	CK_KABI_RESERVE(4)
};

DECLARE_PER_CPU(cpumask_var_t, group_balancer_mask);

struct group_balancer_sched_domain {
	struct group_balancer_sched_domain		*parent;
	struct list_head				child;
	struct list_head				sibling;
	struct list_head				topology_level_sibling;
	struct list_head				size_level_sibling;
	unsigned long					gb_flags;
	char						*topology_name;
	unsigned int					span_weight;
	unsigned int					nr_children;
	/* If free_tg_specs is less than zero, the gb_sd is overloaded. */
	int						free_tg_specs;
	unsigned int					depth;
	raw_spinlock_t					lock;
	struct rb_root					task_groups;
	struct kernfs_node				*kn;
	unsigned long					last_balance_timestamp;
	unsigned long					lower_interval;
	unsigned int					imbalance_pct;
	CK_KABI_RESERVE(1)
	CK_KABI_RESERVE(2)
	CK_KABI_RESERVE(3)
	CK_KABI_RESERVE(4)
	CK_KABI_RESERVE(5)
	CK_KABI_RESERVE(6)
	CK_KABI_RESERVE(7)
	CK_KABI_RESERVE(8)
	CK_KABI_RESERVE(9)
	CK_KABI_RESERVE(10)
	CK_KABI_RESERVE(11)
	CK_KABI_RESERVE(12)
	CK_KABI_RESERVE(13)
	CK_KABI_RESERVE(14)
	CK_KABI_RESERVE(15)
	CK_KABI_RESERVE(16)
	unsigned long					span[];
};

/* The topology that group balancer cares about. */
enum GROUP_BALANCER_TOPOLOGY {
	GROUP_BALANCER_ROOT,
	GROUP_BALANCER_SOCKET,
#ifdef CONFIG_NUMA
	GROUP_BALANCER_NUMA,
#endif
	GROUP_BALANCER_DIE,
	GROUP_BALANCER_LLC,
#ifdef CONFIG_SCHED_MC
	GROUP_BALANCER_MC,
#endif
#ifdef CONFIG_SCHED_CLUSTER
	GROUP_BALANCER_CLUSTER,
#endif
#ifdef CONFIG_SCHED_SMT
	GROUP_BALANCER_SMT,
#endif
	NR_GROUP_BALANCER_TOPOLOGY,
};

enum GROUP_BALANCER_TOPOLOGY_FLAGS {
	GROUP_BALANCER_ROOT_FLAG	= BIT(GROUP_BALANCER_ROOT),
	GROUP_BALANCER_SOCKET_FLAG	= BIT(GROUP_BALANCER_SOCKET),
#ifdef CONFIG_NUMA
	GROUP_BALANCER_NUMA_FLAG	= BIT(GROUP_BALANCER_NUMA),
#endif
	GROUP_BALANCER_DIE_FLAG		= BIT(GROUP_BALANCER_DIE),
	GROUP_BALANCER_LLC_FLAG		= BIT(GROUP_BALANCER_LLC),
#ifdef CONFIG_SCHED_MC
	GROUP_BALANCER_MC_FLAG		= BIT(GROUP_BALANCER_MC),
#endif
#ifdef CONFIG_SCHED_CLUSTER
	GROUP_BALANCER_CLUSTER_FLAG	= BIT(GROUP_BALANCER_CLUSTER),
#endif
#ifdef CONFIG_SCHED_SMT
	GROUP_BALANCER_SMT_FLAG		= BIT(GROUP_BALANCER_SMT),
#endif
};

/* Mapping to the schedule domain. */
unsigned long sched_domain_flags =
#ifdef CONFIG_SCHED_SMT
	BIT(GROUP_BALANCER_SMT) |
#endif
#ifdef CONFIG_SCHED_MC
	BIT(GROUP_BALANCER_MC) |
#endif
	BIT(GROUP_BALANCER_DIE) |
#ifdef CONFIG_NUMA
	BIT(GROUP_BALANCER_NUMA) |
#endif
	BIT(GROUP_BALANCER_ROOT);

struct group_balancer_topology_level {
	sched_domain_mask_f	mask;
	sched_domain_flags_f	sd_flags;
	unsigned long		gb_flags;
	char			*topology_name;
	struct list_head	domains;
	bool			skip;
	CK_KABI_RESERVE(1)
	CK_KABI_RESERVE(2)
	CK_KABI_RESERVE(3)
	CK_KABI_RESERVE(4)
};

struct group_balancer_size_level {
	int			size;
	/* Use list temporarily, we will change to use rb_tree later.*/
	struct list_head	domains;
	CK_KABI_RESERVE(1)
	CK_KABI_RESERVE(2)
	CK_KABI_RESERVE(3)
	CK_KABI_RESERVE(4)
};

LIST_HEAD(group_balancer_sched_domains);

DEFINE_RWLOCK(group_balancer_sched_domain_lock);

struct cpumask root_cpumask;

static struct kernfs_root *group_balancer_fs_root;
static struct kernfs_node *group_balancer_fs_root_kn;
struct group_balancer_fs_context {
	struct kernfs_fs_context	kfc;
	void				*tmp;
	CK_KABI_RESERVE(1)
	CK_KABI_RESERVE(2)
	CK_KABI_RESERVE(3)
	CK_KABI_RESERVE(4)
};

struct gftype {
	char			*name;
	umode_t			mode;
	const struct kernfs_ops	*kf_ops;
	int (*seq_show)(struct kernfs_open_file *of,
			struct seq_file *sf, void *v);
	ssize_t (*write)(struct kernfs_open_file *of,
			 char *buf, size_t nbytes, loff_t off);
	CK_KABI_RESERVE(1)
	CK_KABI_RESERVE(2)
	CK_KABI_RESERVE(3)
	CK_KABI_RESERVE(4)
};

const struct cpumask *cpu_llc_mask(int cpu)
{
	struct sched_domain *llc = rcu_dereference(per_cpu(sd_llc, cpu));

	if (!llc)
		return cpumask_of(cpu);

	return (const struct cpumask *)to_cpumask(llc->span);
}

const struct cpumask *cpu_die_mask(int cpu)
{
	return topology_die_cpumask(cpu);
}

const struct cpumask *cpu_core_mask(int cpu)
{
	return topology_core_cpumask(cpu);
}

const struct cpumask *cpu_root_mask(int cpu)
{
	return (const struct cpumask *)&root_cpumask;
}

#define GB_SD_INIT(type) \
	.gb_flags = GROUP_BALANCER_##type##_FLAG, \
	.topology_name = #type
/*
 * Group Balancer build group_balancer_sched_domains after kernel init,
 * so the following cpumask can be got safely.
 *
 * smt mask:		cpu_smt_mask
 * cluster mask:	cpu_clustergroup_mask
 * mc mask:		cpu_coregroup_mask
 * llc mask:		cpu_llc_mask
 * die mask:		cpu_die_mask
 * numa mask:		cpu_cpu_mask
 * socket mask:		cpu_core_mask
 * all mask:		cpu_root_mask
 */
static struct group_balancer_topology_level default_topology[] = {
	{ cpu_root_mask, GB_SD_INIT(ROOT) },
	{ cpu_core_mask, GB_SD_INIT(SOCKET) },
#ifdef CONFIG_NUMA
	{ cpu_cpu_mask, GB_SD_INIT(NUMA) },
#endif
	{ cpu_die_mask, GB_SD_INIT(DIE) },
	{ cpu_llc_mask, GB_SD_INIT(LLC) },
#ifdef CONFIG_SCHED_MC
	{ cpu_coregroup_mask, cpu_core_flags, GB_SD_INIT(MC) },
#endif
#ifdef CONFIG_SCHED_CLUSTER
	{ cpu_clustergroup_mask, cpu_cluster_flags, GB_SD_INIT(CLUSTER) },
#endif
#ifdef CONFIG_SCHED_SMT
	{ cpu_smt_mask, cpu_smt_flags, GB_SD_INIT(SMT) },
#endif
	{ NULL, },
};

#define for_each_gb_topology_level(tl)			\
	for (tl = default_topology; tl->mask; tl++)

#define for_each_topology_level_sibling(pos, gb_tl)	\
	list_for_each_entry(pos, &gb_tl->domains, topology_level_sibling)

#define for_each_topology_level_sibling_safe(pos, n, gb_tl)	\
	list_for_each_entry_safe(pos, n, &gb_tl->domains, topology_level_sibling)

/* NR_CPUS is 1024 now, we set log(1024) + 1 = 11 levels. */
#define NR_SIZE_LEVELS 11
struct group_balancer_size_level default_size[NR_SIZE_LEVELS];

#define for_each_gb_size_level(sl, i)			\
	for (sl = default_size, i = 0; i < NR_SIZE_LEVELS; sl++, i++)

#define for_each_gb_sd_child(pos, gb_sd)			\
	list_for_each_entry(pos, &gb_sd->child, sibling)

#define for_each_gb_sd_child_safe(pos, n, gb_sd)			\
	list_for_each_entry_safe(pos, n, &gb_sd->child, sibling)

#define group_balancer_sched_domain_first_child(gb_sd)		\
	list_first_entry(&gb_sd->child, struct group_balancer_sched_domain, sibling)

#define __gb_node_2_tg(node)	\
	rb_entry((node), struct task_group, gb_node)

struct group_balancer_sched_domain *group_balancer_root_domain;

#define MAX_NAME_LEN		128
#define GB_OVERLOAD		0x1
#define GB_OVERUTILIZED		0x2

static inline struct cpumask *gb_sd_span(struct group_balancer_sched_domain *gb_sd)
{
	return to_cpumask(gb_sd->span);
}

static unsigned int get_size_level(struct group_balancer_sched_domain *gb_sd)
{
	int size_level = ilog2(gb_sd->span_weight);

	/* Prevent out-of-bound array access. */
	if (unlikely(size_level < 0))
		size_level = 0;
	else if (unlikely(size_level >= NR_SIZE_LEVELS))
		size_level = NR_SIZE_LEVELS - 1;

	return (unsigned int)size_level;
}

static void __add_to_size_level(struct group_balancer_sched_domain *gb_sd,
				unsigned int size_level)
{
	struct group_balancer_size_level *gb_sl;

	gb_sl = &default_size[size_level];
	list_add_tail(&gb_sd->size_level_sibling, &gb_sl->domains);
}

static void add_to_size_level(struct group_balancer_sched_domain *gb_sd)
{
	unsigned int size_level = get_size_level(gb_sd);

	__add_to_size_level(gb_sd, size_level);
}

static int group_balancer_seqfile_show(struct seq_file *m, void *arg)
{
	struct kernfs_open_file *of = m->private;
	struct gftype *gft = of->kn->priv;

	if (gft->seq_show)
		return gft->seq_show(of, m, arg);
	return 0;
}

static ssize_t group_balancer_file_write(struct kernfs_open_file *of, char *buf,
					 size_t nbytes, loff_t off)
{
	struct gftype *gft = of->kn->priv;

	if (gft->write)
		return gft->write(of, buf, nbytes, off);

	return -EINVAL;
}

static const struct kernfs_ops group_balancer_kf_single_ops = {
	.atomic_write_len	= PAGE_SIZE,
	.write			= group_balancer_file_write,
	.seq_show		= group_balancer_seqfile_show,
};

static struct group_balancer_sched_domain *kn_parent_priv(struct kernfs_node *kn)
{
	/*
	 * The parent pointer is only valid within RCU section since it can be
	 * replaced
	 */
	guard(rcu)();
	return rcu_dereference(kn->__parent)->priv;
}

struct group_balancer_sched_domain *kernfs_to_gb_sd(struct kernfs_node *kn)
{
	if (kernfs_type(kn) == KERNFS_DIR)
		return kn->priv;
	else
		return kn_parent_priv(kn);
}

struct group_balancer_sched_domain *group_balancer_kn_lock_live(struct kernfs_node *kn)
{
	struct group_balancer_sched_domain *gb_sd = kernfs_to_gb_sd(kn);

	if (!gb_sd)
		return NULL;

	kernfs_break_active_protection(kn);
	cpus_read_lock();
	write_lock(&group_balancer_sched_domain_lock);

	return gb_sd;
}

void group_balancer_kn_unlock(struct kernfs_node *kn)
{
	struct group_balancer_sched_domain *gb_sd = kernfs_to_gb_sd(kn);

	if (!gb_sd)
		return;

	write_unlock(&group_balancer_sched_domain_lock);
	cpus_read_unlock();
	kernfs_unbreak_active_protection(kn);
}

static ssize_t group_balancer_cpus_write(struct kernfs_open_file *of,
					 char *buf, size_t nbytes, loff_t off)
{
	cpumask_var_t new, tmp;
	int cpu;
	struct rq *rq;
	struct group_balancer_sched_domain *gb_sd, *parent, *sibling, *child;
	int old_size_level, new_size_level;
	int ret = 0;

	if (!buf)
		return -EINVAL;
	if (!zalloc_cpumask_var(&new, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&tmp, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto free_new;
	}

	gb_sd = group_balancer_kn_lock_live(of->kn);
	if (!gb_sd) {
		ret = -ENOENT;
		goto unlock;
	}

	ret = cpulist_parse(buf, new);
	if (ret) {
		ret = -EINVAL;
		goto unlock;
	}

	if (cpumask_equal(new, gb_sd_span(gb_sd)))
		goto unlock;

	parent = gb_sd->parent;
	if (parent) {
		/* New mask must be subset of parent.*/
		if (!cpumask_subset(new, gb_sd_span(parent))) {
			ret = -EINVAL;
			goto unlock;
		}

		/* New mask must not inersect with siblings. */
		for_each_gb_sd_child(sibling, parent) {
			if (gb_sd == sibling)
				continue;
			if (cpumask_intersects(new, gb_sd_span(sibling))) {
				ret = -EINVAL;
				goto unlock;
			}
		}
	}

	/* New mask must include all the cpus of the children. */
	for_each_gb_sd_child(child, gb_sd) {
		if (!cpumask_subset(gb_sd_span(child), new)) {
			ret = -EINVAL;
			goto unlock;
		}
	}

	/*
	 * rq->gb_sd points to the lowest level group_balancer_sched_domain
	 * that includes the cpu.
	 *
	 * We define two types of cpumask here: 'less' and 'more'.
	 * - 'less' is the cpus that new cpumask lacks.
	 * - 'more' is the cpus that new cpumask newly adds.
	 *
	 * As the cpus of a child must be subset of its parent, the cpus in
	 * 'less' and 'more' are not included by any child of gb_sd, and the
	 * lowest level group_balancer_sched_domain that includes 'less' is
	 * the parent of gb_sd, the lowest level group_balancer_sched_domain
	 * that includes 'more' is gb_sd.
	 *
	 * So we need to set the rq->gb_sd of the cpus in 'less' to parent.
	 * and set the rq->gb_sd of the cpus in 'more' to gb_sd.
	 */
	cpumask_andnot(tmp, gb_sd_span(gb_sd), new);
	for_each_cpu(cpu, tmp) {
		rq = cpu_rq(cpu);
		rq->gb_sd = parent;
	}

	cpumask_andnot(tmp, new, gb_sd_span(gb_sd));
	for_each_cpu(cpu, tmp) {
		rq = cpu_rq(cpu);
		rq->gb_sd = gb_sd;
	}

	old_size_level = get_size_level(gb_sd);
	cpumask_copy(gb_sd_span(gb_sd), new);
	gb_sd->span_weight = cpumask_weight(gb_sd_span(gb_sd));
	new_size_level = get_size_level(gb_sd);
	if (old_size_level != new_size_level) {
		list_del(&gb_sd->size_level_sibling);
		__add_to_size_level(gb_sd, new_size_level);
	}
	if (gb_sd == group_balancer_root_domain)
		cpumask_copy(&root_cpumask, new);

unlock:
	group_balancer_kn_unlock(of->kn);
	free_cpumask_var(tmp);
free_new:
	free_cpumask_var(new);

	return ret ?: nbytes;
}

static int group_balancer_cpus_show(struct kernfs_open_file *of,
				    struct seq_file *s, void *v)
{
	struct group_balancer_sched_domain *gb_sd;
	int ret = 0;

	gb_sd = group_balancer_kn_lock_live(of->kn);

	if (!gb_sd) {
		ret = -ENOENT;
		goto unlock;
	}

	seq_printf(s, "%*pbl\n", cpumask_pr_args(gb_sd_span(gb_sd)));
unlock:
	group_balancer_kn_unlock(of->kn);
	return ret;
}

static struct gftype group_balancer_files[] = {
	{
		.name		= "cpus",
		.mode		= 0644,
		.kf_ops		= &group_balancer_kf_single_ops,
		.write		= group_balancer_cpus_write,
		.seq_show	= group_balancer_cpus_show,
	},
};

static int group_balancer_kn_set_ugid(struct kernfs_node *kn)
{
	struct iattr iattr = { .ia_valid = ATTR_UID | ATTR_GID,
				.ia_uid = current_fsuid(),
				.ia_gid = current_fsgid(), };

	if (uid_eq(iattr.ia_uid, GLOBAL_ROOT_UID) &&
	    gid_eq(iattr.ia_gid, GLOBAL_ROOT_GID))
		return 0;

	return kernfs_setattr(kn, &iattr);
}

static int group_balancer_add_file(struct kernfs_node *parent_kn, struct gftype *gft)
{
	struct kernfs_node *kn;
	int ret;

	kn = __kernfs_create_file(parent_kn, gft->name, gft->mode,
				  GLOBAL_ROOT_UID, GLOBAL_ROOT_GID, 0,
				  gft->kf_ops, gft, NULL, NULL);

	if (IS_ERR(kn))
		return PTR_ERR(kn);

	ret = group_balancer_kn_set_ugid(kn);
	if (ret) {
		kernfs_remove(kn);
		return ret;
	}

	return ret;
}

static int group_balancer_add_files(struct kernfs_node *kn)
{
	struct gftype *gfts, *gft;
	int ret, len;

	gfts = group_balancer_files;
	len = ARRAY_SIZE(group_balancer_files);

	for (gft = gfts; gft < gfts + len; gft++) {
		ret = group_balancer_add_file(kn, gft);
		if (ret)
			goto err;
	}

	return 0;
err:
	pr_err("Group Balancer: Failed to add sysfs file %s, err=%d\n", gft->name, ret);
	while (--gft >= gfts)
		kernfs_remove_by_name(kn, gft->name);

	return ret;
}

static inline struct group_balancer_sched_domain
*alloc_init_group_balancer_sched_domain(struct kernfs_node *parent, const char *name, umode_t mode)
{
	struct group_balancer_sched_domain *new, *ret;
	struct kernfs_node *kn;
	int retval;

	if (!parent) {
		ret = ERR_PTR(-ENOENT);
		goto err_out;
	}

	new = kzalloc(sizeof(struct group_balancer_sched_domain) + cpumask_size(), GFP_KERNEL);
	if (!new) {
		ret = ERR_PTR(-ENOMEM);
		goto err_out;
	}

	kn = kernfs_create_dir(parent, name, mode, new);
	if (IS_ERR(kn)) {
		ret = (struct group_balancer_sched_domain *)kn;
		goto free_new;
	}
	new->kn = kn;

	retval = group_balancer_add_files(kn);
	if (retval) {
		ret = ERR_PTR(retval);
		goto remove_kn;
	}

	INIT_LIST_HEAD(&new->child);
	INIT_LIST_HEAD(&new->sibling);
	INIT_LIST_HEAD(&new->topology_level_sibling);
	INIT_LIST_HEAD(&new->size_level_sibling);

	raw_spin_lock_init(&new->lock);
	new->task_groups = RB_ROOT;
	new->imbalance_pct = 117;

	return new;
remove_kn:
	kernfs_remove(kn);
free_new:
	kfree(new);
err_out:
	pr_err("Group Balancer: Failed to allocate and init a new group balancer sched domain.\n");
	return ret;
}

static void add_to_tree(struct group_balancer_sched_domain *gb_sd,
			struct group_balancer_sched_domain *parent)
{
	int cpu;
	struct rq *rq;

	if (parent) {
		list_add_tail(&gb_sd->sibling, &parent->child);
		gb_sd->parent = parent;
		parent->nr_children++;
		/*
		 * When we bi-divide the group balancer sched domain, the parent, middle layer,
		 * hasn't been added to the tree yet, so for this case, we just let the depth
		 * increase by 1.
		 */
		if (parent->depth)
			gb_sd->depth = parent->depth + 1;
		else
			gb_sd->depth++;
	} else {
		gb_sd->depth = 0;
	}
	gb_sd->span_weight = cpumask_weight(gb_sd_span(gb_sd));
	gb_sd->lower_interval = ilog2(gb_sd->span_weight) * gb_sd->span_weight;
	gb_sd->free_tg_specs = 100 * gb_sd->span_weight;
	add_to_size_level(gb_sd);

	if (!gb_sd->nr_children) {
		for_each_cpu(cpu, gb_sd_span(gb_sd)) {
			rq = cpu_rq(cpu);
			rq->gb_sd = gb_sd;
		}
	}
}

#define __node_2_task_group(n) rb_entry((n), struct task_group, gb_node)

static inline bool tg_specs_less(struct rb_node *a, const struct rb_node *b)
{
	struct task_group *tg_a = __node_2_task_group(a);
	struct task_group *tg_b = __node_2_task_group(b);
	int specs_a = tg_a->specs_ratio;
	int specs_b = tg_b->specs_ratio;

	return specs_a < specs_b;
}

static int tg_set_gb_tg_down(struct task_group *tg, void *data)
{
	struct task_group *gb_tg = (struct task_group *)data;

	tg->soft_cpus_allowed_ptr = gb_tg->soft_cpus_allowed_ptr;
	tg->gb_tg = gb_tg;
	tg_inc_soft_cpus_version(tg);

	return 0;
}

static int tg_unset_gb_tg_down(struct task_group *tg, void *data)
{
	tg->soft_cpus_allowed_ptr = &tg->soft_cpus_allowed;
	tg->gb_tg = NULL;
	tg_inc_soft_cpus_version(tg);

	return 0;
}

static void free_group_balancer_sched_domain(struct group_balancer_sched_domain *gb_sd)
{
	int cpu;
	struct rq *rq;
	struct task_group *tg;
	struct group_balancer_sched_domain *parent = gb_sd->parent;
	struct rb_node *node;
	struct rb_root *root = &gb_sd->task_groups;

	if (parent) {
		parent->nr_children--;
		/* Move the task_groups to parent. */
		while (!RB_EMPTY_ROOT(root)) {
			node = root->rb_node;
			tg = __node_2_task_group(node);
			rb_erase(node, root);
			rb_add(node, &parent->task_groups, tg_specs_less);
			walk_tg_tree_from(tg, tg_set_gb_tg_down, tg_nop, tg);
		}
	}

	list_del(&gb_sd->sibling);
	list_del(&gb_sd->topology_level_sibling);
	list_del(&gb_sd->size_level_sibling);

	if (!gb_sd->nr_children) {
		for_each_cpu(cpu, gb_sd_span(gb_sd)) {
			rq = cpu_rq(cpu);
			rq->gb_sd = gb_sd->parent;
		}
	}

	if (gb_sd->kn)
		kernfs_remove(gb_sd->kn);

	kfree(gb_sd);
}

/* free group balancer sched domain tree from the leaf nodes. */
static void free_group_balancer_sched_domains(void)
{
	struct group_balancer_sched_domain *parent, *child, *ancestor, *n;

	parent = group_balancer_root_domain;
down:
	for_each_gb_sd_child_safe(child, n, parent) {
		parent = child;
		goto down;
up:
		continue;
	}

	ancestor = parent->parent;
	/* root domain should always be in memory. */
	if (parent != group_balancer_root_domain && !parent->nr_children) {
		n = list_next_entry(parent, sibling);
		free_group_balancer_sched_domain(parent);
	}

	child = n;
	parent = ancestor;
	if (parent)
		goto up;
}

static int move_group_balancer_kernfs(struct group_balancer_sched_domain *gb_sd,
				      struct group_balancer_sched_domain *new_parent)
{
	char *new_name = NULL;
	int id = new_parent->nr_children;
	int ret = 0;

	if (!gb_sd->kn || !new_parent->kn)
		return -ENOMEM;

	new_name = kmalloc(MAX_NAME_LEN, GFP_KERNEL);
	if (!new_name)
		return -ENOMEM;
	/*
	 * We use domain+id as new name, and if the name is already occupied, we let id++,
	 * until we find an unoccupied name.
	 */
	for (;;) {
		struct kernfs_node *dup;

		sprintf(new_name, "domain%d", id);
		dup = kernfs_find_and_get(new_parent->kn, new_name);
		if (!dup)
			break;
		kernfs_put(dup);
		id++;
	}

	ret = kernfs_rename(gb_sd->kn, new_parent->kn, new_name);
	kfree(new_name);

	return ret;
}

static int move_group_balancer_sched_domain(struct group_balancer_sched_domain *child,
					    struct group_balancer_sched_domain *new_parent,
					    bool *is_first_child)
{
	int ret = 0;

	ret = move_group_balancer_kernfs(child, new_parent);
	if (ret)
		return ret;

	if (*is_first_child) {
		*is_first_child = false;
		new_parent->topology_name = child->topology_name;
	}
	cpumask_or(gb_sd_span(new_parent), gb_sd_span(child), gb_sd_span(new_parent));
	list_del(&child->sibling);
	child->parent->nr_children--;
	list_add_tail(&child->sibling, &new_parent->child);
	new_parent->nr_children++;
	child->parent = new_parent;

	return ret;
}

static int bi_divide_group_balancer_sched_domain(struct group_balancer_sched_domain *gb_sd)
{
	unsigned int weight = gb_sd->span_weight;
	unsigned int half = (weight + 1) / 2;
	unsigned int logn = ilog2(half);
	/*
	 * Find the power of 2 closest to half, and use this number
	 * to split weight into two parts, left and right, and keep
	 * left always the smaller one.
	 */
	unsigned int left = (half - (1 << logn) < (1 << (logn + 1)) - half) ?
			    1 << logn : weight - (1 << (logn + 1));
	bool is_first_child = true;
	struct group_balancer_sched_domain *child, *n;
	struct group_balancer_sched_domain *left_middle, *right_middle;
	int ret = 0;

	/*
	 * If a domain has more than two children, we add a middle level.
	 * For example, if a domain spans 48 cpus and 24 children, we add
	 * a middle level first, which contains two children who span 16
	 * and 32 cpus. And we will divide the new children in the next
	 * loop.
	 *
	 * As for the size of middle level dividing, we choose powers of
	 * two instead of half of span_weight, to make the division of
	 * lower levels simpler.
	 */
	if (gb_sd->nr_children > 2) {
		left_middle = alloc_init_group_balancer_sched_domain(gb_sd->kn,
								     "left", 0);
		if (IS_ERR(left_middle)) {
			ret = PTR_ERR(left_middle);
			goto err;
		}

		right_middle = alloc_init_group_balancer_sched_domain(gb_sd->kn,
								      "right", 0);
		if (IS_ERR(right_middle)) {
			ret = PTR_ERR(right_middle);
			goto free_left_middle;
		}

		for_each_gb_sd_child_safe(child, n, gb_sd) {
			/*
			 * Consider the following case, a domain spans 6
			 * cpus and 3 chidlren(each child spans 2 cpus),
			 * we just need to add right middle which spans 4
			 * cpus.
			 */
			ret = move_group_balancer_sched_domain(child, left_middle,
							       &is_first_child);
			if (ret)
				goto free_right_middle;

			if (cpumask_weight(gb_sd_span(left_middle)) >= left)
				break;
		}

		/*
		 * As left is always the smaller one, it is possible that
		 * left has only one child, if so, we delete the child.
		 */
		if (left_middle->nr_children == 1) {
			child = group_balancer_sched_domain_first_child(left_middle);
			free_group_balancer_sched_domain(child);
		}

		is_first_child = true;
		for_each_gb_sd_child_safe(child, n, gb_sd) {
			ret = move_group_balancer_sched_domain(child, right_middle,
							       &is_first_child);
			if (ret)
				goto free_right_middle;
		}

		list_add_tail(&left_middle->topology_level_sibling, &gb_sd->topology_level_sibling);
		add_to_tree(left_middle, gb_sd);
		left_middle->lower_interval = gb_sd->lower_interval;
		list_add_tail(&right_middle->topology_level_sibling,
				&gb_sd->topology_level_sibling);
		add_to_tree(right_middle, gb_sd);
		right_middle->lower_interval = gb_sd->lower_interval;
		/* Uniform naming format. "left" and "right" are temporary name. */
		ret = kernfs_rename(left_middle->kn, gb_sd->kn, "domain0");
		if (ret)
			goto err;
		ret = kernfs_rename(right_middle->kn, gb_sd->kn, "domain1");
		if (ret)
			goto err;
	}

	return 0;
free_right_middle:
	free_group_balancer_sched_domain(right_middle);
free_left_middle:
	free_group_balancer_sched_domain(left_middle);
err:
	free_group_balancer_sched_domains();
	return ret;
}

/* DFS to bi-divide group balancer sched domains. */
static int bi_divide_group_balancer_sched_domains(void)
{
	struct group_balancer_sched_domain *parent, *child;
	int ret = 0;

	/*
	 * Traverse all the domains from the group_balancer_sched_domains list,
	 * and add the new domains to the tail of the list, to ensure that all
	 * the domains will be traversed.
	 */
	parent = group_balancer_root_domain;
down:
	ret = bi_divide_group_balancer_sched_domain(parent);
	if (ret)
		goto out;
	for_each_gb_sd_child(child, parent) {
		parent = child;
		goto down;
up:
		continue;
	}
	if (parent == group_balancer_root_domain)
		goto out;

	child = parent;
	parent = parent->parent;
	if (parent)
		goto up;
out:
	return ret;
}

/*
 * After we build the tree, the depth may be not correct as we moved
 * the subtree during the build process, so we correct the depth by
 * recalculating.
 */
static void set_group_balancer_sched_domain_depth(void)
{
	struct group_balancer_sched_domain *parent, *child;

	parent = group_balancer_root_domain;
	parent->depth = 0;
down:
	for_each_gb_sd_child(child, parent) {
		child->depth = parent->depth + 1;
		parent = child;
		goto down;
up:
		continue;
	}
	if (parent == group_balancer_root_domain)
		goto out;

	child = parent;
	parent = parent->parent;
	if (parent)
		goto up;
out:
	return;
}

static void set_group_balancer_sched_domain_flags(void)
{
	struct group_balancer_topology_level *tl;
	struct group_balancer_sched_domain *gb_sd;
	unsigned int l;
	unsigned long gb_flags = 0;

	for (l = NR_GROUP_BALANCER_TOPOLOGY - 1; l > 0; l--) {
		tl = &default_topology[l];
		if (list_empty(&tl->domains)) {
			gb_flags |= tl->gb_flags;
			continue;
		}
		for_each_topology_level_sibling(gb_sd, tl)
			gb_sd->gb_flags = gb_flags;
		gb_flags |= tl->gb_flags;
	}
}

static int build_group_balancer_root_domain(void)
{
	struct group_balancer_sched_domain *root;

	root = alloc_init_group_balancer_sched_domain(group_balancer_fs_root_kn, "root_domain", 0);
	if (IS_ERR(root)) {
		pr_err("Group Balancer: Failed to alloc group_balancer root domain.\n");
		return PTR_ERR(root);
	}
	cpumask_copy(gb_sd_span(root), &root_cpumask);
	list_add_tail(&root->topology_level_sibling, &default_topology[0].domains);
	add_to_tree(root, NULL);
	root->lower_interval = ilog2(root->span_weight) * root->span_weight;
	group_balancer_root_domain = root;

	return 0;
}

static void validate_topology_levels(void);

/* BFS to build group balancer sched domain tree. */
static int build_group_balancer_sched_domains(void)
{
	int cpu;
	int ret;
	cpumask_var_t trial_cpumask, child_cpumask;
	struct group_balancer_topology_level *gb_tl, *next_gb_tl;
	struct group_balancer_sched_domain *parent, *n;
	char *name = NULL;

	update_group_balancer_root_cpumask();
	validate_topology_levels();
	/*
	 * The group balancer sched domain is a tree.
	 * If the root was not built on boot, build the root node first.
	 */
	if (unlikely(!group_balancer_root_domain)) {
		ret = build_group_balancer_root_domain();
		if (ret)
			goto err_out;
	} else {
		cpumask_copy(gb_sd_span(group_balancer_root_domain), &root_cpumask);
		group_balancer_root_domain->span_weight =
			cpumask_weight(gb_sd_span(group_balancer_root_domain));
		group_balancer_root_domain->lower_interval =
			ilog2(group_balancer_root_domain->span_weight) *
			group_balancer_root_domain->span_weight;
		group_balancer_root_domain->free_tg_specs =
			100 * group_balancer_root_domain->span_weight;
	}

	if (!zalloc_cpumask_var(&trial_cpumask, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto err_out;
	}
	if (!zalloc_cpumask_var(&child_cpumask, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto err_free_trial_cpumask;
	}

	name = kmalloc(MAX_NAME_LEN, GFP_KERNEL);
	if (!name) {
		ret = -ENOMEM;
		goto err_free_domains;
	}

	/* Build the tree by level. */
	for_each_gb_topology_level(gb_tl) {
		if (gb_tl->skip)
			continue;
		next_gb_tl = gb_tl + 1;
		while (next_gb_tl->skip && next_gb_tl->mask)
			next_gb_tl++;
		if (!next_gb_tl->mask)
			break;
		/* Build children from parent level. */
		rcu_read_lock();
		for_each_topology_level_sibling_safe(parent, n, gb_tl) {
			/*
			 * If the cpumasks of the adjacent topology levels are the same,
			 * we move the domain to the next level, to make the loop
			 * continue.
			 */
			cpu = cpumask_first(gb_sd_span(parent));
			cpumask_and(child_cpumask, &root_cpumask, next_gb_tl->mask(cpu));
			if (cpumask_equal(gb_sd_span(parent), child_cpumask)) {
				list_del(&parent->topology_level_sibling);
				list_add_tail(&parent->topology_level_sibling,
					 &next_gb_tl->domains);
				if (next_gb_tl->gb_flags & sched_domain_flags)
					parent->lower_interval = ilog2(parent->span_weight) *
								 parent->span_weight;
				continue;
			}
			cpumask_copy(trial_cpumask, gb_sd_span(parent));
			for_each_cpu(cpu, trial_cpumask) {
				struct group_balancer_sched_domain *child;

				cpumask_and(child_cpumask, &root_cpumask, next_gb_tl->mask(cpu));
				cpumask_andnot(trial_cpumask, trial_cpumask, child_cpumask);
				/*
				 * parent->nr_children is  a variable that only increases and never
				 * decreases at this stage. So if we use domain+nr_children as name,
				 * there will be no duplicate names.
				 */
				sprintf(name, "domain%d", parent->nr_children);
				child = alloc_init_group_balancer_sched_domain(parent->kn, name, 0);
				if (IS_ERR(child)) {
					ret = PTR_ERR(child);
					rcu_read_unlock();
					goto err_free_name;
				}
				cpumask_copy(gb_sd_span(child), child_cpumask);
				child->topology_name = next_gb_tl->topology_name;
				list_add_tail(&child->topology_level_sibling, &next_gb_tl->domains);
				add_to_tree(child, parent);
				if (next_gb_tl->gb_flags & sched_domain_flags)
					child->lower_interval = ilog2(child->span_weight) *
								child->span_weight;
				else
					child->lower_interval = parent->lower_interval;
			}
		}
		rcu_read_unlock();
	}

	kfree(name);
	free_cpumask_var(child_cpumask);
	free_cpumask_var(trial_cpumask);
	return bi_divide_group_balancer_sched_domains();

err_free_name:
	kfree(name);
err_free_domains:
	free_group_balancer_sched_domains();
	free_cpumask_var(child_cpumask);
err_free_trial_cpumask:
	free_cpumask_var(trial_cpumask);
err_out:
	return ret;
}

static inline struct group_balancer_fs_context *group_balancer_fc2context(struct fs_context *fc)
{
	struct kernfs_fs_context *kfc = fc->fs_private;

	return container_of(kfc, struct group_balancer_fs_context, kfc);
}


static int group_balancer_get_tree(struct fs_context *fc)
{

	return kernfs_get_tree(fc);
}

static void group_balancer_fs_context_free(struct fs_context *fc)
{
	struct group_balancer_fs_context *ctx = group_balancer_fc2context(fc);

	kernfs_free_fs_context(fc);
	kfree(ctx);
}

static const struct fs_context_operations group_balancer_context_ops = {
	.free			= group_balancer_fs_context_free,
	.get_tree		= group_balancer_get_tree,
};

static int group_balancer_init_fs_context(struct fs_context *fc)
{
	struct group_balancer_fs_context *ctx;

	ctx = kzalloc(sizeof(struct group_balancer_fs_context), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->kfc.root = group_balancer_fs_root;
	ctx->kfc.magic = GROUP_BALANCER_MAGIC;
	fc->fs_private = &ctx->kfc;
	fc->ops = &group_balancer_context_ops;
	put_user_ns(fc->user_ns);
	fc->user_ns = get_user_ns(&init_user_ns);
	fc->global = true;
	return 0;
}

static void group_balancer_kill_sb(struct super_block *sb)
{
	kernfs_kill_sb(sb);
}

static struct file_system_type group_balancer_fs_type = {
	.name			= "group_balancer",
	.init_fs_context	= group_balancer_init_fs_context,
	.kill_sb		= group_balancer_kill_sb,
};

static int group_balancer_mkdir(struct kernfs_node *kn, const char *name, umode_t mode)
{
	struct group_balancer_sched_domain *new;
	struct group_balancer_sched_domain *parent = kernfs_to_gb_sd(kn);

	if (kn == group_balancer_fs_root_kn)
		return -EPERM;

	group_balancer_kn_lock_live(kn);
	new = alloc_init_group_balancer_sched_domain(kn, name, mode);
	add_to_tree(new, parent);
	new->lower_interval = ilog2(new->span_weight) * new->span_weight;
	group_balancer_kn_unlock(kn);
	if (IS_ERR(new))
		return PTR_ERR(new);

	return 0;
}

static int group_balancer_rmdir(struct kernfs_node *kn)
{
	struct group_balancer_sched_domain *gb_sd;
	int ret = 0;

	gb_sd = kn->priv;

	if (gb_sd == group_balancer_root_domain) {
		ret = -EPERM;
		goto unlock;
	}
	if (gb_sd->nr_children) {
		ret = -EBUSY;
		goto unlock;
	}

	group_balancer_kn_lock_live(kn);
	free_group_balancer_sched_domain(gb_sd);

unlock:
	group_balancer_kn_unlock(kn);
	return ret;
}

static struct kernfs_syscall_ops group_balancer_kf_syscall_ops = {
	.mkdir		= group_balancer_mkdir,
	.rmdir		= group_balancer_rmdir,
};

void sched_init_group_balancer_levels(void)
{
	struct group_balancer_topology_level *tl;
	struct group_balancer_size_level *sl;
	int i;

	for_each_gb_topology_level(tl)
		INIT_LIST_HEAD(&tl->domains);

	for_each_gb_size_level(sl, i) {
		sl->size = 1<<i;
		INIT_LIST_HEAD(&sl->domains);
	}
}

/*
 * Here are some cases that some topologies are not reported correctly,
 * e.g., on some virtual machines, DIE cpumask is incorrect, which only
 * includes one cpu.
 * To avoid building incorrect group balancer sched domains due to this
 * kind of incorrect topology, we check whether the topology is correct,
 * and if not, we mark it should be skipped.
 */
static void validate_topology_levels(void)
{
	struct group_balancer_topology_level *gb_tl, *next_gb_tl;
	int i;

	for (i = 1; i < NR_GROUP_BALANCER_TOPOLOGY - 1; i++) {
		gb_tl = &default_topology[i];
		next_gb_tl = &default_topology[i + 1];
		if (!next_gb_tl->mask)
			break;
		rcu_read_lock();
		if (!cpumask_subset(next_gb_tl->mask(0), gb_tl->mask(0)) ||
		    (cpumask_weight(gb_tl->mask(0)) <= 1))
			gb_tl->skip = true;
		rcu_read_unlock();
	}
}

void sched_init_group_balancer_sched_domains(void)
{
	int ret;

	cpus_read_lock();
	write_lock(&group_balancer_sched_domain_lock);
	ret = build_group_balancer_sched_domains();
	if (ret)
		pr_err("Group Balancer: Failed to build group balancer sched domains: %d\n", ret);
	else
		pr_info("Group Balancer: Build group balancer sched domains successfully.\n");
	set_group_balancer_sched_domain_depth();
	set_group_balancer_sched_domain_flags();
	write_unlock(&group_balancer_sched_domain_lock);
	cpus_read_unlock();
}

void sched_clear_group_balancer_sched_domains(void)
{
	cpus_read_lock();
	write_lock(&group_balancer_sched_domain_lock);
	free_group_balancer_sched_domains();
	pr_info("Group Balancer: Free group balancer sched domains.\n");
	write_unlock(&group_balancer_sched_domain_lock);
	cpus_read_unlock();
}

static int __init sched_init_group_balancer_kernfs(void)
{
	int ret = 0;

	group_balancer_fs_root = kernfs_create_root(&group_balancer_kf_syscall_ops, 0, NULL);
	if (IS_ERR(group_balancer_fs_root))
		return PTR_ERR(group_balancer_fs_root);

	group_balancer_fs_root_kn = kernfs_root_to_node(group_balancer_fs_root);

	ret = sysfs_create_mount_point(fs_kobj, "group_balancer");
	if (ret)
		goto cleanup_root;

	pr_info("Group Balancer: Created group balancer mount point.\n");
	ret = register_filesystem(&group_balancer_fs_type);
	if (ret)
		goto cleanup_mountpoint;

	pr_info("Group Balancer: Registered group balancer file system.\n");

	return 0;

cleanup_mountpoint:
	sysfs_remove_mount_point(fs_kobj, "group_balancer");
cleanup_root:
	kernfs_destroy_root(group_balancer_fs_root);
	pr_err("Group Balancer: Failed to register group balancer file system.\n");
	return ret;
}

void update_group_balancer_root_cpumask(void)
{
	cpumask_and(&root_cpumask, housekeeping_cpumask(HK_TYPE_DOMAIN), cpu_online_mask);
}

static int __init group_balancer_init(void)
{
	int ret;

	update_group_balancer_root_cpumask();
	sched_init_group_balancer_levels();
	validate_topology_levels();
	ret = sched_init_group_balancer_kernfs();
	if (ret)
		return ret;
	return build_group_balancer_root_domain();
}

late_initcall(group_balancer_init);

static void __exit sched_exit_group_balancer_kernfs(void)
{
	unregister_filesystem(&group_balancer_fs_type);
	sysfs_remove_mount_point(fs_kobj, "group_balancer");
	kernfs_destroy_root(group_balancer_fs_root);
	group_balancer_fs_root_kn = NULL;
}

__exitcall(sched_exit_group_balancer_kernfs);

static unsigned long tg_gb_sd_load(struct task_group *tg, struct group_balancer_sched_domain *gb_sd)
{
	int cpu;
	unsigned long load = 0;

	for_each_cpu(cpu, gb_sd_span(gb_sd))
		load += cfs_h_load(tg->cfs_rq[cpu]);

	return load;
}

static unsigned long tg_gb_sd_util(struct task_group *tg, struct group_balancer_sched_domain *gb_sd)
{
	int cpu;
	unsigned long util = 0;

	for_each_cpu(cpu, gb_sd_span(gb_sd))
		util += READ_ONCE(tg->cfs_rq[cpu]->avg.util_est.enqueued);

	return util;
}

static unsigned long gb_sd_load(struct group_balancer_sched_domain *gb_sd)
{
	int cpu;
	unsigned long load = 0;

	for_each_cpu(cpu, gb_sd_span(gb_sd))
		load += cpu_rq(cpu)->cfs.avg.load_avg;

	return load;
}

static unsigned long gb_sd_capacity(struct group_balancer_sched_domain *gb_sd)
{
	int cpu;
	int cap = 0;

	for_each_cpu(cpu, gb_sd_span(gb_sd))
		cap += cpu_rq(cpu)->cpu_capacity;

	return cap;
}

static struct group_balancer_sched_domain *select_idle_gb_sd(int specs)
{
	struct group_balancer_sched_domain *gb_sd, *child;

	if (specs == -1 || specs > group_balancer_root_domain->span_weight * 100)
		return group_balancer_root_domain;

	gb_sd = group_balancer_root_domain;

	while (gb_sd) {
		struct group_balancer_sched_domain *max_free_child = NULL;
		int max_free_specs = INT_MIN;
		struct group_balancer_sched_domain *max_unsatisfied_free_child = NULL;
		int max_unsatisfied_free_specs = INT_MIN;

		for_each_gb_sd_child(child, gb_sd) {
			if (child->span_weight * 100 >= specs &&
			    child->free_tg_specs > max_free_specs) {
				max_free_child = child;
				max_free_specs = child->free_tg_specs;
			} else if (child->span_weight * 100 < specs &&
				   child->free_tg_specs > max_unsatisfied_free_specs) {
				max_unsatisfied_free_child = child;
				max_unsatisfied_free_specs = child->free_tg_specs;
			}
		}
		if (!max_free_child)
			break;
		/*
		 * Consider the following case:
		 * gb_sd->span_weight = 6, and gb_sd has two children whose weight are 2 and 4,
		 * and there is a task group with specs 300 selected the child with weight 4
		 * before, and there's another task group with specs 300 needs to select a sched
		 * domain.
		 * In this case, it's unreasonable to select the child with weight 4 for both task
		 * groups. So we select gb_sd to workaround.
		 * When compare the free specs of two group balancer sched domain, we'd better to
		 * compare the proportion of the free specs to the span weight, because the free
		 * specs cannot fully represent the degree of idleness if the span weight is
		 * different.
		 */
		if (max_free_specs < specs &&
		    max_free_specs / max_free_child->span_weight <
		    max_unsatisfied_free_specs / max_unsatisfied_free_child->span_weight)
			break;
		gb_sd = max_free_child;
	}

	return gb_sd;
}

static void
check_task_group_leap_level(struct task_group *tg, struct group_balancer_sched_domain *gb_sd)
{
	struct group_balancer_sched_domain *child;
	int specs = tg->specs_ratio;

	for_each_gb_sd_child(child, gb_sd) {
		if (specs <= 100 * child->span_weight) {
			tg->leap_level = true;
			tg->leap_level_timestamp = jiffies;
			return;
		}
	}

	tg->leap_level = false;
}

void update_free_tg_specs(struct group_balancer_sched_domain *gb_sd, int specs)
{
	struct group_balancer_sched_domain *parent;

	if (specs != -1) {
		for (parent = gb_sd; parent; parent = parent->parent) {
			raw_spin_lock(&parent->lock);
			parent->free_tg_specs += specs;
			raw_spin_unlock(&parent->lock);
		}
	}
}

/*
 * When we attach/detach a task group to/from a domain, we hold the read lock
 * group_balancer_sched_domain_lock first, and then hold gb_sd->lock.
 * When we free a domain, we need to move task groups from domain to its parent, we
 * hold the write lock group_balancer_sched_domain_lock.
 * When we balance two domains, we hold the read lock group_balancer_sched_domain_lock
 * first, and then hold the locks of these two domains.
 * So that there will be no race.
 * TODO: Optimize the lock when we move task groups when balance.
 */
void add_tg_to_group_balancer_sched_domain_locked(struct task_group *tg,
						  struct group_balancer_sched_domain *gb_sd,
						  bool enable)
{
	tg->gb_sd = gb_sd;
	rb_add(&tg->gb_node, &gb_sd->task_groups, tg_specs_less);

	tg->soft_cpus_allowed_ptr = gb_sd_span(gb_sd);
	tg_inc_soft_cpus_version(tg);
	if (enable)
		walk_tg_tree_from(tg, tg_set_gb_tg_down, tg_nop, tg);

	check_task_group_leap_level(tg, gb_sd);
	tg->adjust_level_timestamp = jiffies;
}

void add_tg_to_group_balancer_sched_domain(struct task_group *tg,
					   struct group_balancer_sched_domain *gb_sd,
					   bool enable)
{
	raw_spin_lock(&gb_sd->lock);
	add_tg_to_group_balancer_sched_domain_locked(tg, gb_sd, enable);
	raw_spin_unlock(&gb_sd->lock);
	update_free_tg_specs(gb_sd, -tg->specs_ratio);
}

static void
remove_tg_from_group_balancer_sched_domain_locked(struct task_group *tg,
						  struct group_balancer_sched_domain *gb_sd,
						  bool disable)
{
	tg->gb_sd = NULL;
	rb_erase(&tg->gb_node, &gb_sd->task_groups);
	RB_CLEAR_NODE(&tg->gb_node);
	if (disable)
		walk_tg_tree_from(tg, tg_unset_gb_tg_down, tg_nop, NULL);
}

static void
remove_tg_from_group_balancer_sched_domain(struct task_group *tg,
					   struct group_balancer_sched_domain *gb_sd,
					   bool disable)
{
	read_lock(&group_balancer_sched_domain_lock);
	raw_spin_lock(&gb_sd->lock);
	remove_tg_from_group_balancer_sched_domain_locked(tg, gb_sd, disable);
	raw_spin_unlock(&gb_sd->lock);
	update_free_tg_specs(gb_sd, tg->specs_ratio);
	read_unlock(&group_balancer_sched_domain_lock);
}

int attach_tg_to_group_balancer_sched_domain(struct task_group *tg,
					     struct group_balancer_sched_domain *target,
					     bool enable)
{
	struct group_balancer_sched_domain *gb_sd;
	int ret = 0;

	read_lock(&group_balancer_sched_domain_lock);
	if (enable)
		gb_sd = select_idle_gb_sd(tg->specs_ratio);
	else
		gb_sd = target;
	if (!gb_sd) {
		ret = -ESRCH;
		goto out;
	}
	add_tg_to_group_balancer_sched_domain(tg, gb_sd, enable);
out:
	read_unlock(&group_balancer_sched_domain_lock);
	return ret;
}

void detach_tg_from_group_balancer_sched_domain(struct task_group *tg, bool disable)
{
	struct group_balancer_sched_domain *gb_sd = tg->gb_sd;

	if (!gb_sd)
		return;

	remove_tg_from_group_balancer_sched_domain(tg, gb_sd, disable);
}

static void tg_upper_level(struct task_group *tg, struct group_balancer_sched_domain *gb_sd)
{
	detach_tg_from_group_balancer_sched_domain(tg, false);
	/* Considering the case that gb_sd is NULL, we'd better to treat it as enable. */
	attach_tg_to_group_balancer_sched_domain(tg, gb_sd, !gb_sd);
}

static bool tg_lower_level(struct task_group *tg)
{
	struct group_balancer_sched_domain *gb_sd = tg->gb_sd;
	struct group_balancer_sched_domain *child, *dst;
	unsigned long tg_child_load, tg_load = 0, tg_dst_load = 0;
	unsigned long child_load, src_load, dst_load, total_load = 0, migrate_load;
	unsigned long child_cap, total_cap = 0, src_cap, dst_cap = 0;
	unsigned long src_imb, dst_imb;

	if (!gb_sd)
		goto fail;

	raw_spin_lock(&gb_sd->lock);
	if (!time_after(jiffies, gb_sd->last_balance_timestamp + gb_sd->lower_interval)) {
		raw_spin_unlock(&gb_sd->lock);
		goto fail;
	} else {
		gb_sd->last_balance_timestamp = jiffies;
		raw_spin_unlock(&gb_sd->lock);
	}

	/*
	 * The gb_sd may have some children, and the tasks of may spread across each child.
	 * Lowering the level of tg is essentially a gathering task, so we will find a child
	 * that contains the most load of tg to migrate tg to.
	 */
	for_each_gb_sd_child(child, gb_sd) {
		child_load = gb_sd_load(child);
		total_load += child_load;

		child_cap = gb_sd_capacity(child);
		total_cap += child_cap;

		tg_child_load = tg_gb_sd_load(tg, child);
		if (!dst || tg_child_load > tg_dst_load) {
			dst = child;
			tg_dst_load = tg_child_load;
			dst_load = child_load;
			dst_cap = child_cap;
		} else if (tg_child_load == tg_dst_load) {
			if (dst_load * child_cap > child_load * dst_cap) {
				dst = child;
				tg_dst_load = tg_child_load;
				dst_load = child_load;
				dst_cap = child_cap;
			}
		}
		tg_load += tg_child_load;
	}

	if (tg_load == 0)
		goto fail;
	if (tg->specs_ratio > 100 * dst->span_weight)
		goto fail;
#ifdef CONFIG_NUMA
	/* We won't allow a task group span more than two numa nodes too long. */
	if (dst->gb_flags & GROUP_BALANCER_NUMA_FLAG)
		goto lower;
#endif
	/* If we lower the level, we have to make sure that we will not cause imbalance.
	 *
	 * src_load        dst_load
	 * ------------ vs ---------
	 * src_capacity    dst_capacity
	 *
	 */

	migrate_load = tg_load - tg_dst_load;

	src_cap = total_cap - dst_cap;
	src_load = total_load - dst_load;
	src_imb = abs(src_load * dst_cap - dst_load * src_cap);
	dst_imb = abs((src_load - migrate_load) * dst_cap - (dst_load + migrate_load) * src_cap);

	if (dst_load * src_cap > src_load * dst_cap)
		goto fail;

	if (dst_imb > src_imb)
		goto fail;
#ifdef CONFIG_NUMA
lower:
#endif
	detach_tg_from_group_balancer_sched_domain(tg, false);
	attach_tg_to_group_balancer_sched_domain(tg, dst, false);
	/* The task group maybe still leap level, check it. */
	check_task_group_leap_level(tg, gb_sd);

	return true;
fail:
	tg->leap_level_timestamp = jiffies;
	return false;
}

static void gb_task_group_tick(struct task_group *tg)
{
	struct group_balancer_sched_domain *gb_sd = tg->gb_sd;

	if (!gb_sd)
		return;

	if (!tg->leap_level)
		return;

	if (!time_after(jiffies, tg->leap_level_timestamp + gb_sd->lower_interval))
		return;

	read_lock(&group_balancer_sched_domain_lock);
	tg_lower_level(tg);
	read_unlock(&group_balancer_sched_domain_lock);
}

static struct task_group *gb_task_group(struct task_struct *p)
{
	struct task_group *tg = task_group(p);

	if (tg == &root_task_group || task_group_is_autogroup(tg))
		return NULL;

	return task_group(p)->gb_tg;
}

void task_tick_gb(struct task_struct *p)
{
	struct task_group *tg = gb_task_group(p);

	if (!group_balancer_enabled())
		return;

	if (!tg || !tg_group_balancer_enabled(tg))
		return;

	if (!raw_spin_trylock(&tg->gb_lock))
		return;

	gb_task_group_tick(tg);
	raw_spin_unlock(&tg->gb_lock);
}

void tg_specs_change(struct task_group *tg)
{
	struct group_balancer_sched_domain *gb_sd;
	int specs = tg->specs_ratio;

	gb_sd = tg->gb_sd;
	if (!gb_sd)
		/* tg->group_balancer is always true here, so find a gb_sd to attach. */
		goto upper;

	/* If the task group leaps level after specs change, we will lower it later. */
	check_task_group_leap_level(tg, gb_sd);
	if (tg->leap_level)
		return;

	/* This gb_sd still satisfy, don't do anything. */
	if (specs <= gb_sd->span_weight * 100 || gb_sd == group_balancer_root_domain)
		return;

	/* The specs doesn't satisfy anymore, upper to find a satisfied gb_sd. */
	/* Fast path, if the specs is -1 or too large, move it to root domain. */
	if (specs == -1 || specs > group_balancer_root_domain->span_weight * 100) {
		gb_sd = group_balancer_root_domain;
		goto upper;
	}

	for (; gb_sd; gb_sd = gb_sd->parent) {
		if (specs <= gb_sd->span_weight * 100)
			break;
	}

	if (!gb_sd)
		gb_sd = group_balancer_root_domain;
upper:
	tg_upper_level(tg, gb_sd);

}

static struct group_balancer_sched_domain
*find_matching_gb_sd(struct group_balancer_sched_domain **src,
		     struct group_balancer_sched_domain **dst)
{
	int src_depth, dst_depth;

	if (!*src || !*dst || *src == *dst)
		return NULL;

	src_depth = (*src)->depth;
	dst_depth = (*dst)->depth;

	if (!src_depth || !dst_depth)
		return NULL;

	while (src_depth > dst_depth) {
		src_depth--;
		*src = (*src)->parent;
	}

	while (dst_depth > src_depth) {
		dst_depth--;
		*dst = (*dst)->parent;
	}


	while ((*src)->parent != (*dst)->parent) {
		*src = (*src)->parent;
		*dst = (*dst)->parent;
		if (!*src || !*dst)
			return NULL;
	}

	return (*src)->parent;
}

#define gb_for_each_tg_safe(pos, n, root)					\
	for (pos = rb_entry_safe(rb_first(root), struct task_group, gb_node);	\
	     pos && ({ n = rb_entry_safe(rb_next(&pos->gb_node),		\
			struct task_group, gb_node) ; 1; });				\
	     pos = n)

static int
gb_detach_task_groups_from_gb_sd(struct gb_lb_env *gb_env,
				 struct group_balancer_sched_domain *gb_sd)
{
	struct task_group *tg, *n;
	unsigned long load, util;
	int detached = 0;

	raw_spin_lock(&gb_sd->lock);
	/* Try the task cgroups with little specs first. */
	gb_for_each_tg_safe(tg, n, &gb_sd->task_groups) {
		if (!time_after(jiffies, tg->adjust_level_timestamp + 2 * gb_sd->lower_interval))
			continue;
		switch (gb_env->migration_type) {
#ifdef CONFIG_GROUP_IDENTITY
		case migrate_identity:
			fallthrough;
#endif
		case migrate_load:
			load = tg_gb_sd_load(tg, gb_sd);
			if (load == 0)
				continue;
			if (shr_bound(load, gb_env->nr_balance_failed) > gb_env->imbalance)
				continue;
			gb_env->imbalance -= load;
			break;
		case migrate_util:
			util = tg_gb_sd_util(tg, gb_sd);
			if (util == 0)
				continue;
			if (shr_bound(util, gb_env->nr_balance_failed) > gb_env->imbalance)
				continue;
			gb_env->imbalance -= util;
			break;
		case migrate_task:
			gb_env->imbalance = 0;
			break;
		/*TODO: Perfect strategy of migrate_misfit*/
		case migrate_misfit:
			gb_env->imbalance = 0;
			break;
		default:
			break;
		}
		remove_tg_from_group_balancer_sched_domain_locked(tg, gb_sd, false);
		rb_add(&tg->gb_node, &gb_env->task_groups, tg_specs_less);
		detached++;
		if (gb_env->imbalance <= 0) {
			raw_spin_unlock(&gb_sd->lock);
			return detached;
		}
	}
	raw_spin_unlock(&gb_sd->lock);
	return detached;
}

static int gb_detach_task_groups(struct gb_lb_env *gb_env)
{
	struct group_balancer_sched_domain *parent, *child;
	int detached = 0;

	parent = gb_env->src;
	if (!parent)
		return 0;

down:
	if (cpumask_test_cpu(gb_env->src_cpu, gb_sd_span(parent)))
		detached += gb_detach_task_groups_from_gb_sd(gb_env, parent);
	if (detached || gb_env->imbalance <= 0)
		goto out;
	for_each_gb_sd_child(child, parent) {
		parent = child;
		goto down;
up:
		continue;
	}
	if (parent == gb_env->src)
		goto out;
	child = parent;
	parent = parent->parent;
	if (parent)
		goto up;
out:
	return detached;
}

static void gb_attach_task_groups(struct gb_lb_env *gb_env)
{
	struct task_group *tg;
	struct group_balancer_sched_domain *gb_sd = gb_env->gb_sd;
	struct rb_node *node;
	struct rb_root *root = &gb_env->task_groups;

	raw_spin_lock(&gb_sd->lock);
	while (!RB_EMPTY_ROOT(root)) {
		node = root->rb_node;
		tg = __gb_node_2_tg(node);
		rb_erase(node, &gb_env->task_groups);
		add_tg_to_group_balancer_sched_domain_locked(tg, gb_sd, false);
	}
	raw_spin_unlock(&gb_sd->lock);
}

static void __update_gb_sd_status(struct group_balancer_sched_domain *gb_sd, int *gb_sd_status)
{
	int i, nr_running;

	for_each_cpu(i, gb_sd_span(gb_sd)) {
		struct rq *rq = cpu_rq(i);

		nr_running = rq->nr_running;
		if (nr_running > 1)
			*gb_sd_status |= GB_OVERLOAD;

		if (gb_cpu_overutilized(i))
			*gb_sd_status |= GB_OVERUTILIZED;
	}
}

static void update_gb_sd_status(struct gb_lb_env *gb_env, int *gb_sd_status)
{
	if (!gb_env->src)
		return;

	__update_gb_sd_status(gb_env->src, gb_sd_status);
}

void gb_load_balance(struct lb_env *env)
{
	struct rq *src_rq = env->src_rq, *dst_rq = env->dst_rq;
	struct gb_lb_env gb_env;
	struct group_balancer_sched_domain *src, *dst, *gb_sd, *parent;
	struct rb_node *node;
	struct task_group *tg;
	int gb_sd_status = 0;
	struct cpumask *gb_mask = this_cpu_cpumask_var_ptr(group_balancer_mask);
	unsigned long src_load, src_cap, dst_load, dst_cap;

	if (!group_balancer_enabled())
		return;

	/*
	 * src cpu has balanced some task groups to dst cpu during this load balance
	 * process, skip it.
	 */
	if (cpumask_test_cpu(env->src_cpu, gb_mask))
		return;

	if (!src_rq || !dst_rq)
		return;

	read_lock(&group_balancer_sched_domain_lock);

	src = src_rq->gb_sd;
	dst = dst_rq->gb_sd;

	gb_sd = find_matching_gb_sd(&src, &dst);
	if (!gb_sd)
		goto unlock;

	if (!time_after(jiffies, gb_sd->last_balance_timestamp + 2 * gb_sd->lower_interval))
		goto unlock;

	src_load = gb_sd_load(src);
	src_cap = gb_sd_capacity(src);
	dst_load = gb_sd_load(dst);
	dst_cap = gb_sd_capacity(dst);

	if (dst_load * src_cap * gb_sd->imbalance_pct >= src_load * dst_cap * 100)
		goto unlock;

	gb_env = (struct gb_lb_env){
		.src_cpu		= env->src_cpu,
		.src			= src,
		.dst			= dst,
		.gb_sd			= gb_sd,
		.migration_type		= env->migration_type,
		.imbalance		= env->imbalance,
		.nr_balance_failed	= env->sd->nr_balance_failed,
		.task_groups		= RB_ROOT,
	};

	/*
	 * If there are some tasks belongs to gb_sd or any ancestor, they can be migrated,
	 * and we don't migrate tg in this case.
	 */
	for (parent = gb_sd; parent; parent = parent->parent) {
		for (node = rb_first(&parent->task_groups); node; node = rb_next(node)) {
			tg = __node_2_task_group(node);
			if (tg->cfs_rq[env->src_cpu]->h_nr_runnable)
				goto unlock;
		}
	}

	update_gb_sd_status(&gb_env, &gb_sd_status);
	/*
	 * If the src domain is not overloaded, or there no imbalance between src and dst domain,
	 * do not migrate task groups.
	 */
	if (!gb_sd_status || !gb_env.imbalance)
		goto out;

	if (gb_detach_task_groups(&gb_env))
		gb_attach_task_groups(&gb_env);

out:
	cpumask_or(gb_mask, gb_mask, gb_sd_span(gb_sd));
unlock:
	read_unlock(&group_balancer_sched_domain_lock);
}

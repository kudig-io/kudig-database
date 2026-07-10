// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "pagecache_limit: " fmt

#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/pagemap.h>
#include <linux/memcontrol.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/pagecache_limit.h>

DEFINE_STATIC_KEY_FALSE(pagecache_limit_enabled_key);
struct workqueue_struct *memcg_pgcache_limit_wq;

static int __init setup_pagecache_limit(char *s)
{
	if (!strcmp(s, "1"))
		static_branch_enable(&pagecache_limit_enabled_key);
	else if (!strcmp(s, "0"))
		static_branch_disable(&pagecache_limit_enabled_key);
	return 1;
}
__setup("pagecache_limit=", setup_pagecache_limit);

bool is_memcg_pgcache_limit_enabled(struct mem_cgroup *memcg)
{
	if (!pagecache_limit_enabled())
		return false;

	return READ_ONCE(memcg->allow_pgcache_limit);
}

static inline unsigned long memcg_get_pgcache_nr_pages(struct mem_cgroup *memcg)
{
	/*
	 * There use 'NR_INACTIVE_FILE' + 'NR_ACTIVE_FILE'
	 * to represent pagecache.
	 * Due to changes in the memcg state update strategy,
	 * we need to proactively perform a refresh so that
	 * we could read accurate per-memcg lruvec stats.
	 */
	cgroup_rstat_flush(memcg->css.cgroup);

	return memcg_page_state(memcg, NR_INACTIVE_FILE) +
	       memcg_page_state(memcg, NR_ACTIVE_FILE);
}

unsigned long memcg_get_pgcache_overflow_size(struct mem_cgroup *memcg)
{
	unsigned long limit_pgcache, total_pgcache;

	limit_pgcache = READ_ONCE(memcg->pgcache_limit_size) / PAGE_SIZE;
	if (!limit_pgcache)
		return 0;

	total_pgcache = memcg_get_pgcache_nr_pages(memcg);
	if (total_pgcache > limit_pgcache)
		return total_pgcache - limit_pgcache;

	return 0;
}

void memcg_add_pgcache_limit_reclaimed(struct mem_cgroup *memcg,
				       unsigned long nr)
{
	struct mem_cgroup *iter;

	preempt_disable();

	for (iter = memcg; iter; iter = parent_mem_cgroup(iter))
		__this_cpu_add(iter->exstat_cpu->item[MEMCG_PGCACHE_RECLAIM],
			       nr);

	preempt_enable();
}

void memcg_pgcache_limit_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct mem_cgroup *memcg;

	memcg = container_of(dwork, struct mem_cgroup, pgcache_limit_work);
	if (!is_memcg_pgcache_limit_enabled(memcg))
		return;

	current->flags |= PF_MEMALLOC | PF_KSWAPD;
	__memcg_pagecache_shrink(memcg, true, GFP_KERNEL);
	current->flags &= ~(PF_MEMALLOC | PF_KSWAPD);
	if (memcg->pgcache_limit_reclaim_interval != 0 &&
	    memcg_get_pgcache_overflow_size(memcg))
		queue_delayed_work(memcg_pgcache_limit_wq, dwork,
				   memcg->pgcache_limit_reclaim_interval);
}

void memcg_pagecache_shrink(struct mem_cgroup *memcg, gfp_t gfp_mask)
{
	struct mem_cgroup *tmp_memcg = memcg;

	if (!memcg || !is_memcg_pgcache_limit_enabled(memcg))
		return;

	/*
	 * We support pagecache to check not only current memcg, but also
	 * there parent memcg, to prevent the parent group which has large
	 * number of pagecache but not release it in time.
	 */
	do {
		if (!memcg_get_pgcache_overflow_size(tmp_memcg))
			continue;
		/*
		 * In direct memory reclaim path, we default support file pagecache
		 * which is unmapped, but we also concern most of pagecache are mapped,
		 * it would lead to "pagecache limit" has no effect, so in "sc.priority"
		 * traverses, we select the appropriate time to enable mapped pagecache
		 * to be reclaimed.
		 */
		if (tmp_memcg->pgcache_limit_sync == PGCACHE_RECLAIM_DIRECT)
			__memcg_pagecache_shrink(tmp_memcg, false, gfp_mask);
		else
			queue_delayed_work(memcg_pgcache_limit_wq,
					   &tmp_memcg->pgcache_limit_work,
					   tmp_memcg->pgcache_limit_reclaim_interval);
	} while ((tmp_memcg = parent_mem_cgroup(tmp_memcg)) &&
		 is_memcg_pgcache_limit_enabled(tmp_memcg));
}

#ifdef CONFIG_SYSFS
static ssize_t pagecache_limit_enabled_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!static_branch_unlikely(&pagecache_limit_enabled_key));
}

static ssize_t pagecache_limit_enabled_store(struct kobject *kobj,
					     struct kobj_attribute *attr,
					     const char *buf, size_t count)
{
	static DEFINE_MUTEX(mutex);
	ssize_t ret = count;

	mutex_lock(&mutex);

	if (!strncmp(buf, "1", 1))
		static_branch_enable(&pagecache_limit_enabled_key);
	else if (!strncmp(buf, "0", 1))
		static_branch_disable(&pagecache_limit_enabled_key);
	else
		ret = -EINVAL;

	mutex_unlock(&mutex);
	return ret;
}

static struct kobj_attribute pagecache_limit_enabled_attr =
	__ATTR(enabled, 0644, pagecache_limit_enabled_show,
	       pagecache_limit_enabled_store);

static struct attribute *pagecache_limit_attrs[] = {
	&pagecache_limit_enabled_attr.attr,
	NULL,
};

static struct attribute_group pagecache_limit_attr_group = {
	.attrs = pagecache_limit_attrs,
};

static int __init pagecache_limit_init_sysfs(void)
{
	int err;
	struct kobject *pagecache_limit_kobj;

	pagecache_limit_kobj = kobject_create_and_add("pagecache_limit", mm_kobj);
	if (!pagecache_limit_kobj) {
		pr_err("failed to create pagecache_limit kobject\n");
		return -ENOMEM;
	}
	err = sysfs_create_group(pagecache_limit_kobj, &pagecache_limit_attr_group);
	if (err) {
		pr_err("failed to register pagecache_limit group\n");
		goto delete_obj;
	}

	return 0;

delete_obj:
	kobject_put(pagecache_limit_kobj);
	return err;
}
#endif /* CONFIG_SYSFS */

static int __init pagecache_limit_init(void)
{
	int ret = -EINVAL;

#ifdef CONFIG_SYSFS
	ret = pagecache_limit_init_sysfs();
#endif

	return ret;
}
module_init(pagecache_limit_init);

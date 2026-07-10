/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __RECLAIM_COLDPGS_H__
#define __RECLAIM_COLDPGS_H__

/* Correspond to global & memcg control flags */
#define FLAG_IGNORE_MLOCK	0x1
#define FLAG_IGNORE_AGE		0x2
#define	FLAG_DROPPABLE(val)	((val) & 0xffffffff)
#define	FLAG_MODE(val)		((val) >> 32 & 0x7)
/* Get mlock and ignore_age bits */
#define	FLAG_CTRL(val)		((val) >> 35 & 0x3)

enum {
	RECLAIM_MODE_PGCACHE_OUT,
	RECLAIM_MODE_ANON_OUT,
	RECLAIM_MODE_SLAB,
	RECLAIM_MODE_MAX
};

enum {
	THRESHOLD_BASE,
	THRESHOLD_NONROT,
	THRESHOLD_MAX
};

/*
 * Global control struct used to track the information exposed through
 * the sysfs files. @rwsem finely represents access pattern: multiple
 * readers are allowed, but one writer is permitted at once.
 */
struct reclaim_coldpgs_global_control {
	struct rw_semaphore	rwsem;
	unsigned int		version;
	unsigned int		hierarchy;
	unsigned int		batch;
	unsigned int		flags;
	unsigned int		mode;
	unsigned int		thresholds[THRESHOLD_MAX];
};

/*
 * A filter is populated when we're going to reclaim cold memory from
 * the specified memory cgroup. Part of the data is copied over from
 * the global control and the left comes from the memory cgroup's
 * control struct. Its main purpose to gurantee the consistence because
 * the global control or memory cgroup's control block can be modified
 * on the fly.
 */
struct reclaim_coldpgs_filter {
	unsigned int	flags;
	unsigned int	batch;
	unsigned int	mode;
	unsigned int	threshold;
	unsigned long	size;
	unsigned int	thresholds[THRESHOLD_MAX];
};

#define for_each_memcg_tree(root, iter)					\
	for (iter = my_mem_cgroup_iter(root, NULL, NULL);		\
	     iter != NULL;						\
	     iter = my_mem_cgroup_iter(root, iter, NULL))
#define reclaim_coldpgs_resolve_symbol(name)				\
	do {								\
		my_##name = (void *)kallsyms_lookup_name(#name);	\
		if (!my_##name) {					\
			pr_warn("%s: Unable to resolve symbol [%s]",	\
				__func__, #name);			\
			return -ENOENT;					\
		}							\
	} while (0)
#define for_each_frontswap_ops(ops)					\
	for (ops = *my_frontswap_ops; ops; ops = ops->next)
#define RECLAIM_COLDPGS_SYSFS_HANDLER(name, field, hex, min, max)	\
static ssize_t reclaim_coldpgs_show_##name(struct kobject *kobj,	\
					   struct kobj_attribute *attr,	\
					   char *buf)			\
{									\
	unsigned int val;						\
	int ret;							\
									\
	down_read(&global_control.rwsem);				\
	val = global_control.field;					\
	up_read(&global_control.rwsem);					\
									\
	ret = sprintf(buf, hex ? "0x%x\n" : "%u\n", val);		\
									\
	return ret;							\
}									\
									\
static ssize_t reclaim_coldpgs_store_##name(struct kobject *kobj,	\
					    struct kobj_attribute *attr,\
					    const char *buf,		\
					    size_t count)		\
{									\
	unsigned int val;						\
	int ret;							\
									\
	ret = kstrtouint(buf, hex ? 16 : 10, &val);			\
	if (ret || val < min || val > max)				\
		return -EINVAL;						\
									\
	down_write(&global_control.rwsem);				\
	global_control.field = val;					\
	up_write(&global_control.rwsem);				\
									\
	return count;							\
}
#define RECLAIM_COLDPGS_ATTR(name, mode)				\
	static struct kobj_attribute reclaim_coldpgs_attr_##name =	\
		__ATTR(name, mode,					\
		       reclaim_coldpgs_show_##name,			\
		       reclaim_coldpgs_store_##name)

#endif /* __RECLAIM_COLDPGS_H__ */

// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/pre_oom.h>

DEFINE_STATIC_KEY_FALSE(pre_oom_enabled_key);

/*
 * From 0 .. 3, which means the kernel can support up to
 * num_online_cpus / oom_level tasks to reclaim memory.
 */
static int oom_level;
static struct semaphore *sem;
static DEFINE_MUTEX(pre_oom_mutex);

int pre_oom_enter(void)
{
	int result;

	if (!pre_oom_enabled())
		return 0;

	result = down_killable(sem);
	if (!result)
		current->reclaim_stall = 1;

	return result;
}

void pre_oom_leave(void)
{
	if (pre_oom_enabled() && current->reclaim_stall) {
		current->reclaim_stall = 0;
		up(sem);
	}
}

static int adjust_oom_level(int level)
{
	unsigned long flags;
	int count = num_online_cpus() / (level + 1);
	int result = 0;

	raw_spin_lock_irqsave(&sem->lock, flags);

	/* There are no other tasks reclaiming memory */
	if (sem->count == (num_online_cpus() / (oom_level + 1))) {
		sem->count = count;
		oom_level = level;
	} else
		result = -EPERM;

	raw_spin_unlock_irqrestore(&sem->lock, flags);

	return result;
}

#ifdef CONFIG_SYSFS
static ssize_t pre_oom_enabled_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!static_branch_unlikely(&pre_oom_enabled_key));
}

static ssize_t pre_oom_enabled_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t count)
{
	ssize_t ret = count;

	mutex_lock(&pre_oom_mutex);

	if (!strncmp(buf, "1", 1))
		static_branch_enable(&pre_oom_enabled_key);
	else if (!strncmp(buf, "0", 1))
		static_branch_disable(&pre_oom_enabled_key);
	else
		ret = -EINVAL;

	mutex_unlock(&pre_oom_mutex);
	return ret;
}

static ssize_t pre_oom_level_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", oom_level);
}

static ssize_t pre_oom_level_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t count)
{
	ssize_t ret = count;
	unsigned long level;

	mutex_lock(&pre_oom_mutex);

	ret = kstrtoul(buf, 10, &level);
	if (ret)
		return ret;

	if (level < 0 || level > 3)
		ret = -EINVAL;

	adjust_oom_level(level);

	mutex_unlock(&pre_oom_mutex);

	return ret ?: count;

}

static struct kobj_attribute pre_oom_enabled_attr =
		__ATTR(enabled, 0644, pre_oom_enabled_show,
		       pre_oom_enabled_store);

static struct kobj_attribute pre_oom_level_attr =
		__ATTR(level, 0644, pre_oom_level_show,
		       pre_oom_level_store);

static struct attribute *pre_oom_attrs[] = {
	&pre_oom_enabled_attr.attr,
	&pre_oom_level_attr.attr,
	NULL,
};

static const struct attribute_group pre_oom_attr_group = {
	.attrs = pre_oom_attrs,
	.name = "pre_oom",
};
#endif /* CONFIG_SYSFS */

static int __init pre_oom_init(void)
{
	int err;
#ifdef CONFIG_SYSFS
	err = sysfs_create_group(mm_kobj, &pre_oom_attr_group);
	if (err) {
		pr_err("pre_oom: register sysfs failed\n");
		return err;
	}
#endif

	sem = kmalloc(sizeof(*sem), GFP_KERNEL);
	if (!sem) {
		sysfs_remove_group(mm_kobj, &pre_oom_attr_group);
		return -ENOMEM;
	}

	sema_init(sem, num_online_cpus());
	return 0;
}
subsys_initcall(pre_oom_init);

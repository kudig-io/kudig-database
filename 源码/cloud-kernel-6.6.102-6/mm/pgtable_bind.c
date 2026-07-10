// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/pgtable_bind.h>

#ifdef CONFIG_PGTABLE_BIND
#ifdef CONFIG_SYSFS
DEFINE_STATIC_KEY_FALSE(pgtable_bind_enabled_key);
DEFINE_STATIC_KEY_FALSE(pgtable_stat_enabled_key);

static ssize_t pgtable_bind_enabled_show(struct kobject *kobj,
					 struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!static_key_enabled(&pgtable_bind_enabled_key) +
		       !!static_key_enabled(&pgtable_stat_enabled_key));
}

static ssize_t pgtable_bind_enabled_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t count)
{
	static DEFINE_MUTEX(mutex);
	ssize_t ret = count;

	mutex_lock(&mutex);

	if (!strncmp(buf, "2", 1)) {
		static_branch_enable(&pgtable_bind_enabled_key);
		static_branch_enable(&pgtable_stat_enabled_key);
	} else if (!strncmp(buf, "1", 1)) {
		static_branch_disable(&pgtable_bind_enabled_key);
		static_branch_enable(&pgtable_stat_enabled_key);
	} else if (!strncmp(buf, "0", 1)) {
		static_branch_disable(&pgtable_bind_enabled_key);
		static_branch_disable(&pgtable_stat_enabled_key);
	}

	mutex_unlock(&mutex);
	return ret;
}

static struct kobj_attribute pgtable_bind_enabled_attr =
		__ATTR(enabled, 0644, pgtable_bind_enabled_show,
		       pgtable_bind_enabled_store);
static struct attribute *pgtable_bind_attrs[] = {
	&pgtable_bind_enabled_attr.attr,
	NULL,
};
static const struct attribute_group pgtable_bind_attr_group = {
	.attrs = pgtable_bind_attrs,
	.name = "pgtable_bind",
};

static int __init pgtable_bind_init(void)
{
	int ret;

	ret = sysfs_create_group(mm_kobj, &pgtable_bind_attr_group);
	if (ret)
		pr_err("pgtable_bind: register sysfs failed\n");

	return ret;
}
subsys_initcall(pgtable_bind_init);
#endif /* CONFIG_SYSFS */
#endif /* CONFIG_PGTABLE_BIND */

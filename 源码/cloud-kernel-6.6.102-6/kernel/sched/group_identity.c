// SPDX-License-Identifier: GPL-2.0-only
/*
 * Scheduler group identity sysfs ABI.
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>

#define GROUP_IDENTITY_ABI	2
#define GROUP_IDENTITY_SCHEME	"group_identity_2.0"

#define GI_ATTR_RO(_name) \
	static struct kobj_attribute group_identity_attr_##_name = __ATTR_RO(_name)

static ssize_t abi_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d\n", GROUP_IDENTITY_ABI);
}
GI_ATTR_RO(abi);

static ssize_t scheme_show(struct kobject *kobj,
			   struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%s\n", GROUP_IDENTITY_SCHEME);
}
GI_ATTR_RO(scheme);

static struct attribute *group_identity_attrs[] = {
	&group_identity_attr_abi.attr,
	&group_identity_attr_scheme.attr,
	NULL,
};

static const struct attribute_group group_identity_attr_group = {
	.attrs = group_identity_attrs,
};

static struct kset *group_identity_kset;

static int __init group_identity_sysfs_init(void)
{
	int ret;

	group_identity_kset = kset_create_and_add("group_identity", NULL,
						      kernel_kobj);
	if (!group_identity_kset)
		return -ENOMEM;

	ret = sysfs_create_group(&group_identity_kset->kobj,
				 &group_identity_attr_group);
	if (ret) {
		kset_unregister(group_identity_kset);
		group_identity_kset = NULL;
		return ret;
	}

	return 0;
}
subsys_initcall(group_identity_sysfs_init);

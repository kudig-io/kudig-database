// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/rmap.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/file_zeropage.h>
#include <linux/pagemap.h>

#include <linux/pgtable.h>

DEFINE_STATIC_KEY_FALSE(file_zeropage_enabled_key);

struct folio *__alloc_zero_folio(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct folio *folio = NULL;

	if (vmf && !mm_forbids_zeropage(vma->vm_mm) &&
	    !(vma->vm_flags & VM_SHARED) &&
	    !(vmf->flags & FAULT_FLAG_NONZEROPAGE))
		folio = page_folio(ZERO_PAGE(0));

	return folio;
}

inline void unmap_zero_folio(struct folio *folio, struct vm_area_struct *vma,
				  struct address_space *mapping)
{
	if (mapping_zero_folio(mapping) && folio && vma && (vma->vm_flags & VM_SHARED))
		try_to_unmap_zero_folio(folio, TTU_ZEROPAGE);
}

static int __init setup_file_zeropage(char *s)
{
	if (!strcmp(s, "1"))
		static_branch_enable(&file_zeropage_enabled_key);
	else if (!strcmp(s, "0"))
		static_branch_disable(&file_zeropage_enabled_key);
	return 1;
}
__setup("file_zeropage=", setup_file_zeropage);

static ssize_t file_zeropage_enabled_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!static_branch_unlikely(&file_zeropage_enabled_key));
}

static ssize_t file_zeropage_enabled_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t count)
{
	static DEFINE_MUTEX(mutex);
	ssize_t ret = count;

	mutex_lock(&mutex);

	if (!strncmp(buf, "1", 1))
		static_branch_enable(&file_zeropage_enabled_key);
	else if (!strncmp(buf, "0", 1))
		static_branch_disable(&file_zeropage_enabled_key);
	else
		ret = -EINVAL;

	mutex_unlock(&mutex);
	return ret;
}

static struct kobj_attribute file_zeropage_enabled_attr =
		__ATTR(enabled, 0644, file_zeropage_enabled_show,
		       file_zeropage_enabled_store);

static struct attribute *file_zeropage_attrs[] = {
	&file_zeropage_enabled_attr.attr,
	NULL,
};

static const struct attribute_group file_zeropage_attr_group = {
	.attrs = file_zeropage_attrs,
	.name = "file_zeropage",
};

static int __init file_zeropage_init(void)
{
	int err;

	err = sysfs_create_group(mm_kobj, &file_zeropage_attr_group);
	if (err) {
		pr_err("file_zeropage: register sysfs failed\n");
		return err;
	}
	return 0;
}
subsys_initcall(file_zeropage_init);

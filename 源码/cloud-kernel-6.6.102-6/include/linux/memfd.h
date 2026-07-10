/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MEMFD_H
#define __LINUX_MEMFD_H

#include <linux/file.h>

#ifdef CONFIG_MEMFD_CREATE
extern long memfd_fcntl(struct file *file, unsigned int cmd, unsigned int arg);
struct folio *memfd_alloc_folio(struct file *memfd, pgoff_t idx);
struct file *memfd_alloc_file(const char *name, unsigned int flags);
unsigned int *memfd_file_seals_ptr(struct file *file);
int memfd_get_seals(struct file *file);
int memfd_add_seals(struct file *file, unsigned int seals);
#else
static inline long memfd_fcntl(struct file *f, unsigned int c, unsigned int a)
{
	return -EINVAL;
}
static inline struct folio *memfd_alloc_folio(struct file *memfd, pgoff_t idx)
{
	return ERR_PTR(-EINVAL);
}
static inline struct file *memfd_alloc_file(const char *name, unsigned int flags)
{
	return ERR_PTR(-EINVAL);
}

static inline unsigned int *memfd_file_seals_ptr(struct file *file)
{
	return NULL;
}

static inline int memfd_get_seals(struct file *file)
{
	return -EINVAL;
}

static inline int memfd_add_seals(struct file *file, unsigned int seals)
{
	return -EINVAL;
}
#endif

/* Retrieve memfd seals associated with the file, if any. */
static inline unsigned int memfd_file_seals(struct file *file)
{
	unsigned int *sealsp = memfd_file_seals_ptr(file);

	return sealsp ? *sealsp : 0;
}

#endif /* __LINUX_MEMFD_H */

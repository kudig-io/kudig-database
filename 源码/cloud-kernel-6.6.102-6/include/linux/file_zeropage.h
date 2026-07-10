/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FILE_ZEROPAGE_H_
#define _LINUX_FILE_ZEROPAGE_H_

#include <linux/types.h>
#include <linux/jump_label.h>

DECLARE_STATIC_KEY_FALSE(file_zeropage_enabled_key);
static inline bool file_zeropage_enabled(void)
{
	return static_branch_unlikely(&file_zeropage_enabled_key);
}

extern struct folio *__alloc_zero_folio(struct vm_area_struct *vma, struct vm_fault *vmf);

static inline struct folio *alloc_zero_folio(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	if (file_zeropage_enabled())
		return __alloc_zero_folio(vma, vmf);
	return NULL;
}

inline void unmap_zero_folio(struct folio *folio, struct vm_area_struct *vma,
		    struct address_space *mapping);

#endif /* _LINUX_FILE_ZEROPAGE_H_ */

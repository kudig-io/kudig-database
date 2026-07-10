/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PAGE_DUP_H_
#define _LINUX_PAGE_DUP_H_

#include <linux/types.h>

struct page;
struct folio;
struct vm_area_struct;

#ifdef CONFIG_DUPTEXT
DECLARE_STATIC_KEY_FALSE(duptext_enabled_key);
static inline bool duptext_enabled(void)
{
	return static_branch_unlikely(&duptext_enabled_key);
}
static inline bool folio_dup_any(struct folio *folio)
{
	return folio_test_dup(folio);
}
static inline bool folio_dup_master(struct folio *folio)
{
	return folio_test_dup(folio) && !folio_test_dup_slave(folio);
}
static inline bool folio_dup_slave(struct folio *folio)
{
	return folio_test_dup(folio) && folio_test_dup_slave(folio);
}

static inline bool page_dup_any(struct page *page)
{
	if (!PageCompound(page))
		return PageDup(page);

	page = compound_head(page);
	return PageDup(page);
}
#else
static inline bool duptext_enabled(void)
{
	return false;
}
static inline bool folio_dup_any(struct folio *folio)
{
	return false;
}
static inline bool folio_dup_master(struct folio *folio)
{
	return false;
}
static inline bool folio_dup_slave(struct folio *folio)
{
	return false;
}
static inline bool page_dup_any(struct page *page)
{
	return false;
}
#endif /* CONFIG_DUPTEXT */

/*
 * Only text vma is suitable for dup folio
 */
extern bool __dup_folio_suitable(struct vm_area_struct *vma, struct mm_struct *mm);

/*
 * Find get or create a dup folio
 * @folio: master folio
 */
extern struct folio *__dup_folio(struct folio *folio, struct vm_area_struct *vma);

/*
 * Return the master folio
 * @folio: master folio, or slave folio
 */
extern struct folio *__dup_folio_master(struct folio *folio);

/*
 * Has any dup folio mapped
 * @folio: master folio
 */
extern bool __dup_folio_mapped(struct folio *folio);

/*
 * Remove all the dup folio
 * @folio: master folio
 * @locked: hold rmap_lock or not
 * @ignore_mlock: ignore mlock or not
 */
extern bool __dedup_folio(struct folio *folio, bool locked, bool ignore_mlock);

static inline bool dup_folio_suitable(struct vm_area_struct *vma, struct mm_struct *mm)
{
	if (duptext_enabled())
		return __dup_folio_suitable(vma, mm);
	return false;
}

static inline struct folio *dup_folio(struct folio *folio, struct vm_area_struct *vma)
{
	if (duptext_enabled())
		return __dup_folio(folio, vma);
	return NULL;
}

/*
 * NOTE This does not guarantee that the master folio exists when
 * dup_folio_master returns. Caller should first verify and get a
 * reference of the master folio.
 */
static inline struct folio *dup_folio_master(struct folio *folio)
{
	if (folio_dup_any(folio))
		return __dup_folio_master(folio);
	return folio;
}

static inline bool dup_folio_mapped(struct folio *folio)
{
	if (folio_dup_master(folio))
		return __dup_folio_mapped(folio);
	return false;
}

/*
 * Return false if the duplicated slave folio can not be released successfully.
 */
static inline bool dedup_folio(struct folio *folio, bool locked)
{
	if (folio_dup_master(folio))
		return __dedup_folio(folio, locked, true);
	return true;
}

static inline bool dedup_folio2(struct folio *folio, bool locked, bool ignore_mlock)
{
	if (folio_dup_master(folio))
		return __dedup_folio(folio, locked, ignore_mlock);
	return true;
}
#endif /* _LINUX_PAGE_DUP_H_ */

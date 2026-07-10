// SPDX-License-Identifier: GPL-2.0
/*
 * This implements mechanism to reclaim the clean page cache and anonymous
 * memory. The reclaimed page cache could be migrated to low speed storage
 * like persistent memory, or dropped. The reclaimed anonymous memory should
 * be saved somewhere and the possible targets can be persistent memory, zSwap,
 * normal swap partition or file.
 *
 * Copyright Gavin Shan, Alibaba Inc 2019
 * Copyright Zelin Deng, Alibaba Inc 2025
 */

#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/jump_label.h>
#include <linux/memcontrol.h>
#include <linux/mm.h>
#include <linux/mm_inline.h>
#include <linux/mmdebug.h>
#include <linux/buffer_head.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/swapfile.h>
#include <linux/swapops.h>
#include <linux/kidled.h>
#include <linux/sysfs.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/rcupdate.h>

#include "internal.h"
#include "coldpgs.h"

#define DRIVER_VERSION	"3.0.0"
#define DRIVER_DESC	"Reclaim Cold Pages"

static struct reclaim_coldpgs_global_control global_control;

/*
 * The module uses various functions or variables, which aren't exported
 * yet. So we look for and use their symbols directly.
 */
static struct mem_cgroup *(*my_mem_cgroup_iter)(struct mem_cgroup *,
	struct mem_cgroup *, struct mem_cgroup_reclaim_cookie *);
static void (*my_mem_cgroup_iter_break)(struct mem_cgroup *,
	struct mem_cgroup *);
static long (*my_mem_cgroup_get_nr_swap_pages)(struct mem_cgroup *);
static int (*my_add_to_swap)(struct folio *folio);
static int (*my_folio_free_swap)(struct folio *);
static void (*my_end_swap_bio_write)(struct bio *);
static void (*my___swap_writepage)(struct page *, struct writeback_control *);
static void (*my_lru_add_drain)(void);
static int (*my_split_huge_page_to_list)(struct page *, struct list_head *);
static int (*my_can_split_folio)(struct folio *, int *);
static void (*my_try_to_unmap)(struct folio *, enum ttu_flags);
static int (*my___remove_mapping)(struct address_space *, struct folio *, bool,
				  struct mem_cgroup *);
static bool (*my_mem_cgroup_swap_full)(struct folio *);
static void (*my_try_to_unmap_flush_dirty)(void);
static void (*my_try_to_unmap_flush)(void);
static void (*my_putback_lru_page)(struct page *page);
static void (*my_workingset_age_nonresident)(struct lruvec *lruvec, unsigned long nr_pages);
static void (*my_mem_cgroup_update_lru_size)(struct lruvec *,
	enum lru_list, int, int);
static void (*my___mem_cgroup_uncharge)(struct folio *);
static void (*my___mem_cgroup_uncharge_list)(struct list_head *);
static void (*my_free_unref_page_list)(struct list_head *);
static struct vm_area_struct *(*my_vma_interval_tree_iter_first)(
	struct rb_root_cached *, unsigned long, unsigned long);
static struct vm_area_struct *(*my_vma_interval_tree_iter_next)(
	struct vm_area_struct *, unsigned long, unsigned long);
static int (*my_cgroup_add_dfl_cftypes)(struct cgroup_subsys *,
	struct cftype *);
static int (*my_cgroup_add_legacy_cftypes)(struct cgroup_subsys *,
	struct cftype *);
static int (*my_cgroup_rm_cftypes)(struct cftype *);
static int (*my_page_counter_memparse)(const char *buf, const char *max,
				       unsigned long *nr_pages);
static unsigned long (*my_memcg_page_state)(struct mem_cgroup *memcg, int idx);
static int *my_vm_swappiness;
static struct swap_info_struct **my_swap_info;
static struct list_head *my_shrinker_list;
static struct idr *my_shrinker_idr;
static struct mem_cgroup **my_root_mem_cgroup;
static int *my_shrinker_nr_max;
static void (*my_css_task_iter_start)(struct cgroup_subsys_state *,
	unsigned int, struct css_task_iter *);
static struct task_struct *(*my_css_task_iter_next)(struct css_task_iter *);
static void (*my_css_task_iter_end)(struct css_task_iter *);
#if CONFIG_PGTABLE_LEVELS > 4
#ifdef CONFIG_X86_5LEVEL
static unsigned int *my___pgtable_l5_enabled;
#endif
#endif
static void (*my_pgd_clear_bad)(pgd_t *);
#if CONFIG_PGTABLE_LEVELS > 4
static void (*my_p4d_clear_bad)(p4d_t *);
#else
#define my_p4d_clear_bad(p4d) do { } while (0)
#endif
#ifndef __PAGETABLE_PUD_FOLDED
static void (*my_pud_clear_bad)(pud_t *);
#else
#define my_pud_clear_bad(p4d) do { } while (0)
#endif
static void (*my_pmd_clear_bad)(pmd_t *);
static pmd_t *(*my_mm_find_pmd)(struct mm_struct *, unsigned long);
static int (*my_do_swap_page)(struct vm_fault *);
static unsigned long (*my_node_page_state)(struct pglist_data *pgdat,
				enum node_stat_item item);
static void (*my___mod_lruvec_state)(struct lruvec *,
		enum node_stat_item, int val);

static
struct anon_vma *(*my_folio_lock_anon_vma_read)(struct folio *page,
						struct rmap_walk_control *rwc);
static int (*my_page_mapped_in_vma)(struct page *page,
				    struct vm_area_struct *vma);
static struct anon_vma_chain *
(*my_anon_vma_interval_tree_iter_first)(struct rb_root_cached *root,
					unsigned long first, unsigned long last);
static struct anon_vma_chain *
(*my_anon_vma_interval_tree_iter_next)(struct anon_vma_chain *node,
				       unsigned long first, unsigned long last);

#ifndef my_anon_vma_interval_tree_foreach
#define my_anon_vma_interval_tree_foreach(avc, root, start, last)	\
	for (avc = my_anon_vma_interval_tree_iter_first(root, start, last); \
	     avc; avc = my_anon_vma_interval_tree_iter_next(avc, start, last))
#endif

static int (*my_folio_total_mapcount)(struct folio *folio);

static void (*my_destroy_large_folio)(struct folio *folio);

static pte_t * (*my___pte_offset_map)(pmd_t *pmd, unsigned long addr,
				      pmd_t *pmdvalp);

static void (*my_folio_putback_lru)(struct folio *folio);

static bool (*my_zswap_store)(struct folio *folio);

#ifdef CONFIG_ARM64
static int (*my_mte_save_tags)(struct page *page);
#endif

static inline int my_folio_mapcount(struct folio *folio)
{
	if (likely(!folio_test_large(folio)))
		return atomic_read(&folio->_mapcount) + 1;
	return my_folio_total_mapcount(folio);
}

static inline int my_split_folio_to_list(struct folio *folio,
					 struct list_head *list)
{
	return my_split_huge_page_to_list(&folio->page, list);
}

static unsigned long my_lruvec_page_state_local(struct lruvec *lruvec,
						enum node_stat_item idx)
{
	struct mem_cgroup_per_node *pn;
	long x = 0;

	if (mem_cgroup_disabled())
		return my_node_page_state(lruvec_pgdat(lruvec), idx);

	pn = container_of(lruvec, struct mem_cgroup_per_node, lruvec);
	x = READ_ONCE(pn->lruvec_stats.state_local[idx]);
#ifdef CONFIG_SMP
	if (x < 0)
		x = 0;
#endif
	return x;
}

static struct lruvec *my_mem_cgroup_lruvec(struct mem_cgroup *memcg,
						struct pglist_data *pgdat)
{
	struct mem_cgroup_per_node *mz;
	struct lruvec *lruvec;

	if (mem_cgroup_disabled()) {
		lruvec = &pgdat->__lruvec;
		goto out;
	}

	if (!memcg)
		memcg = *my_root_mem_cgroup;

	mz = mem_cgroup_nodeinfo(memcg, pgdat->node_id);
	lruvec = &mz->lruvec;
out:
	/*
	 * Since a node can be onlined after the mem_cgroup was created,
	 * we have to be prepared to initialize lruvec->pgdat here;
	 * and if offlined then reonlined, we need to reinitialize it.
	 */
	if (unlikely(lruvec->pgdat != pgdat))
		lruvec->pgdat = pgdat;
	return lruvec;
}

static inline struct lruvec *my_folio_lruvec(struct folio *folio)
{
	struct mem_cgroup *memcg = folio_memcg(folio);

	VM_WARN_ON_ONCE_FOLIO(!memcg && !mem_cgroup_disabled(), folio);
	return my_mem_cgroup_lruvec(memcg, folio_pgdat(folio));
}

static inline void my_mem_cgroup_uncharge(struct folio *folio)
{
	if (mem_cgroup_disabled())
		return;
	my___mem_cgroup_uncharge(folio);
}

static inline void my_mem_cgroup_uncharge_list(struct list_head *page_list)
{
	if (mem_cgroup_disabled())
		return;
	my___mem_cgroup_uncharge_list(page_list);
}

static void my__update_lru_size(struct lruvec *lruvec,
				enum lru_list lru, enum zone_type zid,
				int nr_pages)
{
	struct pglist_data *pgdat = lruvec_pgdat(lruvec);

	my___mod_lruvec_state(lruvec, NR_LRU_BASE + lru, nr_pages);
	__mod_zone_page_state(&pgdat->node_zones[zid],
				NR_ZONE_LRU_BASE + lru, nr_pages);
}

#define LRU_SLAB			(NR_LRU_LISTS + 1)
#define SHRINKER_REGISTERING		(((struct shrinker *)~0UL))

static inline void reclaim_coldpgs_update_stats(struct mem_cgroup *memcg,
						unsigned int index,
						unsigned long size)
{
	if (index >= RECLAIM_COLDPGS_STAT_MAX)
		return;

	__this_cpu_add(memcg->coldpgs_stats->counts[index], size);
}

static inline bool reclaim_coldpgs_has_mode(
			struct reclaim_coldpgs_filter *filter,
			unsigned int mode)
{
	unsigned int flag = (1 << mode);

	return !!(filter->mode & flag);
}

static inline bool reclaim_coldpgs_has_flag(
			struct reclaim_coldpgs_filter *filter,
			unsigned int flag)
{
	return !!(filter->flags & flag);
}

static bool folio_is_exec(struct address_space *mapping,
			  struct folio *folio)
{
	struct vm_area_struct *vma;
	pgoff_t pgoff;

	/*
	 * The folio lock not only makes sure that folio->mapping cannot
	 * suddenly be NULLified by truncation, it makes sure that the
	 * structure at mapping cannot be freed and reused yet,
	 * so we can safely take mapping->i_mmap_rwsem.
	 */
	VM_BUG_ON_FOLIO(!folio_test_locked(folio), folio);
	if (!folio_mapped(folio))
		return false;

	/*
	 * We don't check if the address in vma again like page_vma_mapped_walk.
	 * since we don't unmap for the page
	 */
	pgoff = folio_pgoff(folio);
	i_mmap_lock_read(mapping);

	vma = my_vma_interval_tree_iter_first(&mapping->i_mmap, pgoff, pgoff);
	while (vma) {
		if (vma->vm_flags & VM_EXEC) {
			i_mmap_unlock_read(mapping);
			return true;
		}

		vma = my_vma_interval_tree_iter_next(vma, pgoff, pgoff);
	}

	i_mmap_unlock_read(mapping);

	return false;
}

static bool anon_folio_is_exec(struct folio *folio)
{
	struct vm_area_struct *vma;
	struct anon_vma *av;
	struct anon_vma_chain *vmac;
	pgoff_t pgoff_start, pgoff_end;
	bool ret = false;

	if (unlikely(!folio_test_anon(folio) ||
		     !folio_test_swapbacked(folio)))
		return false;

	av = my_folio_lock_anon_vma_read(folio, NULL);
	if (av == NULL)
		return false;

	pgoff_start = folio_pgoff(folio);
	pgoff_end = pgoff_start + folio_nr_pages(folio) - 1;
	my_anon_vma_interval_tree_foreach(vmac,
					  &av->rb_root,
					   pgoff_start,
					   pgoff_end) {
		vma = vmac->vma;
		/*
		 * Once we get a vma in which this folio is mapping
		 * with VM_EXEC flag, we regard the whole folio as
		 * executable.
		 */
		if (vma->vm_flags & VM_EXEC) {
			ret = true;
			break;
		}
	}
	anon_vma_unlock_read(av);

	return ret;
}

static inline bool reclaim_coldpgs_may_not_swap(struct mem_cgroup *memcg)
{
	struct mem_cgroup *m;
	unsigned long max;
	unsigned long swapped;
	bool ret = false;

	if (!css_tryget_online(&memcg->css))
		return ret;

	/* Do not care v2, it will be limited at add_to_swap() */
	for (m = memcg; m != *my_root_mem_cgroup;
	     m = parent_mem_cgroup(m)) {
		max = READ_ONCE(m->reclaim_coldpgs_max);
		if (max == PAGE_COUNTER_MAX)
			continue;
		if (max == 0) {
			ret = true;
			break;
		}
		swapped = my_memcg_page_state(m, MEMCG_SWAP) / PAGE_SIZE;
		if (swapped < max)
			continue;

		ret = true;
		break;
	}

	css_put(&memcg->css);

	return ret;
}

/*
 * The function is called for twice to one specific folio, isolation and
 * reclaiming phrase separately. During the period of isolation, the folio's
 * age should be checked, but that's needn't validated again in reclaiming
 * phrase. @validate_age is used to distinguish the cases.
 */
static inline bool folio_is_reclaimable(struct mem_cgroup *memcg,
			struct reclaim_coldpgs_filter *filter,
			pg_data_t *pgdat, struct folio *folio,
			bool validate_age)
{
	struct address_space *mapping;
	struct mem_cgroup *m = folio_memcg(folio);
	int age;

	if (m != memcg)
		return false;

	/*
	 * The folio should be in LRU list in isolation phrase, but
	 * it should have been removed from LRU list in reclaim
	 * phrase.
	 *
	 * validate_age is indicating isolation phase or reclaim phase.
	 * Though even ignore_age bit is true, it still has to do this
	 * check in isolation phase.
	 */
	if (validate_age && !folio_test_lru(folio))
		return false;

	if (folio_is_file_lru(folio)) {
		mapping = folio_mapping(folio);

		/* Bail if we're not allowed to reclaim */
		if (!reclaim_coldpgs_has_mode(filter, RECLAIM_MODE_PGCACHE_OUT))
			return false;

		/* Bail if the folio isn't clean page cache */
		if (folio_test_dirty(folio) ||
		    folio_test_writeback(folio))
			return false;

		/*
		 * The lazy free'd anonymous pages can be put to the inactive
		 * file LRU. Those pages don't have valid address space and
		 * should be marked as anonymous pages. For the page cache,
		 * it should have valid address space, but we bail if the
		 * address space isn't a evictable one.
		 */
		if (!mapping) {
			if (!folio_test_anon(folio))
				return false;
		} else {
			if (folio->mapping != mapping ||
			    mapping_unevictable(mapping))
				return false;
		}

		/*
		 * Bail if the pagecache has execution mode only when
		 * we needn't to validate the page's age.
		 */
		if (!validate_age &&
		    mapping       &&
		    folio_is_exec(mapping, folio))
			return false;
	} else {
		/* Bail if we're not allowed to reclaim */
		if (!reclaim_coldpgs_has_mode(filter, RECLAIM_MODE_ANON_OUT))
			return false;

		/* Bail if the anonymous page isn't backed by swap */
		if (!folio_test_swapbacked(folio))
			return false;

		/* Bail if the anonymous page is being written back */
		if (folio_test_writeback(folio))
			return false;

		/* Bail if there is no enough swap space */
		if (my_mem_cgroup_get_nr_swap_pages(memcg) <
		    folio_nr_pages(folio))
			return false;

		if (reclaim_coldpgs_may_not_swap(memcg))
			return false;

		/* JIT may use executable anonymous page */
		if (reclaim_coldpgs_has_flag(filter, FLAG_IGNORE_AGE) &&
		    anon_folio_is_exec(folio))
			return false;
	}

	/*
	 * Bail on mlock'ed or unevictable page if we're not
	 * allowed to do so.
	 */
	if (folio_test_unevictable(folio)) {
		if (!(filter->flags & FLAG_IGNORE_MLOCK))
			return false;
		else if (!folio_test_mlocked(folio))
			return false;
	}

	/*
	 * We need to validate the page's age if @threshold is bigger
	 * than 0. Otherwise, we're rechecking if the page is eligible
	 * for reclaim and no need to validate the page's age under the
	 * circumstance.
	 */
	if (validate_age &&
	    !reclaim_coldpgs_has_flag(filter, FLAG_IGNORE_AGE)) {
		age = kidled_get_folio_age(pgdat, folio_pfn(folio));
		if (age < filter->threshold)
			return false;
	}

	return true;
}

static unsigned long isolate_coldpgs_from_lru(struct mem_cgroup *memcg,
				struct reclaim_coldpgs_filter *filter,
				pg_data_t *pgdat, struct lruvec *lruvec,
				enum lru_list lru, unsigned long nr_to_reclaim,
				struct list_head *dst)
{
	struct list_head *src = &lruvec->lists[lru];
	struct folio *folio;
	unsigned long nr_pages, nr_taken = 0;
	unsigned long scan, size = my_lruvec_page_state_local(lruvec, NR_LRU_BASE + lru);
	unsigned long nr_zone_taken[MAX_NR_ZONES] = { 0, };
	int zid, batch = 0;

	spin_lock_irq(&lruvec->lru_lock);

	for (scan = 0;
	     !list_empty(src) && scan < size && nr_taken < nr_to_reclaim;
	     scan++) {
		folio = lru_to_folio(src);
		VM_BUG_ON_FOLIO(!folio_test_lru(folio), folio);

		/*
		 * The folios in the LRU list are visited in reverse order.
		 * During the iteration, the folios that aren't eligible for
		 * reclaim are moved to the list head, so that they can be
		 * skipped safely. The eligible folios are moved to separate
		 * (local) list.
		 */
		if (!folio_is_reclaimable(memcg, filter, pgdat, folio, true) ||
		    !folio_try_get(folio)) {
			list_move(&folio->lru, src);
			goto isolate_fail;
		}

		if (folio_test_clear_lru(folio)) {
			nr_pages = folio_nr_pages(folio);
			nr_zone_taken[folio_zonenum(folio)] += nr_pages;
			nr_taken += nr_pages;
			list_move(&folio->lru, dst);
		} else {
			/*
			 * This folio may in other isolation path,
			 * but we still hold lru_lock.
			 */
			folio_put(folio);
			list_move(&folio->lru, src);
		}

isolate_fail:
		/*
		 * To schedule out a moment when reaching filter->batch. This
		 * scheme mainly to avoid hold lru_lock long time if a huge
		 * nr_to_reclaim here.
		 *
		 * This mechanism can be disabled when zero limit is provided.
		 */
		if (filter->batch && ++batch >= filter->batch) {
			spin_unlock_irq(&lruvec->lru_lock);
			cond_resched();

			spin_lock_irq(&lruvec->lru_lock);
			batch = 0;
		}
	}

	for (zid = 0; zid < MAX_NR_ZONES; zid++) {
		if (!nr_zone_taken[zid])
			continue;

		my__update_lru_size(lruvec, lru, zid, -nr_zone_taken[zid]);
		my_mem_cgroup_update_lru_size(lruvec, lru,
				zid, -nr_zone_taken[zid]);
	}

	spin_unlock_irq(&lruvec->lru_lock);

	return nr_taken;
}

static int swapout_folio_to_zram(struct reclaim_coldpgs_filter *filter,
				 struct folio *folio,
				 int age)
{
	VM_BUG_ON(!folio_test_locked(folio));

	/* Bail if zswap isn't preferred or the page isn't cold enough */
	if (!reclaim_coldpgs_has_flag(filter, FLAG_IGNORE_AGE) &&
	    (!filter->thresholds[THRESHOLD_NONROT] ||
	    age > filter->thresholds[THRESHOLD_NONROT]))
		return -ERANGE;

	if (my_zswap_store(folio)) {
		folio_start_writeback(folio);
		folio_unlock(folio);
		folio_end_writeback(folio);
		return 0;
	}

	return -EAGAIN;
}

static inline int my_arch_prepare_to_swap(struct page *page)
{
#ifdef CONFIG_ARM64
	/* TODO: Only support small page now, support large folio later */
	if (system_supports_mte())
		return my_mte_save_tags(page);
	return 0;
#endif
	return arch_prepare_to_swap(page);
}

static int swapout_folio(struct reclaim_coldpgs_filter *filter,
			 struct folio *folio, int age,
			 struct writeback_control *wbc,
			 bool *use_zswap)
{
	int ret;

	if (my_folio_free_swap(folio)) {
		folio_unlock(folio);
		return 0;
	}

	/*
	 * Arch code may have to preserve more data than just the page
	 * contents, e.g. memory tags.
	 */
	ret = my_arch_prepare_to_swap(&folio->page);
	if (ret) {
		folio_mark_dirty(folio);
		folio_unlock(folio);
		return ret;
	}

	if (!swapout_folio_to_zram(filter, folio, age)) {
		if (use_zswap)
			*use_zswap = true;

		return 0;
	}

	my___swap_writepage(&folio->page, wbc);

	return 0;
}

enum {
	PAGE_KEEP,	/* failed to write page out, page is locked */
	PAGE_ACTIVATE,	/* move page to the active list, page is locked */
	PAGE_SUCCESS,	/* page was sent to disk, page is unlocked */
	PAGE_CLEAN,	/* page is clean and locked */
};

#ifndef is_page_cache_freeable
static inline int is_page_cache_freeable(struct folio *folio)
{
	/*
	 * A freeable page cache folio is referenced only by the caller
	 * that isolated the folio, the page cache and optional filesystem
	 * private data at folio->private.
	 */
	return folio_ref_count(folio) - folio_test_private(folio) ==
		1 + folio_nr_pages(folio);
}
#endif

static int pageout(struct mem_cgroup *memcg,
		   struct reclaim_coldpgs_filter *filter,
		   struct address_space *mapping,
		   struct folio *folio, int *nr_pages_ptr,
		   int age)
{
	struct writeback_control wbc = {
		.sync_mode = WB_SYNC_NONE,
		.nr_to_write = SWAP_CLUSTER_MAX,
		.range_start = 0,
		.range_end = LLONG_MAX,
		.for_reclaim = 1,
	};
	bool use_zswap = false;
	int nr_pages = folio_nr_pages(folio);
	int ret;

	if (!is_page_cache_freeable(folio))
		return PAGE_KEEP;

	/*
	 * If the page is dirty, only perform writeback if that write
	 * will be non-blocking.  To prevent this allocation from being
	 * stalled by pagecache activity.  But note that there may be
	 * stalls if we need to run get_block().  We could test
	 * PagePrivate for that.
	 *
	 * If this process is currently in __generic_file_write_iter() against
	 * this page's queue, we can perform writeback even if that
	 * will block.
	 *
	 * If the page is swapcache, write it back even if that would
	 * block, for some throttling. This happens by accident, because
	 * swap_backing_dev_info is bust: it doesn't reflect the
	 * congestion state of the swapdevs.  Easy to fix, if needed.
	 *
	 * Some data journaling orphaned pages can have NULL address
	 * space, but it's obvious out of range to the anonymous pages.
	 * However, it's not harmful to check because it's useful when
	 * we start to reclaim dirty pagecache in future.
	 */
	if (!mapping) {
		if (folio_test_private(folio)) {
			if (try_to_free_buffers(folio)) {
				folio_clear_dirty(folio);
				pr_info("%s: orphaned folio\n", __func__);
				return PAGE_CLEAN;
			}
		}

		return PAGE_KEEP;
	}

	/*
	 * The anonymous pages that are mapped in shared mode is put to the
	 * anonymous lists, but associated with a special file from shmemfs
	 * or ramfs. For these shared anonymous pages, we didn't allocate
	 * slots in the swap pagecache and the writepage() of the address
	 * space does so.
	 */
	if (folio_clear_dirty_for_io(folio)) {
		folio_set_reclaim(folio);
		if (folio_test_anon(folio))
			ret = swapout_folio(filter, folio, age,
					    &wbc, &use_zswap);
		else
			ret = mapping->a_ops->writepage(&folio->page, &wbc);

		if (ret < 0) {
			folio_lock(folio);
			if (folio_mapping(folio) == mapping)
				mapping_set_error(mapping, ret);
			folio_unlock(folio);
		}

		if (ret == AOP_WRITEPAGE_ACTIVATE) {
			folio_clear_reclaim(folio);
			return PAGE_ACTIVATE;
		}

		if (!folio_test_writeback(folio)) {
			/* synchronous write or broken a_ops? */
			folio_clear_reclaim(folio);
		}

		/* Update statistics */
		if (nr_pages_ptr)
			*nr_pages_ptr = nr_pages;

		if (use_zswap)
			reclaim_coldpgs_update_stats(memcg,
				RECLAIM_COLDPGS_STAT_ANON_OUT_ZSWAP,
				nr_pages << PAGE_SHIFT);
		else
			reclaim_coldpgs_update_stats(memcg,
				RECLAIM_COLDPGS_STAT_ANON_OUT_SWAP,
				nr_pages << PAGE_SHIFT);

		return PAGE_SUCCESS;
	}

	return PAGE_CLEAN;
}

static void my_lruvec_add_folio(struct lruvec *lruvec, struct folio *folio)
{
	enum lru_list lru = folio_lru_list(folio);
	int nr_pages = folio_nr_pages(folio);
	int zid = folio_zonenum(folio);

	my__update_lru_size(lruvec, lru, zid, nr_pages);
	my_mem_cgroup_update_lru_size(lruvec, lru, zid, nr_pages);
	if (lru != LRU_UNEVICTABLE)
		list_add(&folio->lru, &lruvec->lists[lru]);
}

static __maybe_unused void my_lruvec_del_folio(struct lruvec *lruvec, struct folio *folio)
{
	enum lru_list lru = folio_lru_list(folio);
	int nr_pages = folio_nr_pages(folio);
	int zid = folio_zonenum(folio);

	if (lru != LRU_UNEVICTABLE)
		list_del(&folio->lru);
	my__update_lru_size(lruvec, lru, zid, -nr_pages);
	my_mem_cgroup_update_lru_size(lruvec, lru, zid, -nr_pages);
}

static void my_putback_inactive_folios(struct lruvec *lruvec, struct list_head *folios_list)
{
	LIST_HEAD(folios_to_free);

	/*
	 * Put back any unfreeable folio.
	 */
	while (!list_empty(folios_list)) {
		struct folio *folio = lru_to_folio(folios_list);

		VM_BUG_ON_FOLIO(folio_test_active(folio) &&
				folio_test_unevictable(folio), folio);
		VM_BUG_ON_FOLIO(folio_test_lru(folio), folio);
		list_del(&folio->lru);
		if (unlikely(!folio_evictable(folio))) {
			spin_unlock_irq(&lruvec->lru_lock);
			my_folio_putback_lru(folio);
			spin_lock_irq(&lruvec->lru_lock);
			continue;
		}

		folio_set_lru(folio);

		if (folio_put_testzero(folio)) {
			__folio_clear_lru_flags(folio);

			if (unlikely(folio_test_large(folio))) {
				spin_unlock_irq(&lruvec->lru_lock);
				my_destroy_large_folio(folio);
				spin_lock_irq(&lruvec->lru_lock);
			} else
				list_add(&folio->lru, &folios_to_free);
		} else {
			my_lruvec_add_folio(lruvec, folio);
			if (folio_test_active(folio))
				my_workingset_age_nonresident(lruvec, folio_nr_pages(folio));
		}
	}

	/*
	 * To save our caller's stack, now use input list for folios to free.
	 */
	list_splice(&folios_to_free, folios_list);
}

static unsigned long reclaim_coldpgs_from_list(struct mem_cgroup *memcg,
				struct reclaim_coldpgs_filter *filter,
				pg_data_t *pgdat, struct lruvec *lruvec,
				enum lru_list lru, struct list_head *list)
{
	struct folio *folio;
	struct address_space *mapping;
	enum ttu_flags flags = (filter->flags & FLAG_IGNORE_MLOCK) &&
		(lru == LRU_UNEVICTABLE) ?
		(TTU_BATCH_FLUSH | TTU_IGNORE_MLOCK) : TTU_BATCH_FLUSH;
	LIST_HEAD(keep_folios); LIST_HEAD(free_folios);
	unsigned long nr_reclaimed = 0;
	unsigned long nr_pagecache_dropped = 0;
	bool is_pagecache;
	/*
	 * age must be initialized in case ignore_age bit is set, as it
	 * is going to be used by pageout().
	 */
	int age = 0, nr_pages, batch, ret;

	while (!list_empty(list)) {
		cond_resched();

		folio = lru_to_folio(list);
		list_del(&folio->lru);
		if (!folio_trylock(folio))
			goto keep;

		VM_BUG_ON_FOLIO(folio_test_active(folio), folio);

		nr_pages = folio_nr_pages(folio);
		is_pagecache = folio_is_file_lru(folio) ? true : false;
		mapping = folio_mapping(folio);
		if (!reclaim_coldpgs_has_flag(filter, FLAG_IGNORE_AGE)) {
			age = kidled_get_folio_age(pgdat, folio_pfn(folio));
			if (age < 0)
				goto keep_unlocked;
		}

		if (!folio_is_reclaimable(memcg, filter, pgdat, folio, false))
			goto keep_unlocked;

		if (folio_test_anon(folio) && folio_test_swapbacked(folio)) {
			if (!folio_test_swapcache(folio)) {
				if (folio_test_large(folio)) {
					if (!my_can_split_folio(folio, NULL))
						goto keep_unlocked;

					if (!folio_entire_mapcount(folio) &&
					    my_split_folio_to_list(folio, list))
						goto keep_unlocked;
				}

				if (!my_add_to_swap(folio)) {
					if (!folio_test_large(folio))
						goto keep_unlocked;

					if (my_split_folio_to_list(folio,
								   list))
						goto keep_unlocked;

					if (!my_add_to_swap(folio))
						goto keep_unlocked;
				}

				/* Update address space */
				mapping = folio_mapping(folio);
			}
		} else if (folio_test_large(folio)) {
			if (my_split_folio_to_list(folio, list))
				goto keep_unlocked;
		}

		/*
		 * The large folio might have been split up, we need
		 * update @nr_pages accordingly.
		 */
		if ((nr_pages > 1) && !folio_test_large(folio))
			nr_pages = 1;

		if (folio_mapped(folio)) {
			if (folio_test_pmd_mappable(folio))
				my_try_to_unmap(folio,
						(flags | TTU_SPLIT_HUGE_PMD));
			else
				my_try_to_unmap(folio, flags);

			if (folio_mapped(folio))
				goto keep_unlocked;
		}

		if (folio_test_dirty(folio)) {
			my_try_to_unmap_flush_dirty();
			ret = pageout(memcg, filter, mapping, folio,
				      &nr_pages, age);
			switch (ret) {
			case PAGE_KEEP:
				goto keep_unlocked;
			case PAGE_ACTIVATE:
				goto activate_unlocked;
			case PAGE_SUCCESS:
				/* Wait until the writeback is completed */
				folio_wait_writeback(folio);

				/*
				 * A synchronous write - probably a ramdisk.
				 * Go ahead and try to reclaim the page.
				 */
				if (!folio_trylock(folio))
					goto keep;

				if (folio_test_dirty(folio) ||
				    folio_test_writeback(folio))
					goto keep_unlocked;
				mapping = folio_mapping(folio);
			case PAGE_CLEAN:
				;
			}
		}

		if (folio_needs_release(folio)) {
			if (!filemap_release_folio(folio, 0))
				goto activate_unlocked;

			if (!mapping && folio_ref_count(folio) == 1) {
				folio_unlock(folio);
				if (folio_put_testzero(folio)) {
					__folio_clear_unevictable(folio);
					goto free_it;
				} else {
					nr_reclaimed += nr_pages;
					continue;
				}
			}
		}

		if (folio_test_anon(folio) && !folio_test_swapbacked(folio)) {
			/* follow __remove_mapping for reference */
			if (!folio_ref_freeze(folio, 1))
				goto keep_unlocked;

			if (folio_test_dirty(folio)) {
				folio_set_count(folio, 1);
				goto keep_unlocked;
			}
		} else if (!mapping ||
			   !my___remove_mapping(mapping, folio, true, memcg)) {
			goto keep_unlocked;
		}

		/*
		 * There shouldn't be anyone referring the folio. It's safe
		 * to clear the unevictable flag, to avoid complaints spew
		 * out on freeing the folio.
		 */
		folio_unlock(folio);
		__folio_clear_unevictable(folio);
		__folio_clear_active(folio);

free_it:
		if (unlikely(folio_test_large(folio))) {
			my_mem_cgroup_uncharge(folio);
			my_destroy_large_folio(folio);
		} else
			list_add(&folio->lru, &free_folios);

		nr_reclaimed += nr_pages;
		kidled_mem_cgroup_move_stats(memcg, NULL, folio, nr_pages);
		if (is_pagecache)
			nr_pagecache_dropped += nr_pages;

		continue;

activate_unlocked:
		/* Not a candidate for swapping, so reclaim swap space. */
		if (folio_test_swapcache(folio) &&
		    (my_mem_cgroup_swap_full(folio) || folio_test_mlocked(folio)))
			my_folio_free_swap(folio);
		if (!folio_test_mlocked(folio))
			folio_set_active(folio);
keep_unlocked:
		folio_unlock(folio);
keep:
		list_add(&folio->lru, &keep_folios);
	}

	/* Update page cache reclaim statistics */
	reclaim_coldpgs_update_stats(memcg,
		RECLAIM_COLDPGS_STAT_PCACHE_OUT_DROP,
		nr_pagecache_dropped << PAGE_SHIFT);

	/* Free folios that are eligible for releasing */
	my_mem_cgroup_uncharge_list(&free_folios);
	my_try_to_unmap_flush();

	if (filter->batch) {
		/* Free folios in batch */
		LIST_HEAD(batch_free_folios);

		batch = 0;

		while (!list_empty(&free_folios)) {
			folio = lru_to_folio(&free_folios);
			list_move(&folio->lru, &batch_free_folios);

			if (++batch >= filter->batch) {
				my_free_unref_page_list(&batch_free_folios);

				cond_resched();
				batch = 0;
				INIT_LIST_HEAD(&batch_free_folios);
			}
		}

		/* Don't forget the remaining pages */
		if (!list_empty(&batch_free_folios))
			my_free_unref_page_list(&batch_free_folios);
	} else {
		/* Free pages in one shot */
		my_free_unref_page_list(&free_folios);
	}

	/* Put all pages back to the list */
	list_splice(&keep_folios, list);

	/*
	 * The folioss that can't be released will be chained up to the
	 * corresponding LRU list. The system might not survive if the
	 * node's LRU lock is taken with interrupt disabled for long
	 * time, so we release the folios in batch mode.
	 */
	batch = 0;
	INIT_LIST_HEAD(&free_folios);
	while (!list_empty(list)) {
		folio = lru_to_folio(list);
		list_move(&folio->lru, &free_folios);

		if (filter->batch && ++batch >= filter->batch) {
			spin_lock_irq(&lruvec->lru_lock);
			my_putback_inactive_folios(lruvec, &free_folios);
			spin_unlock_irq(&lruvec->lru_lock);

			my_mem_cgroup_uncharge_list(&free_folios);
			my_try_to_unmap_flush();
			my_free_unref_page_list(&free_folios);

			cond_resched();
			batch = 0;
			INIT_LIST_HEAD(&free_folios);
		}
	}

	/* Release the remaining pages */
	if (!list_empty(&free_folios)) {
		spin_lock_irq(&lruvec->lru_lock);
		my_putback_inactive_folios(lruvec, &free_folios);
		spin_unlock_irq(&lruvec->lru_lock);

		my_mem_cgroup_uncharge_list(&free_folios);
		my_try_to_unmap_flush();
		my_free_unref_page_list(&free_folios);
	}

	return nr_reclaimed;
}

static unsigned long reclaim_coldpgs_from_lru(struct mem_cgroup *memcg,
				struct reclaim_coldpgs_filter *filter,
				pg_data_t *pgdat, struct lruvec *lruvec,
				enum lru_list lru, unsigned long nr_to_reclaim)
{
	unsigned long nr_isolated;
	LIST_HEAD(list);

	my_lru_add_drain();

	nr_isolated = isolate_coldpgs_from_lru(memcg, filter, pgdat,
				lruvec, lru, nr_to_reclaim, &list);
	if (!nr_isolated)
		return 0;

	return reclaim_coldpgs_from_list(memcg, filter, pgdat, lruvec, lru,
					 &list);
}

#define SHRINK_BATCH 128

static unsigned long reclaim_coldslab_from_shrinker(struct shrinker *shrinker,
						    struct shrink_control *sc,
						    unsigned long nr_to_reclaim)
{
	unsigned long batch_size = shrinker->batch ?: SHRINK_BATCH;
	unsigned long freeable;
	unsigned long nr_reclaimed = 0;

	if (!shrinker->reap_objects)
		return SHRINK_STOP;

	freeable = shrinker->count_objects(shrinker, sc);
	if (freeable == 0 || freeable == SHRINK_EMPTY)
		return nr_reclaimed;

	while (freeable > 0) {
		unsigned long ret;
		unsigned long nr_scanned = min(freeable, batch_size);

		sc->nr_to_scan = nr_scanned;
		ret =  shrinker->reap_objects(shrinker, sc);
		if (ret == SHRINK_STOP)
			break;
		nr_reclaimed += ret;
		if (nr_reclaimed >= nr_to_reclaim)
			break;
		freeable -= nr_scanned;
		cond_resched();
	}

	return nr_reclaimed;
}

static unsigned long
reclaim_coldslab_from_memcg_lru(struct shrink_control *sc,
				unsigned long nr_to_reclaim)
{
	unsigned long nr_reclaimed = 0;
	struct shrinker_info *info;
	struct mem_cgroup *memcg = sc->memcg;
	int i, index = 0;

	if (!mem_cgroup_online(memcg))
		return nr_reclaimed;

again:
	rcu_read_lock();
	info = rcu_dereference(memcg->nodeinfo[sc->nid]->shrinker_info);

	if (unlikely(!info))
		goto out_unlock;

	if (index < (info->map_nr_max / SHRINKER_UNIT_BITS)) {
		struct shrinker_info_unit *unit;

		unit = info->unit[index];

		rcu_read_unlock();
		for_each_set_bit(i, unit->map, SHRINKER_UNIT_BITS) {
			struct shrinker *shrinker;
			unsigned long ret;
			int shrinker_id = index * SHRINKER_UNIT_BITS + i;

			rcu_read_lock();
			shrinker = idr_find(my_shrinker_idr, shrinker_id);
			if (unlikely(!shrinker || !shrinker_try_get(shrinker))) {
				clear_bit(i, unit->map);
				rcu_read_unlock();
				continue;
			}
			rcu_read_unlock();

			ret = reclaim_coldslab_from_shrinker(shrinker, sc,
							     nr_to_reclaim);
			shrinker_put(shrinker);
			if (ret == SHRINK_STOP)
				continue;

			nr_reclaimed += ret;
			if (nr_reclaimed >= nr_to_reclaim)
				goto out;
		}

		index++;
		goto again;
	}

out_unlock:
	rcu_read_unlock();
out:
	return nr_reclaimed;
}

static unsigned long reclaim_coldslab_from_lru(struct mem_cgroup *memcg,
					       int node, unsigned int threshold,
					       unsigned long nr_to_reclaim)
{
	struct shrinker *shrinker;
	unsigned long nr_reclaimed = 0;
	struct shrink_control sc = {
		.gfp_mask = GFP_KERNEL,
		.nid = node,
		.memcg = memcg,
		.threshold = threshold,
	};

	if (!mem_cgroup_disabled() && memcg != *my_root_mem_cgroup)
		return reclaim_coldslab_from_memcg_lru(&sc, nr_to_reclaim);

	rcu_read_lock();
	list_for_each_entry_rcu(shrinker, my_shrinker_list, list) {
		unsigned long ret;

		if (!shrinker_try_get(shrinker))
			continue;

		rcu_read_unlock();

		ret = reclaim_coldslab_from_shrinker(shrinker, &sc,
						     nr_to_reclaim);
		if (ret == SHRINK_STOP) {
			rcu_read_lock();
			shrinker_put(shrinker);
			continue;
		}
		nr_reclaimed += ret;
		if (nr_reclaimed >= nr_to_reclaim) {
			shrinker_put(shrinker);
			goto out;
		}
	}
	rcu_read_unlock();
out:
	return nr_reclaimed;
}

static void reclaim_coldpgs_from_memcg(struct mem_cgroup *memcg,
				       struct reclaim_coldpgs_filter *memcg_orig)
{
	pg_data_t *pgdat;
	struct lruvec *lruvec;
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	struct reclaim_coldpgs_filter memcg_filter;
	struct reclaim_coldpgs_filter *filter = &memcg_filter;
	unsigned long bitmap;
	unsigned long nr_reclaimed = 0;
	int nid, lru;

	memcpy(filter, memcg_orig, sizeof(struct reclaim_coldpgs_filter));
	/*
	 * Filter out the useless mode and flags.
	 */
	filter->mode &= FLAG_MODE(control->flags);
	/*
	 * TODO: mlocked page is not on any LRU in 6.6, in case coldpgs
	 * scans empty LRU_UNEVICTABLE and gets incorrent ptr, mask the flag
	 * temporarily, support it in future.
	 */
	filter->flags &= FLAG_CTRL(control->flags);
	if (filter->flags & FLAG_IGNORE_MLOCK) {
		pr_warn_once("Coldpgs does't support mlock page reclaim, ignore the flag\n");
		filter->flags &= ~FLAG_IGNORE_MLOCK;
	}

	if (reclaim_coldpgs_has_flag(filter, FLAG_IGNORE_AGE))
		pr_debug("Ignoring age to reclaim unconditionally\n");

	/*
	 * Figure out the eligible LRUs. Here we have a bitmap to track the
	 * eligible LRUs, to have thing a bit easier. Note that the pages
	 * resident in the unevictable list can't be reclaimed until the
	 * ignored mlock flag is globablly set.
	 */
	bitmap_zero(&bitmap, BITS_PER_LONG);
	if (reclaim_coldpgs_has_mode(filter, RECLAIM_MODE_PGCACHE_OUT)) {
		bitmap_set(&bitmap, LRU_INACTIVE_FILE, 1);
		bitmap_set(&bitmap, LRU_ACTIVE_FILE, 1);
	}

	/*
	 * When no available swap space, the swapout won't be issued.
	 */
	if (reclaim_coldpgs_has_mode(filter, RECLAIM_MODE_ANON_OUT) &&
	    my_mem_cgroup_get_nr_swap_pages(memcg) > 0) {
		bitmap_set(&bitmap, LRU_INACTIVE_ANON, 1);
		bitmap_set(&bitmap, LRU_ACTIVE_ANON, 1);
	}

	/*
	 * It's pointless to scan the child memcg when memcg_kmem is disabled.
	 */
	if (reclaim_coldpgs_has_mode(filter, RECLAIM_MODE_SLAB)) {
		if (memcg_kmem_online())
			bitmap_set(&bitmap, LRU_SLAB, 1);
		else if (memcg == *my_root_mem_cgroup)
			bitmap_set(&bitmap, LRU_SLAB, 1);
	}

	/*
	 * It's pointless to scan the pages in unevictable LRU list without
	 * reclaiming them. The pages in the unevictable LRU list won't be
	 * iterated until the valid reclaim mode has been given.
	 * FIXME: unevictable page won't be added into LRU_UNEVICTABLE,
	 * page->mlock_count is used instead, scanning LRU_UNEVICTABLE can
	 * cause kernel panic.
	 */
	if (!bitmap_empty(&bitmap, BITS_PER_LONG) &&
	    reclaim_coldpgs_has_flag(filter, FLAG_IGNORE_MLOCK))
		bitmap_set(&bitmap, LRU_UNEVICTABLE, 1);

	/* Reclaim cold memory from LRU list */
	for_each_node_state(nid, N_MEMORY) {
		pgdat = NODE_DATA(nid);
		lruvec = my_mem_cgroup_lruvec(memcg, pgdat);

		for (lru = find_first_bit(&bitmap, BITS_PER_LONG);
		     lru < NR_LRU_LISTS && nr_reclaimed < filter->size;
		     lru = find_next_bit(&bitmap, BITS_PER_LONG, (lru + 1))) {
			unsigned long reclaim, nr_page_reclaimed;

			/*
			 * User specify the size in bytes to break the loop, but
			 * reclaim_coldpgs_from_lru reclaim the memory at the
			 * granularity of a page.
			 */
			reclaim = (filter->size - nr_reclaimed) >> PAGE_SHIFT;
			nr_page_reclaimed = reclaim_coldpgs_from_lru(memcg,
							filter, pgdat, lruvec,
							lru, reclaim);

			if (lru == LRU_UNEVICTABLE)
				reclaim_coldpgs_update_stats(memcg,
					RECLIMA_COLDPGS_STAT_MLOCK_DROP,
					nr_page_reclaimed << PAGE_SHIFT);

			nr_reclaimed += nr_page_reclaimed << PAGE_SHIFT;
		}

		if (test_bit(LRU_SLAB, &bitmap) &&
				nr_reclaimed < filter->size) {
			unsigned long nr_slab_size, nr_to_reclaim;

			/*
			 * The user specified "nr_reclaimed" means it is used
			 * to break the loop rather than the actual numbers
			 * need to free to the system. Because the reclaimed
			 * slab objects maybe are not freed to buddy system,
			 * hence we will reclaim cold slab can be controlled
			 * separately by idlemd tool.
			 */
			nr_to_reclaim = filter->size - nr_reclaimed;
			nr_slab_size = reclaim_coldslab_from_lru(memcg,
							pgdat->node_id,
							filter->threshold,
							nr_to_reclaim);
			nr_reclaimed += nr_slab_size;
			reclaim_coldpgs_update_stats(memcg,
				RECLAIM_COLDPGS_STAT_SLAB_DROP, nr_slab_size);
		}
	}
}

static void reclaim_coldpgs_action(struct mem_cgroup *memcg,
				   unsigned long threshold,
				   unsigned long size)
{
	struct mem_cgroup *m;
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	struct reclaim_coldpgs_filter filter;

	/*
	 * Populate the filter used in reclaiming. The global control could
	 * be modified when the reclaiming is in progress. So we partially
	 * copy over the global control to gurantee the consistency.
	 */
	down_read(&global_control.rwsem);
	filter.flags = global_control.flags;
	filter.batch = global_control.batch;
	filter.mode = global_control.mode;
	filter.threshold = threshold;
	filter.size = size;
	memcpy(filter.thresholds, global_control.thresholds,
	       sizeof(filter.thresholds));
	up_read(&global_control.rwsem);

	if (FLAG_CTRL(control->flags) & FLAG_IGNORE_AGE) {
		/*
		 * For user who wants to use ignore_age mode but do not want
		 * impact on global setting, we make filter to be overwritten
		 * by memcg's setting.
		 */
		pr_debug("Ignoring age mode, overwrite filter flags & mode\n");
		down_read(&control->rwsem);
		filter.flags = FLAG_CTRL(control->flags);
		filter.mode = FLAG_MODE(control->flags);
		up_read(&control->rwsem);
	}

	/*
	 * The memory cgroup might have offlined subordinate memory cgroups,
	 * whose cgroup files have been removed. It means there is no way to
	 * reclaim the cold memory from the offlined memory cgroups through
	 * the cgroup files. So the cold memory of the offlined memory cgroups
	 * is reclaimed. The coldness threshold is inherited from the parent,
	 * but the amount isn't limited.
	 */
	for_each_memcg_tree(memcg, m) {
		if (m != memcg &&
		    (mem_cgroup_online(m) &&
		     !reclaim_coldpgs_has_flag(&filter, FLAG_IGNORE_AGE)))
			continue;

		if (m == memcg) {
			filter.size = size;
			reclaim_coldpgs_from_memcg(m, &filter);
		} else {
			filter.size = 0xFFFFFFFFFF;
			/*
			 * When subordinate memcgs are been reclaimed, use
			 * their parent's reclaim setting is more reasonable.
			 */
			down_read(&control->rwsem);
			m->coldpgs_control.flags = control->flags;
			m->coldpgs_control.threshold = control->threshold;
			up_read(&control->rwsem);
			reclaim_coldpgs_from_memcg(m, &filter);
		}
	}

	/*
	 * Clear the threshold and size, preparing for next round of reclaim.
	 * The fields are untained and no need to be cleared out for the
	 * offlined subordinate memory cgroups.
	 */
	down_write(&control->rwsem);
	control->threshold = 0;
	control->size = 0;
	up_write(&control->rwsem);
}

static int reclaim_coldpgs_read_threshold(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(seq_css(m));
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	unsigned long threshold;

	down_read(&control->rwsem);
	threshold = control->threshold;
	up_read(&control->rwsem);

	seq_printf(m, "%lu\n", threshold);

	return 0;
}

static ssize_t reclaim_coldpgs_write_threshold(struct kernfs_open_file *of,
					       char *buf, size_t count,
					       loff_t off)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(of_css(of));
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	unsigned long threshold, size;
	int ret;

	buf = strstrip(buf);
	ret = kstrtoul(buf, 10, &threshold);
	if (ret || threshold > U8_MAX)
		return -EINVAL;

	down_write(&control->rwsem);
	control->threshold = threshold;
	size = control->size;
	up_write(&control->rwsem);

	if (threshold > 0 && size > 0)
		reclaim_coldpgs_action(memcg, threshold, size);

	return count;
}

static int reclaim_coldpgs_read_size(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(seq_css(m));
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	unsigned long size;

	down_read(&control->rwsem);
	size = control->size;
	up_read(&control->rwsem);

	seq_printf(m, "%lu\n", size);

	return 0;
}

static ssize_t reclaim_coldpgs_write_size(struct kernfs_open_file *of,
					  char *buf, size_t count,
					  loff_t off)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(of_css(of));
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	unsigned long threshold, size, flags;
	int ret;

	buf = strstrip(buf);
	ret = kstrtoul(buf, 10, &size);
	if (ret)
		return -EINVAL;

	down_write(&control->rwsem);
	threshold = control->threshold;
	control->size = size;
	flags = control->flags;
	up_write(&control->rwsem);

	if (size > 0 &&
	    (threshold > 0 || (FLAG_CTRL(flags) & FLAG_IGNORE_AGE)))
		reclaim_coldpgs_action(memcg, threshold, size);

	return count;
}

static int reclaim_coldpgs_read_flags(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(seq_css(m));
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	unsigned long flags;

	down_read(&control->rwsem);
	flags = control->flags;
	up_read(&control->rwsem);

	seq_printf(m, "0x%lx\n", flags);

	return 0;
}

static ssize_t reclaim_coldpgs_write_flags(struct kernfs_open_file *of,
					  char *buf, size_t count,
					  loff_t off)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(of_css(of));
	struct reclaim_coldpgs_control *control = &memcg->coldpgs_control;
	unsigned long flags;
	int ret;

	buf = strstrip(buf);
	ret = kstrtoul(buf, 16, &flags);
	if (ret)
		return -EINVAL;

	down_write(&control->rwsem);
	control->flags = flags;
	up_write(&control->rwsem);

	return count;
}

static int reclaim_coldpgs_read_stats(struct seq_file *m, void *v)
{
	struct mem_cgroup *iter, *memcg = mem_cgroup_from_css(seq_css(m));
	struct reclaim_coldpgs_stats *self, *stats, *total;
	unsigned int hierarchy, cpu, i;
	static char * const coldpgs_stats_desc[] = {
		"pagecache migrate in",
		"pagecache migrate out",
		"pagecache dropped",
		"anon migrate in",
		"anon zswap in",
		"anon swap in",
		"anon migrate out",
		"anon zswap out",
		"anon swap out",
		"slab drop",
		"mlock dropped",
		"mlock refault",
	};

	self = kzalloc(sizeof(*self) * 3, GFP_KERNEL);
	if (!self)
		return -ENOMEM;

	stats = self + 1;
	total = self + 2;
	down_read(&global_control.rwsem);
	hierarchy = global_control.hierarchy;
	up_read(&global_control.rwsem);

	/*
	 * Bail early if hierarchy mode is disabled. The iteration works
	 * perfectly because the root memory cgroup is iterated firstly.
	 */
	for_each_memcg_tree(memcg, iter) {
		if (!hierarchy && iter != memcg) {
			my_mem_cgroup_iter_break(memcg, iter);
			break;
		}

		memset(stats, 0, sizeof(*stats));
		for_each_possible_cpu(cpu) {
			for (i = 0; i < RECLAIM_COLDPGS_STAT_MAX; i++) {
				stats->counts[i] +=
				per_cpu_ptr(iter->coldpgs_stats,
					    cpu)->counts[i];
			}
		}

		/* Save the counter of current memory cgroup */
		if (iter == memcg)
			memcpy(self, stats, sizeof(*stats));

		/*
		 * The current memory cgroup is always accounted, regardless
		 * of the hierarchy mode.
		 */
		for (i = 0; i < RECLAIM_COLDPGS_STAT_MAX; i++)
			total->counts[i] += stats->counts[i];

		/* Avoid taking up CPU too long time. */
		cond_resched();
	}

	for (i = 0; i < RECLAIM_COLDPGS_STAT_MAX; i++) {
		seq_printf(m, "%-32s: %20lu kB\n",
			   coldpgs_stats_desc[i], self->counts[i] >> 10);
	}

	for (i = 0; i < RECLAIM_COLDPGS_STAT_MAX; i++) {
		seq_printf(m, "Total %-26s: %20lu kB\n",
			   coldpgs_stats_desc[i], total->counts[i] >> 10);
	}

	kfree(self);

	return 0;
}

static ssize_t reclaim_coldpgs_write_stats(struct kernfs_open_file *of,
					   char *buf, size_t count,
					   loff_t off)
{
	struct mem_cgroup *iter, *memcg = mem_cgroup_from_css(of_css(of));
	unsigned int hierarchy;
	unsigned long val;
	int cpu, i, ret;

	down_read(&global_control.rwsem);
	hierarchy = global_control.hierarchy;
	up_read(&global_control.rwsem);

	/* Only zero is accepted */
	buf = strstrip(buf);
	ret = kstrtoul(buf, 0, &val);
	if (ret || val)
		return -EINVAL;

	for_each_memcg_tree(memcg, iter) {
		if (!hierarchy && iter != memcg) {
			my_mem_cgroup_iter_break(memcg, iter);
			break;
		}

		for_each_possible_cpu(cpu) {
			for (i = 0; i < RECLAIM_COLDPGS_STAT_MAX; i++) {
				per_cpu_ptr(iter->coldpgs_stats,
					    cpu)->counts[i] = 0;
			}
		}
	}

	return count;
}

static int reclaim_coldpgs_read_swapin(struct seq_file *m, void *v)
{
	const char *magic = "swapin";

	seq_printf(m, "%s\n", magic);

	return 0;
}

static inline bool rcp_pgd_bad(pgd_t pgd)
{
#if CONFIG_PGTABLE_LEVELS > 4
	unsigned long ignore_flags = _PAGE_USER;

#ifdef CONFIG_X86_5LEVEL
	if (!my___pgtable_l5_enabled)
		return false;
#endif

	if (IS_ENABLED(CONFIG_PAGE_TABLE_ISOLATION))
		ignore_flags |= _PAGE_NX;

	return ((pgd_flags(pgd) & ~ignore_flags) != _KERNPG_TABLE);

#else
	return false;
#endif /* CONFIG_PGTABLE_LEVELS > 4 */
}

static inline bool rcp_pgd_none_or_clear_bad(pgd_t *pgd)
{
	if (pgd_none(*pgd))
		return true;

	if (rcp_pgd_bad(*pgd)) {
		my_pgd_clear_bad(pgd);
		return true;
	}

	return false;
}

static inline bool rcp_p4d_none_or_clear_bad(p4d_t *p4d)
{
	if (p4d_none(*p4d))
		return true;

	if (p4d_bad(*p4d)) {
		my_p4d_clear_bad(p4d);
		return true;
	}

	return false;
}

static inline bool rcp_pud_none_or_clear_bad(pud_t *pud)
{
	if (pud_none(*pud))
		return true;

	if (pud_bad(*pud)) {
		my_pud_clear_bad(pud);
		return true;
	}

	return false;
}

static inline bool rcp_pmd_non_or_trans_huge_or_clear_bad(pmd_t *pmd)
{
	pmd_t pmdval = pmdp_get(pmd);

	/*
	 * The barrier will stabilize the pmdval in a register or on
	 * the stack so that it will stop changing under the code.
	 *
	 * When CONFIG_TRANSPARENT_HUGEPAGE=y on x86 32bit PAE,
	 * pmd_read_atomic is allowed to return a not atomic pmdval
	 * (for example pointing to an hugepage that has never been
	 * mapped in the pmd). The below checks will only care about
	 * the low part of the pmd with 32bit PAE x86 anyway, with the
	 * exception of pmd_none(). So the important thing is that if
	 * the low part of the pmd is found null, the high part will
	 * be also null or the pmd_none() check below would be
	 * confused.
	 */
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	barrier();
#endif
	if (pmd_none(pmdval) ||
	    pmd_trans_huge(pmdval))
		return true;

	if (pmd_bad(pmdval)) {
		my_pmd_clear_bad(pmd);
		return true;
	}

	return false;
}

#define SWAPIN_SKIP_VMA	1
#define SWAPIN_SKIP_PMD	2

static int swapin_pte(struct vm_fault *vmf)
{
	struct task_struct *task;
	struct mm_struct *mm = vmf->vma->vm_mm;
	struct vm_area_struct *vma;
	bool is_write = (vmf->vma->vm_flags & VM_WRITE);
	int ret;

	/* Allow to receive signals while issuing page IO */
	vmf->flags = (FAULT_FLAG_ALLOW_RETRY |
		      FAULT_FLAG_KILLABLE);
	if (is_write)
		vmf->flags |= FAULT_FLAG_WRITE;

	/* Try to do swapin */
	ret = my_do_swap_page(vmf);
	if (ret & VM_FAULT_ERROR) {
		task = mm->owner;
		pr_warn("reclaim_coldpgs: [%d][%s] Error %d on swapin 0x%lx\n",
			task->pid, task->comm, ret, vmf->address);

		ret = (ret == VM_FAULT_OOM) ? -ENOMEM : -EFAULT;
	}

	if (ret < 0)
		return ret;

	/* Bail if we're not allowed to retry */
	if (!(ret & VM_FAULT_RETRY))
		return 0;

	if (signal_pending(current))
		return -EINTR;

	/* Recheck the vma */
	down_read(&mm->mmap_lock);
	vma = find_vma(mm, vmf->address);
	if (!vma) {
		up_read(&mm->mmap_lock);
		return -EAGAIN;
	}

	if (vmf->vma != vma || !vma->anon_vma) {
		vmf->vma = vma;
		return SWAPIN_SKIP_VMA;
	}

	if (!my_mm_find_pmd(mm, vmf->address))
		return SWAPIN_SKIP_PMD;

	return 0;
}

static int swapin_pmd(struct vm_fault *vmf,
		      unsigned long addr, unsigned long end)
{
	pte_t *pte;
	int ret = 0;

	do {
		/*
		 * pte_unmap() must be invoked to release RCU read lock
		 * if the pte is not swap pte.
		 * It must not be invoked again after swapin_pte() returns,
		 * as in swapin_pte(), do_swap_page() will do pte_unmap()
		 * at the start of that function.
		 */
		pte = my___pte_offset_map(vmf->pmd, addr, NULL);
		vmf->orig_pte = *pte;
		if (!is_swap_pte(*pte)) {
			pte_unmap(pte);
			continue;
		}

		vmf->address = addr;
		vmf->pte = pte;
		ret = swapin_pte(vmf);
		if (ret)
			break;
	} while (addr += PAGE_SIZE, addr != end);

	return ret;
}

static int swapin_pud(struct vm_fault *vmf, pud_t *pud,
		      unsigned long addr, unsigned long end)
{
	pmd_t *pmd = pmd_offset(pud, addr);
	unsigned long next;
	int ret = 0;

	do {
		next = pmd_addr_end(addr, end);
		if (rcp_pmd_non_or_trans_huge_or_clear_bad(pmd))
			continue;

		vmf->pmd = pmd;
		ret = swapin_pmd(vmf, addr, next);
		if (!ret)
			continue;

		/* Ignore SWAPIN_SKIP_PMD */
		if (ret != SWAPIN_SKIP_PMD)
			break;

		ret = 0;
	} while (pmd++, addr = next, addr != end);

	return ret;
}

static int swapin_p4d(struct vm_fault *vmf, p4d_t *p4d,
		      unsigned long addr, unsigned long end)
{
	pud_t *pud = pud_offset(p4d, addr);
	unsigned long next;
	int ret = 0;

	do {
		next = pud_addr_end(addr, end);
		if (rcp_pud_none_or_clear_bad(pud))
			continue;

		ret = swapin_pud(vmf, pud, addr, next);
		if (ret)
			break;
	} while (pud++, addr = next, addr != end);

	return ret;
}

static int swapin_pgd(struct vm_fault *vmf, pgd_t *pgd,
		      unsigned long addr, unsigned long end)
{
	p4d_t *p4d = p4d_offset(pgd, addr);
	unsigned long next;
	int ret = 0;

	do {
		next = p4d_addr_end(addr, end);
		if (rcp_p4d_none_or_clear_bad(p4d))
			continue;

		ret = swapin_p4d(vmf, p4d, addr, next);
		if (ret)
			break;
	} while (p4d++, addr = next, addr != end);

	return ret;
}

static int swapin_vma(struct vm_fault *vmf)
{
	unsigned long addr = vmf->vma->vm_start;
	unsigned long end = vmf->vma->vm_end;
	pgd_t *pgd = pgd_offset(vmf->vma->vm_mm, addr);
	unsigned long next;
	int ret = 0;

	do {
		next = pgd_addr_end(addr, end);
		if (rcp_pgd_none_or_clear_bad(pgd))
			continue;

		ret = swapin_pgd(vmf, pgd, addr, next);
		if (ret)
			break;
	} while (pgd++, addr = next, addr != end);

	return ret;
}

static int reclaim_coldpgs_swapin_from_task(struct task_struct *task)
{
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	struct vm_fault vmf = { };
	int ret = 0;

	mm = get_task_mm(task);
	if (!mm)
		return 0;

	/*
	 * Bail if we're not the owner because memory is charged to the
	 * owner. Also threads can be assigned to different memory cgroups.
	 */
	if (mm->owner != task)
		goto out;

	VMA_ITERATOR(vmi, mm, 0);
	down_read(&mm->mmap_lock);
again:
	for_each_vma(vmi, vma) {
		if (!vma->anon_vma)
			continue;

		vmf.vma = vma;
		ret = swapin_vma(&vmf);
		if (!ret) {
			cond_resched();
			continue;
		}

		/* Interrupted without taking the lock */
		if (ret == -EINTR)
			goto out;

		/* Skip the vma which has been changed */
		if (ret == SWAPIN_SKIP_VMA) {
			ret = 0;
			vma = vmf.vma;
			continue;
		}

		/* Start over in case the vma is gone */
		if (ret == -EAGAIN)
			goto again;

		/* Abort on serious errors OOM/SIGBUS etc */
		break;
	}

	up_read(&mm->mmap_lock);
out:
	mmput(mm);
	return ret;
}

static int reclaim_coldpgs_swapin_from_memcg(struct mem_cgroup *memcg)
{
	struct task_struct *task;
	struct css_task_iter it;
	int ret = 0;

	my_css_task_iter_start(&memcg->css, 0, &it);

	while (!ret && (task = my_css_task_iter_next(&it))) {
		/* Ignore the tasks which are exiting */
		if (task->flags & PF_EXITING)
			continue;

		ret = reclaim_coldpgs_swapin_from_task(task);
	}

	my_css_task_iter_end(&it);

	return ret;
}

static ssize_t reclaim_coldpgs_write_swapin(struct kernfs_open_file *of,
					    char *buf, size_t count,
					    loff_t off)
{
	struct mem_cgroup *iter, *memcg = mem_cgroup_from_css(of_css(of));
	const char *magic = "swapin\n";
	int ret = 0;

	if (count != strlen(magic) || strcmp(buf, magic))
		return -EINVAL;

	for_each_memcg_tree(memcg, iter) {
		ret = reclaim_coldpgs_swapin_from_memcg(iter);
		if (ret) {
			my_mem_cgroup_iter_break(memcg, iter);
			break;
		}
	}

	return ret ? -EIO : count;
}

static u64 reclaim_coldpgs_read_swap_current(struct cgroup_subsys_state *css,
					     struct cftype *cft)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(css);

	if (cgroup_subsys_on_dfl(memory_cgrp_subsys))
		return (u64)page_counter_read(&memcg->swap) * PAGE_SIZE;

	return my_memcg_page_state(memcg, MEMCG_SWAP);
}

static inline
int seq_puts_memcg_tunable(struct seq_file *m, unsigned long value)
{
	if (value == PAGE_COUNTER_MAX)
		seq_puts(m, "max\n");
	else
		seq_printf(m, "%llu\n", (u64)value * PAGE_SIZE);

	return 0;
}

static int reclaim_coldpgs_swap_max_show(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = mem_cgroup_from_seq(m);

	if (cgroup_subsys_on_dfl(memory_cgrp_subsys))
		return seq_puts_memcg_tunable(m, READ_ONCE(memcg->swap.max));
	else
		return seq_puts_memcg_tunable(m,
				READ_ONCE(memcg->reclaim_coldpgs_max));
}

static
ssize_t reclaim_coldpgs_swap_max_write(struct kernfs_open_file *of,
				       char *buf, size_t nbytes,
				       loff_t off)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(of_css(of));
	unsigned long max;
	int err;

	buf = strstrip(buf);
	err = my_page_counter_memparse(buf, "max", &max);
	if (err)
		return err;

	if (cgroup_subsys_on_dfl(memory_cgrp_subsys)) {
		xchg(&memcg->swap.max, max);
		return nbytes;
	}

	/*
	 * For v1, memsw only reflicts swap usage, but it doesn't
	 * limit swap out routines but affects oom routines during
	 * alloc_page.
	 */
	memcg->reclaim_coldpgs_max = max;
	if (READ_ONCE(memcg->memsw.max) != PAGE_COUNTER_MAX) {
		max = READ_ONCE(memcg->memsw.max) -
			READ_ONCE(memcg->memory.max);
		memcg->reclaim_coldpgs_max = min_t(long, max,
						   memcg->reclaim_coldpgs_max);
	}

	return nbytes;
}

static struct cftype reclaim_coldpgs_files[] = {
	{ .name		= "coldpgs.threshold",
	  .seq_show	= reclaim_coldpgs_read_threshold,
	  .write	= reclaim_coldpgs_write_threshold,
	},
	{ .name		= "coldpgs.size",
	  .seq_show	= reclaim_coldpgs_read_size,
	  .write	= reclaim_coldpgs_write_size,
	},
	{ .name		= "coldpgs.flags",
	  .seq_show	= reclaim_coldpgs_read_flags,
	  .write	= reclaim_coldpgs_write_flags,
	},
	{ .name		= "coldpgs.stats",
	  .seq_show	= reclaim_coldpgs_read_stats,
	  .write	= reclaim_coldpgs_write_stats,
	},
	{ .name		= "coldpgs.swapin",
	  .seq_show	= reclaim_coldpgs_read_swapin,
	  .write	= reclaim_coldpgs_write_swapin,
	},
	{
	  .name		= "coldpgs.swap.current",
	  .flags	= CFTYPE_NOT_ON_ROOT,
	  .read_u64	= reclaim_coldpgs_read_swap_current,
	},
	{
	  .name		= "coldpgs.swap.max",
	  .flags	= CFTYPE_NOT_ON_ROOT,
	  .seq_show	= reclaim_coldpgs_swap_max_show,
	  .write	= reclaim_coldpgs_swap_max_write,
	},
	{ }	/* terminate */
};

static ssize_t reclaim_coldpgs_show_threshold(struct kobject *kobj,
					      struct kobj_attribute *attr,
					      char *buf)
{
	unsigned int val;
	int ret;

	down_read(&global_control.rwsem);
	val = global_control.thresholds[THRESHOLD_BASE];
	up_read(&global_control.rwsem);

	ret = sprintf(buf, "%u\n", val);

	return ret;
}

static ssize_t reclaim_coldpgs_store_threshold(struct kobject *kobj,
					       struct kobj_attribute *attr,
					       const char *buf,
					       size_t count)
{
	struct mem_cgroup *memcg;
	struct reclaim_coldpgs_filter filter;
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret || val < 1 || val > U8_MAX)
		return -EINVAL;

	/*
	 * We needn't access the global control block exclusively when copying
	 * over the information. However, it doesn't matter. It can avoid make
	 * double locking calls and simplify the code at least.
	 */
	down_write(&global_control.rwsem);
	if (global_control.flags & FLAG_IGNORE_MLOCK) {
		pr_err("Cannot reclaim mlock page right now, please change flags\n");
		up_write(&global_control.rwsem);
		return -EINVAL;
	}
	global_control.thresholds[THRESHOLD_BASE] = val;
	filter.flags = global_control.flags;
	filter.batch = global_control.batch;
	filter.mode = global_control.mode;
	filter.threshold = val;
	filter.size = 0xFFFFFFFFFF;
	memcpy(filter.thresholds, global_control.thresholds,
	       sizeof(filter.thresholds));
	up_write(&global_control.rwsem);

	for_each_memcg_tree(NULL, memcg) {
		reclaim_coldpgs_from_memcg(memcg, &filter);
	}

	return count;
}

static ssize_t reclaim_coldpgs_show_swapin(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   char *buf)
{
	const char *magic = "swapin";
	int ret;

	ret = sprintf(buf, "%s\n", magic);

	return ret;
}

static ssize_t reclaim_coldpgs_store_swapin(struct kobject *kobj,
					    struct kobj_attribute *attr,
					    const char *buf,
					    size_t count)
{
	struct mem_cgroup *memcg;
	const char *magic = "swapin\n";
	int ret = 0;

	if (count != strlen(magic) || strcmp(buf, magic))
		return -EINVAL;

	for_each_memcg_tree(NULL, memcg) {
		ret = reclaim_coldpgs_swapin_from_memcg(memcg);
		if (ret) {
			my_mem_cgroup_iter_break(NULL, memcg);
			break;
		}
	}

	return ret ? -EIO : count;
}

/*
 * Define handlers for the sysfs files. Their pattern is fixed. So we
 * leverage the macro to define them as below.
 */
RECLAIM_COLDPGS_SYSFS_HANDLER(version, version, true, 0, UINT_MAX);
RECLAIM_COLDPGS_SYSFS_HANDLER(hierarchy, hierarchy, false, 0, 1);
RECLAIM_COLDPGS_SYSFS_HANDLER(batch, batch, false, 1, UINT_MAX);
RECLAIM_COLDPGS_SYSFS_HANDLER(flags, flags, true,  0, UINT_MAX);
RECLAIM_COLDPGS_SYSFS_HANDLER(mode, mode, true,  0, UINT_MAX);
RECLAIM_COLDPGS_SYSFS_HANDLER(threshold_nonrot, thresholds[THRESHOLD_NONROT],
			      false, 0, U8_MAX);

RECLAIM_COLDPGS_ATTR(version, 0400);
RECLAIM_COLDPGS_ATTR(hierarchy, 0600);
RECLAIM_COLDPGS_ATTR(batch, 0600);
RECLAIM_COLDPGS_ATTR(flags, 0600);
RECLAIM_COLDPGS_ATTR(mode, 0600);
RECLAIM_COLDPGS_ATTR(threshold, 0600);
RECLAIM_COLDPGS_ATTR(threshold_nonrot, 0600);
RECLAIM_COLDPGS_ATTR(swapin, 0600);

static struct attribute *reclaim_coldpgs_attrs[] = {
	&reclaim_coldpgs_attr_version.attr,
	&reclaim_coldpgs_attr_hierarchy.attr,
	&reclaim_coldpgs_attr_batch.attr,
	&reclaim_coldpgs_attr_flags.attr,
	&reclaim_coldpgs_attr_mode.attr,
	&reclaim_coldpgs_attr_threshold.attr,
	&reclaim_coldpgs_attr_threshold_nonrot.attr,
	&reclaim_coldpgs_attr_swapin.attr,
	NULL
};

static struct attribute_group reclaim_coldpgs_attr_group = {
	.name	= "coldpgs",
	.attrs	= reclaim_coldpgs_attrs,
};

static int __init reclaim_coldpgs_resolve_symbols(void)
{
	reclaim_coldpgs_resolve_symbol(mem_cgroup_iter);
	reclaim_coldpgs_resolve_symbol(mem_cgroup_iter_break);
	reclaim_coldpgs_resolve_symbol(mem_cgroup_get_nr_swap_pages);
	reclaim_coldpgs_resolve_symbol(add_to_swap);
	reclaim_coldpgs_resolve_symbol(folio_free_swap);
	reclaim_coldpgs_resolve_symbol(end_swap_bio_write);
	reclaim_coldpgs_resolve_symbol(__swap_writepage);
	reclaim_coldpgs_resolve_symbol(lru_add_drain);
	reclaim_coldpgs_resolve_symbol(can_split_folio);
	reclaim_coldpgs_resolve_symbol(split_huge_page_to_list);
	reclaim_coldpgs_resolve_symbol(try_to_unmap);
	reclaim_coldpgs_resolve_symbol(__remove_mapping);
	reclaim_coldpgs_resolve_symbol(mem_cgroup_swap_full);
	reclaim_coldpgs_resolve_symbol(try_to_unmap_flush);
	reclaim_coldpgs_resolve_symbol(try_to_unmap_flush_dirty);
	reclaim_coldpgs_resolve_symbol(putback_lru_page);
	reclaim_coldpgs_resolve_symbol(workingset_age_nonresident);
	reclaim_coldpgs_resolve_symbol(mem_cgroup_update_lru_size);
	reclaim_coldpgs_resolve_symbol(__mem_cgroup_uncharge);
	reclaim_coldpgs_resolve_symbol(__mem_cgroup_uncharge_list);
	reclaim_coldpgs_resolve_symbol(free_unref_page_list);
	reclaim_coldpgs_resolve_symbol(vma_interval_tree_iter_first);
	reclaim_coldpgs_resolve_symbol(vma_interval_tree_iter_next);
	reclaim_coldpgs_resolve_symbol(cgroup_add_dfl_cftypes);
	reclaim_coldpgs_resolve_symbol(cgroup_add_legacy_cftypes);
	reclaim_coldpgs_resolve_symbol(cgroup_rm_cftypes);
	reclaim_coldpgs_resolve_symbol(vm_swappiness);
	reclaim_coldpgs_resolve_symbol(swap_info);
	reclaim_coldpgs_resolve_symbol(shrinker_list);
	reclaim_coldpgs_resolve_symbol(shrinker_idr);
	reclaim_coldpgs_resolve_symbol(root_mem_cgroup);
	reclaim_coldpgs_resolve_symbol(shrinker_nr_max);
	reclaim_coldpgs_resolve_symbol(css_task_iter_start);
	reclaim_coldpgs_resolve_symbol(css_task_iter_next);
	reclaim_coldpgs_resolve_symbol(css_task_iter_end);
#if CONFIG_PGTABLE_LEVELS > 4
#ifdef CONFIG_X86_5LEVEL
	reclaim_coldpgs_resolve_symbol(__pgtable_l5_enabled);
#endif
#endif
	reclaim_coldpgs_resolve_symbol(pgd_clear_bad);
#if CONFIG_PGTABLE_LEVELS > 4
	reclaim_coldpgs_resolve_symbol(p4d_clear_bad);
#endif
#ifndef __PAGETABLE_PUD_FOLDED
	reclaim_coldpgs_resolve_symbol(pud_clear_bad);
#endif
	reclaim_coldpgs_resolve_symbol(pmd_clear_bad);
	reclaim_coldpgs_resolve_symbol(mm_find_pmd);
	reclaim_coldpgs_resolve_symbol(do_swap_page);
	reclaim_coldpgs_resolve_symbol(node_page_state);
	reclaim_coldpgs_resolve_symbol(__mod_lruvec_state);

	reclaim_coldpgs_resolve_symbol(folio_lock_anon_vma_read);
	reclaim_coldpgs_resolve_symbol(page_mapped_in_vma);
	reclaim_coldpgs_resolve_symbol(anon_vma_interval_tree_iter_first);
	reclaim_coldpgs_resolve_symbol(anon_vma_interval_tree_iter_next);

	reclaim_coldpgs_resolve_symbol(folio_total_mapcount);
	reclaim_coldpgs_resolve_symbol(__pte_offset_map);
	reclaim_coldpgs_resolve_symbol(destroy_large_folio);
	reclaim_coldpgs_resolve_symbol(folio_putback_lru);
	reclaim_coldpgs_resolve_symbol(zswap_store);
	reclaim_coldpgs_resolve_symbol(page_counter_memparse);
	reclaim_coldpgs_resolve_symbol(memcg_page_state);
#ifdef CONFIG_ARM64
	reclaim_coldpgs_resolve_symbol(mte_save_tags);
#endif

	return 0;
}

static int __init reclaim_coldpgs_init(void)
{
	unsigned int major, minor, revision;
	int ret;

	if (mem_cgroup_disabled())
		return -ENXIO;

	if (lru_gen_enabled()) {
		pr_warn("%s: Failed to load coldpgs due to MGLRU enabled\n",
			__func__);
		return -EPERM;
	}

	/* Resolve symbols required by the driver */
	ret = reclaim_coldpgs_resolve_symbols();
	if (ret)
		return ret;

	/*
	 * Initialize global control. The version is figured out from the
	 * pre-defined string so that we needn't define another one with
	 * different type, to ensure the consistence.
	 */
	ret = sscanf(DRIVER_VERSION, "%d.%d.%d", &major, &minor, &revision);
	if (ret != 3 || major > U8_MAX || minor > U8_MAX || revision > U8_MAX) {
		pr_warn("%s: Invalid version [%s] detected\n",
			__func__, DRIVER_VERSION);
		return -EINVAL;
	}

	init_rwsem(&global_control.rwsem);
	global_control.version = ((major << 16) | (minor << 8) | revision);
	global_control.batch = 32;

	/* Populate the sysfs files */
	ret = sysfs_create_group(mm_kobj, &reclaim_coldpgs_attr_group);
	if (ret) {
		pr_warn("%s: Error %d to populate the sysfs files\n",
			__func__, ret);
		return ret;
	}

	/*
	 * Populate the cgroup files. We need different APIs to do that in
	 * cgroup v1/v2
	 */
	if (cgroup_subsys_on_dfl(memory_cgrp_subsys)) {
		ret = my_cgroup_add_dfl_cftypes(&memory_cgrp_subsys,
						reclaim_coldpgs_files);
	} else {
		ret = my_cgroup_add_legacy_cftypes(&memory_cgrp_subsys,
						   reclaim_coldpgs_files);
	}

	if (ret) {
		pr_warn("%s: Error %d to populate the cgroup files\n",
			__func__, ret);
		sysfs_remove_group(mm_kobj, &reclaim_coldpgs_attr_group);
		return ret;
	}

	pr_info("%s (%s) loaded\n", DRIVER_DESC, DRIVER_VERSION);

	return 0;
}

static void __exit reclaim_coldpgs_exit(void)
{
	my_cgroup_rm_cftypes(reclaim_coldpgs_files);
	sysfs_remove_group(mm_kobj, &reclaim_coldpgs_attr_group);

	pr_info("%s (%s) unloaded\n", DRIVER_DESC, DRIVER_VERSION);
}

module_init(reclaim_coldpgs_init);
module_exit(reclaim_coldpgs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);

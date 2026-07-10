// SPDX-License-Identifier: GPL-2.0
#include <linux/kthread.h>
#include <linux/memcontrol.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/mm_inline.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/page-flags.h>
#include <linux/page_idle.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/kidled.h>
#include <linux/slab.h>
#include "slab.h"
#include <linux/swap.h>
#include <linux/memblock.h>
#include <uapi/linux/sched/types.h>

/*
 * Why do we use a kernel thread to scan pages instead of use Vladimir Davydov's
 * idle page tracking directly?
 *
 * We can collect the hot/cold information based on Vladimir's idle page
 * tracking feature which was already merged in mainline source tree.
 * However, Vladimir's patch is to encourage a clever memory manager
 * to scan pages from userspace, but we found it's difficult to use
 * in production environment, the reasons are as below:
 * 1). The idle bitmap is indexed by pfn, which means users have to translate
 *     the virtual address to physical address at first(e.g. through
 *     /proc/PID/pagemap), this may lead high CPU utilization due to many
 *     context switch between user and kernel mode;
 * 2). To get a cgroup's idle page information, users can access
 *     /proc/kpagecgroup to identify which cgroup the page was charged to,
 *     it's also not convenient;
 * 3). It's not easy to handle share mappings, e.g. a child process forked
 *     from parent (COW) or share memory files mapped by different process.
 *     It's easy to influence each other when there exist more than one
 *     scanner. So it's better to have a global scanner to do this job.
 *
 * We named this global scanner *kidled* because it's based on the
 * *idle* page tracking feature.
 *
 * We also found that Michel Lespinasse had developed a similar feature which
 * was called kstaled:
 *
 * https://lore.kernel.org/lkml/20110922161448.91a2e2b2.akpm@google.com/T/
 *
 * In Michel Lespinasse's patch, each page has a corresponding 8 bits attribute
 * which was called ide_page_age, and use buckets to do histogram sampling.
 * This is a good idea! Since Michel's patch was developed on a early kernel
 * version 3.0 and we decided to use Vladimir Davydov's idle page tracking API
 * to check and clear page's reference, so we didn't cherry pick the original
 * kstaled's patch directly. Thanks!
 */

/*
 * In order to speed up the scanning of all PFNs, we use for_each_mem_pfn_range()
 * to skip gigantic holes, especially, the number of invalid PFNs is 85 times
 * that of valid PFNs if NUMA is turned off in arm64. That way is a necessary
 * improvement in kidled. But a function with __init_memblock attribute is used
 * in for_each_mem_pfn_range(). So __ref is needed in these caller to avoid the
 * warning from compiler when CONFIG_ARCH_KEEP_MEMBLOCK disabled.
 */
#ifdef CONFIG_ARCH_KEEP_MEMBLOCK
#define __kidled_ref
#else
#define __kidled_ref __ref
#endif

DEFINE_STATIC_KEY_FALSE(kidled_enabled_key);
static atomic_t kidled_disabling;

unsigned int kidled_scan_target __read_mostly = KIDLED_SCAN_PAGE;
struct kidled_scan_control kidled_scan_control;
/*
 * These bucket values are copied from Michel Lespinasse's patch, they are
 * the default buckets to do histogram sampling.
 *
 * Kidled also supports each memory cgroup has it's own sampling buckets by
 * configuring memory.idle_page_stats file, and the child memcg will inherit
 * parent's bucket values. See Documentation/vm/kidled.rst for more details.
 */
const int kidled_default_buckets[NUM_KIDLED_BUCKETS] = {
	1, 2, 5, 15, 30, 60, 120, 240 };
static DECLARE_WAIT_QUEUE_HEAD(kidled_wait);
static DEFINE_STATIC_KEY_FALSE(kidled_slab_key);
unsigned long kidled_scan_rounds __read_mostly;
static atomic_t kidled_setting;

static inline int kidled_get_bucket(int *idle_buckets, int age)
{
	int bucket;

	if (age < idle_buckets[0])
		return -EINVAL;

	for (bucket = 1; bucket <= (NUM_KIDLED_BUCKETS - 1); bucket++) {
		if (age < idle_buckets[bucket])
			return bucket - 1;
	}

	return NUM_KIDLED_BUCKETS - 1;
}

static inline int kidled_get_idle_type(struct folio *folio)
{
	int idle_type = KIDLE_BASE;

	if (folio_test_slab(folio)) {
		idle_type |= KIDLE_SLAB;
		goto out;
	}

	if (folio_test_dirty(folio) || folio_test_writeback(folio))
		idle_type |= KIDLE_DIRTY;
	if (folio_is_file_lru(folio))
		idle_type |= KIDLE_FILE;
	/*
	 * Couldn't call page_evictable() here, because we have not held
	 * the page lock, so use page flags instead. Different from
	 * PageMlocked().
	 */
	if (folio_test_unevictable(folio))
		idle_type |= KIDLE_UNEVICT;
	if (folio_test_active(folio))
		idle_type |= KIDLE_ACTIVE;
out:
	return idle_type;
}

#ifndef KIDLED_AGE_NOT_IN_PAGE_FLAGS
int kidled_inc_folio_age(pg_data_t *pgdat, unsigned long pfn)
{
	struct folio *folio = pfn_folio(pfn);
	unsigned long old, new;
	int age;

	do  {
		age = ((folio->flags >> KIDLED_AGE_PGSHIFT) & KIDLED_AGE_MASK);
		if (age >= KIDLED_AGE_MASK)
			break;

		age++;
		new = old = folio->flags;
		new &= ~(KIDLED_AGE_MASK << KIDLED_AGE_PGSHIFT);
		new |= ((age & KIDLED_AGE_MASK) << KIDLED_AGE_PGSHIFT);
	} while (unlikely(cmpxchg(&folio->flags, old, new) != old));

	return age;
}
EXPORT_SYMBOL_GPL(kidled_inc_folio_age);

void kidled_set_folio_age(pg_data_t *pgdat, unsigned long pfn, int val)
{
	struct folio *folio = pfn_folio(pfn);
	unsigned long old, new;

	do  {
		new = old = folio->flags;
		new &= ~(KIDLED_AGE_MASK << KIDLED_AGE_PGSHIFT);
		new |= ((val & KIDLED_AGE_MASK) << KIDLED_AGE_PGSHIFT);
	} while (unlikely(cmpxchg(&folio->flags, old, new) != old));

}
EXPORT_SYMBOL_GPL(kidled_set_folio_age);
#endif /* !KIDLED_AGE_NOT_IN_PAGE_FLAGS */

#ifdef CONFIG_MEMCG
void kidled_mem_cgroup_account(struct folio *folio,
			       void *ptr, int age, unsigned long size)
{
	struct mem_cgroup *memcg;
	struct idle_page_stats *stats;
	int type, bucket;
	bool locked = false;

	if (mem_cgroup_disabled())
		return;

	type = kidled_get_idle_type(folio);
	if (type == KIDLE_SLAB) {
		if (!memcg_kmem_online())
			memcg = root_mem_cgroup;
		else {
			memcg = mem_cgroup_from_obj(ptr);
			if (!memcg)
				return;
		}
	} else {
		folio_memcg_lock(folio);
		memcg = folio_memcg(folio);
		if (unlikely(!memcg)) {
			folio_memcg_unlock(folio);
			return;
		}
		locked = true;
	}

	stats = mem_cgroup_get_unstable_idle_stats(memcg);
	bucket = kidled_get_bucket(stats->buckets, age);
	if (bucket >= 0)
		stats->count[type][bucket] += size;

	if (locked)
		folio_memcg_unlock(folio);
}

void kidled_mem_cgroup_move_stats(struct mem_cgroup *from,
				  struct mem_cgroup *to,
				  struct folio *folio,
				  unsigned long size)
{
	pg_data_t *pgdat = folio_pgdat(folio);
	unsigned long pfn = folio_pfn(folio);
	struct idle_page_stats *stats[4] = { NULL, };
	int type, bucket, age;

	if (mem_cgroup_disabled())
		return;

	type = kidled_get_idle_type(folio);
	stats[0] = mem_cgroup_get_stable_idle_stats(from);
	stats[1] = mem_cgroup_get_unstable_idle_stats(from);
	if (to) {
		stats[2] = mem_cgroup_get_stable_idle_stats(to);
		stats[3] = mem_cgroup_get_unstable_idle_stats(to);
	}

	/*
	 * We assume the all page ages are same if this is a compound page.
	 * Also we uses node's cursor (@node_idle_scan_pfn) to check if current
	 * page should be removed from the source memory cgroup or charged
	 * to target memory cgroup, without introducing locking mechanism.
	 * This may lead to slightly inconsistent statistics, but it's fine
	 * as it will be reshuffled in next round of scanning.
	 */
	age = kidled_get_folio_age(pgdat, pfn);
	if (age < 0)
		return;

	bucket = kidled_get_bucket(stats[1]->buckets, age);
	if (bucket < 0)
		return;

	/* Remove from the source memory cgroup */
	if (stats[0]->count[type][bucket] > size)
		stats[0]->count[type][bucket] -= size;
	else
		stats[0]->count[type][bucket] = 0;
	if (pgdat->node_idle_scan_pfn >= pfn) {
		if (stats[1]->count[type][bucket] > size)
			stats[1]->count[type][bucket] -= size;
		else
			stats[1]->count[type][bucket] = 0;
	}

	/* Charge to the target memory cgroup */
	if (!to)
		return;

	bucket = kidled_get_bucket(stats[3]->buckets, age);
	if (bucket < 0)
		return;

	stats[2]->count[type][bucket] += size;
	if (pgdat->node_idle_scan_pfn >= pfn)
		stats[3]->count[type][bucket] += size;
}
EXPORT_SYMBOL_GPL(kidled_mem_cgroup_move_stats);

static inline void
kidled_mem_cgroup_scan_done(struct kidled_scan_control scan_control)
{
	struct mem_cgroup *memcg;
	struct idle_page_stats *stable_stats, *unstable_stats;
	bool slab_only = false;

	for (memcg = mem_cgroup_iter(NULL, NULL, NULL);
	     memcg != NULL;
	     memcg = mem_cgroup_iter(NULL, memcg, NULL)) {

		down_write(&memcg->idle_stats_rwsem);
		stable_stats = mem_cgroup_get_stable_idle_stats(memcg);
		unstable_stats = mem_cgroup_get_unstable_idle_stats(memcg);

		/*
		 * Switch when scanning buckets is valid, or copy buckets
		 * from stable_stats's buckets which may have user's new
		 * buckets(maybe valid or not).
		 */
		if (!KIDLED_IS_BUCKET_INVALID(unstable_stats->buckets)) {
			mem_cgroup_idle_page_stats_switch(memcg);
			if (kidled_has_page_target(&scan_control))
				memcg->idle_page_scans++;
			if (kidled_has_slab_target(&scan_control) &&
			    (memcg_kmem_online() || mem_cgroup_is_root(memcg)))
				memcg->idle_slab_scans++;

			slab_only = kidled_has_slab_target_only(&scan_control);
		} else {
			memcpy(unstable_stats->buckets, stable_stats->buckets,
			       sizeof(unstable_stats->buckets));
		}

		memcg->scan_control = scan_control;
		up_write(&memcg->idle_stats_rwsem);

		unstable_stats = mem_cgroup_get_unstable_idle_stats(memcg);
		memset(&unstable_stats->count, 0,
		       sizeof(unstable_stats->count));

		if (slab_only && !memcg_kmem_online())
			break;
	}
}

/*
 * Reset the specified statistics by scan_type when users want to
 * change the scan target. For example, we should clear the slab
 * statistics when we only want to scan the page and vice versa.
 * Otherwise it will mislead the user about the statistics.
 */
static inline void
kidled_mem_cgroup_reset(enum kidled_scan_type scan_type)
{
	struct mem_cgroup *memcg;
	struct idle_page_stats *stable_stats, *unstable_stats;

	for (memcg = mem_cgroup_iter(NULL, NULL, NULL);
	     memcg != NULL;
	     memcg = mem_cgroup_iter(NULL, memcg, NULL)) {
		down_write(&memcg->idle_stats_rwsem);
		stable_stats = mem_cgroup_get_stable_idle_stats(memcg);
		unstable_stats = mem_cgroup_get_unstable_idle_stats(memcg);
		if (scan_type == SCAN_TARGET_PAGE) {
			int i;

			for (i = 0; i < KIDLE_NR_TYPE - 1; i++)
				memset(&stable_stats->count[i], 0,
				       sizeof(stable_stats->count[i]));
			memcg->scan_control.scan_target = kidled_scan_target;
			up_write(&memcg->idle_stats_rwsem);
			for (i = 0; i < KIDLE_NR_TYPE - 1; i++)
				memset(&unstable_stats->count[i], 0,
				       sizeof(unstable_stats->count[i]));
		} else if (scan_type == SCAN_TARGET_SLAB) {
			memset(&stable_stats->count[KIDLE_SLAB], 0,
			       sizeof(stable_stats->count[KIDLE_SLAB]));
			memcg->scan_control.scan_target = kidled_scan_target;
			up_write(&memcg->idle_stats_rwsem);
			memset(&unstable_stats->count[KIDLE_SLAB], 0,
			       sizeof(unstable_stats->count[KIDLE_SLAB]));

			if (!memcg_kmem_online())
				break;
		} else {
			memset(&stable_stats->count, 0,
			       sizeof(stable_stats->count));
			memcg->idle_page_scans = 0;
			kidled_reset_scan_control(&memcg->scan_control);
			up_write(&memcg->idle_stats_rwsem);
			memset(&unstable_stats->count, 0,
			       sizeof(unstable_stats->count));
		}
	}
}
#else /* !CONFIG_MEMCG */
static inline void kidled_mem_cgroup_account(struct folio *folio,
					     void *ptr, int age, unsigned long size)
{
}
static inline void kidled_mem_cgroup_scan_done(struct kidled_scan_control
						       scan_control)
{
}
static inline void kidled_mem_cgroup_reset(enum kidled_scan_type scan_type)
{
}
#endif /* CONFIG_MEMCG */

/*
 * An idle page with an older age is more likely idle, while a busy page is
 * more likely busy, so we can reduce the sampling frequency to save cpu
 * resource when meet these pages. And we will keep sampling each time when
 * an idle page is young. See tables below:
 *
 *  idle age |   down ratio
 * ----------+-------------
 * [0, 1)    |     1/2      # busy
 * [1, 4)    |      1       # young idle
 * [4, 8)    |     1/2      # idle
 * [8, 16)   |     1/4      # old idle
 * [16, +inf)|     1/8      # older idle
 */
static inline bool kidled_need_check_idle(pg_data_t *pgdat, unsigned long pfn)
{
	struct folio *folio = pfn_folio(pfn);
	int age = kidled_get_folio_age(pgdat, pfn);
	unsigned long pseudo_random;

	if (age < 0)
		return false;

	/*
	 * kidled will check different pages at each round when need
	 * reduce sampling frequency, this depends on current pfn and
	 * global scanning rounds. There exist some special pfns, for
	 * one huge page, we can only check the head page, while tail
	 * pages would be checked in low levels and will be skipped.
	 * Shifting HPAGE_PMD_ORDER bits is to achieve good load balance
	 * for each round when system has many huge pages, 1GB is not
	 * considered here.
	 */
	pfn >>= folio_order(folio);

	pseudo_random = pfn + kidled_scan_rounds;
	if (age == 0)
		return pseudo_random & 0x1UL;
	else if (age < 4)
		return true;
	else if (age < 8)
		return pseudo_random & 0x1UL;
	else if (age < 16)
		return (pseudo_random & 0x3UL) == 0x3UL;
	else
		return (pseudo_random & 0x7UL) == 0x7UL;
}

static inline int kidled_scan_folio(pg_data_t *pgdat, unsigned long pfn)
{
	int age, nr_pages = 1;
	bool idle = false;
	struct folio *folio;

	if (!pfn_valid(pfn))
		goto out;

	folio = pfn_folio(pfn);
	if (!folio || !folio_test_lru(folio)) {
		kidled_set_folio_age(pgdat, pfn, 0);
		goto out;
	}

	/*
	 * Try to skip clear PTE references which is an expensive call.
	 * PG_idle should be cleared when free a page and we have checked
	 * PG_lru flag above, so the race is acceptable to us.
	 */
	if (folio_test_idle(folio)) {
		if (kidled_need_check_idle(pgdat, pfn)) {
			if (!folio_try_get(folio)) {
				kidled_set_folio_age(pgdat, pfn, 0);
				goto out;
			}

			/*
			 * Check again after get a reference count, while in
			 * page_idle_get_page() it gets zone_lru_lock at first,
			 * it seems useless.
			 *
			 * Also we can't hold LRU lock here as the consumed
			 * time to finish the scanning is fixed. Otherwise,
			 * the accumulated statistics will be cleared out
			 * and scan interval (@scan_period_in_seconds) will
			 * be doubled. However, this may incur race between
			 * kidled and page reclaim. The page reclaim may dry
			 * run due to dumped refcount, but it's acceptable.
			 */
			if (unlikely(!folio_test_lru(folio))) {
				folio_put(folio);
				kidled_set_folio_age(pgdat, pfn, 0);
				goto out;
			}

			page_idle_clear_pte_refs(folio);
			if (folio_test_idle(folio))
				idle = true;
			folio_put(folio);
		} else if (kidled_get_folio_age(pgdat, pfn) > 0) {
			idle = true;
		}
	}

	nr_pages = (int) folio_nr_pages(folio);

	if (idle) {
		age = kidled_inc_folio_age(pgdat, pfn);
		if (age > 0)
			kidled_mem_cgroup_account(folio, NULL, age,
						  nr_pages << PAGE_SHIFT);
		else
			age = 0;
	} else {
		age = 0;
		kidled_set_folio_age(pgdat, pfn, 0);
		if (folio_try_get(folio)) {
			if (likely(folio_test_lru(folio)))
				folio_set_idle(folio);
			folio_put(folio);
		}
	}

out:
	return nr_pages;
}

static bool kidled_scan_node(pg_data_t *pgdat,
			     struct kidled_scan_control scan_control,
			     unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long pfn = start_pfn;
	unsigned long node_end = pgdat_end_pfn(pgdat);
#if !defined(CONFIG_ARCH_KEEP_MEMBLOCK) && !defined(CONFIG_MEMORY_HOTPLUG)
	unsigned long sequent_invalid_pfns = 0;
	int nr_nodes = num_online_nodes();
#endif

	if (kidled_has_slab_target_only(&scan_control))
		return false;
	else if (pgdat->node_idle_scan_pfn >= node_end)
		return true;

#ifdef KIDLED_AGE_NOT_IN_PAGE_FLAGS
	if (unlikely(!pgdat->node_folio_age)) {
		u8 *age;
		/* This node has none memory, skip it. */
		if (!pgdat->node_spanned_pages)
			return true;

		age = vzalloc(pgdat->node_spanned_pages);
		if (unlikely(!age))
			return false;
		rcu_assign_pointer(pgdat->node_folio_age, age);
	}
#endif /* KIDLED_AGE_NOT_IN_PAGE_FLAGS */

	while (pfn < end_pfn) {
		/* Restart new scanning when user updates the period */
		if (unlikely(!kidled_is_scan_period_equal(&scan_control) ||
			     !kidled_has_page_target_equal(&scan_control)))
			break;

#if !defined(CONFIG_ARCH_KEEP_MEMBLOCK) && !defined(CONFIG_MEMORY_HOTPLUG)
		if (nr_nodes == 1) {
			if (!pfn_valid(pfn)) {
				sequent_invalid_pfns++;
				if (sequent_invalid_pfns % (2 * HPAGE_PMD_NR) == 0)
					cond_resched();
				pfn++;
				continue;
			}
			sequent_invalid_pfns = 0;
		}
#endif
		cond_resched();
		pfn += kidled_scan_folio(pgdat, pfn);
	}

	pgdat->node_idle_scan_pfn = pfn;
	return pfn >= node_end;
}

/*
 * Here for_each_mem_pfn_range() only used when either CONFIG_ARCH_KEEP_MEMBLOCK
 * or CONFIG_MEMORY_HOTPLUG is turned on. That because these functions with
 * __init_memblock would been discarded after system running, then crash would
 * happen if caller executes to them.
 */
#if defined(CONFIG_ARCH_KEEP_MEMBLOCK) || defined(CONFIG_MEMORY_HOTPLUG)
static __kidled_ref bool kidled_scan_nodes(struct kidled_scan_control scan_control,
					   bool restart)
{
	int i, nid;
	unsigned long start_pfn, end_pfn;
	bool scan_done = true;

	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		unsigned long pages_to_scan = DIV_ROUND_UP(pgdat->node_present_pages,
							   scan_control.duration);
		bool init = !restart;

		if (restart)
			pgdat->node_idle_scan_pfn = pgdat->node_start_pfn;

		for_each_mem_pfn_range(i, nid, &start_pfn, &end_pfn, NULL) {
			if (init) {
				/* Start scanning from the previous range */
				if (end_pfn < pgdat->node_idle_scan_pfn)
					continue;

				/*
				 * There are two cases should been noticed:
				 *
				 * 1) end_pfn = node_idle_scan_pfn: only one pfn
				 * will be scanned, and we must increasing end_pfn
				 * to avoid 'start_pfn = end_pfn';
				 *
				 * 2) start_pfn > node_idle_scan_pfn: this indicates
				 * node_idle_scan_pfn locates in an invalid range.
				 * We should update it to next valid range before
				 * scanning.
				 */
				if (end_pfn == pgdat->node_idle_scan_pfn) {
					end_pfn += 1;
					start_pfn = pgdat->node_idle_scan_pfn;
				} else if (start_pfn > pgdat->node_idle_scan_pfn) {
					pgdat->node_idle_scan_pfn = start_pfn;
				} else
					start_pfn = pgdat->node_idle_scan_pfn;
				init = false;
			}

			if ((end_pfn - start_pfn) > pages_to_scan)
				end_pfn = start_pfn + pages_to_scan;
			scan_done &= kidled_scan_node(pgdat, scan_control,
						      start_pfn, end_pfn);
			/*
			 * That empirical value mainly to ensure that
			 * sufficient PFNs will be scanned in current
			 * period.
			 */
			if ((end_pfn - start_pfn) >= pages_to_scan / 16)
				break; /* Let kidled scans next node */
		}
	}

	return scan_done;
}
#else
static bool kidled_scan_nodes(struct kidled_scan_control scan_control,
			      bool restart)
{
	unsigned long start_pfn, end_pfn;
	pg_data_t *pgdat;
	bool scan_done = true;

	/*
	 * TODO: Perhaps there are massive holes when NUMA disabled.
	 * And this scene only find in arm64.
	 */
	for_each_online_pgdat(pgdat) {
		unsigned long node_end = pgdat_end_pfn(pgdat);

		if (restart)
			pgdat->node_idle_scan_pfn = pgdat->node_start_pfn;
		start_pfn = pgdat->node_idle_scan_pfn;
		end_pfn = min(start_pfn + DIV_ROUND_UP(pgdat->node_spanned_pages,
						       scan_control.duration), node_end);
		scan_done &= kidled_scan_node(pgdat, scan_control, start_pfn,
					      end_pfn);
	}

	return scan_done;
}
#endif

#ifdef KIDLED_AGE_NOT_IN_PAGE_FLAGS
void kidled_free_folio_age(pg_data_t *pgdat)
{
	u8 *age;

	age = rcu_access_pointer(pgdat->node_folio_age);
	if (age) {
		rcu_assign_pointer(pgdat->node_folio_age, NULL);
		synchronize_rcu();
		vfree(age);
	}
}
#endif

static inline void kidled_scan_slab_node(int nid,
				struct kidled_scan_control scan_control)
{
	struct mem_cgroup *memcg;

	memcg = mem_cgroup_iter(NULL, NULL, NULL);
	do {
		kidled_scan_slab(nid, memcg, scan_control);
		if (!memcg_kmem_online())
			break;
	} while ((memcg = mem_cgroup_iter(NULL, memcg, NULL)) != NULL);
}

static inline void kidled_scan_slabs(struct kidled_scan_control scan_control)
{
	int nid;

	if (!kidled_has_slab_target(&scan_control))
		return;

	for_each_online_node(nid)
		kidled_scan_slab_node(nid, scan_control);
}

static inline void kidled_scan_done(struct kidled_scan_control scan_control)
{
	kidled_mem_cgroup_scan_done(scan_control);
	kidled_scan_rounds++;
}

#ifdef KIDLED_AGE_NOT_IN_PAGE_FLAGS
static void kidled_reset(bool free)
{
	pg_data_t *pgdat;

	kidled_mem_cgroup_reset(SCAN_TARGET_ALL);

	get_online_mems();

	for_each_online_pgdat(pgdat) {
		if (!pgdat->node_folio_age)
			continue;

		if (free)
			kidled_free_folio_age(pgdat);
		else {
			memset(pgdat->node_folio_age, 0,
			pgdat->node_spanned_pages);
		}

		cond_resched();
	}
	put_online_mems();
}
#elif defined(CONFIG_ARCH_KEEP_MEMBLOCK) || defined(CONFIG_MEMORY_HOTPLUG)
static __kidled_ref void kidled_reset(void)
{
	pg_data_t *pgdat;
	int i, nid;

	kidled_mem_cgroup_reset(SCAN_TARGET_ALL);

	get_online_mems();
	for_each_online_node(nid) {
		unsigned long pfn, start_pfn, end_pfn;

		pgdat = NODE_DATA(nid);
		for_each_mem_pfn_range(i, nid, &start_pfn, &end_pfn, NULL) {
			for (pfn = start_pfn; pfn <= end_pfn; pfn++) {
				if (pfn_valid(pfn))
					kidled_set_folio_age(pgdat, pfn, 0);

				if (pfn % HPAGE_PMD_NR == 0)
					cond_resched();
			}
		}
	}
	put_online_mems();
}
#else
static void kidled_reset(void)
{
	pg_data_t *pgdat;

	kidled_mem_cgroup_reset(SCAN_TARGET_ALL);

	get_online_mems();
	for_each_online_pgdat(pgdat) {
		unsigned long pfn, end_pfn = pgdat->node_start_pfn +
					     pgdat->node_spanned_pages;

		for (pfn = pgdat->node_start_pfn; pfn < end_pfn; pfn++) {
			if (pfn_valid(pfn))
				kidled_set_folio_age(pgdat, pfn, 0);
			if (pfn % HPAGE_PMD_NR == 0)
				cond_resched();
		}
	}
	put_online_mems();
}
#endif

static inline bool kidled_should_run(struct kidled_scan_control *p,
				     bool *new, int *count_slab_scan)
{
	if (unlikely(!kidled_is_scan_period_equal(p))) {
		/* Any scan period write will enter here, due to val changes */
		struct kidled_scan_control scan_control;

		scan_control  = kidled_get_current_scan_control();
		/*
		 * Both disabling or updating requires kidled is enabled.
		 * If kidled has been disabled already, don't do reset.
		 */
		if (is_kidled_enabled() && p->duration) {
#ifdef KIDLED_AGE_NOT_IN_PAGE_FLAGS
			kidled_reset(!scan_control.duration);
#else
			kidled_reset();
#endif
		}
		if (!scan_control.duration) {
			static_branch_disable(&kidled_enabled_key);
			if (atomic_cmpxchg(&kidled_disabling, 1, 0) != 1)
				pr_warn_ratelimited("%s: kidled_disabling may be not set correctly when disabling kidled\n", __func__);
		}

		*p = scan_control;
		*new = true;
	} else if (unlikely(!kidled_is_scan_target_equal(p))) {
		struct kidled_scan_control scan_control;
		bool page_disabled = false;
		bool slab_disabled = false;

		scan_control = kidled_get_current_scan_control();
		kidled_get_reset_type(p, &page_disabled, &slab_disabled);
		if (slab_disabled) {
			kidled_mem_cgroup_reset(SCAN_TARGET_SLAB);
			*count_slab_scan = 0;
		}
		if (page_disabled)
			kidled_mem_cgroup_reset(SCAN_TARGET_PAGE);

		/*
		 * It need to restart the page scan when user enable
		 * the specified scan type again.
		 */
		if (kidled_has_slab_target_only(p))
			*new = true;
		else
			*new = false;
		*p = scan_control;
	} else {
		*new = false;
	}

	if (p->duration > 0)
		return true;

	return false;
}

static inline bool is_kidled_scan_done(bool scan_done,
				       int count_slab_scan,
				       struct kidled_scan_control scan_control)
{
	u16 duration = scan_control.duration;

	if (kidled_has_slab_target_only(&scan_control))
		return count_slab_scan >= duration;
	else if (kidled_has_page_target_only(&scan_control))
		return scan_done;
	else
		return scan_done && (count_slab_scan >= duration);
}

static int kidled(void *dummy)
{
	int busy_loop = 0;
	bool restart = true;
	struct kidled_scan_control scan_control;
	int count_slab_scan = 0;

	kidled_reset_scan_control(&scan_control);

	while (!kthread_should_stop()) {
		u64 start_jiffies, elapsed;
		bool new, scan_done = true;

		wait_event_interruptible(kidled_wait,
					kidled_should_run(&scan_control,
					&new, &count_slab_scan));
		if (unlikely(new)) {
			restart = true;
			busy_loop = 0;
		}

		if (unlikely(scan_control.duration == 0))
			continue;

		start_jiffies = jiffies_64;
		get_online_mems();
		scan_done = kidled_scan_nodes(scan_control, restart);
		put_online_mems();

		kidled_scan_slabs(scan_control);
		if (is_kidled_scan_done(scan_done,
			count_slab_scan + 1, scan_control)) {
			kidled_scan_done(scan_control);
			restart = true;
			count_slab_scan = 0;
		} else {
			restart = false;
			count_slab_scan++;
		}

		/*
		 * This code snippet of emergency throttle was borrowed from
		 * Michel Lespinasse's patch. And we also set the scheduler
		 * policy of kidled as SCHED_IDLE to make sure it won't disturb
		 * neighbors (e.g. cause spike latency).
		 *
		 * We hope kidled can scan specified pages which depends on
		 * scan_control in each slice, and supposed to finish each
		 * slice in one second:
		 *
		 *	pages_to_scan = total_pages / scan_duration
		 *	for_each_slice() {
		 *		start_jiffies = jiffies_64;
		 *		scan_pages(pages_to_scan);
		 *		elapsed = jiffies_64 - start_jiffies;
		 *		sleep(HZ - elapsed);
		 *	}
		 *
		 * We thought it's busy when elapsed >= (HZ / 2), and if keep
		 * busy for several consecutive times, we'll scale up the
		 * scan duration, But except in one case when we enable the
		 * slab scan. It's acceptable that the cpu load is very high
		 * for a while and we can not scale up the scan duration.
		 * Otherwise it will takes a lot of time to scan an round.
		 *
		 * Because kidled is the lowest priority, and it can be
		 * scheduled easily when other task want to run in current cpu.
		 *
		 * NOTE it's a simple guard, not a promise.
		 */
#define KIDLED_BUSY_RUNNING		(HZ / 2)
#define KIDLED_BUSY_LOOP_THRESHOLD	10
		elapsed = jiffies_64 - start_jiffies;
		if (elapsed < KIDLED_BUSY_RUNNING) {
			busy_loop = 0;
			schedule_timeout_interruptible(HZ - elapsed);
		} else if (++busy_loop == KIDLED_BUSY_LOOP_THRESHOLD) {
			busy_loop = 0;
			if (kidled_try_double_scan_control(scan_control)) {
				pr_warn_ratelimited("%s: period -> %u\n",
					__func__,
					kidled_get_current_scan_duration());
			}

			/* sleep for a while to relax cpu */
			schedule_timeout_interruptible(elapsed);
		}
	}

	return 0;
}

static inline bool kidled_allow_scan_slab(void)
{
	struct kidled_scan_control scan_control =
		kidled_get_current_scan_control();

	if (!scan_control.duration)
		return false;

	if (!kidled_has_slab_target(&scan_control))
		return false;

	return true;
}

static inline void kidled_slab_scan_enabled(void)
{
	if (!static_key_enabled(&kidled_slab_key)) {
		if (kidled_allow_scan_slab())
			static_branch_enable(&kidled_slab_key);
	} else {
		if (!kidled_allow_scan_slab())
			static_branch_disable(&kidled_slab_key);
	}
}

static inline unsigned short *kidled_slab_age(struct slab *slab)
{
	return (unsigned short *)((unsigned long)slab->obj_exts & ~MEMCG_DATA_SLAB_AGE);
}

bool page_has_slab_age(struct slab *slab)
{
	return (((unsigned long)slab->obj_exts & OBJEXTS_FLAGS_MASK) == MEMCG_DATA_SLAB_AGE);
}

static unsigned short *kidled_get_slab_age_array(void *object)
{
	struct slab *slab = virt_to_slab(object);
	unsigned int objects = objs_per_slab(slab->slab_cache, slab);
	unsigned short *slab_age = NULL;

	if (!kidled_available_slab(slab_folio(slab), slab->slab_cache))
		goto out;

	if (!cgroup_memory_nokmem) {
		/* In case fail to allocate memory for cold slab */
		if (likely(slab_obj_exts(slab)))
			slab_age = (unsigned short *)slab_obj_exts(slab)[objects].objcg;
	} else
		slab_age = kidled_slab_age(slab);

out:
	return slab_age;
}

unsigned short kidled_get_slab_age(void *object)
{
	unsigned short *slab_age;
	struct slab *slab;
	unsigned int off;

	if (!static_branch_unlikely(&kidled_slab_key))
		return 0;

	slab_age = kidled_get_slab_age_array(object);
	if (!slab_age)
		return 0;

	slab = virt_to_slab(object);
	off = obj_to_index(slab->slab_cache, slab, object);

	return *(slab_age + off);
}

void kidled_set_slab_age(void *object, unsigned short age)
{
	unsigned short *slab_age;
	struct slab *slab;
	unsigned int off;

	if (!static_branch_unlikely(&kidled_slab_key))
		return;

	slab_age = kidled_get_slab_age_array(object);
	if (!slab_age)
		return;

	slab = virt_to_slab(object);
	off = obj_to_index(slab->slab_cache, slab, object);

	*(slab_age + off) = age;
}

/*
 * each slab object pointer to an memcg respectively when kmem account enable,
 * slab page can be used by root mem_cgroup and children memcg. slab object
 * age is recorded in slab_age of page when kmem account disable. Otherwise,
 * an special obj_cgroups pointer will store the value.
 */

int kidled_alloc_slab_age(struct slab *slab, struct kmem_cache *s, gfp_t flags)
{
	unsigned int objects = objs_per_slab(s, slab);
	void *ver;
	int ret;

	if (!kidled_available_slab(slab_folio(slab), s))
		return 0;

	/* void count the memory to kmem accounting when kmem enable */
	flags &= ~OBJCGS_CLEAR_MASK;
	ver = kzalloc_node(objects * sizeof(unsigned short), flags, slab_nid(slab));
	if (!ver)
		return -ENOMEM;

	if (!cgroup_memory_nokmem) {
		if (!slab_obj_exts(slab)) {
			ret = alloc_slab_obj_exts(slab, s, flags, true);

			if (!ret)
				slab_obj_exts(slab)[objects].objcg = (struct obj_cgroup *)ver;
			else {
				kfree(ver);
				return -ENOMEM;
			}
		} else {
			slab_obj_exts(slab)[objects].objcg = (struct obj_cgroup *)ver;
		}
		return 0;
	}

	slab->obj_exts = ((unsigned long)ver | MEMCG_DATA_SLAB_AGE);
	return 0;
}

void kidled_free_slab_age(struct slab *slab)
{
	kfree(kidled_slab_age(slab));
	slab->obj_exts = 0;
}

static ssize_t kidled_scan_period_show(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       char *buf)
{
	return sprintf(buf, "%u\n", kidled_get_current_scan_duration());
}

bool is_kidled_setting(void)
{
	return !!atomic_read(&kidled_setting);
}

/*
 * We will update the real scan period and do reset asynchronously,
 * avoid stall when kidled is busy waiting for other resources.
 */
static ssize_t kidled_scan_period_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long secs;
	int ret;

	ret = kstrtoul(buf, 10, &secs);
	if (ret || secs > KIDLED_MAX_SCAN_DURATION)
		return -EINVAL;

	ret = count;
	/* Mark kidled as setting first */
	atomic_inc(&kidled_setting);
	if (lru_gen_is_setting()) {
		pr_warn("%s: Failed to enable kidled due to mglru is being set\n", __func__);
		ret = -EBUSY;
		goto out;
	}

	/* Disable kidled when mglru enabled */
	if (secs && lru_gen_enabled()) {
		pr_warn("%s: Failed to enable kidled due to mglru enabled\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	/*
	 * To avoid situation like lru_gen >= 0 && kidled disabled, disable
	 * enabled_key after reset.
	 */
	if (secs) {
		if (atomic_read(&kidled_disabling)) {
			pr_warn("%s: Failed to enable kidled due to kidled is disabling\n", __func__);
			ret = -EINVAL;
			goto out;
		}
		static_branch_enable(&kidled_enabled_key);
	} else {
		if (atomic_cmpxchg(&kidled_disabling, 0, 1) != 0) {
			pr_debug("%s: kidled is still being disabled, ignore this time\n", __func__);
			goto out;
		}
	}

	kidled_set_scan_duration(secs);
	wake_up_interruptible(&kidled_wait);
	kidled_slab_scan_enabled();
out:
	atomic_dec(&kidled_setting);

	return ret;
}

static ssize_t kidled_scan_target_show(struct kobject *kobj,
					     struct kobj_attribute *attr,
					     char *buf)
{
	return sprintf(buf, "%u\n", kidled_scan_target);
}

static ssize_t kidled_scan_target_store(struct kobject *kobj,
					      struct kobj_attribute *attr,
					      const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 10, &val);
	if (ret || !val || val > KIDLED_SCAN_ALL)
		return -EINVAL;

	WRITE_ONCE(kidled_scan_target, val);
	kidled_slab_scan_enabled();
	return count;
}

static struct kobj_attribute kidled_scan_period_attr =
	__ATTR(scan_period_in_seconds, 0644,
	       kidled_scan_period_show, kidled_scan_period_store);

static struct kobj_attribute kidled_scan_target_attr =
	__ATTR(scan_target, 0644,
	       kidled_scan_target_show, kidled_scan_target_store);

static struct attribute *kidled_attrs[] = {
	&kidled_scan_period_attr.attr,
	&kidled_scan_target_attr.attr,
	NULL
};
static struct attribute_group kidled_attr_group = {
	.name = "kidled",
	.attrs = kidled_attrs,
};

static int __init kidled_init(void)
{
	struct task_struct *thread;
	struct sched_param param = { .sched_priority = 0 };
	int ret;

	ret = sysfs_create_group(mm_kobj, &kidled_attr_group);
	if (ret) {
		pr_warn("%s: Error %d on creating sysfs files\n",
		       __func__, ret);
		return ret;
	}

	thread = kthread_run(kidled, NULL, "kidled");
	if (IS_ERR(thread)) {
		sysfs_remove_group(mm_kobj, &kidled_attr_group);
		pr_warn("%s: Failed to start kthread\n", __func__);
		return PTR_ERR(thread);
	}

	/* Make kidled as nice as possible. */
	sched_setscheduler(thread, SCHED_IDLE, &param);

	return 0;
}

module_init(kidled_init);

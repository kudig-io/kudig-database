// SPDX-License-Identifier: GPL-2.0

#include <linux/list.h>
#include <linux/xarray.h>
#include <linux/rcupdate.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/rmap.h>
#include <linux/pagemap.h>
#include <linux/pagevec.h>
#include <linux/migrate.h>
#include <linux/memcontrol.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/page_dup.h>
#include <linux/cpuset.h>
#include <linux/swap.h>
#include <linux/sched/mm.h>

#include "internal.h"

DEFINE_STATIC_KEY_FALSE(duptext_enabled_key);
struct xarray dup_folios[MAX_NUMNODES];

#define DUPTEXT_REFRESH_KICK 0

struct duptext_refresh {
	struct delayed_work dwork;
	struct mm_struct *mm;
};

static void duptext_refresh_workfn(struct work_struct *work);

/* XXX A variant of folio_copy for copying in atomic context */
static void folio_copy_atomic(struct folio *dst, struct folio *src)
{
	long i = 0;
	long nr = folio_nr_pages(src);

	for (;;) {
		copy_highpage(folio_page(dst, i), folio_page(src, i));
		if (++i == nr)
			break;
	}
}

static inline void attach_dup_folio_private(struct folio *dup_folio,
		struct folio *folio)
{
	dup_folio->private = folio;
	folio_set_private(dup_folio);

	folio_set_dup_slave(dup_folio);
}

static inline void detach_dup_folio_private(struct folio *dup_folio)
{
	folio_clear_private(dup_folio);
	dup_folio->private = 0;

	folio_clear_dup_slave(dup_folio);
}

static struct folio *find_get_dup_folio(struct folio *folio, int node)
{
	struct folio *dup_folio, *tmp_folio;
	struct list_head *list;
	int nid = folio_nid(folio);

	XA_STATE(xas, &dup_folios[nid], folio_pfn(folio));

	rcu_read_lock();
repeat:
	dup_folio = NULL;
	xas_reset(&xas);
	list = xas_load(&xas);
	if (xas_retry(&xas, list))
		goto repeat;

	if (!list)
		goto out;

	list_for_each_entry(tmp_folio, list, lru) {
		if (folio_nid(tmp_folio) == node) {
			dup_folio = tmp_folio;
			break;
		}
	}

	if (dup_folio && !folio_try_get(dup_folio))
		goto repeat;

out:
	rcu_read_unlock();
	return dup_folio;
}

static int add_to_dup_folios(struct folio *new_folio, struct folio *folio)
{
	struct list_head *list;
	unsigned long flags;
	int ret = 0;
	int nid = folio_nid(folio);
	int nr_pages = folio_nr_pages(folio);

	XA_STATE(xas, &dup_folios[nid], folio_pfn(folio));

	folio_get(new_folio);
	xas_lock_irqsave(&xas, flags);

	/*
	 * Check the global enabled key inside xa_lock, in order to ensure
	 * this dup_folio not to be added, or truncation not to miss this
	 * dup_folio.
	 */
	if (!static_branch_likely(&duptext_enabled_key)) {
		ret = -EBUSY;
		goto out;
	}

	list = xas_load(&xas);
	if (!list) {
		list = kmalloc_node(sizeof(struct list_head), GFP_ATOMIC, nid);
		if (!list) {
			ret = -ENOMEM;
			goto out;
		}

		INIT_LIST_HEAD(list);
		xas_store(&xas, list);
	}

	new_folio->mapping = folio->mapping;
	new_folio->index = folio->index;
	attach_dup_folio_private(new_folio, folio);
	folio_set_dup(new_folio);
	list_add(&new_folio->lru, list);

	if (!folio_dup_master(folio))
		folio_set_dup(folio);
	__node_stat_mod_folio(folio, NR_DUPTEXT, nr_pages);
	filemap_nr_duptext_add(folio_mapping(folio), nr_pages);

out:
	xas_unlock_irqrestore(&xas, flags);
	if (unlikely(ret))
		folio_put(new_folio);
	return ret;
}

static void __delete_from_dup_folios(struct folio *dup_folio, struct folio *folio)
{
	struct address_space *mapping = folio_mapping(dup_folio);
	int nr_pages = folio_nr_pages(folio);

	list_del(&dup_folio->lru);
	folio_clear_dup(dup_folio);
	detach_dup_folio_private(dup_folio);
	dup_folio->mapping = NULL;
	dup_folio->index = 0;
	folio_put(dup_folio);
	__node_stat_mod_folio(folio, NR_DUPTEXT, -nr_pages);
	filemap_nr_duptext_add(mapping, -nr_pages);
}

static bool delete_from_dup_folios(struct folio *folio, bool locked, bool ignore_mlock)
{
	struct folio *tmp_folio, *next_folio;
	struct list_head *list, *old;
	unsigned long flags;
	enum ttu_flags ttu_flags = TTU_SYNC | TTU_BATCH_FLUSH;
	int nid = folio_nid(folio);
	unsigned int order = folio_order(folio);

	VM_BUG_ON_FOLIO(!folio_test_locked(folio), folio);

	XA_STATE(xas, &dup_folios[nid], folio_pfn(folio));

	xas_lock_irqsave(&xas, flags);
	list = xas_load(&xas);
	if (!list) {
		xas_unlock_irqrestore(&xas, flags);
		goto out;
	}
	xas_store(&xas, NULL);
	xas_unlock_irqrestore(&xas, flags);

	if (locked)
		ttu_flags |= TTU_RMAP_LOCKED;
	if (ignore_mlock)
		ttu_flags |= TTU_IGNORE_MLOCK;
	if (unlikely(folio_test_pmd_mappable(folio)))
		ttu_flags |= TTU_SPLIT_HUGE_PMD;

	list_for_each_entry_safe(tmp_folio, next_folio, list, lru) {
		/* Dup master folio or dup slave folio has been splited */
		VM_BUG_ON_FOLIO(folio_order(tmp_folio) != order, tmp_folio);

		VM_BUG_ON_FOLIO(!folio_dup_slave(tmp_folio), tmp_folio);

		/* Unmap before delete */
		if (folio_mapped(tmp_folio)) {
			folio_lock(tmp_folio);
			try_to_unmap(tmp_folio, ttu_flags);
			if (folio_mapped(tmp_folio)) {
				folio_unlock(tmp_folio);
				goto error;
			}
			folio_unlock(tmp_folio);
		}

		__delete_from_dup_folios(tmp_folio, folio);
		folio_put(tmp_folio);
	}

	kfree(list);
out:
	folio_clear_dup(folio);
	return true;

error:
	xas_lock_irqsave(&xas, flags);
repeat:
	xas_reset(&xas);
	old = xas_load(&xas);
	if (xas_retry(&xas, old))
		goto repeat;
	VM_BUG_ON_FOLIO(old != NULL, folio);
	xas_store(&xas, list);
	xas_unlock_irqrestore(&xas, flags);
	return false;
}

#ifdef CONFIG_MEMCG
static inline bool memcg_allow_duptext(struct mm_struct *mm)
{
	struct mem_cgroup *memcg;
	bool allow_duptext = false;

	memcg = get_mem_cgroup_from_mm(mm);
	if (memcg) {
		allow_duptext = memcg->allow_duptext;
		css_put(&memcg->css);
	}

	return allow_duptext;
}

static inline bool memcg_allow_duptext_refresh(struct mm_struct *mm)
{
	struct mem_cgroup *memcg;
	bool allow_duptext_refresh = false;

	memcg = get_mem_cgroup_from_mm(mm);
	if (memcg) {
		allow_duptext_refresh = memcg->allow_duptext_refresh;
		css_put(&memcg->css);
	}

	return allow_duptext_refresh;
}

static inline int duptext_target_node(struct mm_struct *mm, int folio_node)
{
	struct mem_cgroup *memcg;
	nodemask_t allowed_nodes = cpuset_current_mems_allowed;
	int target_node = numa_node_id();

	memcg = get_mem_cgroup_from_mm(mm);
	if (memcg) {
		nodes_and(allowed_nodes, allowed_nodes, memcg->duptext_nodes);
		css_put(&memcg->css);
	}

	if (unlikely(nodes_empty(allowed_nodes)))
		return folio_node;

	if (!node_isset(target_node, allowed_nodes)) {
		if (!node_isset(folio_node, allowed_nodes))
			target_node = first_node(allowed_nodes);
		else
			target_node = folio_node;
	}

	return target_node;
}
#else
static inline bool memcg_allow_duptext(struct mm_struct *mm)
{
	return true;
}

static inline bool memcg_allow_duptext_refresh(struct mm_struct *mm)
{
	return false;
}

static inline int duptext_target_node(struct mm_struct *mm, int folio_node)
{
	return folio_node;
}
#endif

bool __dup_folio_suitable(struct vm_area_struct *vma, struct mm_struct *mm)
{
	/* Is executable file? */
	if ((vma->vm_flags & VM_EXEC) && vma->vm_file)  {
		struct inode *inode = vma->vm_file->f_inode;

		/* Is read-only ? */
		if (!S_ISREG(inode->i_mode) || inode_is_open_for_write(inode))
			return false;

		/* Memcg allow duptext ? */
		return memcg_allow_duptext(mm);
	}

	return false;
}

struct folio *__dup_folio_master(struct folio *folio)
{
	if (!folio_dup_slave(folio))
		return folio;

	return folio_get_private(folio);
}

bool __dup_folio_mapped(struct folio *folio)
{
	struct folio *tmp_folio;
	struct list_head *list;
	bool ret = false;
	int nid = folio_nid(folio);
	unsigned int order = folio_order(folio);

	XA_STATE(xas, &dup_folios[nid], 0);

	VM_BUG_ON_FOLIO(!folio_test_locked(folio), folio);

	if (!folio_dup_master(folio))
		return false;
	xas_set(&xas, folio_pfn(folio));

	rcu_read_lock();
repeat:
	xas_reset(&xas);
	list = xas_load(&xas);
	if (xas_retry(&xas, list))
		goto repeat;

	if (!list)
		goto out;

	list_for_each_entry(tmp_folio, list, lru) {
		/* Dup master folio or dup slave folio has been splited */
		VM_BUG_ON_FOLIO(folio_order(tmp_folio) != order, folio);

		if (folio_mapped(tmp_folio)) {
			ret = true;
			break;
		}
	}

out:
	rcu_read_unlock();
	return ret;
}

/* NOTE @page can be any order folio */
struct folio *__dup_folio(struct folio *folio, struct vm_area_struct *vma)
{
	struct address_space *mapping = folio_mapping(folio);
	struct folio *dup_folio = NULL;
	struct mm_struct *mm = current->mm;
	int folio_node = folio_nid(folio);
	int target_node;

	VM_BUG_ON_FOLIO(!folio_test_locked(folio), folio);

	if (is_zero_folio(folio))
		return NULL;

	if (!__dup_folio_suitable(vma, mm))
		return NULL;

	target_node = duptext_target_node(mm, folio_node);
	if (likely(folio_node == target_node))
		return NULL;

	if (unlikely(folio_test_dirty(folio) || folio_test_writeback(folio) ||
				!folio_test_uptodate(folio))) {
		struct duptext_refresh *refresh;
		int delay_ms;

		if (memcg_allow_duptext_refresh(mm) &&
				!test_bit(DUPTEXT_REFRESH_KICK, &mm->duptext_flags)) {
			refresh = kmalloc(sizeof(struct duptext_refresh), GFP_ATOMIC);
			if (!refresh)
				return NULL;

			if (test_and_set_bit(DUPTEXT_REFRESH_KICK, &mm->duptext_flags)) {
				kfree(refresh);
				return NULL;
			}

			mmgrab(mm);
			refresh->mm = mm;
			INIT_DELAYED_WORK(&refresh->dwork, duptext_refresh_workfn);
			/*
			 * Dirty page lasts (dirty_writeback_interval +
			 * dirty_expire_interval) centiseconds at most,
			 * if the writeback time doesn't count.
			 */
			delay_ms = (dirty_writeback_interval + dirty_expire_interval) * 10;
			schedule_delayed_work(&refresh->dwork, msecs_to_jiffies(delay_ms));
		}
		return NULL;
	}

	if (folio_needs_release(folio)) {
		if (!filemap_release_folio(folio, GFP_ATOMIC))
			return NULL;
	}

	if (folio_dup_master(folio))
		dup_folio = find_get_dup_folio(folio, target_node);

	if (!dup_folio) {
		/*
		 * XXX GFP_ATOMIC is used, since dup_folio is called
		 * inside rcu lock in filemap_map_pages.
		 */
		gfp_t gfp_mask = GFP_ATOMIC | __GFP_THISNODE;
		unsigned int order = 0;
		struct folio *new_folio = NULL;
		int ret;

		if (folio_test_large(folio)) {
			gfp_mask |= __GFP_COMP | __GFP_NOMEMALLOC | __GFP_NOWARN | __GFP_MOVABLE;
			order = folio_order(folio);
		}

		new_folio = __folio_alloc_node(gfp_mask, order, target_node);
		if (!new_folio)
			return NULL;

		if (folio_nid(new_folio) != target_node) {
			folio_put(new_folio);
			return NULL;
		}

		folio_copy_atomic(new_folio, folio);

		ret = add_to_dup_folios(new_folio, folio);
		if (ret) {
			folio_put(new_folio);
			return NULL;
		}

		/*
		 * Paired with smp_mb() in do_dentry_open() to ensure
		 * i_writecount is up to date and the update to nr_duptext
		 * is visible. Ensures the page cache will be truncated if
		 * the file is opened writable.
		 */
		smp_mb();
		if (inode_is_open_for_write(mapping->host)) {
			__delete_from_dup_folios(new_folio, folio);
			folio_put(new_folio);
			return NULL;
		}

		dup_folio = new_folio;
		folio_get(dup_folio);
	}

	/* dup_folio is returned with refcount increased, but !PageLocked */
	return dup_folio;
}

/*
 * NOTE Be careful if you want to call __dedup_folio with ignore_mlock as false.
 *
 * Truncating routines should always succeed, perhaps with multiple attempts,
 * usually accompanied by unmap_mapping_range().  In order to coordinate with
 * truncating, __dedup_folio() should better succeed in this scenario, with
 * ignore_mlock as true.
 *
 * On the other hand, invalidating routines have __dup_folio_mapped() check in
 * common helper, i.e., invalidate_inode_page(). That is to say, mapped slave
 * pages should not be invalidated irrespective of whether mlocked or not.
 *
 * Finally the only place currently where mlock is honoured is reclaiming
 * routines, e.g., shrink_folio_list, where __dedup_folio() can be called with
 * ignore_mlock as false.
 */
bool __dedup_folio(struct folio *folio, bool locked, bool ignore_mlock)
{
	VM_BUG_ON_FOLIO(!folio_test_locked(folio), folio);

	if (!folio_dup_master(folio))
		return true;
	return delete_from_dup_folios(folio, locked, ignore_mlock);
}

static unsigned int find_get_master_folios(struct xa_state *xas,
		struct folio_batch *fbatch, unsigned long end_pfn)
{
	struct page *page;
	struct folio *folio;
	struct list_head *entry;

	folio_batch_init(fbatch);

	rcu_read_lock();
	for (;;) {
		entry = xas_find(xas, end_pfn);
		if (!entry)
			break;

		if (xas_retry(xas, entry))
			continue;

		page = pfn_to_online_page(xas->xa_index);
		if (!page)
			continue;

		folio = page_folio(page);
		if (!folio_try_get(folio))
			continue;

		VM_BUG_ON_FOLIO(!folio_dup_master(folio), folio);

		if (unlikely(page_folio(page) != folio)) {
			folio_put(folio);
			continue;
		}

		if (unlikely(entry != xas_reload(xas))) {
			folio_put(folio);
			xas_reset(xas);
			continue;
		}

		if (folio_batch_add(fbatch, folio) == 0)
			break;

	}
	rcu_read_unlock();

	return folio_batch_count(fbatch);
}

static void truncate_dup_folios(void)
{
	int nid;

	for_each_online_node(nid) {
		unsigned long start_pfn = node_start_pfn(nid);
		unsigned long end_pfn = node_end_pfn(nid);
		struct folio_batch fbatch;
		struct folio *folio;
		int i;

		/* TODO: Update the start_pfn to optimize the XArray traversal*/
		XA_STATE(xas, &dup_folios[nid], start_pfn);
		while (find_get_master_folios(&xas, &fbatch, end_pfn)) {
			for (i = 0; i < folio_batch_count(&fbatch); i++) {
				folio = fbatch.folios[i];

				folio_lock(folio);
				__dedup_folio(folio, false, true);
				folio_unlock(folio);
				folio_put(folio);

				cond_resched();
			}
		}
	}
}

static int __init setup_duptext(char *s)
{
	if (!strcmp(s, "1"))
		static_branch_enable(&duptext_enabled_key);
	else if (!strcmp(s, "0"))
		static_branch_disable(&duptext_enabled_key);
	return 1;
}
__setup("duptext=", setup_duptext);

#ifdef CONFIG_SYSFS
static ssize_t duptext_enabled_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!static_branch_unlikely(&duptext_enabled_key));
}
static ssize_t duptext_enabled_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	static DEFINE_MUTEX(mutex);
	ssize_t ret = count;

	mutex_lock(&mutex);
	if (!strncmp(buf, "1", 1))
		static_branch_enable(&duptext_enabled_key);
	else if (!strncmp(buf, "0", 1)) {
		int nid;

		static_branch_disable(&duptext_enabled_key);
		/*
		 * Grab xa_lock of each dup_folios xarray after disable the
		 * global enabled key, in order to prevent new dup_folio from
		 * being added, or wait for all inflight dup_folio to be added.
		 *
		 * On the other hand, PG_locked will serialize
		 * page_add_file_rmap() and truncate_dup_folios() for each
		 * identical page.
		 */
		for_each_online_node(nid) {
			xa_lock(&dup_folios[nid]);
			xa_unlock(&dup_folios[nid]);
		}

		truncate_dup_folios();
	} else {
		ret = -EINVAL;
	}

	mutex_unlock(&mutex);
	return ret;
}
static struct kobj_attribute duptext_enabled_attr =
__ATTR(enabled, 0644, duptext_enabled_show,
		duptext_enabled_store);

static struct attribute *duptext_attrs[] = {
	&duptext_enabled_attr.attr,
	NULL,
};

static struct attribute_group duptext_attr_group = {
	.attrs = duptext_attrs,
};

static int __init duptext_init_sysfs(void)
{
	int err;
	struct kobject *duptext_kobj;

	duptext_kobj = kobject_create_and_add("duptext", mm_kobj);
	if (!duptext_kobj) {
		pr_err("failed to create duptext kobject\n");
		return -ENOMEM;
	}
	err = sysfs_create_group(duptext_kobj, &duptext_attr_group);
	if (err) {
		pr_err("failed to register duptext group\n");
		goto delete_obj;
	}
	return 0;

delete_obj:
	kobject_put(duptext_kobj);
	return err;
}
#endif /* CONFIG_SYSFS */

static int __init duptext_init(void)
{
	int ret = 0, nid;

	for_each_node(nid)
		xa_init_flags(&dup_folios[nid], XA_FLAGS_LOCK_IRQ);

#ifdef CONFIG_SYSFS
	ret = duptext_init_sysfs();
#endif

	return ret;
}
module_init(duptext_init);

/*
 * Currently the way to refresh duptext, in order to apply duptext to
 * pages which were dirty, wirteback, or !uptodate before, is simply
 * to zap the page range of corresponding vma, and then make process
 * fault again.
 *
 * FIXME Optimize with walk_page_range() if obvious overhead is found
 * in current implementation. However, there are a few points to note.
 * 1. How to determine "numa_node_id()" of specific mm, in the context
 *    of asynchronous work.
 * 2. Implementation with walk_page_range() is complex and error-prone.
 */
static void duptext_refresh_mm(struct mm_struct *mm)
{
	struct vm_area_struct *vma;
	struct vma_iterator vmi;

	mmap_read_lock(mm);
	vma_iter_init(&vmi, mm, 0);
	for_each_vma(vmi, vma) {
		if (!__dup_folio_suitable(vma, mm))
			continue;
		zap_page_range_single(vma, vma->vm_start,
				vma->vm_end - vma->vm_start, NULL);
		cond_resched();
	}
	mmap_read_unlock(mm);
}

static void duptext_refresh_workfn(struct work_struct *work)
{
	struct duptext_refresh *refresh = container_of(to_delayed_work(work),
			struct duptext_refresh, dwork);
	struct mm_struct *mm = refresh->mm;

	if (!duptext_enabled() || atomic_read(&mm->mm_users) == 0)
		goto out;

	duptext_refresh_mm(mm);

out:
	mmdrop(mm);
	kfree(refresh);
}

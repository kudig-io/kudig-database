// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mm_inline.h>
#include <linux/mmu_notifier.h>
#include <linux/oom.h>
#include <linux/rmap.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/hugetlb.h>
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/userfaultfd_k.h>

#include <asm/tlb.h>
#include <asm/tlbflush.h>

#include "internal.h"

static inline void recover_pmd_range(pud_t *src_pud, unsigned long addr,
				     unsigned long end,
				     struct vm_area_struct *src_vma)
{
	pmd_t *src_pmd;
	unsigned long next;

	src_pmd = pmd_offset(src_pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		/*
		 * We cannot use pmd_none_or_clear_bad() in advance, because
		 * hugepage does not follow _KERNPG_TABLE restriction.
		 * Instead, hugepage is checked in is_pmd_copied_slow().
		 */
		if (pmd_none(*src_pmd))
			continue;
		if (is_pmd_copied_slow(*src_pmd)) {
			/*
			 * [addr, end) must be 2M aligned and continuous.
			 * Note that a bad pmd will cause memory leak.
			 */
			if ((addr & ~PMD_MASK) || next != (addr + PMD_SIZE)) {
				WARN_ON(1);
				continue;
			}
			pmdp_clear_wp(src_pmd, src_vma);
		}
	} while (src_pmd++, addr = next, addr != end);
}

static inline void recover_pud_range(p4d_t *src_p4d, unsigned long addr,
				     unsigned long end,
				     struct vm_area_struct *src_vma)
{
	pud_t *src_pud;
	unsigned long next;

	src_pud = pud_offset(src_p4d, addr);
	do {
		next = pud_addr_end(addr, end);
		if (pud_trans_huge(*src_pud) || pud_devmap(*src_pud))
			continue;
		if (pud_none_or_clear_bad(src_pud))
			continue;
		recover_pmd_range(src_pud, addr, next, src_vma);
	} while (src_pud++, addr = next, addr != end);
}

static inline void recover_p4d_range(pgd_t *src_pgd, unsigned long addr,
				     unsigned long end,
				     struct vm_area_struct *src_vma)
{
	p4d_t *src_p4d;
	unsigned long next;

	src_p4d = p4d_offset(src_pgd, addr);
	do {
		next = p4d_addr_end(addr, end);
		if (p4d_none_or_clear_bad(src_p4d))
			continue;
		recover_pud_range(src_p4d, addr, next, src_vma);
	} while (src_p4d++, addr = next, addr != end);
}

static inline void recover_page_range(struct mm_struct *src_mm,
				      struct vm_area_struct *src_vma)
{
	pgd_t *src_pgd;
	unsigned long next;
	unsigned long addr = src_vma->vm_start;
	unsigned long end = src_vma->vm_end;

	src_pgd = pgd_offset(src_mm, addr);
	do {
		next = pgd_addr_end(addr, end);
		if (pgd_none_or_clear_bad(src_pgd))
			continue;
		recover_p4d_range(src_pgd, addr, next, src_vma);
	} while (src_pgd++, addr = next, addr != end);
}

/*
 * If async fork is done or fails, needs to clear parent's PMDs
 * WP attribute, reset each other's mm->async_fork_mm and
 * vma->async_fork_vma.
 *
 * @mm: child's mm
 * @oldmm: parent's mm
 */
static inline void async_fork_recover_mm(struct mm_struct *mm,
					 struct mm_struct *oldmm)
{
	struct vm_area_struct *vma, *mpnt;

	VMA_ITERATOR(vmi, mm, 0);

	for_each_vma(vmi, vma) {
		mpnt = vma->async_fork_vma;
		if (!mpnt || IS_ERR(mpnt))
			continue;

		/*
		 * We should reset parent mpnt->async_fork_vma and recover
		 * parent pmd from async_fork status, skip TLB flush even we
		 * modify the page table because it is only changed from ro
		 * to rw.
		 */
		recover_page_range(oldmm, mpnt);
		smp_wmb(); /* See comment in __pte_alloc() */
		vma->async_fork_vma = NULL;
		mpnt->async_fork_vma = NULL;
		cond_resched();
	}
}

/*
 * Async fork fast path by using copy_page_range_fast(),
 * so parent can return user space ASAP
 *
 * This function is only called by parent in fork(2).
 *
 * @vma: child's vma
 * @mpnt: parent's vma
 */
int async_fork_cpr_fast(struct vm_area_struct *vma,
			struct vm_area_struct *mpnt)
{
	int ret;

	ret = copy_page_range_fast(vma, mpnt);
	/*
	 * VMA_FAST_COPIED means the copy did occurred,
	 * bind their vma, but ret indicates the result of copying.
	 */
	if (mpnt->async_fork_vma == VMA_FAST_COPIED) {
		vma->async_fork_vma = mpnt;
		mpnt->async_fork_vma = vma;
	}

	return ret;
}


/*
 * async fork has done or failed. In both scenarios, pairing
 * async_fork_mm and async_fork_vma must be reset, pmds WP
 * attribute has to be cleared.
 *
 * This function are called at:
 *   1. parent's dup_mmap().
 *   2. child's schedule_tail() which is doing rest of mm copying.
 *
 * @mm: child's mm
 * @recover: reset async_fork_{mm,vma} and clear PMDs WP attribute or not
 * @locked: caller has hold parent's mmap write lock or not
 */
void async_fork_cpr_done(struct mm_struct *mm, bool recover, bool locked)
{
	struct mm_struct *oldmm = mm->async_fork_mm;

	/* locked == true means in fork path, otherwise in schedule_tail */
	if (recover) {
		/* Hold parent's mmap read lock to avoid warning */
		if (!locked)
			mmap_read_lock(oldmm);

		/*
		 * If in child's schedule_tail path, parent fixup path can
		 * modify page table concurrently in its fixup path. The
		 * former caller must be holding child's read lock when
		 * copy page range, so must acquire its write lock to avoid
		 * race.
		 */
		mmap_write_lock_nested(mm, SINGLE_DEPTH_NESTING);
		async_fork_recover_mm(mm, oldmm);
		mmap_write_unlock(mm);
		if (!locked)
			mmap_read_unlock(oldmm);
	}

	if (!locked)
		mmap_write_lock(oldmm);

	mm->async_fork_mm = NULL;
	mm->async_fork_flags = 0;

	/* In case there's new fork in parent, must check before clear it */
	if (oldmm->async_fork_mm == mm) {
		oldmm->async_fork_mm = NULL;
		oldmm->async_fork_flags = 0;
		atomic_set(&oldmm->async_fork_refcnt, 0);
	}

#ifdef CONFIG_ARM64
	flush_tlb_mm(oldmm);
#endif

	if (!locked)
		mmap_write_unlock(oldmm);
	mmput(oldmm); /* paired with mmget() in async_fork_cpr_bind() */
}

/*
 * Call in parent's fork path to bind pairing async_fork_mm
 *
 * @oldmm: parent's mm
 * @mm: child's mm
 * @err: error in dup_mmap()
 *
 * NOTE: parent's mm is got referenced and will be dereferenced in
 * async_fork_cpr_done()
 */
void async_fork_cpr_bind(struct mm_struct *oldmm, struct mm_struct *mm,
			 int err)
{
	mmget(oldmm);
	mm->async_fork_mm = oldmm;

	if (!err) {
		set_bit(ASYNC_FORK_CHILD, &mm->async_fork_flags);

		oldmm->async_fork_mm = mm;
		set_bit(ASYNC_FORK_PARENT, &oldmm->async_fork_flags);
		atomic_set(&oldmm->async_fork_refcnt, 0);
	} else
		async_fork_cpr_done(mm, true, true);
}

static inline bool async_fork_check_vma(struct vm_area_struct *mpnt)
{
	struct mm_struct *oldmm = mpnt->vm_mm;
	struct vm_area_struct *vma = mpnt->async_fork_vma;
	struct mm_struct *mm;
	bool ret = true;

	if (unlikely(!vma || vma->async_fork_vma != mpnt)) {
		pr_err("async_fork: mismatched vma: process %d: mpnt: %pK, "
		       "mpnt->async_fork_vma: %pK, mpnt->async_fork_vma->async_fork_vma: %pK\n",
		       current->pid, mpnt,
		       vma, vma ? vma->async_fork_vma : NULL);
		return false;
	}

	if (unlikely((vma->vm_start != mpnt->vm_start) ||
		     (vma->vm_end != mpnt->vm_end))) {
		pr_err("async_fork: mismatched vma range: process %d: "
		       "parent: [0x%lx, 0x%lx), child: [0x%lx, 0x%lx)\n",
		       current->pid,
		       mpnt->vm_start, mpnt->vm_end,
		       vma->vm_start, vma->vm_end);
		ret = false;
	}

	mm = vma->vm_mm;
	if (unlikely((oldmm->async_fork_mm != mm) || (mm->async_fork_mm != oldmm))) {
		pr_err("async_fork: mismatched mm: process %d: "
		       "parent: oldmm: %pK, oldmm->async_fork_mm: %pK, "
		       "oldmm->async_fork_mm->async_fork_mm: %pK "
		       "child: mm: %pK, mm->async_fork_mm: %pK, "
		       "mm->async_fork_mm->async_fork_mm: %pK\n",
		       current->pid,
		       oldmm, oldmm->async_fork_mm,
		       oldmm->async_fork_mm ? oldmm->async_fork_mm->async_fork_mm : NULL,
		       mm, mm->async_fork_mm,
		       mm->async_fork_mm ? mm->async_fork_mm->async_fork_mm : NULL);
		ret = false;
	}

	return ret;
}

/*
 * Real page table copy function, to copy whole vma in slow path
 *
 * @mm: child's mm
 * @vma: child's dst vma
 * @mpnt: parent's src vma
 *
 * This function can be called concurrently by both child and parent,
 * so caller must hold parent's mmap read or write lock.
 */
static int async_fork_copy_vma(struct mm_struct *mm, struct vm_area_struct *vma,
			       struct vm_area_struct *mpnt)
{
	struct mm_struct *oldmm;
	int retval = 0;

	mmap_read_lock_nested(mm, SINGLE_DEPTH_NESTING + 1);
	if (!mpnt) {
		mpnt = vma->async_fork_vma;
		/* mpnt has been copied or error occurred */
		if (!mpnt || IS_ERR(mpnt)) {
			retval = PTR_ERR(mpnt);
			goto out_readunlock;
		}
	} else if (!vma) {
		vma = mpnt->async_fork_vma;
		/* vma has been copied */
		if (!vma)
			goto out_readunlock;
	}

	oldmm = mpnt->vm_mm;

	BUG_ON(!async_fork_check_vma(mpnt));

	retval = copy_page_range_slow(vma, mpnt);
	if (retval)
		pr_err_ratelimited("async_fork: process %d copy vma %pK failed %d\n",
				   current->pid, mpnt, retval);
	mmap_read_unlock(mm);

	mmap_write_lock_nested(mm, SINGLE_DEPTH_NESTING + 1);
	if (mpnt->async_fork_vma) {
		if (retval) {
			recover_page_range(oldmm, mpnt);
			vma->async_fork_vma = ERR_PTR(-EFAULT);
		} else {
			vma->async_fork_vma = NULL;
		}
		smp_wmb(); /* See comment in __pte_alloc() */
		mpnt->async_fork_vma = NULL;
	}
	mmap_write_unlock(mm);
	return retval;

out_readunlock:
	mmap_read_unlock(mm);
	return retval;
}

/*
 * Scenarios
 *   This fixes up the vma managed by async_fork, which is going to be modified.
 *   This covers APIs which will change vma from userspace as follows.
 *     1. stack expanding.
 *     2. fork(2) which duplicates a new vma.
 *     3. mmap(2) a new vma which overlaps an old vma.
 *     4. munamp(2) an old vma.
 *     5. mlock(2)/mremap(2)/madvise(2)/mprotect(2) which will modify the
 *        vma flag, range, or corresponding page tables.
 *     6. brk(2)
 * @mptn: parent's src vma
 *
 * NOTE: assume it is only called by parent's context, if any other context
 * is calling it there could be a bug.
 */
void async_fork_fixup_vma(struct vm_area_struct *mpnt)
{
	if (unlikely(READ_ONCE(mpnt->async_fork_vma))) {
		async_fork_copy_vma(mpnt->vm_mm->async_fork_mm, NULL, mpnt);
#ifdef CONFIG_ARM64
		flush_tlb_range(mpnt, mpnt->vm_start, mpnt->vm_end);
#endif
	}
}

/*
 * Scenarios
 *   This fixes up the pmd managed by async_fork, which is going to be changed.
 *   Cow fault is such a typical scenario. Moreover, zap routines called
 *   by oom killer will directily (i.e., __oom_reap_task_mm) change such
 *   pmds. We should fix up such pmds in advance.
 *
 *   GUP is special, which will hold a reference of page, then send it to
 *   a device driver for read or write (e.g. DMA). Similarly, fix up the
 *   pmds before getting a page's reference.
 *
 * @mpnt: parent's src vma
 * @pmd: parent's pmd of addr
 * @addr: target addr
 *
 * NOTE: assume it is only called by parent's context, if any other context
 * is calling it there could be a bug.
 */
void async_fork_fixup_pmd(struct vm_area_struct *mpnt, pmd_t *pmd,
			  unsigned long addr)
{
	struct mm_struct *oldmm = mpnt->vm_mm;
	struct mm_struct *mm = oldmm->async_fork_mm;
	pmd_t orig_pmd = *pmd;
	int ret;

	if (!is_pmd_copied_slow(orig_pmd) || WARN_ON_ONCE(!mm))
		return;

	mmap_read_lock_nested(mm, SINGLE_DEPTH_NESTING);
	/* check again to avoid race with slowpath or recover routine */
	orig_pmd = READ_ONCE(*pmd);
	if (!is_pmd_copied_slow(orig_pmd)) {
		mmap_read_unlock(mm);
		return;
	}

	BUG_ON(!async_fork_check_vma(mpnt));

	ret = __copy_page_range(mpnt->async_fork_vma, mpnt, addr & PMD_MASK,
				(addr & PMD_MASK) + PMD_SIZE, CPR_SLOW);
	mmap_read_unlock(mm);

	if (!ret) {
#ifdef CONFIG_ARM64
		/*
		 * X86 invalidates stale tlb when page fault, However ARM64
		 * seems not to have this guarantee.  For insurance, flush
		 * corresponding tlb range in CPR_SLOW mode on ARM64 platform.
		 *
		 * On the other hand, flush_tlb_mm after dup_mmap is invoked
		 * in the CPR_FAST mode.
		 */
		flush_tlb_range(mpnt, addr & PMD_MASK,
				(addr & PMD_MASK) + PMD_SIZE);
#endif
	} else {
		/* fixup (recover) mpnt if error occurred */
		async_fork_fixup_vma(mpnt);
	}
}

/*
 * Real page table copying called by child in schedule_tail().
 * child is still in kernel space, so there's no need to hold
 * any mmap write lock.
 * However parent can call fixup functions to modify vma
 * conccurently, child has to hold parent's mmap read lock.
 */
void async_fork_cpr_rest(void)
{
	struct task_struct *p;
	struct mm_struct *mm, *oldmm;
	struct vm_area_struct *vma;
	int retval = 0;

	mm = current->mm;
	p = current;

	oldmm = mm->async_fork_mm;
	BUG_ON(!oldmm);

	mmap_read_lock(oldmm);
	VMA_ITERATOR(vmi, mm, 0);
	for_each_vma(vmi, vma) {
		/*
		 * Don't duplicate remaining vmas is we are going to
		 * die, e.g., oom killed.
		 */
		if (fatal_signal_pending(p)) {
			retval = -EINTR;
			break;
		}
		if (!READ_ONCE(vma->async_fork_vma))
			continue;
		retval = async_fork_copy_vma(mm, vma, NULL);
		if (retval)
			break;
	}
	mmap_read_unlock(oldmm);

	async_fork_cpr_done(mm, retval, false);

	if (retval)
		send_sig(SIGKILL, p, 1);
}

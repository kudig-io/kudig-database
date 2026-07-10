// SPDX-License-Identifier: GPL-2.0-only
/*
 * Pin Process Code Section:
 *   echo PID > /proc/unevictable/add_pid
 *   echo PID > /proc/unevictable/del_pid
 *   cat /proc/unevictable/add_pid
 *
 * Copyright (C) 2019 Alibaba
 * Author: Xunlei Pang <xlpang@linux.alibaba.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/types.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/sched/mm.h>
#include <linux/swap.h>
#include <linux/ksm.h>
#include <linux/pgtable.h>
#include <linux/hugetlb.h>
#include <linux/rbtree.h>
#include <linux/uaccess.h>
#include <linux/kprobes.h>
#include <linux/workqueue.h>
#include <linux/pid_namespace.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_TEXT_UNEVICTABLE
#include <linux/unevictable.h>
#endif

#define PROC_NAME	"unevictable"
#define NAME_BUF	8

#ifdef CONFIG_TEXT_UNEVICTABLE
DEFINE_STATIC_KEY_FALSE(unevictable_enabled_key);

#define for_each_mem_cgroup(iter)			\
	for (iter = mem_cgroup_iter(NULL, NULL, NULL);	\
	     iter != NULL;				\
	     iter = mem_cgroup_iter(NULL, iter, NULL))
#endif

struct evict_pids_t {
	struct rb_root root;
};

struct evict_pid_entry {
	struct rb_node node;
	struct list_head list;
	pid_t rootpid;
	u64 start_time;
#ifdef CONFIG_TEXT_UNEVICTABLE
	u64 unevict_size;
#endif
	struct task_struct *tsk;
	bool done;
};

static void execute_vm_lock(struct work_struct *unused);
static struct evict_pids_t *base_tree;
static DEFINE_MUTEX(pid_mutex);

LIST_HEAD(pid_list);
static int proc_pids_count;

static DECLARE_DELAYED_WORK(evict_work, execute_vm_lock);

struct proc_pids_t {
	struct rb_root proc_pids_tree;
};

/* Called with pid_mutex held always */
static void __remove_entry(struct evict_pid_entry *pid)
{
	if (pid == NULL)
		return;

	rb_erase(&pid->node, &base_tree->root);
	proc_pids_count--;
}

/* should not be in atomic context(i.e. hrtimer) */
static void __evict_pid(struct evict_pid_entry *pid)
{
	struct task_struct *tsk;
	struct mm_struct *mm;

	if (!pid)
		return;

	rcu_read_lock();
	tsk = find_task_by_pid_ns(pid->rootpid, &init_pid_ns);
	if (tsk)
		get_task_struct(tsk);
	rcu_read_unlock();

	if (!tsk)
		return;

	if (tsk == pid->tsk && pid->start_time == tsk->start_boottime) {
		mm = get_task_mm(tsk);
		if (mm) {
			VMA_ITERATOR(vmi, mm, 0);

			if (!(mm->def_flags & VM_LOCKED)) {
				struct vm_area_struct *vma, *prev = NULL;
				vm_flags_t flag;
				int error;
#ifdef CONFIG_TEXT_UNEVICTABLE
				unsigned long size = 0;
				struct mem_cgroup *memcg = get_mem_cgroup_from_mm(mm);
#endif

				mmap_write_lock(mm);
				for_each_vma(vmi, vma) {
					if (vma->vm_file &&
					    (vma->vm_flags & VM_EXEC) &&
					    (vma->vm_flags & VM_READ)) {
						flag = vma->vm_flags & ~VM_LOCKED_MASK;
						error = mlock_fixup(&vmi, vma, &prev,
							    vma->vm_start, vma->vm_end, flag);
						/* Ignore errors, but prev needs fixing up. */
						if (error)
							prev = vma;
#ifdef CONFIG_TEXT_UNEVICTABLE
						size += vma->vm_end - vma->vm_start;
#endif
					}
				}
				mmap_write_unlock(mm);
#ifdef CONFIG_TEXT_UNEVICTABLE
				if (memcg) {
					memcg_decrease_unevict_size(memcg, size);
					css_put(&memcg->css);
				}
				pid->unevict_size -= size;
#endif
			}
			mmput(mm);
		}
	}
	put_task_struct(tsk);
}

static struct evict_pid_entry *lookup_unevict_entry(struct task_struct *tsk)
{
	struct evict_pid_entry *entry, *result;
	struct rb_node *parent = NULL;
	struct rb_node **link;
	pid_t rootpid;

	if (!tsk)
		return NULL;

	rcu_read_lock();
	get_task_struct(tsk);
	rootpid = __task_pid_nr_ns(tsk, PIDTYPE_PID, &init_pid_ns);
	put_task_struct(tsk);
	rcu_read_unlock();

	result = NULL;
	link = &base_tree->root.rb_node;
	/*maybe unevictable feature not ready */
	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct evict_pid_entry, node);
		if (rootpid < entry->rootpid)
			link = &(*link)->rb_left;
		else if (rootpid > entry->rootpid)
			link = &(*link)->rb_right;
		else {
			result = entry;
			break;
		}
	}

	return result;
}

void del_unevict_task(struct task_struct *tsk)
{
	struct evict_pid_entry *result;

	if (!tsk) {
		struct evict_pid_entry *pid_entry, *tmp;

		mutex_lock(&pid_mutex);
		list_for_each_entry_safe(pid_entry, tmp, &pid_list, list) {
			rcu_read_lock();
			tsk = find_task_by_pid_ns(pid_entry->rootpid,
					&init_pid_ns);
			rcu_read_unlock();
			if (!tsk) {
				list_del(&pid_entry->list);
				__remove_entry(pid_entry);
				kfree(pid_entry);
			}
		}
		mutex_unlock(&pid_mutex);
		return;
	}

	mutex_lock(&pid_mutex);
	result = lookup_unevict_entry(tsk);
	if (result) {
		list_del(&result->list);
		__remove_entry(result);
		mutex_unlock(&pid_mutex);
		__evict_pid(result);
		kfree(result);
	} else
		mutex_unlock(&pid_mutex);
}

static void evict_pid(pid_t pid)
{
	struct task_struct *tsk;

	if (pid <= 0)
		return;

	rcu_read_lock();
	tsk = find_task_by_pid_ns(pid, task_active_pid_ns(current));
	if (!tsk) {
		rcu_read_unlock();
		return;
	}
	get_task_struct(tsk);
	rcu_read_unlock();

	del_unevict_task(tsk);
	put_task_struct(tsk);
}

static void add_unevict_task(struct task_struct *tsk)
{
	struct evict_pid_entry *entry, *new_entry, *result;
	struct rb_node *parent = NULL;
	struct rb_node **link;
	pid_t rootpid;

	if (!tsk)
		return;

	new_entry = kzalloc(sizeof(*new_entry), GFP_NOWAIT);
	if (!new_entry)
		return;

	result = NULL;
	get_task_struct(tsk);
	rootpid = __task_pid_nr_ns(tsk, PIDTYPE_PID, &init_pid_ns);
	put_task_struct(tsk);
	mutex_lock(&pid_mutex);
	link = &base_tree->root.rb_node;
	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct evict_pid_entry, node);
		if (rootpid < entry->rootpid) {
			link = &(*link)->rb_left;
		} else if (rootpid > entry->rootpid) {
			link = &(*link)->rb_right;
		} else {
			result = entry;
			break;
		}
	}

	if (!result) {
		result = new_entry;
		result->rootpid = rootpid;
#ifdef CONFIG_TEXT_UNEVICTABLE
		result->unevict_size = 0;
#endif
		rb_link_node(&result->node, parent, link);
		rb_insert_color(&result->node, &base_tree->root);
		list_add_tail(&result->list, &pid_list);
		proc_pids_count++;
		mutex_unlock(&pid_mutex);
	} else {
		rcu_read_lock();
		tsk = find_task_by_pid_ns(rootpid, &init_pid_ns);
		if (tsk)
			get_task_struct(tsk);
		rcu_read_unlock();
		if (!tsk) {
			list_del(&result->list);
			__remove_entry(result);
			mutex_unlock(&pid_mutex);
			kfree(result);
			kfree(new_entry);
			return;
		} else if (tsk != result->tsk ||
		    result->start_time != tsk->start_boottime) {
			result->done = false;
		}
		put_task_struct(tsk);
		mutex_unlock(&pid_mutex);
		kfree(new_entry);
	}
}

static void unevict_pid(pid_t pid)
{
	struct task_struct *tsk;

	if (pid <= 0)
		return;

	rcu_read_lock();
	tsk = find_task_by_pid_ns(pid, task_active_pid_ns(current));
	if (!tsk) {
		rcu_read_unlock();
		return;
	}
	get_task_struct(tsk);
	rcu_read_unlock();

#ifdef CONFIG_TEXT_UNEVICTABLE
	if (is_memcg_unevictable_enabled(mem_cgroup_from_task(tsk))) {
		put_task_struct(tsk);
		return;
	}
#endif
	add_unevict_task(tsk);
	put_task_struct(tsk);
}

struct add_pid_seq_context {
	int idx;
	int count;
	int pids[0];
};

/*
 * Note there exists a race condition that we may get inconsistent snapshots
 * of pid array if call add_pid_start() more than one round due to users add
 * or delete the pid. However, I think it's acceptable because the pid may
 * still change even we get a consistent snapshot to show.
 */
static void *add_pid_start(struct seq_file *m, loff_t *pos)
{
	struct add_pid_seq_context *ctx = NULL;
	struct evict_pid_entry *pid_entry;
	struct task_struct *tsk;
	struct evict_pid_entry *tmp;
	pid_t pid;

	mutex_lock(&pid_mutex);
	if (*pos >= proc_pids_count)
		goto done;
	ctx = vzalloc(sizeof(*ctx) + proc_pids_count * sizeof(int));
	if (unlikely(!ctx))
		goto done;

	if (proc_pids_count > 0) {
		list_for_each_entry_safe(pid_entry, tmp, &pid_list, list) {
			rcu_read_lock();
			tsk = find_task_by_pid_ns(pid_entry->rootpid,
					&init_pid_ns);
			if (tsk) {
				get_task_struct(tsk);
				pid = __task_pid_nr_ns(tsk, PIDTYPE_PID,
						task_active_pid_ns(current));
				put_task_struct(tsk);
			} else {
				pid = -1;
			}
			rcu_read_unlock();

			if (pid != -1) {
				ctx->pids[ctx->count++] = pid;
			} else {
				list_del(&pid_entry->list);
				__remove_entry(pid_entry);
				kfree(pid_entry);
			}
		}
	}
	if (*pos >= ctx->count)
		goto done;
	mutex_unlock(&pid_mutex);
	ctx->idx = *pos;
	m->private = ctx;
	return ctx;
done:
	mutex_unlock(&pid_mutex);
	vfree(ctx);
	return NULL;
}

static void *add_pid_next(struct seq_file *m, void *p, loff_t *pos)
{
	struct add_pid_seq_context *ctx = p;

	ctx->idx = ++*pos;
	return (ctx->idx < ctx->count) ? ctx : NULL;
}

static void add_pid_stop(struct seq_file *m, void *p)
{
	vfree(m->private);
	m->private = NULL;
}

static int add_pid_show(struct seq_file *m, void *p)
{
	struct add_pid_seq_context *ctx = p;

	seq_printf(m, "%d", ctx->pids[ctx->idx]);
	seq_putc(m, (ctx->idx == ctx->count - 1) ? '\n' : ',');
	return 0;
}

static const struct seq_operations seq_add_pid_op = {
	.start = add_pid_start,
	.next  = add_pid_next,
	.stop  = add_pid_stop,
	.show  = add_pid_show,
};

static int proc_open_add_pid(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_add_pid_op);
}

static void execute_vm_lock(struct work_struct *unused)
{
	struct task_struct *tsk;
	struct mm_struct *mm;
	struct evict_pid_entry *result, *tmp;
	bool need_again = false;
	pid_t rootpid;

	if (!mutex_trylock(&pid_mutex)) {
		need_again = true;
		goto out;
	}

	if (proc_pids_count <= 0) {
		mutex_unlock(&pid_mutex);
		goto out;
	}

	list_for_each_entry_safe(result, tmp, &pid_list, list) {
		rootpid = result->rootpid;
		if (result->done || rootpid <= 0)
			continue;

		rcu_read_lock();
		tsk = find_task_by_pid_ns(rootpid, &init_pid_ns);
		if (tsk)
			get_task_struct(tsk);
		rcu_read_unlock();
		if (!tsk) {
			list_del(&result->list);
			__remove_entry(result);
			kfree(result);
			continue;
		}

		mm = get_task_mm(tsk);
		if (mm && !(mm->def_flags & VM_LOCKED)) {
			VMA_ITERATOR(vmi, mm, 0);
#ifdef CONFIG_ASYNC_FORK
			/*
			 * Before child complete its page table copying, do not
			 * touch its vmas.
			 */
			if (mm->async_fork_mm &&
			    test_bit(ASYNC_FORK_CHILD, &mm->async_fork_flags)) {
				need_again = true;
				goto release_refcnt;
			}
#endif

			if (mmap_write_trylock(mm)) {
				struct vm_area_struct *vma, *prev = NULL;
				vm_flags_t flag;
				int error;
#ifdef CONFIG_TEXT_UNEVICTABLE
				struct mem_cgroup *memcg = get_mem_cgroup_from_mm(mm);
#endif

				for_each_vma(vmi, vma) {
#ifdef CONFIG_TEXT_UNEVICTABLE
					if (memcg && is_unevictable_size_overflow(memcg))
						break;
#endif
					if (vma->vm_file &&
					    (vma->vm_flags & VM_EXEC) &&
					    (vma->vm_flags & VM_READ)) {
						flag = vma->vm_flags & ~VM_LOCKED_MASK;
						flag |= (VM_LOCKED | VM_LOCKONFAULT);
						error = mlock_fixup(&vmi, vma, &prev,
							    vma->vm_start, vma->vm_end, flag);
						/* Ignore errors, but prev needs fixing up. */
						if (error)
							prev = vma;
#ifdef CONFIG_TEXT_UNEVICTABLE
						result->unevict_size += vma->vm_end - vma->vm_start;
#endif
					}
				}

				result->tsk = tsk;
				result->start_time = tsk->start_boottime;
				result->done = true;
				mmap_write_unlock(mm);
#ifdef CONFIG_TEXT_UNEVICTABLE
				if (memcg) {
					memcg_increase_unevict_size(memcg,
						result->unevict_size);
					css_put(&memcg->css);
				}
#endif
			} else {
				need_again = true;
			}
		} else {
			list_del(&result->list);
			__remove_entry(result);
			kfree(result);
		}
#ifdef CONFIG_ASYNC_FORK
release_refcnt:
#endif
		if (mm)
			mmput(mm);
		if (tsk)
			put_task_struct(tsk);
	}
	mutex_unlock(&pid_mutex);

out:
	if (need_again)
		schedule_delayed_work(&evict_work, HZ);
}


static ssize_t proc_write_add_pid(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	char buf[NAME_BUF];
	int err;
	long pid;
	int ret = count;

	if (count > NAME_BUF - 1) {
		ret = -EINVAL;
		goto out;
	}

	memset(buf, 0, sizeof(buf));
	if (copy_from_user(buf, buffer, count)) {
		ret = -EFAULT;
		goto out;
	}

	err = kstrtol(strstrip(buf), 0, &pid);
	if (err || pid <= 0) {
		ret = -EINVAL;
		goto out;
	} else {
		unevict_pid((pid_t)pid);
		schedule_delayed_work(&evict_work, HZ);
	}

out:
	return ret;
}

static ssize_t proc_write_del_pid(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	char buf[NAME_BUF];
	int err;
	long pid;
	int ret = count;

	memset(buf, 0, sizeof(buf));
	if (count > NAME_BUF - 1) {
		ret = -EINVAL;
		goto out;
	}

	if (copy_from_user(buf, buffer, count)) {
		ret = -EFAULT;
		goto out;
	}

	err = kstrtol(strstrip(buf), 0, &pid);
	if (err || pid <= 0) {
		ret = -EINVAL;
		goto out;
	} else {
		evict_pid(pid);
	}

out:
	return ret;
}

static const struct proc_ops add_proc_fops = {
	.proc_open  = proc_open_add_pid,
	.proc_read  = seq_read,
	.proc_write = proc_write_add_pid,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};

static const struct proc_ops del_proc_fops = {
	.proc_write = proc_write_del_pid,
};

#ifdef CONFIG_TEXT_UNEVICTABLE
void clean_task_unevict_size(struct task_struct *tsk)
{
	struct evict_pid_entry *result;
	struct mem_cgroup *memcg;

	/*
	 * There must make sure unevictable
	 * function is finished.
	 */
	if (!tsk || !base_tree)
		return;

	mutex_lock(&pid_mutex);
	result = lookup_unevict_entry(tsk);
	if (result) {
		if (result->unevict_size) {
			rcu_read_lock();
			memcg = mem_cgroup_from_task(tsk);
			if (memcg)
				memcg_decrease_unevict_size(memcg, result->unevict_size);
			rcu_read_unlock();
		}
		list_del(&result->list);
		__remove_entry(result);
		mutex_unlock(&pid_mutex);
		kfree(result);
	} else
		mutex_unlock(&pid_mutex);
}

bool is_memcg_unevictable_enabled(struct mem_cgroup *memcg)
{
	if (!unevictable_enabled())
		return false;

	if (!memcg)
		return false;

	if (memcg->allow_unevictable)
		return true;

	return false;
}

void memcg_increase_unevict_size(struct mem_cgroup *memcg, unsigned long size)
{
	atomic_long_add(size,  &memcg->unevictable_size);
}

void memcg_decrease_unevict_size(struct mem_cgroup *memcg, unsigned long size)
{
	atomic_long_sub(size,  &memcg->unevictable_size);
}

bool is_unevictable_size_overflow(struct mem_cgroup *memcg)
{
	struct page_counter *counter;
	u64 res_limit;
	u64 size;

	counter = &memcg->memory;
	res_limit = (u64)counter->max * PAGE_SIZE;
	size = atomic_long_read(&memcg->unevictable_size);
	size = size * 100 / res_limit;
	if (size >= memcg->unevictable_percent)
		return true;

	return false;
}

unsigned long memcg_exstat_text_unevict_gather(struct mem_cgroup *memcg)
{
	return atomic_long_read(&memcg->unevictable_size);
}

void mem_cgroup_can_unevictable(struct task_struct *tsk, struct mem_cgroup *to)
{
	struct mem_cgroup *from;

	if (!unevictable_enabled())
		return;

	from = mem_cgroup_from_task(tsk);
	VM_BUG_ON(from == to);

	if (to->allow_unevictable && !from->allow_unevictable) {
		add_unevict_task(tsk);
		schedule_delayed_work(&evict_work, HZ);
	}

	if (!to->allow_unevictable && from->allow_unevictable)
		del_unevict_task(tsk);
}

void mem_cgroup_cancel_unevictable(struct cgroup_taskset *tset)
{
	struct task_struct *tsk;
	struct cgroup_subsys_state *dst_css;
	struct mem_cgroup *memcg;

	if (!unevictable_enabled())
		return;

	cgroup_taskset_for_each(tsk, dst_css, tset) {
		memcg = mem_cgroup_from_task(tsk);

		if (memcg && memcg->allow_unevictable)
			del_unevict_task(tsk);
	}
}

static inline int schedule_unevict_task(struct task_struct *tsk, void *arg)
{
	add_unevict_task(tsk);
	schedule_delayed_work(&evict_work, HZ);

	return 0;
}

static inline int schedule_evict_task(struct task_struct *tsk, void *arg)
{
	del_unevict_task(tsk);

	return 0;
}

static inline void make_all_memcg_evictable(void)
{
	struct mem_cgroup *memcg;

	for_each_mem_cgroup(memcg) {
		if (!memcg->allow_unevictable)
			continue;
		mem_cgroup_scan_tasks(memcg, schedule_unevict_task, NULL);
		memcg->allow_unevictable = 0;
		memcg->unevictable_percent = 100;
		atomic_long_set(&memcg->unevictable_size, 0);
	}
}

void memcg_all_processes_unevict(struct mem_cgroup *memcg, bool enable)
{
	struct mem_cgroup *tmp_memcg;

	if (!unevictable_enabled())
		return;

	if (!memcg)
		tmp_memcg = root_mem_cgroup;
	else
		tmp_memcg = memcg;

	if (enable)
		mem_cgroup_scan_tasks(tmp_memcg, schedule_unevict_task, NULL);
	else
		mem_cgroup_scan_tasks(tmp_memcg, schedule_evict_task, NULL);
}

static int __init setup_unevictable(char *s)
{
	if (!strcmp(s, "1"))
		static_branch_enable(&unevictable_enabled_key);
	else if (!strcmp(s, "0"))
		static_branch_disable(&unevictable_enabled_key);
	return 1;
}
__setup("unevictable=", setup_unevictable);

#ifdef CONFIG_SYSFS
static ssize_t unevictable_enabled_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", !!static_branch_unlikely(&unevictable_enabled_key));
}
static ssize_t unevictable_enabled_store(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	static DEFINE_MUTEX(mutex);
	ssize_t ret = count;

	mutex_lock(&mutex);

	if (!strncmp(buf, "1", 1))
		static_branch_enable(&unevictable_enabled_key);
	else if (!strncmp(buf, "0", 1)) {
		static_branch_disable(&unevictable_enabled_key);
		make_all_memcg_evictable();
	} else
		ret = -EINVAL;

	mutex_unlock(&mutex);
	return ret;
}
static struct kobj_attribute unevictable_enabled_attr =
	__ATTR(enabled, 0644, unevictable_enabled_show,
	       unevictable_enabled_store);

static struct attribute *unevictable_attrs[] = {
	&unevictable_enabled_attr.attr,
	NULL,
};

static struct attribute_group unevictable_attr_group = {
	.attrs = unevictable_attrs,
};

static int __init unevictable_init_sysfs(void)
{
	int err;
	struct kobject *unevictable_kobj;

	unevictable_kobj = kobject_create_and_add("unevictable", mm_kobj);
	if (!unevictable_kobj) {
		pr_err("failed to create unevictable kobject\n");
		return -ENOMEM;
	}
	err = sysfs_create_group(unevictable_kobj, &unevictable_attr_group);
	if (err) {
		pr_err("failed to register unevictable group\n");
		goto delete_obj;
	}
	return 0;

delete_obj:
	kobject_put(unevictable_kobj);
	return err;
}
#endif /* CONFIG_SYSFS */
#endif /* CONFIG_TEXT_UNEVICTABLE */

static int __init unevictable_init(void)
{
	struct proc_dir_entry *monitor_dir, *add_pid_file, *del_pid_file;

	monitor_dir = proc_mkdir(PROC_NAME, NULL);
	if (!monitor_dir)
		goto out;

	add_pid_file = proc_create("add_pid", 0600,
			monitor_dir, &add_proc_fops);
	if (!add_pid_file)
		goto out_dir;

	del_pid_file = proc_create("del_pid", 0200,
			monitor_dir, &del_proc_fops);
	if (!del_pid_file)
		goto out_add_pid;

	base_tree = kzalloc(sizeof(*base_tree), GFP_KERNEL);
	if (!base_tree)
		goto out_del_pid;

	INIT_LIST_HEAD(&pid_list);

#if defined(CONFIG_SYSFS) && defined(CONFIG_TEXT_UNEVICTABLE)
	if (unevictable_init_sysfs())
		pr_err("memcg text unevictable sysfs create failed\n");
#endif
	return 0;

	pr_err("unevictpid create proc dir failed\n");

out_del_pid:
	remove_proc_entry("del_pid", monitor_dir);
out_add_pid:
	remove_proc_entry("add_pid", monitor_dir);
out_dir:
	remove_proc_entry(PROC_NAME, NULL);
out:
	return -ENOMEM;
}

module_init(unevictable_init);

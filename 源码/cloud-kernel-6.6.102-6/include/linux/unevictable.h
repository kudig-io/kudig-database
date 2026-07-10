/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _TEXT_UNEVICTABLE_H
#define _TEXT_UNEVICTABLE_H

struct mem_cgroup;

#ifdef CONFIG_TEXT_UNEVICTABLE
DECLARE_STATIC_KEY_FALSE(unevictable_enabled_key);

static inline bool unevictable_enabled(void)
{
	return static_branch_unlikely(&unevictable_enabled_key);
}
bool is_memcg_unevictable_enabled(struct mem_cgroup *memcg);
void memcg_increase_unevict_size(struct mem_cgroup *memcg, unsigned long size);
void memcg_decrease_unevict_size(struct mem_cgroup *memcg, unsigned long size);
bool is_unevictable_size_overflow(struct mem_cgroup *memcg);
unsigned long memcg_exstat_text_unevict_gather(struct mem_cgroup *memcg);
void mem_cgroup_can_unevictable(struct task_struct *tsk, struct mem_cgroup *to);
void mem_cgroup_cancel_unevictable(struct cgroup_taskset *tset);
void memcg_all_processes_unevict(struct mem_cgroup *memcg, bool enable);
void del_unevict_task(struct task_struct *tsk);
void clean_task_unevict_size(struct task_struct *tsk);
#else
static inline bool unevictable_enabled(void)
{
	return false;
}
static inline bool is_memcg_unevictable_enabled(struct mem_cgroup *memcg)
{
	return false;
}
static inline void memcg_increase_unevict_size(struct mem_cgroup *memcg,
					       unsigned long size)
{
}
static inline void memcg_decrease_unevict_size(struct mem_cgroup *memcg,
					       unsigned long size)
{
}
static inline bool is_unevictable_size_overflow(struct mem_cgroup *memcg)
{
	return false;
}
static inline unsigned long memcg_exstat_text_unevict_gather(struct mem_cgroup *memcg)
{
	return 0;
}
static inline void mem_cgroup_can_unevictable(struct task_struct *tsk,
					      struct mem_cgroup *to)
{
}
static inline void mem_cgroup_cancel_unevictable(struct cgroup_taskset *tset)
{
}
static inline void memcg_all_processes_unevict(struct mem_cgroup *memcg, bool enable)
{
}
static inline void del_unevict_task(struct task_struct *tsk)
{
}
static inline void clean_task_unevict_size(struct task_struct *tsk)
{
}
#endif
#endif

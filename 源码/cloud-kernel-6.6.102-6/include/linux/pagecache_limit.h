/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _PAGECACHE_LIMIT_H
#define _PAGECACHE_LIMIT_H

#ifdef CONFIG_PAGECACHE_LIMIT

DECLARE_STATIC_KEY_FALSE(pagecache_limit_enabled_key);
extern struct workqueue_struct *memcg_pgcache_limit_wq;

enum pgcache_limit_reclaim_type {
	/* per-memcg or global pagecaeche reclaim defaut way is async */
	PGCACHE_RECLAIM_ASYNC = 0,
	PGCACHE_RECLAIM_DIRECT
};

static inline bool pagecache_limit_enabled(void)
{
	return static_branch_unlikely(&pagecache_limit_enabled_key);
}
bool is_memcg_pgcache_limit_enabled(struct mem_cgroup *memcg);
void memcg_add_pgcache_limit_reclaimed(struct mem_cgroup *memcg,
				       unsigned long nr);
unsigned long memcg_get_pgcache_overflow_size(struct mem_cgroup *memcg);
void __memcg_pagecache_shrink(struct mem_cgroup *memcg,
			      bool may_unmap, gfp_t gfp_mask);
void memcg_pagecache_shrink(struct mem_cgroup *memcg, gfp_t gfp_mask);
void memcg_pgcache_limit_work_func(struct work_struct *work);

#else
static inline bool pagecache_limit_enabled(void)
{
	return false;
}
static inline bool is_memcg_pgcache_limit_enabled(struct mem_cgroup *memcg)
{
	return false;
}
static inline void memcg_add_pgcache_limit_reclaimed(struct mem_cgroup *memcg,
						     unsigned long nr)
{
}
static inline unsigned long memcg_get_pgcache_overflow_size(struct mem_cgroup *memcg)
{
	return 0;
}
static inline void __memcg_pagecache_shrink(struct mem_cgroup *memcg,
					    bool may_unmap, gfp_t gfp_mask)
{
}
static inline void memcg_pagecache_shrink(struct mem_cgroup *memcg,
					  gfp_t gfp_mask)
{
}
static inline void memcg_pgcache_limit_work_func(struct work_struct *work)
{
}
#endif
#endif

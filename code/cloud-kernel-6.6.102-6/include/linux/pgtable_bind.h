/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PGTABLE_BIND_H_
#define _LINUX_PGTABLE_BIND_H_

#include <linux/types.h>
#include <linux/jump_label.h>
#include <linux/memcontrol.h>

#ifdef CONFIG_PGTABLE_BIND
DECLARE_STATIC_KEY_FALSE(pgtable_bind_enabled_key);
DECLARE_STATIC_KEY_FALSE(pgtable_stat_enabled_key);
static inline bool pgtable_bind_enabled(void)
{
	return static_key_enabled(&pgtable_bind_enabled_key);
}

static inline bool pgtable_stat_enabled(void)
{
	return static_key_enabled(&pgtable_stat_enabled_key);
}
#else
static inline bool pgtable_bind_enabled(void)
{
	return false;
}

static inline bool pgtable_stat_enabled(void)
{
	return false;
}
#endif

#endif /* _LINUX_PGTABLE_BIND_H_ */

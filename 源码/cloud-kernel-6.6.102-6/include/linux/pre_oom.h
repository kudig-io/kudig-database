#ifndef _LINUX_PRE_OOM_H
#define _LINUX_PRE_OOM_H

#include <linux/sched.h>

#ifdef CONFIG_PRE_OOM

#include <linux/types.h>
#include <linux/jump_label.h>

DECLARE_STATIC_KEY_FALSE(pre_oom_enabled_key);
static inline bool pre_oom_enabled(void)
{
	return static_branch_unlikely(&pre_oom_enabled_key);
}


int pre_oom_enter(void);
void pre_oom_leave(void);

#else

static inline bool pre_oom_enabled(void)
{
	return false;
}

static inline int pre_oom_enter(void)
{
	return 0;
}

static inline void pre_oom_leave(void) {}

#endif /* CONFIG_PRE_OOM */
#endif /* _LINUX_PRE_OOM_H */

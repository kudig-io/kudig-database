// SPDX-License-Identifier: GPL-2.0-only

static unsigned long sched_core_alloc_cookie(u32 flags)
{
	struct sched_core_cookie *ck = kmalloc(sizeof(*ck), GFP_KERNEL);

	if (!ck)
		return 0;

	refcount_set(&ck->refcnt, 1);
	ck->flags = flags;
	sched_core_get();

	return (unsigned long)ck;
}

static bool sched_core_put_cookie(unsigned long cookie)
{
	struct sched_core_cookie *ptr = (void *)cookie;

	if (ptr && refcount_dec_and_test(&ptr->refcnt)) {
		kfree(ptr);
		sched_core_put();
		return true;
	}

	return false;
}

static unsigned long sched_core_get_cookie(unsigned long cookie)
{
	struct sched_core_cookie *ptr = (void *)cookie;

	if (ptr)
		refcount_inc(&ptr->refcnt);

	return cookie;
}

/*
 * sched_core_update_cookie - replace the cookie on a task
 * @p: the task to update
 * @cookie: the new cookie
 *
 * Effectively exchange the task cookie; caller is responsible for lifetimes on
 * both ends.
 *
 * Returns: the old cookie
 */
static unsigned long sched_core_update_cookie(struct task_struct *p,
					      unsigned long cookie)
{
	unsigned long old_cookie;
	struct rq_flags rf;
	struct rq *rq;

	rq = task_rq_lock(p, &rf);

	/*
	 * Since creating a cookie implies sched_core_get(), and we cannot set
	 * a cookie until after we've created it, similarly, we cannot destroy
	 * a cookie until after we've removed it, we must have core scheduling
	 * enabled here.
	 */
	SCHED_WARN_ON((p->core_cookie || cookie) && !sched_core_enabled(rq));

	if (sched_core_enqueued(p))
		sched_core_dequeue(rq, p, DEQUEUE_SAVE);

	old_cookie = p->core_cookie;
	p->core_cookie = cookie;

	/*
	 * Consider the cases: !prev_cookie and !cookie.
	 */
	if (cookie && task_on_rq_queued(p))
		sched_core_enqueue(rq, p);

	/*
	 * If task is currently running, it may not be compatible anymore after
	 * the cookie change, so enter the scheduler on its CPU to schedule it
	 * away.
	 *
	 * Note that it is possible that as a result of this cookie change, the
	 * core has now entered/left forced idle state. Defer accounting to the
	 * next scheduling edge, rather than always forcing a reschedule here.
	 */
	if (task_on_cpu(rq, p))
		resched_curr(rq);

	task_rq_unlock(rq, p, &rf);

	return old_cookie;
}

static unsigned long sched_core_clone_cookie(struct task_struct *p)
{
	unsigned long cookie, flags;

	raw_spin_lock_irqsave(&p->pi_lock, flags);
	cookie = sched_core_get_cookie(p->core_cookie);
	raw_spin_unlock_irqrestore(&p->pi_lock, flags);

	return cookie;
}

void sched_core_fork(struct task_struct *p)
{
	RB_CLEAR_NODE(&p->core_node);
	p->core_cookie = sched_core_clone_cookie(current);
}

void sched_core_free(struct task_struct *p)
{
	sched_core_put_cookie(p->core_cookie);
}

static void __sched_core_set(struct task_struct *p, unsigned long cookie)
{
	cookie = sched_core_get_cookie(cookie);
	cookie = sched_core_update_cookie(p, cookie);
	sched_core_put_cookie(cookie);
}

static bool cookie_may_access(unsigned long cookie)
{
	struct sched_core_cookie *ptr = (struct sched_core_cookie *)sched_core_get_cookie(cookie);
	bool ret = true;

	if (ptr && ptr->flags && !capable(CAP_SYS_NICE))
		ret = false;

	sched_core_put_cookie(cookie);

	return ret;
}

static bool task_cookie_may_access(struct task_struct *p)
{
	unsigned long flags;
	bool ret;

	raw_spin_lock_irqsave(&p->pi_lock, flags);
	ret = cookie_may_access(p->core_cookie);
	raw_spin_unlock_irqrestore(&p->pi_lock, flags);

	return ret;
}

/* Called from prctl interface: PR_SCHED_CORE */
int sched_core_share_pid(unsigned int cmd, pid_t pid, enum pid_type type,
			 unsigned long uaddr)
{
	unsigned long cookie = 0, id = 0;
	struct task_struct *task, *p;
	struct pid *grp;
	int err = 0;

	if (!static_branch_likely(&sched_smt_present))
		return -ENODEV;

	BUILD_BUG_ON(PR_SCHED_CORE_SCOPE_THREAD != PIDTYPE_PID);
	BUILD_BUG_ON(PR_SCHED_CORE_SCOPE_THREAD_GROUP != PIDTYPE_TGID);
	BUILD_BUG_ON(PR_SCHED_CORE_SCOPE_PROCESS_GROUP != PIDTYPE_PGID);

	if (type > PIDTYPE_PGID || cmd >= PR_SCHED_CORE_MAX || pid < 0)
		return -EINVAL;

	if (uaddr) {
		switch (cmd) {
		case PR_SCHED_CORE_GET:
		case PR_SCHED_CORE_CREATE: /* reuse uaddr for flags */
			break;
		default:
			return -EINVAL;
		}
	}

	if (cmd > PR_SCHED_CORE_SHARE_FROM && cmd < PR_SCHED_CORE_CLEAR)
		return -EINVAL;

	rcu_read_lock();
	if (pid == 0) {
		task = current;
	} else {
		task = find_task_by_vpid(pid);
		if (!task) {
			rcu_read_unlock();
			return -ESRCH;
		}
	}
	get_task_struct(task);
	rcu_read_unlock();

	/*
	 * Check if this process has the right to modify the specified
	 * process. Use the regular "ptrace_may_access()" checks.
	 */
	if (!ptrace_may_access(task, PTRACE_MODE_READ_REALCREDS) ||
	    !task_cookie_may_access(task)) {
		err = -EPERM;
		goto out;
	}

	switch (cmd) {
	case PR_SCHED_CORE_GET:
		if (type != PIDTYPE_PID || uaddr & 7) {
			err = -EINVAL;
			goto out;
		}
		cookie = sched_core_clone_cookie(task);
		if (cookie) {
			/* XXX improve ? */
			ptr_to_hashval((void *)cookie, &id);
		}
		err = put_user(id, (u64 __user *)uaddr);
		goto out;

	case PR_SCHED_CORE_CREATE:
		if ((u32)uaddr & ~SCHED_COOKIE_FLAGS_MASK) {
			err = -EINVAL;
			goto out;
		}
		cookie = sched_core_alloc_cookie((u32)uaddr);
		if (!cookie) {
			err = -ENOMEM;
			goto out;
		}
		break;

	case PR_SCHED_CORE_SHARE_TO:
		cookie = sched_core_clone_cookie(current);
		break;

	case PR_SCHED_CORE_SHARE_FROM:
		if (type != PIDTYPE_PID) {
			err = -EINVAL;
			goto out;
		}
		cookie = sched_core_clone_cookie(task);
		if (!cookie_may_access(cookie)) {
			err = -EPERM;
			goto out;
		}
		__sched_core_set(current, cookie);
		goto out;

	case PR_SCHED_CORE_CLEAR:
		cookie = 0;
		break;

	default:
		err = -EINVAL;
		goto out;
	}

	if (!cookie_may_access(cookie)) {
		err = -EPERM;
		goto out;
	}

	if (type == PIDTYPE_PID) {
		__sched_core_set(task, cookie);
		goto out;
	}

	read_lock(&tasklist_lock);
	grp = task_pid_type(task, type);

	do_each_pid_thread(grp, type, p) {
		if (!ptrace_may_access(p, PTRACE_MODE_READ_REALCREDS) ||
		    !task_cookie_may_access(p)) {
			err = -EPERM;
			goto out_tasklist;
		}
	} while_each_pid_thread(grp, type, p);

	do_each_pid_thread(grp, type, p) {
		__sched_core_set(p, cookie);
	} while_each_pid_thread(grp, type, p);
out_tasklist:
	read_unlock(&tasklist_lock);

out:
	sched_core_put_cookie(cookie);
	put_task_struct(task);
	return err;
}

#ifdef CONFIG_SCHEDSTATS

/* REQUIRES: rq->core's clock recently updated. */
void __sched_core_account_sibidle(struct rq *rq)
{
	const struct cpumask *smt_mask = cpu_smt_mask(cpu_of(rq));
	u64 delta, now = rq_clock(rq->core);
	u64 delta_task, now_task = rq_clock_task(rq->core);
	struct rq *rq_i;
	struct task_struct *p;
	int i;

	lockdep_assert_rq_held(rq);

	WARN_ON_ONCE(!rq->core->core_sibidle_count);

	/* can't be forced idle without a running task */
	WARN_ON_ONCE(!rq->core->core_sibidle_occupation &&
		     rq->core->core_forceidle_count);

	if (rq->core->core_sibidle_start == 0 ||
	    rq->core->core_sibidle_occupation == 0)
		goto out;

	delta = now - rq->core->core_sibidle_start;
	delta_task = now_task - rq->core->core_sibidle_start_task;
	if (unlikely((s64)delta <= 0))
		goto out;

	rq->core->core_sibidle_start = now;
	rq->core->core_sibidle_start_task = now_task;

	if (rq->core->core_sibidle_count > 1 ||
	    rq->core->core_sibidle_occupation > 1) {
		/*
		 * For larger SMT configurations, we need to scale the charged
		 * forced idle amount since there can be more than one forced
		 * idle sibling and more than one running cookied task.
		 */
		delta *= rq->core->core_sibidle_count;
		delta = div_u64(delta, rq->core->core_sibidle_occupation);
		delta_task *= rq->core->core_sibidle_count;
		delta_task = div_u64(delta_task, rq->core->core_sibidle_occupation);
	}

	for_each_cpu(i, smt_mask) {
		rq_i = cpu_rq(i);
		p = rq_i->core_pick ?: rq_i->curr;

		if (p == rq_i->idle)
			continue;

		/*
		 * Note: this will account sibidle to the current cpu, even
		 * if it comes from our SMT sibling.
		 */
		__account_sibidle_time(p, delta, delta_task,
				       !!rq->core->core_forceidle_count);
		account_ht_aware_quota(p, delta_task);
	}

out:;
#ifdef CONFIG_SCHED_ACPU
	for_each_cpu(i, smt_mask) {
		rq_i = cpu_rq(i);
		rq->last_acpu_update_time = now;
	}
#endif
}

void __sched_core_tick(struct rq *rq)
{
	if (!rq->core->core_sibidle_count)
		return;

	if (rq != rq->core)
		update_rq_clock(rq->core);

	__sched_core_account_sibidle(rq);
}

#endif /* CONFIG_SCHEDSTATS */

#ifdef CONFIG_GROUP_IDENTITY
unsigned long sched_core_expeller_cookie;
unsigned long sched_core_expellee_cookie;
static DEFINE_MUTEX(sched_core_expeller_mutex);
static DEFINE_MUTEX(sched_core_expellee_mutex);

static unsigned long sched_core_get_expeller_cookie(void)
{
	guard(mutex)(&sched_core_expeller_mutex);
	if (sched_core_expeller_cookie)
		return sched_core_get_cookie(sched_core_expeller_cookie);
	sched_core_expeller_cookie =
		sched_core_alloc_cookie(SCHED_COOKIE_MATCH_UNSET | SCHED_COOKIE_NO_GATHER);
	return sched_core_expeller_cookie;
}

static unsigned long sched_core_get_expellee_cookie(void)
{
	guard(mutex)(&sched_core_expellee_mutex);
	if (sched_core_expellee_cookie)
		return sched_core_get_cookie(sched_core_expellee_cookie);
	sched_core_expellee_cookie = sched_core_alloc_cookie(SCHED_COOKIE_MATCH_UNSET);
	return sched_core_expellee_cookie;
}

static void sched_core_put_expeller_cookie(void)
{
	guard(mutex)(&sched_core_expeller_mutex);
	if (sched_core_put_cookie(sched_core_expeller_cookie))
		sched_core_expeller_cookie = 0UL;
}

static void sched_core_put_expellee_cookie(void)
{
	guard(mutex)(&sched_core_expellee_mutex);
	if (sched_core_put_cookie(sched_core_expellee_cookie))
		sched_core_expellee_cookie = 0UL;
}

static inline bool task_has_identity(struct task_struct *p)
{
	return p->core_cookie &&
	       (p->core_cookie == sched_core_expeller_cookie ||
	       p->core_cookie == sched_core_expellee_cookie);
}

int set_task_group_identity_locked(struct task_group *tg, int identity)
{
	unsigned long cookie = 0;
	int old_identity = tg->identity;
	struct css_task_iter it;
	struct task_struct *task;

	if (old_identity == identity)
		return 0;

	switch (identity) {
	case -1:
		cookie = sched_core_get_expellee_cookie();
		break;
	case 0:
		cookie = 0;
		break;
	case 1:
		cookie = sched_core_get_expeller_cookie();
		break;
	default:
		return -EINVAL;
	}

	css_task_iter_start(&tg->css, 0, &it);
	while ((task = css_task_iter_next(&it)))
		__sched_core_set(task, cookie);
	css_task_iter_end(&it);

	tg->identity = identity;

	switch (old_identity) {
	case -1:
		sched_core_put_expellee_cookie();
		break;
	case 0:
		break;
	case 1:
		sched_core_put_expeller_cookie();
		break;
	default:
		break;
	}

	return 0;
}

int set_task_group_identity(struct task_group *tg, int identity)
{
	int ret;

	cgroup_lock();
	ret = set_task_group_identity_locked(tg, identity);
	cgroup_unlock();

	return ret;
}

void sched_core_identity_attach(struct cgroup_taskset *tset)
{
	struct task_struct *task;
	struct cgroup_subsys_state *css;
	struct task_group *tg;
	unsigned long cookie;

	cgroup_taskset_for_each(task, css, tset) {
		tg = css_tg(css);

		switch (tg->identity) {
		case -1:
			cookie = sched_core_get_expellee_cookie();
			break;
		case 1:
			cookie = sched_core_get_expeller_cookie();
			break;
		default:
			cookie = 0;
			break;
		}

		/*
		 * If a task moves between task groups with identity,
		 * the cookie should be updated.
		 */
		if (task_has_identity(task) || cookie)
			__sched_core_set(task, cookie);

		if (cookie)
			sched_core_put_cookie(cookie);
	}
}
#endif

// SPDX-License-Identifier: GPL-2.0

#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../kselftest.h"
#include "cgroup_util.h"

#ifndef PR_SCHED_CORE
#define PR_SCHED_CORE                   62
#define PR_SCHED_CORE_GET               0
#endif

#ifndef PIDTYPE_PID
#define PIDTYPE_PID                     0
#endif

#define SCHED_FEATURES_PATH             "/sys/kernel/debug/sched/features"
#define SCHED_DEBUG_PATH                "/sys/kernel/debug/sched/debug"
#define SYSCTL_IDLE_SHARES_RELAX_PATH   "/proc/sys/kernel/sched_idle_shares_relax"
#define CPU_TOPOLOGY_PATH               "/sys/devices/system/cpu"
#define TEST_PREFIX                     "cpu_identity_test"
#define BENCH_DURATION_MS               1800
#define BENCH_ROUNDS                    5

enum cgroup_mode {
	CGROUP_MODE_V1 = 1,
	CGROUP_MODE_V2 = 2,
};

struct cgroup_test_env {
	enum cgroup_mode mode;
	char root[PATH_MAX];
	const char *identity_ctrl;
	const char *idle_ctrl;
	const char *weight_ctrl;
};

struct worker_result {
	unsigned long long iterations;
	int last_cpu;
};

struct worker_proc {
	pid_t pid;
	int start_fd;
	int result_fd;
	struct worker_result result;
};

struct cpu_pair {
	int cpu0;
	int cpu1;
};

struct sched_debug_counts {
	long expeller;
	long expellee;
};

struct cpu_stat_snapshot {
	long usage_usec;
	long nr_periods;
	long nr_throttled;
	long throttled_usec;
};

struct sched_debug_throttle_state {
	long throttled;
	long throttle_count;
};

static int read_text_file(const char *path, char *buf, size_t size)
{
	ssize_t len;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -errno;

	len = read(fd, buf, size - 1);
	close(fd);
	if (len < 0)
		return -errno;

	buf[len] = '\0';
	return 0;
}

static int write_text_file(const char *path, const char *value)
{
	ssize_t len = strlen(value);
	ssize_t ret;
	int fd;

	fd = open(path, O_WRONLY | O_TRUNC);
	if (fd < 0)
		return -errno;

	ret = write(fd, value, len);
	close(fd);
	if (ret < 0)
		return -errno;
	if (ret != len)
		return -EIO;

	return 0;
}

static int parse_long(const char *buf, long *value)
{
	char *end;
	long v;

	errno = 0;
	v = strtol(buf, &end, 10);
	if (errno)
		return -errno;
	if (end == buf)
		return -EINVAL;

	*value = v;
	return 0;
}

static int read_long_file(const char *path, long *value)
{
	char buf[128];
	int ret;

	ret = read_text_file(path, buf, sizeof(buf));
	if (ret)
		return ret;

	return parse_long(buf, value);
}

static int file_contains_token(const char *path, const char *token)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	int found = 0;

	fp = fopen(path, "re");
	if (!fp)
		return -errno;

	while (getline(&line, &len, fp) >= 0) {
		if (strstr(line, token)) {
			found = 1;
			break;
		}
	}

	free(line);
	fclose(fp);
	return found;
}

static int feature_enabled(const char *feature)
{
	char buf[8192];
	char needle[128];
	char no_needle[128];
	int ret;

	ret = read_text_file(SCHED_FEATURES_PATH, buf, sizeof(buf));
	if (ret)
		return ret;

	snprintf(needle, sizeof(needle), "%s", feature);
	snprintf(no_needle, sizeof(no_needle), "NO_%s", feature);

	if (strstr(buf, no_needle))
		return 0;
	if (strstr(buf, needle))
		return 1;

	return -ENOENT;
}

static int set_feature_enabled(const char *feature, bool enabled)
{
	char buf[128];

	snprintf(buf, sizeof(buf), "%s%s", enabled ? "" : "NO_", feature);
	return write_text_file(SCHED_FEATURES_PATH, buf);
}

static int find_cgroup_root_v1(char *root, size_t len)
{
	char buf[10 * PAGE_SIZE];
	char *fs, *mount, *type, *options;
	static const char delim[] = "\n\t ";

	if (read_text_file("/proc/self/mounts", buf, sizeof(buf)))
		return -1;

	for (fs = strtok(buf, delim); fs; fs = strtok(NULL, delim)) {
		mount = strtok(NULL, delim);
		type = strtok(NULL, delim);
		options = strtok(NULL, delim);
		strtok(NULL, delim);
		strtok(NULL, delim);

		if (strcmp(type, "cgroup"))
			continue;
		if (strstr(options, "cpu") || strstr(options, "cpuacct")) {
			snprintf(root, len, "%s", mount);
			return 0;
		}
	}

	return -1;
}

static int detect_cgroup_env(struct cgroup_test_env *env)
{
	memset(env, 0, sizeof(*env));

	if (!cg_find_unified_root(env->root, sizeof(env->root), NULL)) {
		env->mode = CGROUP_MODE_V2;
		env->identity_ctrl = "cpu.identity";
		env->idle_ctrl = "cpu.idle";
		env->weight_ctrl = "cpu.weight";
		return 0;
	}

	if (!find_cgroup_root_v1(env->root, sizeof(env->root))) {
		env->mode = CGROUP_MODE_V1;
		env->identity_ctrl = "identity";
		env->idle_ctrl = "idle";
		env->weight_ctrl = "shares";
		return 0;
	}

	return -1;
}

static int enable_cpu_controller_if_needed(const struct cgroup_test_env *env,
					   const char *root)
{
	if (env->mode != CGROUP_MODE_V2)
		return 0;

	if (cg_read_strstr(root, "cgroup.subtree_control", "cpu"))
		return cg_write(root, "cgroup.subtree_control", "+cpu");

	return 0;
}

static unsigned long long get_cs_cookie(pid_t pid)
{
	unsigned long long cookie = 0;

	if (prctl(PR_SCHED_CORE, PR_SCHED_CORE_GET, pid, PIDTYPE_PID,
		  (unsigned long)&cookie))
		return 0;

	return cookie;
}

static int set_affinity_pair(int cpu0, int cpu1)
{
	cpu_set_t cpus;

	CPU_ZERO(&cpus);
	CPU_SET(cpu0, &cpus);
	CPU_SET(cpu1, &cpus);

	if (sched_setaffinity(0, sizeof(cpus), &cpus))
		return -errno;

	return 0;
}

static int set_affinity_single(int cpu)
{
	cpu_set_t cpus;

	CPU_ZERO(&cpus);
	CPU_SET(cpu, &cpus);

	if (sched_setaffinity(0, sizeof(cpus), &cpus))
		return -errno;

	return 0;
}

static int worker_main(int start_fd, int result_fd, int cpu0, int cpu1,
		       bool pair_affinity, unsigned int duration_ms,
		       volatile struct timespec *shared_start)
{
	char start;
	struct timespec start_ts, now;
	unsigned long long iters = 0;
	struct worker_result result = {};
	int ret;

	ret = pair_affinity ? set_affinity_pair(cpu0, cpu1) : set_affinity_single(cpu0);
	if (ret)
		_exit(100);

	if (read(start_fd, &start, 1) != 1)
		_exit(101);

	if (shared_start) {
		while (shared_start->tv_sec == 0 && shared_start->tv_nsec == 0)
			;
		start_ts = *(const struct timespec *)shared_start;
	} else {
		clock_gettime(CLOCK_MONOTONIC, &start_ts);
	}

	for (;;) {
		iters++;
		if ((iters & 0x3ffff) == 0) {
			long long elapsed_ms;

			clock_gettime(CLOCK_MONOTONIC, &now);
			elapsed_ms = (long long)(now.tv_sec - start_ts.tv_sec) * 1000 +
				     (long long)(now.tv_nsec - start_ts.tv_nsec) / 1000000;
			if (elapsed_ms >= (long long)duration_ms)
				break;
		}
	}

	result.iterations = iters;
	result.last_cpu = sched_getcpu();
	if (write(result_fd, &result, sizeof(result)) != sizeof(result))
		_exit(102);

	_exit(0);
}

static int spawn_worker(struct worker_proc *worker, const char *cgroup,
			int cpu0, int cpu1, bool pair_affinity,
			unsigned int duration_ms,
			volatile struct timespec *shared_start)
{
	int start_pipe[2], result_pipe[2];
	pid_t pid;

	if (pipe(start_pipe) || pipe(result_pipe))
		return -errno;

	pid = fork();
	if (pid < 0)
		return -errno;

	if (pid == 0) {
		close(start_pipe[1]);
		close(result_pipe[0]);
		worker_main(start_pipe[0], result_pipe[1], cpu0, cpu1,
			    pair_affinity, duration_ms, shared_start);
	}

	close(start_pipe[0]);
	close(result_pipe[1]);

	if (cg_enter(cgroup, pid)) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		close(start_pipe[1]);
		close(result_pipe[0]);
		return -1;
	}

	worker->pid = pid;
	worker->start_fd = start_pipe[1];
	worker->result_fd = result_pipe[0];
	return 0;
}

static int start_worker(struct worker_proc *worker)
{
	return write(worker->start_fd, "1", 1) == 1 ? 0 : -EIO;
}

static int reap_worker(struct worker_proc *worker)
{
	int status;
	ssize_t ret;

	close(worker->start_fd);
	ret = read(worker->result_fd, &worker->result, sizeof(worker->result));
	close(worker->result_fd);
	waitpid(worker->pid, &status, 0);

	if (ret != sizeof(worker->result))
		return -EIO;
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		return -ECHILD;

	return 0;
}

static int read_sched_debug_counts(const char *cg_path, struct sched_debug_counts *counts)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	bool in_block = false;
	int ret = 0;
	long expeller = 0;
	long expellee = 0;

	memset(counts, 0, sizeof(*counts));

	fp = fopen(SCHED_DEBUG_PATH, "re");
	if (!fp)
		return -errno;

	while (getline(&line, &len, fp) >= 0) {
		if (!strncmp(line, "cfs_rq[", 7)) {
			char *path = strchr(line, ':');

			if (path) {
				char *end = path + strlen(path);

				while (end > path && isspace(end[-1]))
					*--end = '\0';
				in_block = !strcmp(path + 1, cg_path);
			} else {
				in_block = false;
			}
			continue;
		}

		if (!in_block)
			continue;

		if (strstr(line, "h_nr_expeller")) {
			char *p = strrchr(line, ':');

			if (p)
				expeller += atol(p + 1);
		} else if (strstr(line, "h_nr_expellee")) {
			char *p = strrchr(line, ':');

			if (p)
				expellee += atol(p + 1);
		}
	}

	if (ferror(fp))
		ret = -EIO;

	counts->expeller = expeller;
	counts->expellee = expellee;
	free(line);
	fclose(fp);
	return ret;
}

static int wait_for_counts_ge(const char *cg_path, long min_expeller,
			      long min_expellee,
			      unsigned int retries,
			      unsigned int wait_us)
{
	struct sched_debug_counts counts;
	unsigned int i;

	for (i = 0; i < retries; i++) {
		if (read_sched_debug_counts(cg_path, &counts))
			return -1;
		if (counts.expeller >= min_expeller &&
		    counts.expellee >= min_expellee)
			return 0;
		usleep(wait_us);
	}

	return -1;
}

static int wait_for_cookie_value(pid_t pid, unsigned long long expected,
				 unsigned int retries,
				 unsigned int wait_us)
{
	unsigned int i;

	for (i = 0; i < retries; i++) {
		if (get_cs_cookie(pid) == expected)
			return 0;
		usleep(wait_us);
	}

	return -1;
}

static int wait_for_cookie_nonzero(pid_t pid, unsigned long long *cookie,
				   unsigned int retries,
				   unsigned int wait_us)
{
	unsigned int i;
	unsigned long long val;

	for (i = 0; i < retries; i++) {
		val = get_cs_cookie(pid);
		if (val) {
			*cookie = val;
			return 0;
		}
		usleep(wait_us);
	}

	return -1;
}

static int wait_for_cookie_distinct(pid_t pid, unsigned long long old_cookie,
				    unsigned long long *new_cookie,
				    unsigned int retries,
				    unsigned int wait_us)
{
	unsigned int i;
	unsigned long long val;

	for (i = 0; i < retries; i++) {
		val = get_cs_cookie(pid);
		if (val && val != old_cookie) {
			*new_cookie = val;
			return 0;
		}
		usleep(wait_us);
	}

	return -1;
}

static int
set_cfs_quota_period(const struct cgroup_test_env *env, const char *cg,
		     long quota_us, long period_us)
{
	char buf[64];

	if (env->mode == CGROUP_MODE_V2) {
		snprintf(buf, sizeof(buf), "%ld %ld", quota_us, period_us);
		return cg_write(cg, "cpu.max", buf);
	}

	if (cg_write_numeric(cg, "cpu.cfs_period_us", period_us))
		return -1;
	return cg_write_numeric(cg, "cpu.cfs_quota_us", quota_us);
}

static int enable_cfs_throttle(const struct cgroup_test_env *env, const char *cg)
{
	return set_cfs_quota_period(env, cg, 1000, 100000);
}

static int read_cpu_stat_snapshot(const char *cg, struct cpu_stat_snapshot *stat)
{
	stat->usage_usec = cg_read_key_long(cg, "cpu.stat", "usage_usec");
	stat->nr_periods = cg_read_key_long(cg, "cpu.stat", "nr_periods");
	stat->nr_throttled = cg_read_key_long(cg, "cpu.stat", "nr_throttled");
	stat->throttled_usec = cg_read_key_long(cg, "cpu.stat", "throttled_usec");

	if (stat->usage_usec < 0 || stat->nr_periods < 0 ||
	    stat->nr_throttled < 0 || stat->throttled_usec < 0)
		return -1;
	return 0;
}

static int read_sched_debug_throttle_state(const char *cg_path,
					   struct sched_debug_throttle_state *state)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	bool in_block = false;
	int ret = 0;
	long throttled = 0;
	long throttle_count = 0;

	memset(state, 0, sizeof(*state));

	fp = fopen(SCHED_DEBUG_PATH, "re");
	if (!fp)
		return -errno;

	while (getline(&line, &len, fp) >= 0) {
		if (!strncmp(line, "cfs_rq[", 7)) {
			char *path = strchr(line, ':');

			if (path) {
				char *end = path + strlen(path);

				while (end > path && isspace(end[-1]))
					*--end = '\0';
				in_block = !strcmp(path + 1, cg_path);
			} else {
				in_block = false;
			}
			continue;
		}

		if (!in_block)
			continue;

		if (strstr(line, ".throttled")) {
			char *p = strrchr(line, ':');
			long val;

			if (p) {
				val = atol(p + 1);
				if (val > throttled)
					throttled = val;
			}
		} else if (strstr(line, "throttle_count")) {
			char *p = strrchr(line, ':');
			long val;

			if (p) {
				val = atol(p + 1);
				if (val > throttle_count)
					throttle_count = val;
			}
		}
	}

	if (ferror(fp))
		ret = -EIO;

	state->throttled = throttled;
	state->throttle_count = throttle_count;
	free(line);
	fclose(fp);
	return ret;
}

static int wait_for_cpu_stat_throttle(const char *cg,
				      const struct cpu_stat_snapshot *baseline,
				      unsigned int retries,
				      unsigned int wait_us)
{
	struct cpu_stat_snapshot now;
	unsigned int i;

	for (i = 0; i < retries; i++) {
		if (read_cpu_stat_snapshot(cg, &now))
			return -1;
		if (now.usage_usec > baseline->usage_usec &&
		    now.nr_periods > baseline->nr_periods &&
		    now.nr_throttled > baseline->nr_throttled &&
		    now.throttled_usec > baseline->throttled_usec)
			return 0;
		usleep(wait_us);
	}

	return -1;
}

static int
wait_for_throttle_state_any(const char *cg_path, unsigned int retries,
			    unsigned int wait_us)
{
	struct sched_debug_throttle_state state;
	unsigned int i;

	for (i = 0; i < retries; i++) {
		if (read_sched_debug_throttle_state(cg_path, &state))
			return -1;
		if (state.throttle_count > 0 || state.throttled > 0)
			return 0;
		usleep(wait_us);
	}

	return -1;
}

static int
wait_for_throttle_state_clear(const char *cg_path, unsigned int retries,
			      unsigned int wait_us)
{
	struct sched_debug_throttle_state state;
	unsigned int i;

	for (i = 0; i < retries; i++) {
		if (read_sched_debug_throttle_state(cg_path, &state))
			return -1;
		if (state.throttle_count == 0 && state.throttled == 0)
			return 0;
		usleep(wait_us);
	}

	return -1;
}

static void cleanup_workers(struct worker_proc *workers, int nr_workers)
{
	int i;

	for (i = 0; i < nr_workers; i++) {
		if (workers[i].pid > 0) {
			kill(workers[i].pid, SIGKILL);
			waitpid(workers[i].pid, NULL, 0);
		}
	}
}

static int read_thread_siblings(int cpu, struct cpu_pair *pair)
{
	char path[PATH_MAX];
	char buf[128];
	char *token, *saveptr;
	int cpus[64];
	int nr = 0;
	int ret;

	snprintf(path, sizeof(path), CPU_TOPOLOGY_PATH "/cpu%d/topology/thread_siblings_list", cpu);
	ret = read_text_file(path, buf, sizeof(buf));
	if (ret)
		return ret;

	for (token = strtok_r(buf, ",\n", &saveptr); token;
	     token = strtok_r(NULL, ",\n", &saveptr)) {
		char *dash = strchr(token, '-');

		if (dash) {
			int start = atoi(token);
			int end = atoi(dash + 1);
			int c;

			for (c = start; c <= end && nr < (int)ARRAY_SIZE(cpus); c++)
				cpus[nr++] = c;
		} else if (nr < (int)ARRAY_SIZE(cpus)) {
			cpus[nr++] = atoi(token);
		}
	}

	if (nr < 2)
		return -ENOENT;

	pair->cpu0 = cpus[0];
	pair->cpu1 = cpus[1];
	return 0;
}

static int find_smt_pair(struct cpu_pair *pair)
{
	int cpu;

	for (cpu = 0; cpu < get_nprocs_conf(); cpu++) {
		if (!access(CPU_TOPOLOGY_PATH, F_OK)) {
			if (!read_thread_siblings(cpu, pair) && pair->cpu0 != pair->cpu1)
				return 0;
		}
	}

	return -ENOENT;
}

static int make_cpucg_tree(const struct cgroup_test_env *env, const char *root,
			   char **parent_out, char **expeller_out,
			   char **expellee_out, char **normal_out)
{
	char *parent, *expeller, *expellee, *normal;

	parent = cg_name(root, TEST_PREFIX "_parent");
	expeller = cg_name(parent, "expeller");
	expellee = cg_name(parent, "expellee");
	normal = cg_name(parent, "normal");
	if (!parent || !expeller || !expellee || !normal)
		return -ENOMEM;

	if (cg_create(parent))
		goto err;
	if (enable_cpu_controller_if_needed(env, parent))
		goto err;
	if (cg_create(expeller) || cg_create(expellee) || cg_create(normal))
		goto err;

	*parent_out = parent;
	*expeller_out = expeller;
	*expellee_out = expellee;
	*normal_out = normal;
	return 0;
err:
	cg_destroy(normal);
	cg_destroy(expellee);
	cg_destroy(expeller);
	cg_destroy(parent);
	free(normal);
	free(expellee);
	free(expeller);
	free(parent);
	return -1;
}

static void free_cpucg_tree(char *parent, char *expeller, char *expellee, char *normal)
{
	cg_destroy(expeller);
	cg_destroy(expellee);
	cg_destroy(normal);
	cg_destroy(parent);
	free(expeller);
	free(expellee);
	free(normal);
	free(parent);
}

static int test_identity_interface(const struct cgroup_test_env *env, const char *root)
{
	char *cg = NULL;
	long value;
	int ret = KSFT_FAIL;

	cg = cg_name(root, TEST_PREFIX "_identity");
	if (!cg)
		return KSFT_FAIL;

	if (cg_create(cg))
		goto out;

	if (access(cg_control(cg, env->identity_ctrl), F_OK))
		goto out;

	value = cg_read_long(cg, env->identity_ctrl);
	if (value != 0)
		goto out;

	if (cg_write(cg, env->identity_ctrl, "-1"))
		goto out;
	if (cg_read_long(cg, env->identity_ctrl) != -1)
		goto out;
	if (cg_write(cg, env->identity_ctrl, "1"))
		goto out;
	if (cg_read_long(cg, env->identity_ctrl) != 1)
		goto out;
	if (cg_write(cg, env->identity_ctrl, "0"))
		goto out;
	if (cg_read_long(cg, env->identity_ctrl) != 0)
		goto out;

	ret = KSFT_PASS;
out:
	cg_destroy(cg);
	free(cg);
	return ret;
}

static int test_sysctl_interface(void)
{
	long old_value, value;
	int ret = KSFT_FAIL;

	if (read_long_file(SYSCTL_IDLE_SHARES_RELAX_PATH, &old_value))
		return KSFT_SKIP;

	if (write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, "0"))
		goto out;
	if (read_long_file(SYSCTL_IDLE_SHARES_RELAX_PATH, &value))
		goto out;
	if (value != 0)
		goto out;

	if (write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, "1"))
		goto out;
	if (read_long_file(SYSCTL_IDLE_SHARES_RELAX_PATH, &value))
		goto out;
	if (value != 1)
		goto out;

	if (write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, "2") >= 0)
		goto out;
	if (write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, "-1") >= 0)
		goto out;
	if (write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, "abc") >= 0)
		goto out;

	ret = KSFT_PASS;
out:
	write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, old_value ? "1" : "0");
	return ret;
}

static int test_feature_interface(void)
{
	int smt_expel;
	int share_core;

	smt_expel = feature_enabled("ID_SMT_EXPEL");
	share_core = feature_enabled("ID_EXPELLER_SHARE_CORE");
	if (smt_expel < 0 || share_core < 0)
		return KSFT_SKIP;

	return KSFT_PASS;
}

static int test_sched_debug_interface(void)
{
	int ret1, ret2;

	ret1 = file_contains_token(SCHED_DEBUG_PATH, "h_nr_expeller");
	ret2 = file_contains_token(SCHED_DEBUG_PATH, "h_nr_expellee");
	if (ret1 < 0 || ret2 < 0)
		return KSFT_SKIP;
	if (ret1 != 1 || ret2 != 1)
		return KSFT_SKIP;
	return KSFT_PASS;
}

static int test_identity_hierarchy_counts(const struct cgroup_test_env *env, const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc workers[3] = {};
	struct sched_debug_counts counts;
	char parent_path[PATH_MAX];
	char expeller_path[PATH_MAX];
	char expellee_path[PATH_MAX];
	volatile struct timespec *shared_start = NULL;
	int i;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	if (cg_write(expeller, env->identity_ctrl, "1") ||
	    cg_write(expellee, env->identity_ctrl, "-1") ||
	    cg_write(normal, env->identity_ctrl, "0"))
		goto out;

	shared_start = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared_start == MAP_FAILED) {
		shared_start = NULL;
		goto out;
	}
	shared_start->tv_sec = 0;
	shared_start->tv_nsec = 0;

	if (spawn_worker(&workers[0], expeller, 0, 0, false, 1200, shared_start) ||
	    spawn_worker(&workers[1], expellee, 0, 0, false, 1200, shared_start) ||
	    spawn_worker(&workers[2], normal, 0, 0, false, 1200, shared_start))
		goto out;

	for (i = 0; i < 3; i++)
		if (start_worker(&workers[i]))
			goto out;

	clock_gettime(CLOCK_MONOTONIC, (struct timespec *)shared_start);

	usleep(200000);
	snprintf(parent_path, sizeof(parent_path), "/%s", strrchr(parent, '/') + 1);
	snprintf(expeller_path, sizeof(expeller_path), "%s/%s", parent_path,
		 strrchr(expeller, '/') + 1);
	snprintf(expellee_path, sizeof(expellee_path), "%s/%s", parent_path,
		 strrchr(expellee, '/') + 1);
	if (wait_for_counts_ge(expeller_path, 1, 0, 30, 20000))
		goto out;
	if (wait_for_counts_ge(expellee_path, 0, 1, 30, 20000))
		goto out;

	for (i = 0; i < 3; i++)
		if (reap_worker(&workers[i]))
			goto out;

	usleep(100000);
	if (read_sched_debug_counts(expeller_path, &counts))
		goto out;
	if (counts.expeller != 0 || counts.expellee != 0)
		goto out;
	if (read_sched_debug_counts(expellee_path, &counts))
		goto out;
	if (counts.expeller != 0 || counts.expellee != 0)
		goto out;

	ret = KSFT_PASS;
out:
	if (shared_start)
		munmap((void *)shared_start, sizeof(struct timespec));
	for (i = 0; i < 3; i++) {
		if (workers[i].pid > 0) {
			kill(workers[i].pid, SIGKILL);
			waitpid(workers[i].pid, NULL, 0);
		}
	}
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int test_core_sched_cookie(const struct cgroup_test_env *env, const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc worker = {};
	unsigned long long cookie_expeller, cookie_expellee;
	int ret = KSFT_FAIL;

	if (!get_cs_cookie(getpid()))
		ksft_print_msg("core-sched cookie on current task is 0; "
			       "continuing with target tasks only\n");

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	if (cg_write(expeller, env->identity_ctrl, "1") ||
	    cg_write(expellee, env->identity_ctrl, "-1"))
		goto out;

	if (spawn_worker(&worker, expeller, 0, 0, false, 1200, NULL) || start_worker(&worker))
		goto out;

	if (wait_for_cookie_nonzero(worker.pid, &cookie_expeller, 30, 20000))
		goto out;

	if (cg_enter(normal, worker.pid))
		goto out;
	if (wait_for_cookie_value(worker.pid, 0, 30, 20000))
		goto out;
	if (cg_enter(expellee, worker.pid))
		goto out;
	if (wait_for_cookie_distinct(worker.pid, cookie_expeller,
				     &cookie_expellee, 30, 20000))
		goto out;

	if (reap_worker(&worker))
		goto out;
	worker.pid = 0;

	ret = KSFT_PASS;
out:
	if (worker.pid > 0) {
		kill(worker.pid, SIGKILL);
		waitpid(worker.pid, NULL, 0);
	}
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int test_identity_switch_onrq_counts(const struct cgroup_test_env *env, const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc worker = {};
	unsigned long long cookie1, cookie2;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	if (spawn_worker(&worker, normal, 0, 0, false, 900, NULL) || start_worker(&worker))
		goto out;

	usleep(150000);
	if (cg_write(normal, env->identity_ctrl, "1"))
		goto out;
	usleep(50000);
	cookie1 = get_cs_cookie(worker.pid);
	if (!cookie1)
		goto out;

	if (cg_write(normal, env->identity_ctrl, "-1"))
		goto out;
	usleep(50000);
	cookie2 = get_cs_cookie(worker.pid);
	if (!cookie2 || cookie2 == cookie1)
		goto out;

	if (cg_write(normal, env->identity_ctrl, "0"))
		goto out;
	usleep(50000);
	if (get_cs_cookie(worker.pid) != 0)
		goto out;

	if (reap_worker(&worker))
		goto out;
	worker.pid = 0;
	ret = KSFT_PASS;
out:
	if (worker.pid > 0) {
		kill(worker.pid, SIGKILL);
		waitpid(worker.pid, NULL, 0);
	}
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int test_identity_migration_interleaving(const struct cgroup_test_env *env, const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc worker = {};
	unsigned long long cookie_expeller, cookie_expellee;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	if (cg_write(expeller, env->identity_ctrl, "1") ||
	    cg_write(expellee, env->identity_ctrl, "-1")) {
		ksft_print_msg("migration: initial identity write failed\n");
		goto out;
	}

	if (spawn_worker(&worker, expeller, 0, 0, false, 2200, NULL) ||
	    start_worker(&worker)) {
		ksft_print_msg("migration: spawn/start worker failed\n");
		goto out;
	}

	if (wait_for_cookie_nonzero(worker.pid, &cookie_expeller, 50, 20000)) {
		ksft_print_msg("migration: expeller cookie did not appear, cookie=%llu\n",
			       get_cs_cookie(worker.pid));
		goto out;
	}

	if (cg_enter(normal, worker.pid)) {
		ksft_print_msg("migration: move to normal failed\n");
		goto out;
	}
	if (wait_for_cookie_value(worker.pid, 0, 50, 20000)) {
		ksft_print_msg("migration: cookie did not clear in normal, cookie=%llu\n",
			       get_cs_cookie(worker.pid));
		goto out;
	}

	if (cg_write(normal, env->identity_ctrl, "-1")) {
		ksft_print_msg("migration: write normal=-1 failed\n");
		goto out;
	}
	if (wait_for_cookie_distinct(worker.pid, cookie_expeller,
				     &cookie_expellee, 50, 20000)) {
		ksft_print_msg("migration: expellee cookie did not appear, old=%llu now=%llu\n",
			       cookie_expeller, get_cs_cookie(worker.pid));
		goto out;
	}

	if (cg_enter(expeller, worker.pid)) {
		ksft_print_msg("migration: move back to expeller failed\n");
		goto out;
	}
	if (wait_for_cookie_value(worker.pid, cookie_expeller, 50, 20000)) {
		ksft_print_msg("migration: expeller cookie did not restore, want=%llu now=%llu\n",
			       cookie_expeller, get_cs_cookie(worker.pid));
		goto out;
	}

	if (reap_worker(&worker)) {
		ksft_print_msg("migration: reap worker failed\n");
		goto out;
	}
	worker.pid = 0;
	ret = KSFT_PASS;
out:
	if (worker.pid > 0) {
		kill(worker.pid, SIGKILL);
		waitpid(worker.pid, NULL, 0);
	}
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int test_identity_throttle_switch_counts(const struct cgroup_test_env *env, const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc worker = {};
	unsigned long long cookie_expeller, cookie_expellee;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	if (enable_cfs_throttle(env, normal))
		goto out;
	if (spawn_worker(&worker, normal, 0, 0, false, 1500, NULL) || start_worker(&worker))
		goto out;

	usleep(200000);
	if (cg_write(normal, env->identity_ctrl, "1"))
		goto out;
	usleep(50000);
	cookie_expeller = get_cs_cookie(worker.pid);
	if (!cookie_expeller)
		goto out;

	if (cg_write(normal, env->identity_ctrl, "-1"))
		goto out;
	usleep(50000);
	cookie_expellee = get_cs_cookie(worker.pid);
	if (!cookie_expellee || cookie_expellee == cookie_expeller)
		goto out;

	if (cg_write(normal, env->identity_ctrl, "0"))
		goto out;
	usleep(50000);
	if (get_cs_cookie(worker.pid) != 0)
		goto out;

	if (reap_worker(&worker))
		goto out;
	worker.pid = 0;
	ret = KSFT_PASS;
out:
	if (worker.pid > 0) {
		kill(worker.pid, SIGKILL);
		waitpid(worker.pid, NULL, 0);
	}
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int test_nested_identity_throttle_pressure(const struct cgroup_test_env *env,
						  const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	char *nested = NULL, *nested_hot = NULL, *nested_cold = NULL;
	struct worker_proc workers[4] = {};
	char parent_path[PATH_MAX];
	struct sched_debug_counts counts;
	volatile struct timespec *shared_start = NULL;
	int i;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	nested = cg_name(normal, "nested");
	nested_hot = cg_name(nested, "hot");
	nested_cold = cg_name(nested, "cold");
	if (!nested || !nested_hot || !nested_cold) {
		ksft_print_msg("nested alloc failed\n");
		goto out;
	}
	if (enable_cpu_controller_if_needed(env, normal) ||
	    cg_create(nested) || enable_cpu_controller_if_needed(env, nested) ||
	    cg_create(nested_hot) || cg_create(nested_cold)) {
		ksft_print_msg("nested cgroup create failed\n");
		goto out;
	}
	if (enable_cfs_throttle(env, nested)) {
		ksft_print_msg("enable nested throttle failed\n");
		goto out;
	}

	snprintf(parent_path, sizeof(parent_path), "/%s", strrchr(parent, '/') + 1);

	shared_start = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared_start == MAP_FAILED) {
		shared_start = NULL;
		ksft_print_msg("nested shared_start mmap failed\n");
		goto out;
	}
	shared_start->tv_sec = 0;
	shared_start->tv_nsec = 0;

	if (spawn_worker(&workers[0], expeller, 0, 0, false, 1800, shared_start) ||
	    spawn_worker(&workers[1], expellee, 0, 0, false, 1800, shared_start) ||
	    spawn_worker(&workers[2], nested_hot, 0, 0, false, 1800, shared_start) ||
	    spawn_worker(&workers[3], nested_cold, 1, 1, false, 1800, shared_start)) {
		ksft_print_msg("spawn nested workers failed\n");
		goto out;
	}

	for (i = 0; i < 4; i++)
		if (start_worker(&workers[i])) {
			ksft_print_msg("start nested worker %d failed\n", i);
			goto out;
		}

	clock_gettime(CLOCK_MONOTONIC, (struct timespec *)shared_start);

	if (cg_write(expeller, env->identity_ctrl, "1") ||
	    cg_write(expellee, env->identity_ctrl, "-1") ||
	    cg_write(normal, env->identity_ctrl, "1")) {
		ksft_print_msg("initial nested identity writes failed\n");
		goto out;
	}

	usleep(200000);
	if (cg_write(normal, env->identity_ctrl, "-1")) {
		ksft_print_msg("flip to -1 failed\n");
		goto out;
	}
	usleep(100000);
	if (cg_write(normal, env->identity_ctrl, "0")) {
		ksft_print_msg("flip to 0 failed\n");
		goto out;
	}
	usleep(100000);
	if (cg_write(normal, env->identity_ctrl, "1")) {
		ksft_print_msg("flip back to 1 failed\n");
		goto out;
	}
	usleep(150000);

	if (read_sched_debug_counts(parent_path, &counts)) {
		ksft_print_msg("read nested counts during run failed\n");
		goto out;
	}
	if (counts.expeller < 0 || counts.expellee < 0) {
		ksft_print_msg("nested counts went negative: expeller=%ld expellee=%ld\n",
			       counts.expeller, counts.expellee);
		goto out;
	}

	for (i = 0; i < 4; i++)
		if (reap_worker(&workers[i])) {
			ksft_print_msg("reap nested worker %d failed\n", i);
			goto out;
		}
	memset(workers, 0, sizeof(workers));

	usleep(100000);
	if (read_sched_debug_counts(parent_path, &counts)) {
		ksft_print_msg("read nested counts after exit failed\n");
		goto out;
	}
	if (counts.expeller != 0 || counts.expellee != 0) {
		ksft_print_msg("nested residual counts: expeller=%ld expellee=%ld\n",
			       counts.expeller, counts.expellee);
		goto out;
	}

	ret = KSFT_PASS;
out:
	cleanup_workers(workers, 4);
	if (shared_start)
		munmap((void *)shared_start, sizeof(struct timespec));
	if (nested_cold)
		cg_destroy(nested_cold);
	if (nested_hot)
		cg_destroy(nested_hot);
	if (nested)
		cg_destroy(nested);
	free(nested_cold);
	free(nested_hot);
	free(nested);
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int test_identity_repeated_flip_pressure(const struct cgroup_test_env *env,
						const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc workers[3] = {};
	char parent_path[PATH_MAX];
	struct sched_debug_counts counts;
	volatile struct timespec *shared_start = NULL;
	int i;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	snprintf(parent_path, sizeof(parent_path), "/%s", strrchr(parent, '/') + 1);

	shared_start = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared_start == MAP_FAILED) {
		shared_start = NULL;
		goto out;
	}
	shared_start->tv_sec = 0;
	shared_start->tv_nsec = 0;

	if (spawn_worker(&workers[0], normal, 0, 0, false, 2200, shared_start) ||
	    spawn_worker(&workers[1], normal, 1, 1, false, 2200, shared_start) ||
	    spawn_worker(&workers[2], normal, 2, 2, false, 2200, shared_start))
		goto out;

	for (i = 0; i < 3; i++)
		if (start_worker(&workers[i]))
			goto out;

	clock_gettime(CLOCK_MONOTONIC, (struct timespec *)shared_start);

	for (i = 0; i < 12; i++) {
		const char *val;

		switch (i % 3) {
		case 0:
			val = "1";
			break;
		case 1:
			val = "-1";
			break;
		default:
			val = "0";
			break;
		}

		if (cg_write(normal, env->identity_ctrl, (char *)val))
			goto out;
		usleep(70000);
	}

	if (cg_write(normal, env->identity_ctrl, "0"))
		goto out;

	for (i = 0; i < 3; i++)
		if (reap_worker(&workers[i]))
			goto out;
	memset(workers, 0, sizeof(workers));

	usleep(100000);
	if (read_sched_debug_counts(parent_path, &counts))
		goto out;
	if (counts.expeller != 0 || counts.expellee != 0)
		goto out;

	ret = KSFT_PASS;
out:
	cleanup_workers(workers, 3);
	if (shared_start)
		munmap((void *)shared_start, sizeof(struct timespec));
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int test_identity_combo_matrix(const struct cgroup_test_env *env,
				      const char *root)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc workers[3] = {};
	char parent_path[PATH_MAX];
	struct sched_debug_counts counts;
	unsigned long long cookie_expeller, cookie_expellee, cookie_normal;
	volatile struct timespec *shared_start = NULL;
	int i;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	snprintf(parent_path, sizeof(parent_path), "/%s", strrchr(parent, '/') + 1);

	if (cg_write(expeller, env->identity_ctrl, "1") ||
	    cg_write(expellee, env->identity_ctrl, "-1")) {
		ksft_print_msg("combo: initial identity write failed\n");
		goto out;
	}

	if (enable_cfs_throttle(env, normal)) {
		ksft_print_msg("combo: enable throttle failed\n");
		goto out;
	}
	shared_start = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared_start == MAP_FAILED) {
		shared_start = NULL;
		ksft_print_msg("combo: shared_start mmap failed\n");
		goto out;
	}
	shared_start->tv_sec = 0;
	shared_start->tv_nsec = 0;

	if (spawn_worker(&workers[0], expeller, 0, 0, false, 2600, shared_start) ||
	    spawn_worker(&workers[1], expellee, 1, 1, false, 2600, shared_start) ||
	    spawn_worker(&workers[2], normal, 2, 2, false, 2600, shared_start)) {
		ksft_print_msg("combo: spawn workers failed\n");
		goto out;
	}

	for (i = 0; i < 3; i++)
		if (start_worker(&workers[i])) {
			ksft_print_msg("combo: start worker %d failed\n", i);
			goto out;
		}

	clock_gettime(CLOCK_MONOTONIC, (struct timespec *)shared_start);

	if (wait_for_counts_ge(parent_path, 1, 1, 30, 20000)) {
		if (!read_sched_debug_counts(parent_path, &counts))
			ksft_print_msg("combo: initial counts missing e=%ld x=%ld\n",
				       counts.expeller, counts.expellee);
		goto out;
	}

	if (wait_for_cookie_nonzero(workers[0].pid, &cookie_expeller, 50, 20000)) {
		ksft_print_msg("combo: expeller cookie missing, cookie=%llu\n",
			       get_cs_cookie(workers[0].pid));
		goto out;
	}
	if (wait_for_cookie_nonzero(workers[1].pid, &cookie_expellee, 50, 20000)) {
		ksft_print_msg("combo: expellee cookie missing, cookie=%llu\n",
			       get_cs_cookie(workers[1].pid));
		goto out;
	}

	if (cg_enter(normal, workers[0].pid)) {
		ksft_print_msg("combo: move expeller worker into normal failed\n");
		goto out;
	}
	if (wait_for_cookie_value(workers[0].pid, 0, 50, 20000)) {
		ksft_print_msg("combo: cookie did not clear after move to normal, cookie=%llu\n",
			       get_cs_cookie(workers[0].pid));
		goto out;
	}

	if (cg_write(normal, env->identity_ctrl, "1")) {
		ksft_print_msg("combo: write normal=1 failed\n");
		goto out;
	}
	if (wait_for_cookie_nonzero(workers[0].pid, &cookie_normal, 50, 20000)) {
		ksft_print_msg("combo: cookie did not become nonzero after normal=1, cookie=%llu\n",
			       get_cs_cookie(workers[0].pid));
		goto out;
	}

	if (cg_write(normal, env->identity_ctrl, "-1")) {
		ksft_print_msg("combo: write normal=-1 failed\n");
		goto out;
	}
	if (wait_for_cookie_distinct(workers[0].pid, cookie_normal,
				     &cookie_normal, 50, 20000)) {
		ksft_print_msg("combo: cookie did not switch on normal=-1, cookie=%llu\n",
			       get_cs_cookie(workers[0].pid));
		goto out;
	}

	if (cg_write(normal, env->identity_ctrl, "0")) {
		ksft_print_msg("combo: write normal=0 failed\n");
		goto out;
	}
	if (wait_for_cookie_value(workers[0].pid, 0, 50, 20000)) {
		ksft_print_msg("combo: cookie did not clear after normal=0, cookie=%llu\n",
			       get_cs_cookie(workers[0].pid));
		goto out;
	}

	for (i = 0; i < 3; i++)
		if (reap_worker(&workers[i])) {
			ksft_print_msg("combo: reap worker %d failed\n", i);
			goto out;
		}
	memset(workers, 0, sizeof(workers));

	usleep(100000);
	if (read_sched_debug_counts(parent_path, &counts)) {
		ksft_print_msg("combo: final count read failed\n");
		goto out;
	}
	if (counts.expeller != 0 || counts.expellee != 0) {
		ksft_print_msg("combo: residual counts expeller=%ld expellee=%ld\n",
			       counts.expeller, counts.expellee);
		goto out;
	}

	ret = KSFT_PASS;
out:
	cleanup_workers(workers, 3);
	if (shared_start)
		munmap((void *)shared_start, sizeof(struct timespec));
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int
run_multi_child_parent_throttle_pressure(const struct cgroup_test_env *env,
					 const char *root, int nr_children,
					 bool churn_identity, const char *tag)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	char *throttle_parent = NULL;
	char *children[3] = {};
	char child_paths[3][PATH_MAX];
	struct cpu_stat_snapshot base_children[3];
	struct cpu_stat_snapshot now_children[3];
	struct worker_proc workers[10] = {};
	struct cpu_stat_snapshot base_parent, now_parent;
	struct sched_debug_counts counts;
	unsigned long long cookie_expeller, cookie_expellee;
	char parent_path[PATH_MAX];
	char throttle_parent_path[PATH_MAX];
	volatile struct timespec *shared_start = NULL;
	int worker_idx = 0;
	int i;
	int ret = KSFT_FAIL;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		return KSFT_FAIL;

	throttle_parent = cg_name(normal, "throttle_parent");
	if (!throttle_parent) {
		ksft_print_msg("%s: cgroup name alloc failed\n", tag);
		goto out;
	}

	for (i = 0; i < nr_children; i++) {
		char name[16];

		snprintf(name, sizeof(name), "child%d", i);
		children[i] = cg_name(throttle_parent, name);
		if (!children[i]) {
			ksft_print_msg("%s: child cgroup name alloc failed\n", tag);
			goto out;
		}
	}

	if (enable_cpu_controller_if_needed(env, normal) ||
	    cg_create(throttle_parent) ||
	    enable_cpu_controller_if_needed(env, throttle_parent)) {
		ksft_print_msg("%s: nested cgroup create failed\n", tag);
		goto out;
	}

	for (i = 0; i < nr_children; i++)
		if (cg_create(children[i])) {
			ksft_print_msg("%s: child cgroup create failed\n", tag);
			goto out;
		}

	snprintf(parent_path, sizeof(parent_path), "/%s", strrchr(parent, '/') + 1);
	snprintf(throttle_parent_path, sizeof(throttle_parent_path), "%s/%s/%s",
		 parent_path, strrchr(normal, '/') + 1, strrchr(throttle_parent, '/') + 1);
	for (i = 0; i < nr_children; i++)
		snprintf(child_paths[i], sizeof(child_paths[i]), "%s/%s",
			 throttle_parent_path, strrchr(children[i], '/') + 1);

	if (cg_write(expeller, env->identity_ctrl, "1") ||
	    cg_write(expellee, env->identity_ctrl, "-1")) {
		ksft_print_msg("%s: initial identity write failed\n", tag);
		goto out;
	}

	if (set_cfs_quota_period(env, throttle_parent, 40000, 100000)) {
		ksft_print_msg("%s: parent quota failed\n", tag);
		goto out;
	}
	for (i = 0; i < nr_children; i++)
		if (set_cfs_quota_period(env, children[i], 20000, 100000)) {
			ksft_print_msg("%s: child%d quota failed\n", tag, i);
			goto out;
		}

	if (read_cpu_stat_snapshot(throttle_parent, &base_parent)) {
		ksft_print_msg("%s: baseline parent cpu.stat read failed\n", tag);
		goto out;
	}
	for (i = 0; i < nr_children; i++)
		if (read_cpu_stat_snapshot(children[i], &base_children[i])) {
			ksft_print_msg("%s: baseline child%d cpu.stat read failed\n", tag, i);
			goto out;
		}

	shared_start = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared_start == MAP_FAILED) {
		shared_start = NULL;
		ksft_print_msg("%s: shared_start mmap failed\n", tag);
		goto out;
	}
	shared_start->tv_sec = 0;
	shared_start->tv_nsec = 0;

	if (spawn_worker(&workers[worker_idx], expeller, 0, 0, false, 2600, shared_start)) {
		ksft_print_msg("%s: expeller worker spawn failed\n", tag);
		goto out;
	}
	worker_idx++;
	if (spawn_worker(&workers[worker_idx], expellee, 1, 1, false, 2600, shared_start)) {
		ksft_print_msg("%s: expellee worker spawn failed\n", tag);
		goto out;
	}
	worker_idx++;

	for (i = 0; i < nr_children; i++) {
		if (spawn_worker(&workers[worker_idx], children[i], 0, 0, false, 2600, shared_start)) {
			ksft_print_msg("%s: child%d worker0 failed\n", tag, i);
			goto out;
		}
		worker_idx++;
		if (spawn_worker(&workers[worker_idx], children[i], 1, 1, false, 2600, shared_start)) {
			ksft_print_msg("%s: child%d worker1 failed\n", tag, i);
			goto out;
		}
		worker_idx++;
	}

	for (i = 0; i < worker_idx; i++)
		if (start_worker(&workers[i])) {
			ksft_print_msg("%s: start worker %d failed\n", tag, i);
			goto out;
		}

	clock_gettime(CLOCK_MONOTONIC, (struct timespec *)shared_start);

	if (wait_for_cookie_nonzero(workers[0].pid, &cookie_expeller, 50, 20000) ||
	    wait_for_cookie_nonzero(workers[1].pid, &cookie_expellee, 50, 20000) ||
	    cookie_expeller == cookie_expellee) {
		ksft_print_msg("%s: identity cookies not established expeller=%llu expellee=%llu\n",
			       tag, cookie_expeller, cookie_expellee);
		goto out;
	}

	if (wait_for_counts_ge(parent_path, 1, 1, 50, 20000)) {
		if (!read_sched_debug_counts(parent_path, &counts))
			ksft_print_msg("%s: top counts not visible expeller=%ld expellee=%ld\n",
				       tag, counts.expeller, counts.expellee);
		goto out;
	}

	if (wait_for_cpu_stat_throttle(throttle_parent, &base_parent, 80, 20000)) {
		read_cpu_stat_snapshot(throttle_parent, &now_parent);
		ksft_print_msg("%s: parent throttle miss u=%ld p=%ld nt=%ld tu=%ld\n", tag,
			       now_parent.usage_usec - base_parent.usage_usec,
			       now_parent.nr_periods - base_parent.nr_periods,
			       now_parent.nr_throttled - base_parent.nr_throttled,
			       now_parent.throttled_usec - base_parent.throttled_usec);
		goto out;
	}

	if (read_cpu_stat_snapshot(throttle_parent, &now_parent)) {
		ksft_print_msg("%s: parent cpu.stat readback failed\n", tag);
		goto out;
	}
	for (i = 0; i < nr_children; i++)
		if (read_cpu_stat_snapshot(children[i], &now_children[i])) {
			ksft_print_msg("%s: child%d cpu.stat readback failed\n", tag, i);
			goto out;
		}

	if (!(now_parent.nr_throttled > base_parent.nr_throttled &&
	      now_parent.throttled_usec > base_parent.throttled_usec)) {
		ksft_print_msg("%s: parent did not throttle as expected\n", tag);
		goto out;
	}

	for (i = 0; i < nr_children; i++)
		if (!(now_children[i].usage_usec > base_children[i].usage_usec)) {
			ksft_print_msg("%s: child%d was not active usage=%ld\n",
				       tag, i,
				       now_children[i].usage_usec - base_children[i].usage_usec);
			goto out;
		}

	for (i = 0; i < nr_children; i++)
		if (wait_for_throttle_state_any(child_paths[i], 80, 20000)) {
			struct sched_debug_throttle_state st = {};

			read_sched_debug_throttle_state(child_paths[i], &st);
			ksft_print_msg("%s: child%d throttle miss t=%ld c=%ld\n",
				       tag, i, st.throttled, st.throttle_count);
			break;
		}

	if (wait_for_throttle_state_any(throttle_parent_path, 80, 20000)) {
		struct sched_debug_throttle_state tp = {};

		read_sched_debug_throttle_state(throttle_parent_path, &tp);
		ksft_print_msg("%s: parent throttle miss t=%ld c=%ld\n", tag,
			       tp.throttled, tp.throttle_count);
	}

	if (churn_identity) {
		if (cg_read_long(children[0], env->identity_ctrl) != 0) {
			ksft_print_msg("%s: child0 initial identity readback failed\n", tag);
			goto out;
		}
		if (cg_write(children[0], env->identity_ctrl, "1") ||
		    cg_read_long(children[0], env->identity_ctrl) != 1) {
			ksft_print_msg("%s: child0 identity did not switch to 1\n", tag);
			goto out;
		}
		if (cg_write(children[0], env->identity_ctrl, "-1") ||
		    cg_read_long(children[0], env->identity_ctrl) != -1) {
			ksft_print_msg("%s: child0 identity did not switch to -1\n", tag);
			goto out;
		}
		if (cg_write(children[0], env->identity_ctrl, "0") ||
		    cg_read_long(children[0], env->identity_ctrl) != 0) {
			ksft_print_msg("%s: child0 identity did not switch back to 0\n", tag);
			goto out;
		}
	}

	for (i = 0; i < worker_idx; i++)
		if (reap_worker(&workers[i])) {
			ksft_print_msg("%s: reap worker %d failed\n", tag, i);
			goto out;
		}
	memset(workers, 0, sizeof(workers));

	if (!wait_for_throttle_state_any(throttle_parent_path, 80, 20000))
		if (wait_for_throttle_state_clear(throttle_parent_path, 80, 20000)) {
			ksft_print_msg("%s: residual parent throttle state remained\n", tag);
			goto out;
		}
	for (i = 0; i < nr_children; i++)
		if (!wait_for_throttle_state_any(child_paths[i], 80, 20000))
			if (wait_for_throttle_state_clear(child_paths[i], 80, 20000)) {
				ksft_print_msg("%s: residual child%d throttle state remained\n",
					       tag, i);
				goto out;
			}

	if (read_sched_debug_counts(parent_path, &counts)) {
		ksft_print_msg("%s: final parent count read failed\n", tag);
		goto out;
	}
	if (counts.expeller != 0 || counts.expellee != 0) {
		ksft_print_msg("%s: residual counts expeller=%ld expellee=%ld\n",
			       tag, counts.expeller, counts.expellee);
		goto out;
		}

	ret = KSFT_PASS;
out:
	cleanup_workers(workers, ARRAY_SIZE(workers));
	if (shared_start)
		munmap((void *)shared_start, sizeof(struct timespec));
	for (i = nr_children - 1; i >= 0; i--) {
		if (children[i])
			cg_destroy(children[i]);
		free(children[i]);
	}
	if (throttle_parent)
		cg_destroy(throttle_parent);
	free(throttle_parent);
	free_cpucg_tree(parent, expeller, expellee, normal);
	return ret;
}

static int
test_identity_multi_child_parent_throttle_pressure(
	const struct cgroup_test_env *env,
					      const char *root)
{
	return run_multi_child_parent_throttle_pressure(env, root, 2, false,
					      "multi-throttle");
}

static int test_identity_three_child_parent_throttle_pressure(const struct cgroup_test_env *env,
							      const char *root)
{
	return run_multi_child_parent_throttle_pressure(env, root, 3, false,
					      "three-child-throttle");
}

static int test_identity_throttle_with_identity_churn(const struct cgroup_test_env *env,
					      const char *root)
{
	return run_multi_child_parent_throttle_pressure(env, root, 2, true,
					      "throttle-churn");
}

static int cmp_ull(const void *a, const void *b)
{
	const unsigned long long *ua = a;
	const unsigned long long *ub = b;

	if (*ua < *ub)
		return -1;
	if (*ua > *ub)
		return 1;
	return 0;
}

static int run_smt_benchmark_once(const struct cgroup_test_env *env, const char *root,
				  bool smt_expel, bool share_core,
				  unsigned long long *expeller_iters,
				  unsigned long long *expellee_iters)
{
	char *parent = NULL, *expeller = NULL, *expellee = NULL, *normal = NULL;
	struct worker_proc wexpeller = {}, wexpellee = {};
	struct cpu_pair pair;
	volatile struct timespec *shared_start = NULL;
	int old_smt, old_share;
	int ret = -1;

	old_smt = feature_enabled("ID_SMT_EXPEL");
	old_share = feature_enabled("ID_EXPELLER_SHARE_CORE");
	if (old_smt < 0 || old_share < 0)
		return -1;
	if (find_smt_pair(&pair))
		return -2;

	if (set_feature_enabled("ID_SMT_EXPEL", smt_expel) ||
	    set_feature_enabled("ID_EXPELLER_SHARE_CORE", share_core))
		return -1;

	if (make_cpucg_tree(env, root, &parent, &expeller, &expellee, &normal))
		goto out_restore;
	if (cg_write(expeller, env->identity_ctrl, "1") ||
	    cg_write(expellee, env->identity_ctrl, "-1"))
		goto out;

	shared_start = mmap(NULL, sizeof(struct timespec), PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared_start == MAP_FAILED) {
		shared_start = NULL;
		goto out;
	}
	shared_start->tv_sec = 0;
	shared_start->tv_nsec = 0;

	if (spawn_worker(&wexpeller, expeller, pair.cpu0, pair.cpu1, true, BENCH_DURATION_MS, shared_start) ||
	    spawn_worker(&wexpellee, expellee, pair.cpu0, pair.cpu1, true, BENCH_DURATION_MS, shared_start))
		goto out;

	if (start_worker(&wexpeller) || start_worker(&wexpellee))
		goto out;

	clock_gettime(CLOCK_MONOTONIC, (struct timespec *)shared_start);

	if (reap_worker(&wexpeller) || reap_worker(&wexpellee))
		goto out;

	*expeller_iters = wexpeller.result.iterations;
	*expellee_iters = wexpellee.result.iterations;
	ret = 0;
out:
	if (wexpeller.pid > 0) {
		kill(wexpeller.pid, SIGKILL);
		waitpid(wexpeller.pid, NULL, 0);
	}
	if (wexpellee.pid > 0) {
		kill(wexpellee.pid, SIGKILL);
		waitpid(wexpellee.pid, NULL, 0);
	}
	if (shared_start)
		munmap((void *)shared_start, sizeof(struct timespec));
	free_cpucg_tree(parent, expeller, expellee, normal);
out_restore:
	set_feature_enabled("ID_SMT_EXPEL", old_smt);
	set_feature_enabled("ID_EXPELLER_SHARE_CORE", old_share);
	return ret;
}

static int run_smt_benchmark(const struct cgroup_test_env *env, const char *root,
			     bool smt_expel, bool share_core,
			     unsigned long long *expeller_iters,
			     unsigned long long *expellee_iters)
{
	unsigned long long expeller_samples[BENCH_ROUNDS];
	unsigned long long expellee_samples[BENCH_ROUNDS];
	int i, ret;

	for (i = 0; i < BENCH_ROUNDS; i++) {
		ret = run_smt_benchmark_once(env, root, smt_expel, share_core,
					     &expeller_samples[i], &expellee_samples[i]);
		if (ret)
			return ret;
	}

	qsort(expeller_samples, BENCH_ROUNDS, sizeof(expeller_samples[0]), cmp_ull);
	qsort(expellee_samples, BENCH_ROUNDS, sizeof(expellee_samples[0]), cmp_ull);
	*expeller_iters = expeller_samples[BENCH_ROUNDS / 2];
	*expellee_iters = expellee_samples[BENCH_ROUNDS / 2];
	return 0;
}

static int test_id_smt_expel_behavior(const struct cgroup_test_env *env, const char *root)
{
	unsigned long long on_expeller, on_expellee;
	unsigned long long off_expeller, off_expellee;
	long long on_gap, off_gap;
	int enabled;
	int ret;

	enabled = feature_enabled("ID_SMT_EXPEL");
	if (enabled <= 0)
		return KSFT_SKIP;

	ret = run_smt_benchmark(env, root, true, true, &on_expeller, &on_expellee);
	if (ret == -2)
		return KSFT_SKIP;
	if (ret)
		return KSFT_FAIL;

	ret = run_smt_benchmark(env, root, false, true, &off_expeller, &off_expellee);
	if (ret)
		return KSFT_FAIL;

	on_gap = (long long)on_expeller - (long long)on_expellee;
	off_gap = (long long)off_expeller - (long long)off_expellee;
	ksft_print_msg("ID_SMT_EXPEL on: expeller=%llu expellee=%llu\n",
		       on_expeller, on_expellee);
	ksft_print_msg("ID_SMT_EXPEL off: expeller=%llu expellee=%llu\n",
		       off_expeller, off_expellee);

	if (on_gap <= 0)
		return KSFT_FAIL;
	if (on_gap <= off_gap)
		return KSFT_FAIL;

	return KSFT_PASS;
}

static int test_id_expeller_share_core_behavior(const struct cgroup_test_env *env, const char *root)
{
	unsigned long long share_expeller, share_expellee;
	unsigned long long noshare_expeller, noshare_expellee;
	long long share_gap, noshare_gap;
	int enabled;
	int ret;

	enabled = feature_enabled("ID_EXPELLER_SHARE_CORE");
	if (enabled <= 0)
		return KSFT_SKIP;

	ret = run_smt_benchmark(env, root, false, true, &share_expeller, &share_expellee);
	if (ret == -2)
		return KSFT_SKIP;
	if (ret)
		return KSFT_FAIL;

	ret = run_smt_benchmark(env, root, false, false, &noshare_expeller, &noshare_expellee);
	if (ret)
		return KSFT_FAIL;

	share_gap = (long long)share_expeller - (long long)share_expellee;
	noshare_gap = (long long)noshare_expeller - (long long)noshare_expellee;
	ksft_print_msg("ID_EXPELLER_SHARE_CORE on: expeller=%llu expellee=%llu\n",
		       share_expeller, share_expellee);
	ksft_print_msg("ID_EXPELLER_SHARE_CORE off: expeller=%llu expellee=%llu\n",
		       noshare_expeller, noshare_expellee);

	if (share_gap == noshare_gap)
		return KSFT_FAIL;
	if (llabs(share_gap - noshare_gap) < 1000000)
		return KSFT_SKIP;
	return KSFT_PASS;
}

static int test_sched_idle_shares_relax(const struct cgroup_test_env *env, const char *root)
{
	char *parent = NULL, *idle = NULL, *normal = NULL;
	long old_sysctl;
	int ret = KSFT_FAIL;

	if (read_long_file(SYSCTL_IDLE_SHARES_RELAX_PATH, &old_sysctl))
		return KSFT_SKIP;

	parent = cg_name(root, TEST_PREFIX "_idle_parent");
	idle = cg_name(parent, "idle");
	normal = cg_name(parent, "normal");
	if (!parent || !idle || !normal)
		goto out;

	if (cg_create(parent) || enable_cpu_controller_if_needed(env, parent) ||
	    cg_create(idle) || cg_create(normal))
		goto out;

	if (cg_write(idle, env->idle_ctrl, "1"))
		goto out;

	if (write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, "0"))
		goto out;
	if (cg_write(idle, env->weight_ctrl, "200") >= 0)
		goto out;

	if (write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, "1"))
		goto out;
	if (cg_write(idle, env->weight_ctrl, "200"))
		goto out;
	if (cg_read_long(idle, env->weight_ctrl) != 200)
		goto out;

	ret = KSFT_PASS;
out:
	if (old_sysctl >= 0)
		write_text_file(SYSCTL_IDLE_SHARES_RELAX_PATH, old_sysctl ? "1" : "0");
	cg_destroy(idle);
	cg_destroy(normal);
	cg_destroy(parent);
	free(idle);
	free(normal);
	free(parent);
	return ret;
}

#define T(_fn) { .name = #_fn, .fn = _fn }
struct test_case {
	const char *name;
	int (*fn)(const struct cgroup_test_env *env, const char *root);
};

static int wrap_sysctl_interface(const struct cgroup_test_env *env,
				 const char *root)
{
	(void)env;
	(void)root;
	return test_sysctl_interface();
}

static int wrap_feature_interface(const struct cgroup_test_env *env,
				  const char *root)
{
	(void)env;
	(void)root;
	return test_feature_interface();
}

static int
wrap_sched_debug_interface(const struct cgroup_test_env *env,
			   const char *root)
{
	(void)env;
	(void)root;
	return test_sched_debug_interface();
}

static struct test_case tests[] = {
	T(test_identity_interface),
	{ .name = "test_sysctl_interface", .fn = wrap_sysctl_interface },
	{ .name = "test_feature_interface", .fn = wrap_feature_interface },
	{ .name = "test_sched_debug_interface", .fn = wrap_sched_debug_interface },
	T(test_identity_hierarchy_counts),
	T(test_core_sched_cookie),
	T(test_identity_switch_onrq_counts),
	T(test_identity_migration_interleaving),
	T(test_identity_throttle_switch_counts),
	T(test_nested_identity_throttle_pressure),
	T(test_identity_repeated_flip_pressure),
	T(test_identity_combo_matrix),
	T(test_identity_multi_child_parent_throttle_pressure),
	T(test_identity_three_child_parent_throttle_pressure),
	T(test_identity_throttle_with_identity_churn),
	T(test_id_smt_expel_behavior),
	T(test_id_expeller_share_core_behavior),
	T(test_sched_idle_shares_relax),
};

#undef T

int main(void)
{
	struct cgroup_test_env env;
	int i;
	int ret = EXIT_SUCCESS;
	int old_smt_expel;

	ksft_print_header();
	ksft_set_plan(ARRAY_SIZE(tests));

	if (geteuid() != 0)
		ksft_exit_skip("test requires root\n");

	if (detect_cgroup_env(&env))
		ksft_exit_skip("neither cgroup v1 cpu controller nor cgroup v2 is mounted\n");

	if (enable_cpu_controller_if_needed(&env, env.root))
		ksft_exit_skip("Failed to enable cpu controller\n");

	old_smt_expel = feature_enabled("ID_SMT_EXPEL");
	if (old_smt_expel >= 0)
		set_feature_enabled("ID_SMT_EXPEL", true);

	for (i = 0; i < (int)ARRAY_SIZE(tests); i++) {
		switch (tests[i].fn(&env, env.root)) {
		case KSFT_PASS:
			ksft_test_result_pass("%s\n", tests[i].name);
			break;
		case KSFT_SKIP:
			ksft_test_result_skip("%s\n", tests[i].name);
			break;
		default:
			ret = EXIT_FAILURE;
			ksft_test_result_fail("%s\n", tests[i].name);
			break;
		}
	}

	if (old_smt_expel == 0)
		set_feature_enabled("ID_SMT_EXPEL", false);

	return ret;
}

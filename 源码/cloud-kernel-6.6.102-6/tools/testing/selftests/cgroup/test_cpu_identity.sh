#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

set -euo pipefail

SELFTEST_DIR="$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
TEST_BIN="${SELFTEST_DIR}/test_cpu_identity"
BENCH_MODE="${BENCH_MODE:-0}"

mount_if_needed()
{
	local src="$1"
	local target="$2"
	local fstype="$3"

	mkdir -p "${target}"
	if ! grep -q " ${target} " /proc/mounts; then
		mount -t "${fstype}" "${src}" "${target}"
	fi
}

print_logs()
{
	echo "=== /sys/kernel/debug/sched/features ==="
	cat /sys/kernel/debug/sched/features 2>/dev/null || true
	echo "=== SMT siblings ==="
	for f in /sys/devices/system/cpu/cpu*/topology/thread_siblings_list; do
		printf '%s: ' "$f"
		cat "$f" 2>/dev/null || true
	done
	echo "=== /sys/kernel/debug/sched/debug (expeller/expellee excerpt) ==="
	grep -n -E 'cfs_rq\[|h_nr_expeller|h_nr_expellee' \
		/sys/kernel/debug/sched/debug 2>/dev/null || true
}

require_smt()
{
	local found=0
	for f in /sys/devices/system/cpu/cpu*/topology/thread_siblings_list; do
		[[ -e "$f" ]] || continue
		if grep -Eq '[0-9]+,[0-9]+|[0-9]+-[0-9]+' "$f"; then
			found=1
			break
		fi
	done

	if [[ "$found" -eq 0 ]]; then
		echo "SKIP: no SMT sibling topology detected"
		exit 4
	fi
}

main()
{
	if [[ ! -x "${TEST_BIN}" ]]; then
		echo "test binary not found: ${TEST_BIN}" >&2
		exit 1
	fi

	if ! grep -q ' cgroup2 ' /proc/mounts && ! grep -q ' cgroup ' /proc/mounts; then
		mount_if_needed cgroup2 /sys/fs/cgroup cgroup2 || true
	fi
	mount_if_needed debugfs /sys/kernel/debug debugfs

	sysctl -w kernel.sched_schedstats=1 >/dev/null
	require_smt

	set +e
	"${TEST_BIN}"
	ret=$?
	set -e

	if [[ "$ret" -ne 0 ]]; then
		print_logs
	fi

	if [[ "$BENCH_MODE" = "1" ]] && command -v perf >/dev/null 2>&1; then
		echo "=== optional perf bench sched messaging ==="
		perf bench sched messaging -g 2 -l 2000 || true
	fi

	exit "$ret"
}

main "$@"

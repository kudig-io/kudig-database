---
title: Linux Container Foundation
description: Linux Container Foundation — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- linux
- namespaces
- cgroups
- security
- kernel
- kubelet
- scheduler
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linux Container Foundation 是什么
- 如何 Linux Container Foundation
trigger_keywords:
- Linux
- Container
- Foundation
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Linux Container Foundation

Containers are not a standalone technology but a composition of Linux kernel features. Understanding these features is essential for effective K8s troubleshooting.

## Seven Namespaces

Namespaces provide isolation by giving processes their own view of system resources:

| Namespace | Isolates | K8s Application | Key File |
|-----------|----------|-----------------|----------|
| PID | Process IDs | Per-Pod process tree | /proc/<pid>/ns/pid |
| Network | Network stack, interfaces, routes | Pod network isolation | /proc/<pid>/ns/net |
| Mount | Filesystem mount points | Container filesystem | /proc/<pid>/ns/mnt |
| UTS | Hostname and domain | [[Pod Hostname|Pod hostname]] | /proc/<pid>/ns/uts |
| IPC | Inter-process communication | Shared memory isolation | /proc/<pid>/ns/ipc |
| User | User/group ID mapping | Rootless containers | /proc/<pid>/ns/user |
| Cgroup | Cgroup root view | Per-container cgroup view | /proc/<pid>/ns/cgroup |

## cgroups v2 and K8s QoS

cgroups v2 unifies resource controllers under a single hierarchy. K8s maps resource limits to cgroup files:

| K8s Setting | cgroups v2 File | Behavior |
|-------------|----------------|----------|
| limits.cpu | cpu.max | Hard CPU quota (CFS quota/period) |
| requests.cpu | cpu.weight | CPU scheduling weight (1-10000) |
| limits.memory | memory.max | Hard memory limit, triggers OOM |
| (none) | memory.high | Soft limit, throttles before OOM |
| limits.ephemeral-storage | io.max | I/O bandwidth and IOPS limits |
| (none) | pids.max | Max process count (fork bomb protection) |

K8s QoS classes map to cgroup behavior:
- **Guaranteed**: Fixed cpu.max, high cpu.weight, memory.oom.group=1 (kill entire cgroup on OOM)
- **Burstable**: cpu.max=max (no hard limit), cpu.weight by requests
- **BestEffort**: cpu.max=max, lowest weight, first to be OOM killed

## OOM Killer Mechanics

The OOM Killer activates when system or cgroup memory is exhausted. Selection algorithm computes `oom_score` based on memory usage percentage, process importance (oom_score_adj), and K8s QoS tier (BestEffort highest, Guaranteed lowest).

Debug OOM kills: `dmesg | grep -i oom` or `journalctl -k --grep="Out of memory"`

## Linux Security Modules for Containers

| Module | Type | Granularity | Default Distro | K8s Integration |
|--------|------|-------------|----------------|-----------------|
| AppArmor | MAC | File/network/capabilities | Ubuntu, Debian | SecurityContext annotation |
| SELinux | MAC | File/process/port/type | RHEL, CentOS | SecurityContext selinuxOptions |
| seccomp | Syscall filter | Individual syscalls | All (kernel-level) | SecurityContext seccompProfile |

## K8s-to-Linux Mapping

| K8s Problem | Linux Root Cause | Debug Tool |
|-------------|-----------------|------------|
| Pod OOMKilled | cgroups memory.max triggers OOM Killer | dmesg, /proc/<pid>/cgroup |
| Service unreachable | iptables/IPVS rules broken, conntrack full | iptables-save, conntrack -L |
| Container I/O slow | OverlayFS COW or I/O scheduler | iostat, fio, perf |
| Node NotReady | Kernel parameters, disk full, kubelet crash | systemctl, journalctl, df -h |
| CPU throttling | cgroups cpu.max CFS quota exhausted | /sys/fs/cgroup/cpu.max, perf |
| Security violation | SELinux/AppArmor blocks container ops | ausearch, dmesg, aa-status |

## Related

- [[docker]] — Docker
- [[concepts/docker-architecture|docker-architecture]] — Docker Architecture and Container Runtime
- [[concepts/linux-sysctl-tuning|linux-sysctl-tuning]] — Linux Sysctl Tuning for Kubernetes
- [[entities/kubelet|kubelet]] — kubelet
- [[concepts/linux-security-modules|linux-security-modules]] — Linux Security Modules for Containers
- [[concepts/docker-architecture|Docker Architecture]]
- [[concepts/overlayfs-storage|OverlayFS Storage]]
- [[concepts/linux-sysctl-tuning|Linux Sysctl Tuning]]
- [[concepts/linux-security-modules|Linux Security Modules]]
- [[containerd|containerd]]

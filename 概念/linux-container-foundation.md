---
title: Linux Container Foundation
description: Linux Container Foundation — Kubernetes 生产运维知识库
summary: Linux Container Foundation — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Linux Container Foundation

Containers are not a standalone technology but a composition of Linux kernel features. Understanding these features is essential for effective K8s troubleshooting.

## Seven Namespaces

Namespaces provide isolation by giving processes their own view of system resources:

| Namespace | Isolates | K8s Application | Key File |
|-----------|----------|-----------------|----------|
| PID | Process IDs | Per-Pod process tree | /proc/<pid>/ns/pid |
| Network | Network stack, interfaces, routes | Pod network isolation | /proc/<pid>/ns/net |
| Mount | Filesystem mount points | Container filesystem | /proc/<pid>/ns/mnt |
| UTS | Hostname and domain | Pod hostname | /proc/<pid>/ns/uts |
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

## 源码实现分析

### runc 创建容器（namespace + cgroup）

```go
// runc/libcontainer/container_linux.go
func (c *Container) Start(process *Process) error {
    // 1. 创建 cgroup 并设置资源限制
    c.cgroupManager.Set(&configs.Cgroup{
        Resources: &configs.Resources{
            Memory:    256 * 1024 * 1024,  // memory.max
            CpuShares: 1024,               // cpu.weight
            CpuQuota:  100000,             // cpu.max (100ms/100ms)
        },
    })
    
    // 2. clone() 创建新 namespace
    // CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWNS |
    // CLONE_NEWUTS | CLONE_NEWIPC
    cmd := exec.Command("/proc/self/exe", "init")
    cmd.SysProcAttr = &syscall.SysProcAttr{
        Cloneflags: syscall.CLONE_NEWPID | syscall.CLONE_NEWNET |
                    syscall.CLONE_NEWNS | syscall.CLONE_NEWUTS,
    }
    
    // 3. 配置网络（CNI）、挂载文件系统（OverlayFS）
    // 4. exec 容器进程
}
```

### cgroups v2 目录结构

```
/sys/fs/cgroup/
├── kubepods.slice/                    # K8s 所有 Pod
│   ├── kubepods-burstable.slice/      # Burstable QoS
│   │   ├── pod<pod-uid>/              # 单个 Pod
│   │   │   ├── <container-id>/        # 单个容器
│   │   │   │   ├── cpu.max            # CPU 硬限制
│   │   │   │   ├── cpu.weight         # CPU 权重
│   │   │   │   ├── memory.max         # 内存硬限制
│   │   │   │   ├── memory.current     # 当前内存使用
│   │   │   │   └── pids.max           # 进程数限制
│   │   │   └── ...
│   ├── kubepods-besteffort.slice/     # BestEffort QoS
│   └── kubepods-guaranteed.slice/     # Guaranteed QoS
```

## 使用场景

### 场景一：诊断 CPU Throttling

```bash
# 🟢 低风险 - 检查容器 CPU 限制
cat /sys/fs/cgroup/kubepods.slice/kubepods-burstable.slice/pod<uid>/<ctr>/cpu.max
# 输出: 100000 100000  (100ms quota / 100ms period = 1 CPU)

# 🟢 低风险 - 检查 throttling 统计
cat /sys/fs/cgroup/.../cpu.stat
# nr_throttled: 被限流次数
# throttled_usec: 被限流总时间

# 🟢 低风险 - 通过 kubectl 查看
kubectl top pod <pod> --containers
kubectl describe pod <pod> | grep -A5 "Limits"
```

### 场景二：诊断 OOM Kill

```bash
# 🟢 低风险 - 查看 OOM 日志
dmesg | grep -i "oom\|killed process"
# Memory cgroup out of memory: Killed process 12345 (java)

# 🟢 低风险 - 检查容器内存使用
cat /sys/fs/cgroup/.../memory.current   # 当前使用
cat /sys/fs/cgroup/.../memory.max       # 硬限制
cat /sys/fs/cgroup/.../memory.events    # OOM 事件计数

# 🟢 低风险 - K8s 层面确认
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'
# 输出: OOMKilled
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 容器是轻量级虚拟机 | 容器是进程隔离（namespace+cgroup），共享宿主机内核，无独立 OS |
| CPU limit 是预留资源 | limit 是硬上限（throttle），requests 才是调度预留 |
| OOM Kill 随机杀进程 | 按 oom_score 选择：BestEffort 先死，Guaranteed 最后 |
| 容器内 top 看到的是容器资源 | 默认看到宿主机全部 CPU/内存，需 lxcfs 或 cgroup-aware top |
| namespace 提供安全隔离 | namespace 只提供视图隔离，不提供安全边界，需配合 seccomp/MAC |
| cgroups v1 和 v2 无区别 | v2 统一层级、更好的 IO 控制、PSI 压力指标，K8s 1.25+ 默认 v2 |

## 面试要点

1. **容器的本质是什么？** — 容器 = namespace（视图隔离）+ cgroup（资源限制）+ rootfs（OverlayFS）+ 安全模块（seccomp/MAC）。本质是一个被隔离和限制的 Linux 进程，不是虚拟机。

2. **K8s QoS 如何映射到 cgroup？** — Guaranteed：cpu.max=固定值、memory.max=limits、oom.group=1（整组杀）；Burstable：cpu.max=max、cpu.weight按requests比例；BestEffort：无限制、最低权重、最先被 OOM Kill。

3. **CPU Throttling 的原因和解决？** — CFS 调度器在 100ms period 内用完 quota 后暂停进程。解决：增大 CPU limit；移除 CPU limit（仅保留 requests）；优化应用并发模型。监控：cpu.stat 中 nr_throttled。

4. **为什么 Pod 内容器共享网络 namespace？** — 同一 Pod 的容器共享 net namespace（通过 pause 容器），因此共享 IP、端口空间、localhost。实现 sidecar 模式（如 Envoy 代理可直接访问应用容器的 localhost:port）。

## Related

- [[docker]] — Docker
- [[概念/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[概念/linux-sysctl-tuning.md|linux-sysctl-tuning]] — Linux Sysctl Tuning for Kubernetes
- [[实体/kubelet.md|kubelet]] — kubelet
- [[概念/linux-security-modules.md|linux-security-modules]] — Linux Security Modules for Containers
- [[概念/docker-architecture.md|Docker Architecture]]
- [[概念/overlayfs-storage.md|OverlayFS Storage]]
- [[概念/linux-sysctl-tuning.md|Linux Sysctl Tuning]]
- [[概念/linux-security-modules.md|Linux Security Modules]]
- [[containerd|containerd]]


<!-- risk-assessed -->

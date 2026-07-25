---
title: Linux Sysctl Tuning for Kubernetes
description: Linux Sysctl Tuning for Kubernetes — Kubernetes 生产运维知识库
summary: Linux Sysctl Tuning for Kubernetes — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- linux
- sysctl
- performance
- kernel
- tuning
- kubelet
- containerd
- docker
- redis
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linux Sysctl Tuning for Kubernetes 是什么
- 如何 Linux Sysctl Tuning for Kubernetes
trigger_keywords:
- Linux
- Sysctl
- Tuning
- for
- Kubernetes
prerequisites:
- kubectl-basics
- redis-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Linux Sysctl Tuning for Kubernetes

## Critical Network Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| net.ipv4.ip_forward | 1 | Required for Pod cross-node communication |
| net.bridge.bridge-nf-call-iptables | 1 | Required for kube-proxy [[Service|Service]] routing |
| net.bridge.bridge-nf-call-ip6tables | 1 | Required for IPv6 dual-stack |
| net.netfilter.nf_conntrack_max | 1048576 | Conntrack table size for large clusters |
| net.core.somaxconn | 32768 | Socket listen queue for high-concurrency Services |
| net.ipv4.tcp_max_syn_backlog | 8096 | SYN queue for high-concurrency short connections |
| net.ipv4.tcp_tw_reuse | 1 | Reuse TIME_WAIT sockets for connection recycling |
| net.ipv4.tcp_keepalive_time | 600 | TCP keepalive interval (seconds) |

## Critical Filesystem Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| fs.inotify.max_user_watches | 524288 | [[kubelet|kubelet]] file watcher, must be increased |
| fs.inotify.max_user_instances | 8192 | kubelet + container runtime |
| fs.file-max | 2097152 | System-wide file descriptor limit |
| fs.nr_open | 1048576 | Per-process file descriptor limit |
| fs.may_detach_mounts | 1 | Required for containerd/docker container deletion |

## Critical Memory Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| vm.swappiness | 0-10 | K8s recommends 0 to avoid swap performance jitter |
| vm.overcommit_memory | 1 | Always allow overcommit (Redis recommends 1) |
| vm.max_map_count | 262144 | Elasticsearch requires > default 65530 |
| vm.panic_on_oom | 0 | Let K8s OOM Killer handle, not kernel panic |

## Critical Kernel Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| kernel.panic | 10 | Auto-reboot delay after kernel panic (seconds) |
| kernel.panic_on_oops | 1 | Kernel oops triggers panic for node auto-recovery |
| user.max_user_namespaces | 28633 | Required for rootless containers |

## Required Kernel Modules

```bash
# Load required modules
modprobe br_netfilter   # Bridge netfilter for kube-proxy
modprobe overlay        # OverlayFS for container images

# Persist module loading
echo "br_netfilter" >> /etc/modules-load.d/k8s.conf
echo "overlay" >> /etc/modules-load.d/k8s.conf
```

## Application

```bash
# Write configuration
cat > /etc/sysctl.d/99-kubernetes.conf << 'EOF'
# Network
net.ipv4.ip_forward = 1
net.bridge.bridge-nf-call-iptables = 1
net.netfilter.nf_conntrack_max = 1048576
# Filesystem
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192
# Memory
vm.swappiness = 0
vm.max_map_count = 262144
EOF

# Apply
sysctl --system
```

## 源码实现分析

### sysctl 内核参数生效机制

```c
// Linux 内核 sysctl 实现原理
// kernel/sysctl.c - sysctl 系统调用入口
SYSCALL_DEFINE5(sysctl, struct __sysctl_args __user *, args) {
    // 1. 解析 /proc/sys/ 路径对应的内核参数
    // 2. 检查权限（CAP_SYS_ADMIN 或特定参数允许普通用户）
    // 3. 调用对应子系统 handler
    //    - net.* → net/core/sysctl_net_core.c
    //    - vm.*  → mm/mmap.c, mm/page-writeback.c
    //    - fs.*  → fs/dcache.c, fs/inotify/
    // 4. 写入全局变量（立即生效，无需重启）
}
// 容器场景：namespace 感知的 sysctl
// 部分参数是 per-netns（如 net.ipv4.ip_forward）
// 部分参数是全局的（如 vm.swappiness）——容器内修改影响宿主机
```

### K8s 节点 sysctl 配置架构

```
┌──────────────────────────────────────────────────────────┐
│            K8s 节点 sysctl 配置层次                    │
├──────────────────────────────────────────────────────────┤
│  /etc/sysctl.conf          ← 全局默认（优先级最低）    │
│  /etc/sysctl.d/*.conf      ← 按文件名字母序加载        │
│  /run/sysctl.d/*.conf      ← 运行时临时配置            │
│  sysctl -w key=val         ← 立即生效，重启丢失        │
│                                                          │
│  K8s 相关:                                              │
│  /etc/sysctl.d/99-kubernetes.conf  ← 节点初始化时设置  │
│  kubelet --allowed-unsafe-sysctls  ← Pod 级别 sysctl   │
│  Pod.spec.securityContext.sysctls  ← 容器级别 sysctl   │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：高并发网络场景调优

```bash
# 🟠 高危：实时修改内核参数，全局生效
# 记录当前值用于回滚
sysctl net.netfilter.nf_conntrack_max  # 记录原值
sysctl net.core.somaxconn
# 调整 conntrack 表大小（高并发服务必须）
sysctl -w net.netfilter.nf_conntrack_max=2097152
sysctl -w net.netfilter.nf_conntrack_tcp_timeout_time_wait=30
# 调整 TCP 连接队列
sysctl -w net.core.somaxconn=65535
sysctl -w net.ipv4.tcp_max_syn_backlog=65535
# 调整 TIME_WAIT 回收
sysctl -w net.ipv4.tcp_tw_reuse=1
sysctl -w net.ipv4.tcp_fin_timeout=15
# 持久化
cat > /etc/sysctl.d/99-high-conn.conf << 'EOF'
net.netfilter.nf_conntrack_max = 2097152
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 15
EOF
sysctl --system  # 🟠 重新加载所有配置
```

### 场景二：Pod 级别 sysctl（安全 sysctl）

```yaml
# 🟡 中风险：Pod 级别 sysctl 影响容器网络行为
apiVersion: v1
kind: Pod
metadata:
  name: network-tuned
spec:
  securityContext:
    sysctls:
    - name: net.core.somaxconn  # 安全 sysctl（per-netns）
      value: "1024"
    - name: net.ipv4.tcp_syncookies
      value: "1"
  containers:
  - name: app
    image: nginx:latest
```

### 场景三：诊断 sysctl 相关问题

```bash
# 🟢 低风险：只读诊断
# 检查 conntrack 表是否溢出
dmesg | grep "nf_conntrack: table full"  # 溢出标志
cat /proc/sys/net/netfilter/nf_conntrack_count  # 当前连接数
cat /proc/sys/net/netfilter/nf_conntrack_max    # 最大值
# 检查文件描述符限制
cat /proc/sys/fs/file-nr  # 已分配/未使用/最大值
# 检查 inotify 限制
cat /proc/sys/fs/inotify/max_user_watches
cat /proc/sys/fs/inotify/max_user_instances
# 检查 Pod 内 sysctl 是否生效
kubectl exec -it <pod> -- sysctl net.core.somaxconn
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 容器内 sysctl 与宿主机无关 | 全局 sysctl（如 vm.swappiness）容器内修改影响宿主机；仅 per-netns 参数隔离 |
| 2 | sysctl -w 重启后仍生效 | sysctl -w 仅内存生效；必须写入 /etc/sysctl.d/ 才能持久化 |
| 3 | 所有 sysctl 都可以在 Pod 中设置 | 只有“安全 sysctl”（per-netns）默认允许；“不安全 sysctl”需 kubelet --allowed-unsafe-sysctls |
| 4 | conntrack_max 越大越好 | 过大消耗内核内存（每条 ~300B）；应根据实际并发连接数设置，通常 1M 足够 |
| 5 | vm.swappiness=0 禁用 swap | 0 只是尽量避免 swap，不完全禁用；彻底禁用需 swapoff 或 cgroup memory.swap.max=0 |
| 6 | 生产节点不需要调优 | 默认值面向通用场景；K8s 节点必须调整 conntrack/inotify/file-max/bridge-nf 等参数 |

## 面试要点

1. **Q: K8s 节点必须调整哪些 sysctl 参数？为什么？**
   A: ① net.bridge.bridge-nf-call-iptables=1：让 bridge 流量经过 iptables（kube-proxy 必需）；② net.ipv4.ip_forward=1：允许 Pod 跨节点通信；③ net.netfilter.nf_conntrack_max：默认 65536 对高并发服务不足，需调至 1M+；④ fs.inotify.max_user_watches/instances：kubelet 和日志采集器需要大量 inotify；⑤ vm.max_map_count=262144：Elasticsearch 等应用需要。

2. **Q: Pod 级别 sysctl 如何工作？安全与不安全 sysctl 的区别？**
   A: 安全 sysctl：per-namespace 隔离（如 net.core.somaxconn），容器内修改不影响其他 Pod/宿主机，默认允许。不安全 sysctl：全局影响或可能影响其他容器（如 kernel.shm*），需 kubelet 显式允许（--allowed-unsafe-sysctls）。Pod 通过 spec.securityContext.sysctls 设置，由 kubelet 在创建容器时通过 netns 设置。

3. **Q: conntrack 表溢出会导致什么问题？如何解决？**
   A: 症状：dmesg 出现 "nf_conntrack: table full, dropping packet"，表现为随机丢包、连接超时。解决：① 增大 nf_conntrack_max（每条约 300B 内存）；② 减少 tcp_timeout_time_wait（默认 120s → 30s）；③ 启用 tcp_tw_reuse 加速 TIME_WAIT 回收；④ 排查是否有连接泄漏（大量 CLOSE_WAIT）；⑤ 考虑使用 IPVS 代替 iptables（IPVS 不依赖 conntrack）。

4. **Q: 如何在不重启节点的情况下安全修改 sysctl？**
   A: ① 记录当前值（sysctl key 或 cat /proc/sys/...）；② 评估影响范围（全局 vs per-netns）；③ 低峰期执行 sysctl -w；④ 观察业务指标 5-10min；⑤ 确认无异常后写入 /etc/sysctl.d/ 持久化；⑥ 异常时立即回滚到记录的原值。注意：部分参数（如 net.ipv4.tcp_mem）修改后需等待现有连接关闭才完全生效。

## Related

- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/03-网络/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[22-概念/04-存储/block-file-object-storage.md|block-file-object-storage]] — Block, File, and Object Storage
- [[22-概念/15-运行时与系统/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[22-概念/15-运行时与系统/linux-container-foundation.md|Linux Container Foundation]]
- [[22-概念/03-网络/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[22-概念/04-存储/block-file-object-storage.md|Block, File, and Object Storage]]


<!-- risk-assessed -->

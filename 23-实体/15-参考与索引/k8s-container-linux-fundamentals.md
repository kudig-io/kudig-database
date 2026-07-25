---
title: 容器技术、Linux 系统与网络存储基础
description: '# 容器技术、Linux 系统与网络存储基础'
summary: '# 容器技术、Linux 系统与网络存储基础'
category: reference
tags:
- k8s
- docker
- containerd
- linux
- networking-basics
- storage-basics
- cilium
- calico
- ceph
- minio
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器技术、Linux 系统与网络存储基础 是什么
- 如何 容器技术、Linux 系统与网络存储基础
trigger_keywords:
- 容器技术
- Linux
- 系统与网络存储基础
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 容器技术、Linux 系统与网络存储基础

> **CNCF 状态**: 基础知识 | **类别**: Linux Fundamentals | **主要语言**: Shell, YAML

## 概述

Kubernetes 底层的 Linux 基础是理解容器和 K8s 工作原理的关键知识领域。它涵盖 Linux Namespace（命名空间隔离）、Cgroups（资源限制）、UnionFS（联合文件系统）、Seccomp/AppArmor/SELinux（安全策略）、网络虚拟化（veth、bridge、iptables/eBPF）等核心技术。理解这些 Linux 内核机制对于 K8s 运维排障、性能调优和安全加固至关重要。本文档系统梳理容器化工作负载依赖的核心 Linux 技术栈。

## Key Features（核心能力）

- **Namespace 隔离**：PID、Network、Mount、UTS、IPC、User 六大命名空间提供容器隔离
- **Cgroups v2**：统一的资源控制层级，管理 CPU、内存、IO、PID 等资源限制
- **UnionFS**：OverlayFS/AUFS 提供镜像分层存储机制
- **网络虚拟化**：veth pair、bridge、iptables/eBPF 构建容器网络
- **安全机制**：Seccomp、AppArmor、SELinux 提供多层安全防护
- **存储管理**：Device Mapper、LVM、OverlayFS 等存储驱动

## 架构与工作原理

容器技术的 Linux 基础由三个核心机制构成：Namespace 提供 process 级别的隔离（进程看到的系统环境是独立的）；Cgroups 提供资源限制和计量（限制 CPU、内存、IO 等）；UnionFS 提供镜像分层存储（只读层 + 可写层叠加）。网络方面，每个容器通过 veth pair 连接到虚拟网桥，通过 iptables 或 eBPF 程序实现 Service 代理和 NetworkPolicy。

## K8s 集成

Kubernetes 完全依赖这些 Linux 机制运行：kubelet 通过 CRI 调用 containerd，containerd 通过 runc 创建基于 Namespace+Cgroups 的容器；kube-proxy 通过 iptables/ipvs/eBPF 实现 Service 负载均衡；Pod Security Standards 通过 Seccomp/AppArmor 策略约束容器行为；CSI 驱动通过 Linux 块设备和文件系统提供存储。

## 生产用例

- **容器排障**：通过 nsenter、ip netns 等工具进入容器网络命名空间诊断网络问题
- **性能调优**：通过 cgroups 和 CPU Manager 优化容器性能
- **安全加固**：配置 Seccomp/AppArmor 策略限制容器权限
- **网络理解**：理解 Pod 间通信链路（veth → bridge → iptables → Service）

## 安装与配置

### 容器 Namespace 诊断工具

```bash
# 🟢 查看容器进程的 namespace
PID=$(crictl inspect <container-id> | jq .info.pid)
ls -la /proc/$PID/ns/
# 输出: cgroup, ipc, mnt, net, pid, pid_for_children, user, uts

# 🟢 进入容器网络命名空间
nsenter -t $PID -n ip addr show
nsenter -t $PID -n ip route show
nsenter -t $PID -n iptables -t nat -L -n

# 🟢 查看系统所有 namespace
lsns -t net,pid,mnt,uts,ipc,user
lsns -t net | wc -l  # 网络命名空间数量

# 🟢 查看 Pod 内所有容器的 namespace
crictl pods --name <pod-name> -q | xargs -I{} crictl inspectp {} | jq '.info.pid'
```

### Cgroups v2 资源检查

```bash
# 🟢 查看 cgroup v2 层级
mount | grep cgroup2
# cgroup2 on /sys/fs/cgroup type cgroup2

# 🟢 查看 Pod 的 cgroup 路径
cat /proc/$PID/cgroup
# 0::/kubepods.slice/kubepods-burstable.slice/kubepods-burstable-pod<uid>.slice

# 🟢 检查 CPU 限制
cat /sys/fs/cgroup/kubepods.slice/*/cpu.max
# 输出: 200000 100000  (表示 2 CPU: quota/period)

# 🟢 检查内存限制
cat /sys/fs/cgroup/kubepods.slice/*/memory.max
cat /sys/fs/cgroup/kubepods.slice/*/memory.current
cat /sys/fs/cgroup/kubepods.slice/*/memory.events  # OOM 事件

# 🟢 检查 IO 限制
cat /sys/fs/cgroup/kubepods.slice/*/io.max
cat /sys/fs/cgroup/kubepods.slice/*/io.stat

# 🟢 检查 PID 限制
cat /sys/fs/cgroup/kubepods.slice/*/pids.max
cat /sys/fs/cgroup/kubepods.slice/*/pids.current
```

### 网络虚拟化诊断

```bash
# 🟢 查看 veth pair 关系
ip link show type veth
# 在宿主机查看 Pod 对应的 veth
ip link | grep -A1 "veth"

# 🟢 查看网桥和端口
brctl show 2>/dev/null || bridge link show
ip link show type bridge

# 🟢 查看 iptables 规则（kube-proxy）
iptables -t nat -L KUBE-SERVICES -n | head -20
iptables -t filter -L KUBE-NODEPORTS -n
iptables -t mangle -L KUBE-MARK-MASQ -n

# 🟢 查看 IPVS 规则（如使用 ipvs 模式）
ipvsadm -Ln | head -30
ipvsadm -Ln -t <cluster-ip>:<port>

# 🟢 eBPF 程序查看（Cilium）
bpftool prog list | head -20
bpftool map list | grep cilium
cilium bpf policy get <endpoint-id>

# 🟢 连接跟踪
conntrack -L | wc -l  # 连接数
conntrack -L -p tcp --dport 80 | head -10
cat /proc/sys/net/netfilter/nf_conntrack_max
```

### 存储和文件系统

```bash
# 🟢 查看 OverlayFS 挂载
mount | grep overlay
cat /proc/mounts | grep overlay

# 🟢 查看容器分层存储
crictl inspect <container-id> | jq '.info.runtimeSpec.root.path'
ls /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/

# 🟢 查看块设备和 LVM
lsblk -f
pvs; vgs; lvs
dmsetup ls

# 🟢 查看 CSI 挂载
mount | grep csi
ls /var/lib/kubelet/pods/*/volumes/kubernetes.io~csi/
```

### 安全机制检查

```bash
# 🟢 检查 Seccomp 状态
grep Seccomp /proc/$PID/status
# Seccomp: 2  (表示 filter 模式)

# 🟢 检查 AppArmor
cat /proc/$PID/attr/current
aa-status  # 查看已加载的 profile

# 🟢 检查 SELinux
getenforce
sestatus
ls -Z /var/lib/containerd/  # 查看安全上下文

# 🟢 检查容器 Capabilities
grep Cap /proc/$PID/status
# CapEff: 00000000a80425fb  (解码)
capsh --decode=00000000a80425fb

# 🟢 检查只读文件系统
mount | grep "ro," | grep $PID
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 网络不通 | veth/bridge 配置异常 | `ip link`; `brctl show` | 重启 CNI 插件/重建 Pod |
| OOMKilled | 内存 cgroup 限制 | `cat memory.events`; `dmesg` | 调整 resources.limits |
| CPU Throttling | cfs_quota 过低 | `cat cpu.stat` (nr_throttled) | 增加 CPU limits |
| 磁盘 IO 慢 | IO cgroup 限制/存储后端 | `iostat -x 1`; `cat io.stat` | 调整 IO 限制/升级存储 |
| conntrack 表满 | 连接数超限 | `conntrack -C`; `dmesg` | 增大 nf_conntrack_max |
| 容器无法启动 | namespace/cgroup 泄漏 | `lsns`; `systemctl status containerd` | 清理泄漏 ns/重启运行时 |

### 分层排查流程

```
容器异常排查
├── 网络问题？
│   ├── nsenter -t $PID -n ip addr → 有 IP？
│   ├── nsenter -t $PID -n ip route → 有默认路由？
│   ├── nsenter -t $PID -n ping <gateway> → 网关可达？
│   └── iptables/conntrack → 规则正常？
├── 资源问题？
│   ├── cat cpu.stat → nr_throttled 高？
│   ├── cat memory.events → oom_kill > 0？
│   └── cat io.stat → IO 等待高？
├── 存储问题？
│   ├── mount | grep csi → 挂载正常？
│   ├── df -h → 磁盘空间？
│   └── dmesg | grep -i error → 存储错误？
└── 安全问题？
    ├── grep Seccomp /proc/$PID/status → 被拦截？
    ├── dmesg | grep -i apparmor → 拒绝日志？
    └── ausearch -m avc → SELinux 拒绝？
```

## 生产案例

### 案例1：conntrack 表满导致服务间歇性不可用

- **场景**：高并发服务每天 14:00-15:00 出现大量连接超时
- **排查**：`dmesg` 发现 "nf_conntrack: table full, dropping packet"；`conntrack -C` 显示 262144（已达上限）
- **方案**：`sysctl -w net.netfilter.nf_conntrack_max=1048576`；优化连接池复用减少连接数；启用 IPVS 替代 iptables
- **效果**：连接超时消失，conntrack 使用率降至 30%

### 案例2：CPU Throttling 导致延迟飙升

- **场景**：Java 服务 P99 延迟从 50ms 飙升到 2s，CPU 使用率仅 60%
- **排查**：`cat cpu.stat` 显示 nr_throttled 持续增长；JVM GC 线程瞬间突发 CPU 被 cfs_quota 截断
- **方案**：移除 CPU limits（仅保留 requests）；或设置 `cpu.cfs_burst_us` 允许突发
- **效果**：P99 延迟回落到 80ms，无 throttling

## 对比替代方案

| 方案 | 隔离级别 | 性能开销 | 适用场景 |
|------|----------|----------|----------|
| Linux Namespace+Cgroups | 进程级 | 极低（<1%） | 标准容器（默认） |
| Kata Containers | VM级（轻量） | 低（~5%） | 多租户/不可信工作负载 |
| gVisor (runsc) | 用户态内核 | 中（~10-20%） | 安全敏感/ syscall 过滤 |
| Firecracker microVM | VM级 | 低（~5%） | Serverless/强隔离 |
| 传统 VM (KVM) | 完全虚拟化 | 高（~15-30%） | 遗留应用/完全隔离 |

## 检查清单

- [ ] 确认 cgroup v2 已启用（统一层级）
- [ ] 容器资源限制已配置（requests + limits）
- [ ] 网络命名空间配置正确（CNI 插件正常）
- [ ] conntrack 表大小足够（高并发场景）
- [ ] Seccomp/AppArmor 策略已应用
- [ ] OverlayFS 存储空间充足
- [ ] 内核参数已调优（net.core.somaxconn, vm.overcommit_memory 等）
- [ ] 节点监控覆盖 cgroup 指标（CPU throttling, OOM events）

## Related

- [[docker]] — Docker
- [[cilium]] — Cilium
- [[containerd]] — containerd


<!-- risk-assessed -->

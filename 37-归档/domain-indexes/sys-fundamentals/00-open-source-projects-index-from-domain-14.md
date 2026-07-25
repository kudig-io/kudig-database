---
title: Domain-14 Linux — 开源项目索引
description: '- open-source-projects-index的最佳实践'
summary: '- open-source-projects-index的最佳实践'
category: general
tags:
- k8s
- etcd
- kubelet
- cilium
- containerd
- cri-o
- docker
- falco
- mysql
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Domain-14 Linux — 开源项目索引 是什么
- 如何 Domain-14 Linux — 开源项目索引
- Kubernetes 17 system foundation 最佳实践
trigger_keywords:
- Domain-14
- Linux
- 开源项目索引
- system
- foundation
prerequisites:
- kubectl-basics
- cloud-provider-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
tags:
- linux
- system-admin
- guide
intent_queries:
- open-source-projects-index是什么？
- open-source-projects-index的使用方法
- open-source-projects-index的最佳实践

tier: peripheral---
title: Domain-14 Linux — 开源项目索引
description: '<!-- chunk: 概述' -->## 概述'
category: linux
tags:
- linux
- system
- kernel
- etcd
- kubelet
- cilium
- containerd
- cri-o
- docker
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 运维工程师
- SRE
- 系统管理员
estimated_read_time: 5min
intent_queries:
- Domain-14 Linux — 开源项目索引 是什么
- 如何 Domain-14 Linux — 开源项目索引
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Domain-14
- Linux
- 开源项目索引
- linux
cross_refs:
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/linux.md
  label: '速查卡: linux'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Domain-14 Linux — 开源项目索引

> **最后更新**: 2026-04-24

---

<!-- chunk: 概述 -->## 概述

Linux 生态系统中有大量开源项目支撑着现代基础设施的运行。从内核子系统到用户空间工具，从容器运行时到安全框架，每一个项目都在云原生技术栈中扮演着关键角色。本文档系统性地梳理了与 Linux 基础知识体系相关的核心开源项目，涵盖系统管理、容器技术、网络安全、性能监控和安全审计等关键领域。

对于 Kubernetes 运维人员而言，理解这些底层项目的原理和演进方向至关重要。Linux 内核本身作为最大的开源项目，其每个版本的更新都会直接影响容器运行时、网络插件和存储驱动的行为。例如，内核 5.6 引入的 Time Namespace 为高精度时间同步的容器提供了支持，eBPF 的持续增强使得 Cilium 等新一代网络插件成为可能，cgroups v2 的成熟让 Kubernetes 的资源管理更加精确和高效。

---

<!-- chunk: 核心项目 -->## 核心项目

## 系统管理与服务治理

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **systemd** | Linux 系统与服务管理 | systemd | v257.0 | - | LGPL-2.1+ |
| **chrony** | NTP 时间同步 | chrony | v4.6.0 | - | GPL-2.0 |
| **rsyslog** | 系统日志处理 | rsyslog | v8.2406.0 | - | GPL-3.0 |
| **logrotate** | 日志轮转管理 | logrotate | v3.22.0 | - | GPL-2.0 |
| **polkit** | 系统权限策略 | freedesktop | v124 | - | LGPL-2.0+ |
| **dbus** | 进程间通信总线 | freedesktop | v1.14.10 | - | AFL-2.1/GPL-2.0 |
| **udev/eudev** | 设备管理 | systemd/gentoo | v255 | - | GPL-2.0 |

## systemd 在云原生中的关键作用

systemd 不仅是 Linux 系统的服务管理器，它还深度集成了 cgroup 管理、日志收集和资源控制功能。在 Kubernetes 节点上，systemd 负责管理 kubelet、containerd 等关键服务的生命周期。当 cgroup 驱动设置为 systemd 时，kubelet 通过 systemd 的 slice/scope 机制管理 Pod 的 cgroup 层级。

```yaml
systemd与Kubernetes关系:
  服务管理:
    - kubelet: 通过systemd unit文件管理
    - containerd: 通过systemd管理容器运行时
    - kube-proxy: 通过systemd管理网络代理
  
  cgroup管理:
    - K8s cgroup驱动: systemd
    - Pod cgroup路径: /sys/fs/cgroup/kubepods.slice/
    - slice/scope层次: system.slice → kubepods.slice → podxxx.slice
  
  日志收集:
    - journald: 收集所有服务日志
    - containerd日志: 通过journald收集
    - 日志查询: journalctl -u kubelet -u containerd
  
  资源控制:
    - CPUQuota: 对应K8s CPU limit
    - MemoryMax: 对应K8s memory limit
    - TasksMax: 对应pids.max
```

## 容器操作系统

容器优化操作系统（Container OS）是专为运行容器工作负载而设计的精简 Linux 发行版。它们通常采用不可变基础设施理念，通过原子更新机制确保系统一致性，大幅减少运维开销。

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Flatcar Container Linux** | 容器优化 OS | CNCF Incubating | v4081.0.0 | - | Apache-2.0 |
| **Bottlerocket** | AWS 容器 OS | AWS | v1.33.0 | 8k+ | Apache-2.0 |
| **Talos Linux** | K8s 专用 OS | Sidero Labs | v1.9.0 | 6k+ | MPL-2.0 |
| **LinuxKit** | 容器化 Linux 构建 | Docker | v1.5.0 | 8k+ | Apache-2.0 |
| **CoreOS Ignition** | 机器配置供应 | CoreOS/Red Hat | v2.18.0 | - | Apache-2.0 |
| **cloud-init** | 云实例初始化 | Canonical | v24.2 | 3k+ | Apache-2.0/GPL-3.0 |

## 容器 OS 选型对比

| 特性 | Flatcar | Bottlerocket | Talos | Ubuntu |
|:---|:---|:---|:---|:---|
| **更新机制** | Omaha/更新引擎 | A/B 分区更新 | 原子更新 | apt |
| **配置方式** | Ignition/Butane | TOML 配置 | API/Config | cloud-init |
| **包管理** | 无（不可变） | 无（不可变） | 无（不可变） | apt/dpkg |
| **Shell 访问** | 有限制 | 无（仅 API） | 仅 API | 完整 |
| **K8s 集成** | 通用 | AWS 优化 | 原生 | 手动配置 |
| **适用场景** | 通用 K8s | AWS EKS | 专用 K8s/边缘 | 通用 |

```yaml
# Flatcar Container Linux Ignition 配置示例
apiVersion: v1
kind: Ignition
spec:
  storage:
    files:
      - path: /etc/hostname
        contents:
          source: data:,k8s-node-01
        mode: 420
      - path: /etc/sysctl.d/99-k8s.conf
        contents:
          source: data:,net.ipv4.ip_forward%3D1%0Anet.bridge.bridge-nf-call-iptables%3D1
        mode: 420
    systemd:
      units:
        - name: docker.service
          enabled: false
        - name: containerd.service
          enabled: true
```

## 内核关键子系统

Linux 内核的每个子系统都对容器和 Kubernetes 的运行方式产生深远影响。cgroups 实现了资源隔离和限制，namespaces 提供了进程视图的隔离，eBPF 开创了内核可编程的新纪元，seccomp 为容器提供了系统调用级别的安全防护。

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **cgroups** | 容器资源控制 | Linux 内核 | v3 | - | GPL-2.0 |
| **eBPF** | 内核可编程框架 | Linux 内核 | - | - | GPL-2.0/BSD |
| **KVM** | 内核虚拟化 | Linux 内核 | - | - | GPL-2.0 |
| **seccomp** | 系统调用过滤 | Linux 内核 | - | - | GPL-2.0 |
| **netfilter** | 网络包过滤框架 | Linux 内核 | - | - | GPL-2.0 |
| **io_uring** | 高性能异步 I/O | Linux 内核 | 5.1+ | - | GPL-2.0 |
| **CRIU** | 检查点/恢复 | OpenVZ | v0.12 | 3k+ | GPL-2.0 |

## eBPF 的革命性影响

eBPF 正在从根本上改变 Linux 内核的扩展方式。它允许在不修改内核源码的情况下，安全地在内核中运行沙箱程序。

```yaml
eBPF在Kubernetes中的应用:
  网络数据路径优化:
    工具: Cilium
    原理: 绕过iptables，直接在socket层面转发
    性能: 延迟降低50%+，吞吐量提升2x
    功能: NetworkPolicy, 服务网格, 透明加密
  
  运行时安全监控:
    工具: Falco
    原理: 跟踪系统调用，匹配攻击模式
    检测: 容器Shell, 敏感文件读取, 异常网络连接
    部署: DaemonSet, 每节点一个实例
  
  网络可观测性:
    工具: Hubble (Cilium)
    原理: eBPF捕获网络流量元数据
    功能: 服务依赖映射, 流量可视化, 故障排查
  
  性能分析:
    工具: bpftrace, BCC
    原理: 低开销内核追踪
    功能: 函数调用追踪, 延迟分析, CPU火焰图
```

```bash
#!/bin/bash
# eBPF 常用工具使用示例

# 1. 跟踪新进程创建
execsnoop-bpf

# 2. 跟踪文件打开
opensnoop-bpf -n postgres

# 3. 分析块IO延迟
biolatency-bpf 10 5

# 4. 跟踪TCP连接
tcplife-bpf

# 5. 跟踪TCP重传
tcpretrans-bpf

# 6. 使用bpftrace一行式分析
bpftrace -e 'tracepoint:syscalls:sys_enter_open { printf("%s -> %s\n", comm, str(args->filename)); }'
bpftrace -e 'kprobe:do_sys_open { printf("open: %s\n", str(arg1)); }'
bpftrace -e 'profile:hz:99 /pid == 12345/ { @[ustack] = count(); }'
```

## 安全框架

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **SELinux** | 强制访问控制 | Red Hat | v3.7.0 | - | GPL-2.0+ |
| **AppArmor** | 安全模块 | Canonical | v4.0.0 | - | GPL-2.0 |
| **OpenSCAP** | 安全合规扫描 | OpenSCAP | v1.4.0 | 1k+ | LGPL-2.1 |
| **Lynis** | 安全审计工具 | CISOfy | v3.1.1 | 13k+ | GPL-3.0 |
| **Falco** | 运行时安全检测 | CNCF | v0.38.0 | 7k+ | Apache-2.0 |
| **AIDE** | 文件完整性检测 | AIDE | v0.18.6 | - | GPL-2.0 |
| **ClamAV** | 开源反病毒引擎 | Cisco | v1.3.1 | 4k+ | GPL-2.0 |

## SELinux 与 Kubernetes

SELinux 通过为进程和文件分配安全上下文标签，实现强制访问控制。在 Kubernetes 环境中，SELinux 可以防止容器进程访问不属于它的文件，即使该进程以 root 身份运行。

```yaml
SELinux与K8s集成:
  K8s配置:
    - securityContext.seLinuxOptions.level: "s0:c123,c456"
    - securityContext.seLinuxOptions.role: "sysadm_r"
    - securityContext.seLinuxOptions.type: "sysadm_t"
  
  容器运行时:
    - containerd自动为容器进程分配SVirt标签
    - 每个容器获得唯一的MCS标签(s0:cXXX,cYYY)
    - 防止容器间文件访问
  
  故障排查:
    - 查看拒绝日志: ausearch -m avc -ts recent
    - 生成允许规则: audit2allow -a -M mypolicy
    - 临时切换模式: setenforce 0 (Permissive)
    - 查看文件上下文: ls -Z /path/to/file
```

```bash
# SELinux 常用运维命令
# 查看当前模式
getenforce

# 查看详细状态
sestatus

# 切换模式（临时）
setenforce 1  # Enforcing
setenforce 0  # Permissive

# 查看文件安全上下文
ls -Z /var/lib/containerd/

# 修改文件安全上下文
semanage fcontext -a -t container_file_t "/data/mysql(/.*)?"
restorecon -Rv /data/mysql/

# 查看SELinux拒绝日志
ausearch -m avc -ts today | audit2allow -w

# 生成自定义策略模块
ausearch -m avc -ts recent | audit2allow -M mypolicy
semodule -i mypolicy.pp

# 查看布尔值
getsebool -a | grep container
setsebool -P container_manage_cgroup on
```

## 网络工具

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **iptables/nftables** | 网络包过滤 | netfilter | v1.1.0 | - | GPL-2.0 |
| **ipvsadm** | IPVS 负载均衡管理 | Linux-HA | v1.31.0 | - | GPL-2.0+ |
| **conntrack-tools** | 连接跟踪工具 | netfilter | v1.4.8 | - | GPL-2.0 |
| **iproute2** | 网络配置工具集 | kernel.org | v6.9.0 | - | GPL-2.0 |
| **bridge-utils** | 网桥管理 | kernel.org | v1.7.1 | - | GPL-2.0 |
| **WireGuard** | VPN 隧道 | WireGuard | v1.0.2 | 5k+ | GPL-2.0 |

## kube-proxy 依赖的网络工具

```yaml
kube-proxy网络模式:
  iptables模式:
    依赖: iptables, conntrack
    原理: 每个Service生成KUBE-SVC链，每个Endpoint生成KUBE-SEP链
    优点: 成熟稳定，兼容性好
    缺点: 规则数量线性增长，大规模集群性能下降
    查看命令:
      - iptables -t nat -L KUBE-SERVICES
      - iptables -t nat -L KUBE-SVC-XXXX
      - iptables -t nat -L KUBE-SEP-XXXX
  
  IPVS模式:
    依赖: ipvsadm, ip_vs内核模块, conntrack
    原理: 使用内核IPVS子系统实现O(1)查找的负载均衡
    优点: 性能更好，支持多种调度算法
    缺点: 需要额外内核模块
    查看命令:
      - ipvsadm -Ln
      - ipvsadm -Ln -t 10.96.0.1:443
      - ipvsadm -Ln --rate
  
  NFTables模式 (新):
    依赖: nft
    原理: 使用nftables替代iptables
    优点: 更高效，语法更清晰
    状态: Kubernetes 1.29+支持
```

```bash
#!/bin/bash
# K8s 网络诊断脚本
set -euo pipefail

echo "=== K8s 网络诊断 ==="

echo "[1] 内核模块检查"
lsmod | grep -E "overlay|br_netfilter|ip_vs|nf_conntrack" || true

echo ""
echo "[2] sysctl 参数检查"
sysctl net.ipv4.ip_forward
sysctl net.bridge.bridge-nf-call-iptables 2>/dev/null || true
sysctl net.netfilter.nf_conntrack_max

echo ""
echo "[3] conntrack 使用情况"
conntrack -C
sysctl net.netfilter.nf_conntrack_max
echo "使用率: $(echo "scale=2; $(conntrack -C) * 100 / $(sysctl -n net.netfilter.nf_conntrack_max)" | bc)%"

echo ""
echo "[4] kube-proxy 模式"
if ipvsadm -Ln 2>/dev/null | head -5; then
    echo "模式: IPVS"
else
    echo "模式: iptables"
    iptables -t nat -L KUBE-SERVICES 2>/dev/null | head -10 || true
fi

echo ""
echo "[5] 网络命名空间"
ip netns list | head -5

echo ""
echo "[6] CNI 配置"
ls /etc/cni/net.d/ 2>/dev/null || echo "CNI配置目录不存在"

echo ""
echo "[7] 网络接口统计"
ip -s link show | grep -E "^[0-9]+:|RX:|TX:" | head -20
```

## 容器与虚拟化工具

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **runc** | OCI 容器运行时 | OCI | v1.1.12 | 12k+ | Apache-2.0 |
| **crun** | C 语言容器运行时 | containers | v1.15 | 3k+ | GPL-2.0 |
| **containerd** | 容器运行时 | CNCF | v2.0.0 | 17k+ | Apache-2.0 |
| **CRI-O** | K8s 容器运行时 | CRI-O | v1.30.0 | 5k+ | Apache-2.0 |
| **nsenter** | 命名空间进入 | util-linux | v2.40.0 | - | GPL-2.0+ |
| **Cilium** | eBPF 网络插件 | Isovalent/CNCF | v1.16.0 | 19k+ | Apache-2.0 |

## 容器运行时选型对比

| 特性 | containerd | CRI-O | Docker (已弃用) |
|:---|:---|:---|:---|
| **默认运行时** | GKE, EKS, AKS | OpenShift | 旧版 K8s |
| **镜像管理** | 内置 | 内置 | Docker Engine |
| **CRI 兼容** | 原生 | 原生 | 需 dockershim |
| **资源占用** | 低 | 低 | 较高 |
| **社区活跃度** | 非常高 | 高 | 维护模式 |
| **K8s 版本支持** | 1.24+ | 1.24+ | 1.23 及以下 |

## 性能监控

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **perf** | 内核性能分析 | Linux 内核 | - | - | GPL-2.0 |
| **bcc/bpftrace** | eBPF 性能工具 | iovisor | v0.30.0 | 20k+ | Apache-2.0 |
| **sysstat** | 系统性能统计 | sysstat | v12.7.5 | 3k+ | GPL-2.0 |
| **pcp** | 性能 Co-Pilot | pcp | v6.2.0 | - | GPL-2.0/LGPL-2.1 |
| **bcc-tools** | BCC 工具集 | iovisor | v0.30.0 | - | Apache-2.0 |
| **bpftrace** | 高级 eBPF 追踪 | iovisor | v0.21.0 | 8k+ | Apache-2.0 |

## USE 方法论工具对照

```yaml
USE方法论_工具对照表:
  CPU:
    Utilization: mpstat, top, perf
    Saturation: runq-sz (sar -q), /proc/pressure/cpu
    Errors: perf (hardware errors), dmesg
  
  内存:
    Utilization: free, vmstat, /proc/meminfo
    Saturation: /proc/pressure/memory, pgscan (vmstat)
    Errors: dmesg (OOM), edac (ECC errors)
  
  网络IO:
    Utilization: ip -s link, sar -n DEV
    Saturation: /proc/pressure/io, drop (ip -s link)
    Errors: ifconfig (errors), ethtool -S
  
  磁盘IO:
    Utilization: iostat -x, sar -d
    Saturation: /proc/pressure/io, avgqu-sz (iostat)
    Errors: smartctl, dmesg (IO errors)
```

## 存储管理

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **LVM2** | 逻辑卷管理 | Red Hat | v2.03.23 | - | GPL-2.0 |
| **mdadm** | 软件 RAID | kernel.org | v4.1 | - | GPL-2.0 |
| **stratis** | 存储管理 | Stratis | v3.6.0 | - | MPL-2.0 |
| **fio** | I/O 性能测试 | fio | v3.37 | 5k+ | GPL-2.0 |
| **NFS-Ganesha** | NFS 用户态服务 | NFS-Ganesha | v5.0 | - | LGPL-2.1 |

---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

## kube-proxy 依赖的内核项目

```yaml
kube-proxy依赖:
  iptables/nftables:
    用途: 默认模式，Service转发和负载均衡
    规则链: KUBE-SVC-*, KUBE-SEP-*, KUBE-FW-*
    规模: 每个Service约2条规则 + 每个Endpoint约2条规则
    
  IPVS:
    用途: 高性能模式
    模块: ip_vs, ip_vs_rr, ip_vs_wrr, ip_vs_lc
    配置: ipvsadm -Ln
    
  conntrack:
    用途: 连接跟踪
    参数: net.netfilter.nf_conntrack_max
    推荐: 1048576+
    
  br_netfilter:
    用途: 网桥流量经过iptables
    参数: net.bridge.bridge-nf-call-iptables=1
```

## 容器运行时依赖的内核项目

```yaml
容器运行时依赖:
  namespaces:
    PID: 进程隔离
    Network: 网络隔离
    Mount: 文件系统隔离
    UTS: 主机名隔离
    IPC: 进程间通信隔离
    User: 用户隔离 (rootless容器)
    Cgroup: Cgroup视图隔离
  
  cgroups_v2:
    cpu.max: CPU时间限制
    cpu.weight: CPU权重
    memory.max: 内存限制
    memory.high: 内存软限制
    io.max: IO带宽限制
    pids.max: 进程数限制
  
  OverlayFS:
    用途: 容器镜像分层存储
    原理: upperdir + lowerdir = merged
    挂载: mount -t overlay overlay -o lowerdir=...,upperdir=...,workdir=...
  
  seccomp:
    用途: 限制容器系统调用
    默认: RuntimeDefault profile
    配置: SecurityContext.seccompProfile
  
  capabilities:
    用途: 细粒度权限控制
    推荐: drop ["ALL"], add ["NET_BIND_SERVICE"]
    配置: SecurityContext.capabilities
```

---

<!-- chunk: 内核版本选择策略 -->## 内核版本选择策略

| K8s 版本 | 推荐内核版本 | 原因 |
|:---|:---|:---|
| K8s 1.28-1.30 | 5.15 LTS | cgroups v2 成熟，eBPF 功能完善 |
| K8s 1.31+ | 6.1 LTS 或更新 | User Namespace 支持、更好的 eBPF 性能 |
| 边缘/嵌入式 | 5.10 LTS | 长期支持，硬件兼容性好 |

## 必须验证的内核功能

```bash
#!/bin/bash
# K8s 节点内核功能验证脚本
echo "=== K8s 节点内核功能验证 ==="

echo "[1] 内核版本"
uname -r
echo "推荐: >= 5.15"
echo ""

echo "[2] cgroups v2"
if [ -f /sys/fs/cgroup/cgroup.controllers ]; then
    echo "cgroups v2: 已启用"
    cat /sys/fs/cgroup/cgroup.controllers
else
    echo "cgroups v2: 未启用 (需要启用)"
fi
echo ""

echo "[3] overlay 模块"
lsmod | grep overlay && echo "overlay: 已加载" || echo "overlay: 未加载"
echo ""

echo "[4] br_netfilter 模块"
lsmod | grep br_netfilter && echo "br_netfilter: 已加载" || echo "br_netfilter: 未加载"
echo ""

echo "[5] IP 转发"
sysctl net.ipv4.ip_forward
echo "期望值: 1"
echo ""

echo "[6] bridge-nf-call-iptables"
sysctl net.bridge.bridge-nf-call-iptables 2>/dev/null || echo "br_netfilter未加载，无法检查"
echo "期望值: 1"
echo ""

echo "[7] eBPF 支持"
if [ -f /proc/config.gz ]; then
    zcat /proc/config.gz | grep CONFIG_BPF_SYSCALL=y && echo "eBPF: 已启用" || echo "eBPF: 未启用"
else
    echo "无法检查（/proc/config.gz 不存在）"
fi
echo ""

echo "[8] Swap"
swapon --show
echo "推荐: 禁用或 swappiness <= 10"
sysctl vm.swappiness
```

---

<!-- chunk: 版本兼容性矩阵 -->## 版本兼容性矩阵

| 内核功能 | 最低内核版本 | K8s 用途 | 推荐版本 |
|:---|:---|:---|:---|
| cgroups v2 | 4.5 | 资源限制 | 5.8+ |
| User Namespace | 3.8 | rootless 容器 | 5.11+ |
| eBPF (BTF) | 5.2 | Cilium/Falco | 5.10+ |
| OverlayFS | 3.18 | 容器存储 | 5.4+ |
| IPVS | 2.6 | kube-proxy | 所有 |
| Seccomp | 3.5 | 系统调用过滤 | 5.4+ |
| Time Namespace | 5.6 | 容器时间隔离 | 5.6+ |
| io_uring | 5.1 | 高性能 I/O | 5.10+ |

---

<!-- chunk: 安全工具详解 -->## 安全工具详解

## Falco 运行时安全配置

Falco 是 CNCF 毕业项目，是云原生环境下最重要的运行时安全检测工具。它使用内核模块或 eBPF 探针来捕获系统调用，然后通过规则引擎匹配已知的攻击模式。

```yaml
# Falco 自定义规则 - K8s安全检测
apiVersion: falco.org/v1
kind: FalcoRule
metadata:
  name: k8s-security-rules
spec:
  rules:
    - name: Terminal Shell in Container
      desc: A shell was spawned in a container
      condition: >
        spawned_process and container and
        proc.name in (bash, sh, zsh, fish) and
        not proc.pname in (docker-entrypoint)
      output: "Shell spawned in container (user=%user.name container=%container.name shell=%proc.name parent=%proc.pname cmdline=%proc.cmdline)"
      priority: WARNING
      tags: [container, shell]
    
    - name: Read Sensitive File
      desc: Attempt to read sensitive file
      condition: >
        open_read and
        fd.name in (/etc/shadow, /etc/passwd, /etc/kubernetes/admin.conf) and
        not proc.name in (sshd, systemd, login)
      output: "Sensitive file read (user=%user.name file=%fd.name)"
      priority: WARNING
      tags: [filesystem, sensitive]
```

## OpenSCAP 安全扫描

```bash
# OpenSCAP 扫描示例
oscap xccdf eval --profile xccdf_org.ssgproject.content_profile_cis \
  --results /tmp/results.xml \
  --report /tmp/report.html \
  /usr/share/xml/scap/ssg/content/ssg-rhel9-ds.xml

# Lynis 扫描示例
lynis audit system --quick
lynis audit system --pentest

# 查看扫描报告
cat /var/log/lynis-report.dat
```

## AIDE 文件完整性监控

```bash
# 初始化AIDE数据库
aide --init
cp /var/lib/aide/aide.db.new.gz /var/lib/aide/aide.db.gz

# 执行完整性检查
aide --check

# 更新数据库（在合法变更后）
aide --update
cp /var/lib/aide/aide.db.new.gz /var/lib/aide/aide.db.gz

# 配置监控K8s关键文件
cat >> /etc/aide.conf << 'EOF'
# Kubernetes files
/etc/kubernetes AUTO
/etc/etcd AUTO
/var/lib/etcd AUTO
/etc/cni AUTO
/etc/containerd AUTO
EOF
```

---

<!-- chunk: 参考链接 -->## 参考链接

- [Linux Kernel Archives](https://www.kernel.org/)
- [systemd 文档](https://www.freedesktop.org/wiki/Software/systemd/)
- [Flatcar 文档](https://www.flatcar.org/docs/)
- [Bottlerocket 文档](https://bottlerocket.dev/)
- [Talos 文档](https://www.talos.dev/)
- [eBPF.io](https://ebpf.io/)
- [Cilium 文档](https://docs.cilium.io/)
- [containerd 文档](https://containerd.io/)
- [OpenSCAP](https://www.open-scap.org/)
- [Lynis](https://cisofy.com/lynis/)
- [Falco 文档](https://falco.org/docs/)

---

**维护者**: Allen Galler (allengaller@gmail.com) | **许可证**: MIT

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 系统基础 MOC
- [[17-系统基础/README.md|Domain-14: Linux 基础知识体系]]
- 01 - Linux 系统架构与内核深度解析：生产环境运维专家指南
- 02 - Linux 进程管理与系统监控：生产环境运维专家实践
- 03 - Linux 文件系统深度解析：生产环境存储管理专家指南
- 04 - Linux 网络配置与性能优化：生产环境网络运维专家指南
- 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南
- 06 - Linux 性能调优与瓶颈分析：生产环境性能优化专家指南
- 07 - Linux 安全加固与合规管理：生产环境安全运维专家指南
- 08 - Linux 容器技术深度解析：生产环境容器运维专家指南
- 09 - Linux 运维基础与应急响应：生产环境运维专家实践指南
- Linux 命令大全参考


<!-- risk-assessed -->

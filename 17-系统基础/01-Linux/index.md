---
title: Linux 知识体系
description: 云原生 Linux 知识体系，覆盖系统架构、进程管理、文件系统、网络、存储、性能调优、安全加固、容器基础、内核调优、ARM 架构、Windows 容器等 15 个子领域
summary: Linux 知识体系总索引，覆盖系统架构、进程/文件/网络/存储、性能调优、安全加固、容器基础、内核参数、ARM/Windows
category: index
tags:
- index
- linux
- kernel
- container
- performance
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 开发工程师
---

# Linux 知识体系

> 本知识体系覆盖云原生工程师必备的 Linux 全域知识，是理解容器运行时、优化 K8s 节点性能、排查系统级故障的权威参考。

## 领域概述

Linux 是 Kubernetes 的操作系统基石，包括：

- **系统架构**：内核、系统调用、启动流程、模块
- **进程管理**：进程/线程、调度、cgroup、namespace
- **文件系统**：VFS、ext4/xfs、OverlayFS、inotify
- **网络配置**：TCP/IP、netfilter、iptables/nftables、eBPF
- **存储管理**：块设备、LVM、RAID、IO 调度
- **性能调优**：CPU/内存/IO/网络调优、perf、bpftrace
- **安全加固**：SELinux/AppArmor、seccomp、capabilities
- **容器基础**：namespace、cgroup、OCI、运行时
- **内核调优**：sysctl、容器场景优化
- **多架构**：ARM 优化、Windows 容器

## 文档索引

### 核心基础

| 文档 | 内容 | 行数 |
|------|------|------|
| [[17-系统基础/01-Linux/01-linux-system-architecture.md|系统架构]] | 内核、系统调用、启动流程 | 1091 |
| [[17-系统基础/01-Linux/03-linux-process-management.md|进程管理]] | 进程/线程、调度、cgroup | 976 |
| [[17-系统基础/01-Linux/04-linux-filesystem-deep-dive.md|文件系统]] | VFS、ext4/xfs、OverlayFS | 1006 |
| [[17-系统基础/01-Linux/05-linux-networking-configuration.md|网络配置]] | TCP/IP、netfilter、eBPF | 870 |
| [[17-系统基础/01-Linux/06-linux-storage-management.md|存储管理]] | 块设备、LVM、IO 调度 | 993 |

### 运维与调优

| 文档 | 内容 | 行数 |
|------|------|------|
| [[17-系统基础/01-Linux/07-linux-performance-tuning.md|性能调优]] | CPU/内存/IO/网络调优 | 872 |
| [[17-系统基础/01-Linux/08-linux-security-hardening.md|安全加固]] | SELinux、seccomp、capabilities | 941 |
| [[17-系统基础/01-Linux/09-linux-container-fundamentals.md|容器基础]] | namespace、cgroup、OCI | 933 |
| [[17-系统基础/01-Linux/10-linux-operations-basics.md|运维基础]] | 系统管理、日志、服务 | 967 |
| [[17-系统基础/01-Linux/15-linux-commands-reference.md|命令参考]] | 常用命令速查 | 1963 |

### 专项技术

| 文档 | 内容 | 行数 |
|------|------|------|
| [[17-系统基础/01-Linux/11-k8s-node-os-image-hardening-baseline.md|节点 OS 加固基线]] | 镜像加固、安全基线 | 337 |
| [[17-系统基础/01-Linux/12-arm-architecture-k8s-optimization.md|ARM 架构优化]] | ARM64、多架构、性能 | 440 |
| [[17-系统基础/01-Linux/13-kernel-tuning-container-performance.md|内核调优]] | sysctl、容器场景优化 | 436 |
| [[17-系统基础/01-Linux/14-windows-containers-k8s.md|Windows 容器]] | Windows 节点、混合集群 | 444 |

## 核心概念速查

### Namespace（命名空间）

| 类型 | 隔离内容 | K8s 用途 |
|------|----------|----------|
| PID | 进程 ID | 容器进程隔离 |
| NET | 网络栈 | Pod 网络隔离 |
| MNT | 挂载点 | 容器文件系统 |
| UTS | 主机名 | 容器主机名 |
| IPC | 进程间通信 | 容器 IPC 隔离 |
| USER | 用户/组 | 容器用户映射 |
| CGROUP | cgroup 根 | cgroup 隔离 |

```bash
# 查看进程 namespace
ls -la /proc/<pid>/ns/
# 进入特定 namespace
nsenter -t <pid> -n -m -p -- /bin/bash
# 创建新 namespace
unshare --net --pid --fork --mount-proc /bin/bash
```

### Cgroup（控制组）

| 控制器 | 功能 | K8s 用途 |
|--------|------|----------|
| cpu | CPU 时间分配 | CPU request/limit |
| cpuset | CPU 核心绑定 | CPU 亲和性 |
| memory | 内存限制 | Memory request/limit |
| io | 磁盘 IO | IO 限制 |
| pids | 进程数限制 | PID 限制 |
| devices | 设备访问 | 设备控制 |

```bash
# cgroup v2 查看
cat /sys/fs/cgroup/kubepods.slice/memory.max
cat /sys/fs/cgroup/kubepods.slice/cpu.max
# 查看 Pod cgroup
systemd-cgls | grep kubepods
```

### 关键内核参数

```bash
# 网络优化
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_tw_reuse = 1

# 文件描述符
fs.file-max = 1048576
fs.nr_open = 1048576

# 内存优化
vm.swappiness = 1
vm.overcommit_memory = 1
vm.max_map_count = 262144

# 网络转发（K8s 必须）
net.ipv4.ip_forward = 1
net.bridge.bridge-nf-call-iptables = 1
net.bridge.bridge-nf-call-ip6tables = 1

# conntrack（ kube-proxy iptables 模式）
net.netfilter.nf_conntrack_max = 1048576
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
```

## 容器场景 Linux 排查

### 容器网络排查

```bash
# 查看容器网络 namespace
ip netns list
# 进入容器网络 namespace
nsenter -t $(docker inspect -f '{{.State.Pid}}' <container>) -n ip addr

# 查看 veth 对
ip link show type veth
# 查看网桥
brctl show
bridge link show

# 查看 iptables 规则（kube-proxy）
iptables -t nat -L -n -v
iptables -t filter -L -n -v
# IPVS 规则
ipvsadm -Ln

# 查看 conntrack 表
conntrack -L | wc -l
conntrack -L -p tcp --dport 8080
```

### 容器资源排查

```bash
# 查看容器 cgroup 限制
cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<uid>.slice/memory.max
cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<uid>.slice/cpu.max

# 查看容器资源使用
cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<uid>.slice/memory.current
cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<uid>.slice/cpu.stat

# 查看 OOM 记录
dmesg | grep -i "oom\|killed process"
journalctl -k | grep -i oom
```

### 容器文件系统排查

```bash
# 查看 OverlayFS 挂载
mount | grep overlay
df -h /var/lib/containerd

# 查看容器文件系统层
ls /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/

# 查看 inode 使用
df -i
# 查看大文件
find /var/lib/containerd -size +100M -exec ls -lh {} \;
```

## 性能分析工具

| 工具 | 用途 | 示例 |
|------|------|------|
| top/htop | CPU/内存概览 | htop |
| vmstat | 虚拟内存统计 | vmstat 1 5 |
| iostat | 磁盘 IO 统计 | iostat -xz 1 |
| mpstat | CPU 统计 | mpstat -P ALL 1 |
| sar | 系统活动报告 | sar -u 1 5 |
| perf | 性能剖析 | perf top |
| bpftrace | eBPF 追踪 | bpftrace -e 'tracepoint:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }' |
| strace | 系统调用追踪 | strace -p <pid> -f |
| tcpdump | 网络抓包 | tcpdump -i eth0 port 8080 |
| ss | 网络连接 | ss -tlnp |
| dstat | 综合统计 | dstat -cdngy |
| pidstat | 进程统计 | pidstat -u -p <pid> 1 |
| free | 内存使用 | free -h |
| df | 磁盘使用 | df -h |
| du | 目录大小 | du -sh /var/lib/containerd |
| lsof | 打开文件 | lsof -i :8080 |
| netstat | 网络统计 | netstat -s |
| nstat | 网络计数器 | nstat -az |

## K8s 节点 Linux 检查清单

### 节点就绪检查

- [ ] 内核版本 ≥ 5.4（推荐 5.15+）
- [ ] cgroup v2 已启用（K8s 1.25+）
- [ ] 必要内核模块已加载（br_netfilter、overlay、ip_vs）
- [ ] net.ipv4.ip_forward = 1
- [ ] net.bridge.bridge-nf-call-iptables = 1
- [ ] swap 已禁用（或配置 swapBehavior）
- [ ] 文件描述符限制已调整（≥ 1048576）
- [ ] conntrack 表大小已调整（≥ 1048576）
- [ ] 时区已同步（chrony/ntp）
- [ ] 内核日志无硬件错误
- [ ] 容器运行时已安装并运行
- [ ] kubelet 已安装并运行
- [ ] 防火墙规则已配置（或禁用）
- [ ] SELinux/AppArmor 已配置

### 性能优化检查

- [ ] CPU 调度器已优化（容器场景）
- [ ] IO 调度器已设置（SSD: none/mq-deadline）
- [ ] 网络缓冲区已调整
- [ ] TCP 参数已优化
- [ ] 内存 swappiness 已设置（1 或 0）
- [ ] 透明大页已禁用（数据库场景）
- [ ] NUMA 拓扑已确认
- [ ] 磁盘预读已优化

## 常见 Linux 故障与 K8s 影响

| Linux 故障 | K8s 影响 | 排查命令 |
|------------|----------|----------|
| 磁盘满 | 节点 DiskPressure、Pod 驱逐 | df -h、du -sh |
| inode 耗尽 | 无法创建文件、Pod 失败 | df -i |
| 内存不足 | 节点 MemoryPressure、OOM | free -h、dmesg |
| CPU 过载 | Pod 响应慢、探针失败 | top、mpstat |
| 网络丢包 | 服务超时、DNS 失败 | ss -s、netstat -s |
| conntrack 满 | 新连接失败 | conntrack -C、dmesg |
| 文件描述符耗尽 | 连接失败、API 错误 | lsof \| wc -l |
| 内核 panic | 节点宕机 | dmesg、/var/crash |
| OOM Killer | Pod 被杀 | dmesg \| grep oom |
| 时钟偏移 | 证书验证失败、日志混乱 | chronyc tracking |
| 僵尸进程 | 资源泄漏 | ps aux \| grep Z |
| 内核模块缺失 | CNI/CSI 失败 | lsmod、modprobe |

## 学习路径

```
入门: 系统架构 → 进程管理 → 文件系统 → 运维基础
中级: 网络配置 → 存储管理 → 性能调优 → 命令参考
高级: 安全加固 → 容器基础 → 内核调优
专家: ARM 优化 → Windows 容器 → eBPF → 内核开发
```

## 参考链接

- https://www.kernel.org/doc/html/latest/
- https://man7.org/linux/man-pages/
- https://www.brendangregg.com/linuxperf.html
- https://docs.kernel.org/admin-guide/sysctl/index.html
- https://www.redhat.com/en/topics/linux
- https://ebpf.io/
- https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html
- https://www.kernel.org/doc/html/latest/networking/index.html

## Related

- [[17-系统基础/02-硬件/index.md|硬件知识]]
- [[17-系统基础/05-速查卡/linux.md|Linux 速查卡]]
- [[17-系统基础/06-知识字典/fundamentals/index.md|K8s 基础知识]]

## 文档

- [[17-系统基础/01-Linux/02-linux-kernel-container-fundamentals.md|02-linux-kernel-container-fundamentals]]

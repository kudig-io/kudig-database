---
title: 06 - Linux 性能调优与瓶颈分析：生产环境性能优化专家指南
description: '# 06 - Linux 性能调优与瓶颈分析：生产环境性能优化专家指南'
summary: '性能调优是 Linux 系统运维中最具挑战性的领域之一。在 [[Kubernetes|Kubernetes]] 环境中，性能问题往往会层层传导——宿主机的 CPU 调度延迟会影响到容器内应用的响应时间，节点的内存压力会触发 OOM Killer 终止 Pod，磁盘 I/O 瓶颈会导致 [[etcd|etcd]] 读写超时进而影响整个集群的稳定性。'
category: linux
tags:
- linux
- system
- kernel
- etcd
- scheduler
- prometheus
- cilium
- containerd
- docker
- falco
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 运维工程师
- SRE
- 系统管理员
estimated_read_time: 5min
intent_queries:
- Linux 性能调优与瓶颈分析：生产环境性能优化专家指南 是什么
- 如何 Linux 性能调优与瓶颈分析：生产环境性能优化专家指南
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 性能调优与瓶颈分析：生产环境性能优化专家指南
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/linux.md
  label: '速查卡: linux'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 06 - Linux 性能调优与瓶颈分析：生产环境性能优化专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 概述 -->## 概述

性能调优是 Linux 系统运维中最具挑战性的领域之一。在 [[Kubernetes|Kubernetes]] 环境中，性能问题往往会层层传导——宿主机的 CPU 调度延迟会影响到容器内应用的响应时间，节点的内存压力会触发 OOM Killer 终止 Pod，磁盘 I/O 瓶颈会导致 [[etcd|etcd]] 读写超时进而影响整个集群的稳定性。本文档系统性地讲解 Linux 性能分析方法论、全栈监控工具链、以及针对 CPU、内存、I/O、网络四大子系统的调优策略，特别关注容器和 Kubernetes 场景下的性能优化实践。

---

<!-- chunk: 核心概念详解 -->## 核心概念详解

## 性能分析方法论

科学的性能分析方法论比工具本身更重要。以下是业界常用的三种方法论：

## USE 方法 (Utilization, Saturation, Errors)

USE 方法由 Brendan Gregg 提出，适用于任何资源类型。对每种资源检查三个指标：

```
┌─────────────────────────────────────────────────────────────────┐
│                     USE 方法检查清单                              │
│                                                                  │
│  资源类型     │ 利用率 (U)     │ 饱和度 (S)    │ 错误 (E)       │
│  ───────────┼───────────────┼──────────────┼─────────────── │
│  CPU         │ %us, %sy      │ runq-sz      │ 运行时错误      │
│  内存        │ 已用/总量      │ swap in/out  │ OOM events     │
│  网络接口    │ 带宽使用率     │ 丢包/重传     │ 接口错误        │
│  存储设备    │ %util         │ iowait, 队列  │ I/O errors     │
│  存储容量    │ 使用百分比     │ 空间不足告警  │ ENOSPC         │
│  文件描述符  │ 已用/限制      │ 分配失败      │ EMFILE         │
│  连接跟踪    │ 已用/限制      │ 新连接失败    │ nf_conntrack   │
│  ───────────┼───────────────┼──────────────┼─────────────── │
│                                                                  │
│  Utilization > 80% → 检查 Saturation                            │
│  Saturation > 0    → 性能瓶颈确认                                │
│  Errors > 0        → 立即调查                                    │
└─────────────────────────────────────────────────────────────────┘
```

## TSA 方法 (Trend, Statistic, Analysis)

```
1. 趋势分析 (Trend)
   ├── 收集历史性能数据 (sar, Prometheus)
   ├── 识别增长趋势 (CPU、内存、磁盘)
   └── 预测资源耗尽时间

2. 统计分析 (Statistic)
   ├── 计算百分位数 (P50, P90, P99)
   ├── 识别异常值和离群点
   └── 建立性能基线

3. 深入分析 (Analysis)
   ├── 火焰图分析 (Flame Graph)
   ├── 系统调用追踪 (strace, perf)
   └── 根因定位 (Root Cause Analysis)
```

## Linux 性能分析工具全景

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────┐
│                     性能分析工具全景图                            │
│                                                                  │
│  观察范围    │ 工具                                             │
│  ──────────┼────────────────────────────────────────────────── │
│  系统级     │ top, htop, vmstat, mpstat, iostat, sar           │
│  进程级     │ ps, pstree, pmap, lsof, strace, ltrace           │
│  CPU       │ perf, turbostat, cpupower, bpftrace              │
│  内存      │ free, slabtop, numastat, bpftrace                 │
│  文件系统  │ df, du, mountstats, bpftrace                      │
│  磁盘 I/O  │ iostat, iotop, blktrace, bpftrace                │
│  网络      │ ss, ip, tcpdump, ethtool, bpftrace                │
│  eBPF      │ bpftrace, BCC tools, Cilium Hubble               │
│  容器级     │ cgroups, docker stats, kubectl top               │
│  应用级     │ perf, flamegraph, async-profiler (Java)          │
└─────────────────────────────────────────────────────────────────┘
```
---

## CPU 性能分析

## CPU 调度原理

Linux 使用 CFS（完全公平调度器）管理普通进程的 CPU 分配。理解 CPU 调度有助于排查容器中 CPU 限制相关的问题。

```
┌─────────────────────────────────────────────────────────────────┐
│                     CPU 调度与容器                               │
│                                                                  │
│  物理机 (4 核)                                                   │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│  │  CPU 0   │ │  CPU 1   │ │  CPU 2   │ │  CPU 3   │           │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘           │
│                                                                  │
│  Kubernetes Pod 的 CPU 限制通过 cgroups 实现:                    │
│                                                                  │
│  Pod A (limits.cpu=2)                                           │
│  ┌──────────────────────────────────────────┐                   │
│  │ cgroup: cpu.max = "200000 100000"         │                   │
│  │ 每 100ms 周期内可使用 200ms CPU 时间       │                   │
│  │ = 200% CPU = 2 核                         │                   │
│  │                                           │                   │
│  │ 超过配额后: 进程被限流 (throttled)          │                   │
│  │ 容器内观察到: CPU 使用率被压制              │                   │
│  └──────────────────────────────────────────┘                   │
│                                                                  │
│  Pod B (limits.cpu=1)                                           │
│  ┌──────────────────────────────────────────┐                   │
│  │ cgroup: cpu.max = "100000 100000"         │                   │
│  │ 每 100ms 周期内可使用 100ms CPU 时间       │                   │
│  │ = 100% CPU = 1 核                         │                   │
│  └──────────────────────────────────────────┘                   │
│                                                                  │
│  CPU throttling 指标:                                            │
│  container_cpu_cfs_throttled_periods_total                       │
│  container_cpu_cfs_periods_total                                 │
│  throttled_ratio = throttled_periods / total_periods             │
└─────────────────────────────────────────────────────────────────┘
```

## CPU 性能监控

```bash
# 全局 CPU 使用
top -bn1 | head -5
mpstat -P ALL 1 3                  # 每核使用率

# 负载均衡
uptime                              # 1/5/15 分钟负载
cat /proc/loadavg                   # 详细负载

# CPU 各项指标解读
# %us  - 用户态 CPU (应用计算)
# %sy  - 内核态 CPU (系统调用、中断)
# %ni  - nice 值非零的用户进程
# %id  - 空闲
# %wa  - I/O 等待 (等待磁盘/网络 I/O)
# %hi  - 硬中断
# %si  - 软中断
# %st  - 虚拟机被宿主机偷取的时间

# 上下文切换
vmstat 1 5                          # cs 列 = 上下文切换数
cat /proc/stat | grep ctxt          # 累计上下文切换

# 中断统计
cat /proc/interrupts                # 中断统计
watch -n1 "cat /proc/softirqs"      # 软中断

# 进程级 CPU 分析
pidstat -u 1                        # 进程 CPU 使用
pidstat -p <pid> 1                  # 指定进程

# CPU 频率信息
cpupower frequency-info
cat /proc/cpuinfo | grep -i "model name|cpu MHz"
```

## perf 深度分析

```bash
# 实时热点函数
perf top
perf top -g                          # 显示调用栈
perf top -p <pid>                    # 指定进程

# 记录性能数据
perf record -g -a sleep 30          # 记录 30 秒
perf record -g -p <pid> sleep 30    # 记录指定进程
perf record -e cpu-clock -g -- command  # 指定事件

# 分析记录
perf report                           # 交互式分析
perf report --stdio                   # 文本输出

# 生成火焰图
perf record -g -p <pid> sleep 30
perf script | stackcollapse-perf.pl | flamegraph.pl > cpu.svg

# 统计硬件事件
perf stat command
perf stat -e cache-misses,instructions,cycles command

# BCC 工具 (eBPF 高级分析)
# 安装 BCC
# RHEL: yum install bcc-tools
# Ubuntu: apt install bpfcc-tools

# 常用 BCC 工具
execsnoop                           # 跟踪进程执行
opensnoop                           # 跟踪文件打开
biosnoop                            # 跟踪块 I/O
tcplife                             # 跟踪 TCP 连接生命周期
tcpconnect                          # 跟踪 TCP 连接发起
slabratetop                         # slab 缓存分配速率
offcputime                          # CPU 之外的等待时间
```

---

## 内存性能分析

## Linux 内存管理模型

```
┌─────────────────────────────────────────────────────────────────┐
│                     Linux 内存架构                               │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    物理内存 (RAM)                          │  │
│  │                                                            │  │
│  │  ┌─────────┐  ┌──────────┐  ┌──────────┐                 │  │
│  │  │ 用户进程 │  │ Page     │  │ Buffer   │                 │  │
│  │  │ 匿名页   │  │ Cache    │  │ Cache    │                 │  │
│  │  │ (RSS)   │  │ (文件缓存)│  │ (块设备) │                 │  │
│  │  └─────────┘  └──────────┘  └──────────┘                 │  │
│  │                                                            │  │
│  │  ┌─────────┐  ┌──────────┐  ┌──────────┐                 │  │
│  │  │ Kernel  │  │ Slab     │  │ Huge     │                 │  │
│  │  │ Stack   │  │ Cache    │  │ Pages    │                 │  │
│  │  │         │  │ (dentry, │  │ (2MB/1GB)│                 │  │
│  │  │         │  │  inode)  │  │          │                 │  │
│  │  └─────────┘  └──────────┘  └──────────┘                 │  │
│  └──────────────────────────────────────────────────────────┘  │
│                            │                                    │
│                    ┌───────┴───────┐                             │
│                    │   Swap Space  │ ← 当物理内存不足时使用       │
│                    │   (磁盘交换)   │                             │
│                    └───────────────┘                             │
│                                                                  │
│  关键公式:                                                       │
│  MemAvailable ≈ MemFree + Buffers + Cached + SReclaimable       │
│  MemUsed ≈ MemTotal - MemAvailable                              │
│  实际应用可用 ≈ MemAvailable (不是 MemFree)                       │
└─────────────────────────────────────────────────────────────────┘
```

## 内存监控命令

```bash
# 系统内存概览
free -h                             # 人性化显示
free -b                             # 字节单位
cat /proc/meminfo                   # 详细内存信息

# 关键指标解读 (/proc/meminfo):
# MemTotal        - 总物理内存
# MemFree         - 完全空闲的内存
# MemAvailable    - 实际可用内存 (包含可回收的缓存)
# Buffers         - 块设备缓冲
# Cached          - 页缓存 (文件内容)
# SwapCached      - 同时在 swap 和内存中的页
# Slab            - 内核 slab 缓存
# SReclaimable    - 可回收的 slab
# PageTables      - 页表占用的内存

# 虚拟内存统计
vmstat 1 5                          # 每秒统计
# si (swap in)  - 从 swap 读入的内存页
# so (swap out) - 写入 swap 的内存页
# 如果 si/so 持续非零，说明内存不足

# 进程内存使用
ps aux --sort=-%mem | head -20      # 按内存排序
smem -t -k                          # 按进程显示 (含共享内存分摊)
pidstat -r 1                        # 实时进程内存

# 进程内存映射
pmap -x <pid>                       # 详细内存映射
cat /proc/<pid>/smaps_rollup        # 内存使用汇总
cat /proc/<pid>/smaps | grep -E "^Size|^Rss|^Pss"  # 段级别详情

# slab 缓存分析
slabtop                             # 实时 slab 统计
cat /proc/slabinfo                  # slab 信息
cat /sys/kernel/slab/*/objects      # slab 对象数

# NUMA 统计
numastat                            # NUMA 节点统计
numastat -p <pid>                   # 进程 NUMA 分布
```

## 内存调优参数

```bash
# /etc/sysctl.d/99-memory.conf

# Swap 倾向 (0=尽可能不用swap, 100=积极使用swap)
# K8s 节点建议设为低值
vm.swappiness = 10

# 脏页比例 (开始刷盘的阈值)
vm.dirty_background_ratio = 5       # 后台异步刷盘
vm.dirty_ratio = 20                 # 同步阻塞刷盘

# 脏页过期时间 (cs)
vm.dirty_expire_centisecs = 3000    # 30 秒

# 脏页唤醒间隔 (cs)
vm.dirty_writeback_centisecs = 500  # 5 秒

# 内存过量分配策略
# 0=启发式, 1=总是允许, 2=严格限制
vm.overcommit_memory = 0
vm.overcommit_ratio = 50            # 当 overcommit_memory=2 时生效

# OOM 策略
vm.panic_on_oom = 0                 # 不 panic

# 最小空闲内存 (KB) - 低于此值内核会强制回收
vm.min_free_kbytes = 65536          # 建议总内存的 0.5%-1%

# vfs 缓存压力 (越大越积极回收 dentry/inode 缓存)
vm.vfs_cache_pressure = 100

# 最大内存映射数量
vm.max_map_count = 262144           # Elasticsearch 等应用需要

# 透明大页 (Transparent Huge Pages)
# 数据库建议关闭
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag

sysctl --system
```

---

## I/O 性能分析

## 存储栈与 I/O 调度

```
┌─────────────────────────────────────────────────────────────────┐
│                     Linux 存储 I/O 栈                            │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                   用户空间应用                             │  │
│  │   open() → write() → read() → close()                    │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │ VFS                                 │
│  ┌────────────────────────┴─────────────────────────────────┐  │
│  │                  虚拟文件系统 (VFS)                         │  │
│  │   Page Cache → Buffer Cache → 文件系统 (ext4/xfs/btrfs)   │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│  ┌────────────────────────┴─────────────────────────────────┐  │
│  │                  块设备层 (Block Layer)                    │  │
│  │   I/O 调度器 → 合并/排序 → 请求队列                        │  │
│  │   调度器: none, mq-deadline, bfq, kyber                   │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│  ┌────────────────────────┴─────────────────────────────────┐  │
│  │                  设备驱动层                                │  │
│  │   SCSI/SATA/NVMe/Virtio                                   │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│  ┌────────────────────────┴─────────────────────────────────┐  │
│  │                  物理设备                                  │  │
│  │   HDD / SATA SSD / NVMe SSD                               │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  I/O 调度器选择:                                                 │
│  NVMe SSD  → none (无调度, 设备自带队列)                        │
│  SATA SSD  → mq-deadline (截止时间调度)                         │
│  HDD       → bfq (公平带宽分配)                                 │
│  KVM 虚拟机 → none (宿主机已做调度)                              │
└─────────────────────────────────────────────────────────────────┘
```

## I/O 监控命令

```bash
# iostat - I/O 统计
iostat -xz 1                       # 每秒输出，扩展格式
iostat -xz 1 10                    # 输出 10 次
iostat -xz -p sda 1                # 指定设备

# iostat 关键字段:
# r/s      - 每秒读请求数
# w/s      - 每秒写请求数
# rMB/s    - 读吞吐量
# wMB/s    - 写吞吐量
# await    - 平均 I/O 等待时间 (ms)
# r_await  - 平均读等待时间
# w_await  - 平均写等待时间
# %util    - 设备利用率 (100% = 设备饱和)
# avgqu-sz - 平均队列深度

# iotop - 进程级 I/O
iotop -oP                           # 只显示有 I/O 的进程
iotop -b -o -n 3                    # 批处理模式

# pidstat - 进程 I/O
pidstat -d 1                        # 每秒进程 I/O
pidstat -d -p <pid> 1               # 指定进程

# 块设备信息
cat /sys/block/sda/queue/scheduler   # 当前调度器
cat /sys/block/sda/queue/nr_requests # 请求队列深度
cat /sys/block/sda/queue/read_ahead_kb  # 预读大小

# I/O 错误检查
dmesg | grep -i "i/o error|buffer i/o error"
cat /sys/block/sda/stat              # 块设备统计
```

## I/O 性能基准测试

```bash
# fio - 灵活的 I/O 测试工具

# 顺序读 (大文件吞吐)
fio --name=seq-read --rw=read --bs=1M --numjobs=1 \
    --size=4G --runtime=60 --filename=/dev/sdb --direct=1 \
    --group_reporting

# 顺序写 (大文件吞吐)
fio --name=seq-write --rw=write --bs=1M --numjobs=1 \
    --size=4G --runtime=60 --filename=/dev/sdb --direct=1 \
    --group_reporting

# 随机读 (数据库场景)
fio --name=rand-read --rw=randread --bs=4k --numjobs=4 \
    --size=2G --runtime=60 --filename=/dev/sdb --direct=1 \
    --ioengine=libaio --iodepth=32 --group_reporting

# 随机写 (数据库场景)
fio --name=rand-write --rw=randwrite --bs=4k --numjobs=4 \
    --size=2G --runtime=60 --filename=/dev/sdb --direct=1 \
    --ioengine=libaio --iodepth=32 --group_reporting

# 混合读写 (70% 读 / 30% 写)
fio --name=mixed --rw=randrw --rwmixread=70 --bs=4k --numjobs=4 \
    --size=2G --runtime=60 --filename=/dev/sdb --direct=1 \
    --ioengine=libaio --iodepth=32 --group_reporting

# dd 简单测试 (仅用于快速检查)
dd if=/dev/zero of=/tmp/test bs=1M count=1024 oflag=direct    # 写
dd if=/tmp/test of=/dev/null bs=1M iflag=direct               # 读
```

---

## 网络性能分析

```bash
# 网络连接统计
ss -s                               # 连接统计摘要
ss -tlnp                            # TCP 监听端口
ss -tnp                             # TCP 已建立连接

# 网络流量统计
sar -n DEV 1                        # 网络设备统计
cat /proc/net/dev                   # 接口统计
ifstat                              # 接口流量

# 网络错误检查
netstat -s                          # 协议统计 (含错误)
cat /proc/net/snmp                  # SNMP 统计

# TCP 重传统计
netstat -s | grep -i retrans
cat /proc/net/snmp | grep Tcp | awk '{print $13}'  # RetransSegs

# 带宽测试
iperf3 -s                           # 服务端
iperf3 -c <server>                  # 客户端
iperf3 -c <server> -u -b 1G        # UDP 测试
iperf3 -c <server> -P 4            # 多线程测试

# 延迟测试
ping -c 100 <host> | tail -2
hping3 --tcp -p 80 -c 100 <host>   # TCP 延迟

# DNS 解析延迟
dig @<dns-server> <domain> | grep "Query time"
```

---

<!-- chunk: 常用命令参考 -->## 常用命令参考

## 综合性能监控

| 工具 | CPU | 内存 | I/O | 网络 | 安装 |
|:---:|:---:|:---:|:---:|:---:|:---|
| top/htop | Y | Y | - | - | 默认/epel |
| vmstat | Y | Y | Y | - | 默认 |
| iostat | - | - | Y | - | sysstat |
| mpstat | Y | - | - | - | sysstat |
| sar | Y | Y | Y | Y | sysstat |
| dstat | Y | Y | Y | Y | epel |
| pidstat | Y | Y | Y | - | sysstat |
| perf | Y | - | - | - | perf |
| bpftrace | Y | Y | Y | Y | bpftrace |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# sysstat 包安装和配置
yum install -y sysstat              # RHEL
apt install -y sysstat              # Ubuntu

# 启用 sar 数据收集
systemctl enable sysstat
systemctl start sysstat

# 查看历史数据
sar -u                             # CPU 历史
sar -r                             # 内存历史
sar -b                             # I/O 历史
sar -n DEV                         # 网络历史
sar -s 08:00:00 -e 18:00:00        # 指定时间范围
```
---

<!-- chunk: 性能调优 -->## 性能调优

## 生产环境内核参数模板

```bash
# /etc/sysctl.d/99-production.conf

# ===== CPU =====
kernel.sched_migration_cost_ns = 5000000
kernel.sched_autogroup_enabled = 0

# ===== 内存 =====
vm.swappiness = 10
vm.dirty_ratio = 20
vm.dirty_background_ratio = 5
vm.min_free_kbytes = 65536
vm.max_map_count = 262144
vm.overcommit_memory = 0

# ===== 网络 =====
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.ip_forward = 1
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
net.ipv4.tcp_fin_timeout = 15
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_slow_start_after_idle = 0
net.netfilter.nf_conntrack_max = 1048576

# ===== 文件系统 =====
fs.file-max = 2097152
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192
fs.nr_open = 1048576

# ===== 安全 =====
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
kernel.dmesg_restrict = 1
kernel.kptr_restrict = 2

sysctl --system
```

## ulimit 配置

```bash
# /etc/security/limits.d/99-production.conf
*       soft    nofile    65536
*       hard    nofile    65536
*       soft    nproc     65536
*       hard    nproc     65536
root    soft    nofile    65536
root    hard    nofile    65536

# 查看当前限制
ulimit -a
```

---

<!-- chunk: 安全加固 -->## 安全加固

## 性能监控安全

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# 限制 perf 访问 (防止信息泄露)
sysctl -w kernel.perf_event_paranoid=2

# 限制 eBPF 使用
sysctl -w kernel.unprivileged_bpf_disabled=1

# 审计性能相关系统调用
auditctl -a always,exit -F arch=b64 -S perf_event_open -k perf_access
```

---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

## 容器资源限制与性能

Kubernetes 通过 cgroups 实现容器资源限制，理解底层机制对于排查性能问题至关重要：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看容器的 cgroup 路径
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[0].containerID}'
# 输出: docker://<container_id> 或 containerd://<container_id>

# 在节点上查看容器的 cgroup 统计
# cgroups v2
cat /sys/fs/cgroup/kubepods.slice/.../cpu.stat
cat /sys/fs/cgroup/kubepods.slice/.../memory.current
cat /sys/fs/cgroup/kubepods.slice/.../memory.max
cat /sys/fs/cgroup/kubepods.slice/.../cpu.max

# 查看 CPU throttling（限流）
cat /sys/fs/cgroup/.../cpu.stat | grep nr_throttled
cat /sys/fs/cgroup/.../cpu.stat | grep throttled_time

# Prometheus 查询 CPU throttling
# rate(container_cpu_cfs_throttled_periods_total[5m])
# / rate(container_cpu_cfs_periods_total[5m])
```
## Kubernetes 性能监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubectl top 命令
kubectl top nodes                  # 节点资源使用
kubectl top pods                   # Pod 资源使用
kubectl top pods -n <ns> --sort-by=memory  # 按内存排序

# 查看资源请求和限制
kubectl get pods -o custom-columns=\
'NAME:.metadata.name,CPU_REQ:.spec.containers[*].resources.requests.cpu,\
CPU_LIM:.spec.containers[*].resources.limits.cpu,\
MEM_REQ:.spec.containers[*].resources.requests.memory,\
MEM_LIM:.spec.containers[*].resources.limits.memory'

# 查看 ResourcePressure 条件
kubectl describe node <node> | grep -A5 "Pressure"
```
---

<!-- chunk: 最佳实践 -->## 最佳实践

1. **设置合理的资源请求和限制**: CPU requests 影响调度权重，limits 控制最大使用量
2. **监控 CPU throttling**: 如果容器频繁被限流，说明 CPU limit 设置过低
3. **使用 QoS Guaranteed**: 关键工作负载应设置 requests == limits
4. **关闭透明大页**: 数据库类应用建议关闭 THP
5. **使用 local PV**: 对 I/O 敏感的工作负载使用本地存储
6. **分离 etcd 磁盘**: etcd 对磁盘延迟极度敏感，建议使用专用 SSD

---

<!-- chunk: 故障排查 -->## 故障排查

## 性能故障诊断流程

```bash
#!/bin/bash
# 性能快速诊断脚本

echo "=== 系统性能快照 $(date) ==="
echo "主机: $(hostname) 内核: $(uname -r)"

echo -e "\n--- CPU ---"
uptime
mpstat -P ALL 1 1 | tail -n +4
echo "Top CPU 进程:"
ps -eo pid,user,%cpu,comm --sort=-%cpu | head -6

echo -e "\n--- 内存 ---"
free -h
echo "Top 内存进程:"
ps -eo pid,user,%mem,rss,comm --sort=-%mem | head -6
echo "Swap 使用:"
swapon --show

echo -e "\n--- 磁盘 I/O ---"
iostat -xz 1 1 | tail -n +4
echo "磁盘使用:"
df -h --type=ext4 --type=xfs 2>/dev/null

echo -e "\n--- 网络 ---"
ss -s
echo "接口流量:"
cat /proc/net/dev | column -t

echo -e "\n--- 内核错误 ---"
dmesg | grep -i -E "error|oom|hung_task|blocked for" | tail -5
```

---

## eBPF 高级性能分析

eBPF (extended Berkeley Packet Filter) 是现代 Linux 性能分析的革命性技术，允许在内核中安全地运行沙箱程序，无需修改内核源码。Cilium、[[Falco|Falco]] 等云原生工具都基于 eBPF 构建。

```bash
# BCC 工具集安装
# RHEL/CentOS
yum install -y bcc-tools python3-bcc
# Ubuntu
apt install -y bpfcc-tools linux-headers-$(uname -r)

# 常用 BCC 性能分析工具
# CPU 分析
execsnoop-bpfcc                     # 跟踪新进程创建
offcputime-bpfcc -p <pid>           # CPU 之外的等待时间
profile-bpfcc -F 99 -p <pid> 30     # CPU profiling

# 内存分析
memleak-bpfcc -p <pid>              # 检测内存泄漏
slabratetop-bpfcc                   # slab 缓存分配速率
oomkill-bpfcc                       # 跟踪 OOM Killer 事件

# I/O 分析
biosnoop-bpfcc                      # 块设备 I/O 延迟
biolatency-bpfcc                    # 块设备 I/O 延迟分布
bitesize-bpfcc                      # I/O 大小分布

# 网络分析
tcpconnect-bpfcc                    # 跟踪 TCP 连接发起
tcpaccept-bpfcc                     # 跟踪 TCP 被动连接
tcplife-bpfcc                       # TCP 连接生命周期
tcpretrans-bpfcc                    # TCP 重传事件
sockstat-bpfcc                      # socket 统计

# bpftrace 单行命令
bpftrace -e 'tracepoint:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }'
bpftrace -e 'profile:hz:99 /pid == <pid>/ { @[ustack] = count(); }'
```

## 容器性能调优场景

| 场景 | 优化方法 | 工具 |
|:---|:---|:---|
| Java 应用 CPU throttling | 增加 CPU limit 或使用 CPU 管理策略 | top, perf, Prometheus |
| 数据库 I/O 延迟高 | 使用 local PV + SSD, 调整 I/O 调度器 | iostat, fio |
| 内存使用持续增长 | 分析内存泄漏, 调整 JVM 堆参数 | pmap, jemalloc, async-profiler |
| 网络延迟高 | 调整 TCP 参数, 检查 conntrack | ss, tcpdump, bpftrace |
| 大量短连接 | 调整 TIME_WAIT 参数, 使用连接池 | ss -s, netstat -s |

## 性能调优决策树

```
性能问题报告
    │
    ├── CPU 瓶颈?
    │   ├── %us 高 → 应用优化, perf 分析热点
    │   ├── %sy 高 → 系统调用过多, strace 分析
    │   ├── %wa 高 → I/O 瓶颈 (转 I/O 分析)
    │   └── %st 高 → 虚拟化偷取, 联系云厂商
    │
    ├── 内存瓶颈?
    │   ├── 物理内存不足 → 增加内存, 调整 swappiness
    │   ├── swap 使用高 → 增加物理内存, 调整 vm.swappiness
    │   ├── 页缓存不足 → 调整 dirty_ratio
    │   └── slab 使用高 → 分析 slabtop, 调整 vfs_cache_pressure
    │
    ├── I/O 瓶颈?
    │   ├── %util 高 → 增加 IOPS, 使用 SSD
    │   ├── await 高 → 优化调度器, 检查 RAID
    │   └── 队列深 → 增加队列深度, 调整 nr_requests
    │
    └── 网络瓶颈?
        ├── 带宽饱和 → 升级网络, 启用压缩
        ├── 延迟高 → 优化 TCP 参数, 检查路由
        ├── 丢包 → 检查网络设备, 调整缓冲区
        └── 连接数高 → 调整 conntrack, TIME_WAIT 优化
```

---

<!-- chunk: 相关文档 -->## 相关文档

- [01-linux-system-architecture](./01-linux-system-architecture.md) - 系统架构
- [02-linux-process-management](32-发布/package/2026-07-02_18-29/corpus/supporting/domain-17-system-foundation/01-linux/01-linux-process-management.md) - 进程管理
- [04-linux-networking-configuration](03-linux-networking-configuration.md) - 网络配置

---

**维护者**: Allen Galler (allengaller@gmail.com) | **许可证**: MIT

## See Also

- 04-linux-networking-configuration
- 05-linux-storage-management
- 07-linux-security-hardening
- 08-linux-container-fundamentals

## Related

- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->

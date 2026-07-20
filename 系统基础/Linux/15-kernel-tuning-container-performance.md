---
title: "内核调优：容器性能优化的 sysctl、cgroups 与调度器"
description: "Linux 内核参数调优对容器性能的影响，涵盖 sysctl、cgroups v2、调度器、网络栈和文件系统优化"
summary: "系统讲解 Linux 内核调优如何影响容器性能：sysctl 关键参数、cgroups v2 资源控制、CFS/BPF 调度器调优、网络栈优化及文件系统选择对容器工作负载的影响"
category: 系统基础
tags:
- kernel-tuning
- sysctl
- cgroups
- scheduler
- networking
- performance
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "如何调优 Linux 内核提升容器性能"
- "cgroups v2 怎么配置"
- "容器网络性能怎么优化"
trigger_keywords:
- kernel-tuning
- sysctl
- cgroups-v2
- scheduler
- network-optimization
prerequisites:
- linux-fundamentals
- container-fundamentals
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 内核调优：容器性能优化

## 概述

Linux 内核是容器运行的基石。容器本质上是由 namespace（隔离）和 cgroup（资源限制）组合而成的内核特性。内核参数的默认值通常面向通用场景，对于高并发容器工作负载（数万 Pod、高 PPS 网络、密集 I/O）往往不是最优配置。

本文系统讲解影响容器性能的五大内核调优维度：sysctl 参数、cgroups v2 配置、CPU 调度器、网络栈和文件系统。每个维度都会说明参数含义、推荐值、以及对容器工作负载的具体影响。

## 核心概念

### 内核与容器的关系

```
用户态：
  kubelet → containerd → runc/crun → 容器进程

内核态：
  ├── Namespace（隔离）：PID, NET, MNT, UTS, IPC, USER, CGROUP
  ├── Cgroup v2（资源控制）：cpu, memory, io, pids, cpuset
  ├── 调度器（CPU 分配）：CFS / EEVDF / sched_ext
  ├── 网络栈（容器通信）：veth, bridge, iptables/nftables, eBPF
  └── 文件系统（存储）：overlayfs, xfs, ext4, btrfs
```

### 调优层次

| 层次 | 工具 | 影响范围 | 风险 |
|------|------|---------|------|
| sysctl | /etc/sysctl.d/ | 全节点 | 中（可回滚） |
| cgroup v2 | cgroupfs / systemd | 容器级 | 低 |
| 调度器 | sched_debug / BPF | 全节点 | 高 |
| 网络栈 | ethtool / tc / eBPF | 全节点 | 中-高 |
| 文件系统 | mount options / mkfs | 磁盘级 | 高（需格式化） |

## 生产部署

### sysctl 关键参数

```bash
# 🟡 中风险：sysctl 内核参数调优
# /etc/sysctl.d/99-k8s-container-optimization.conf

# ===== 网络优化 =====
# 连接队列（高并发服务必须调大）
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_max_syn_backlog = 65535

# TCP 连接复用（减少 TIME_WAIT）
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 15
net.ipv4.tcp_max_tw_buckets = 1048576

# TCP 缓冲区（高带宽场景）
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216

# conntrack（K8s Service 依赖）
net.netfilter.nf_conntrack_max = 1048576
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 30

# IP 转发（容器网络必须）
net.ipv4.ip_forward = 1
net.bridge.bridge-nf-call-iptables = 1
net.bridge.bridge-nf-call-ip6tables = 1

# ===== 内存优化 =====
# 减少 swap 使用（容器场景推荐）
vm.swappiness = 10

# 脏页回写（高 I/O 场景）
vm.dirty_ratio = 40
vm.dirty_background_ratio = 10
vm.dirty_expire_centisecs = 3000
vm.dirty_writeback_centisecs = 500

# 内存过量提交（容器 limits 已控制）
vm.overcommit_memory = 1

# OOM 行为
vm.panic_on_oom = 0
vm.oom_kill_allocating_task = 1

# ===== 文件系统 =====
# 文件描述符（高并发必须）
fs.file-max = 2097152
fs.nr_open = 2097152

# inotify（大量 Pod 的节点）
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192

# ===== 调度器 =====
# CFS 调度粒度（减少上下文切换）
kernel.sched_min_granularity_ns = 3000000
kernel.sched_wakeup_granularity_ns = 4000000
kernel.sched_migration_cost_ns = 5000000

# 应用配置
sudo sysctl --system

# 验证
sysctl -a | grep -E "somaxconn|conntrack_max|swappiness|file-max"
```

### cgroups v2 配置

```bash
# 🟡 中风险：cgroups v2 配置
# 确认 cgroup v2 已启用
stat -fc %T /sys/fs/cgroup/
# 输出：cgroup2fs 表示 v2 已启用

# 查看 cgroup v2 控制器
cat /sys/fs/cgroup/cgroup.controllers
# 应包含：cpuset cpu io memory pids

# kubelet 配置使用 cgroup v2
# /var/lib/kubelet/config.yaml
cat <<'EOF' > /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cgroupDriver: systemd
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
# 启用 cgroup v2 资源管理
cgroupsPerQOS: true
enforceNodeAllocatable:
- pods
- system-reserved
systemReserved:
  cpu: "1"
  memory: "2Gi"
  ephemeral-storage: "10Gi"
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
  ephemeral-storage: "5Gi"
EOF

# 查看容器的 cgroup 限制
# 找到容器 cgroup 路径
CONTAINER_ID=$(crictl ps -q | head -1)
CGROUP_PATH=$(crictl inspect $CONTAINER_ID | jq -r '.info.runtimeSpec.linux.cgroupsPath')
echo "Cgroup path: $CGROUP_PATH"

# 查看 CPU 限制
cat /sys/fs/cgroup/$CGROUP_PATH/cpu.max
# 输出格式：$MAX $PERIOD（如 200000 100000 = 2 CPU）

# 查看内存限制
cat /sys/fs/cgroup/$CGROUP_PATH/memory.max
cat /sys/fs/cgroup/$CGROUP_PATH/memory.current

# 查看 IO 限制
cat /sys/fs/cgroup/$CGROUP_PATH/io.max
```

### CPU 调度器调优

```bash
# 🟡 中风险：调度器调优
# 查看当前调度器
cat /sys/kernel/debug/sched/features
# 5.15+: CFS; 6.6+: EEVDF

# 查看 CPU 拓扑
lscpu | grep -E "Socket|Core|Thread|NUMA"

# 设置 CPU 亲和性（高性能 Pod）
# 通过 kubelet CPU Manager（静态分配）
cat <<'EOF' >> /var/lib/kubelet/config.yaml
cpuManagerPolicy: static
cpuManagerPolicyOptions:
  full-pcpus-only: "true"
topologyManagerPolicy: single-numa-node
topologyManagerScope: pod
reservedSystemCPUs: "0-3"  # 保留前 4 核给系统
EOF

# 验证 CPU 分配
kubectl get pod <guaranteed-pod> -o jsonpath='{.status.cpuManagerPolicy}'

# 查看进程调度信息
cat /proc/<pid>/sched
# 关注：nr_switches（上下文切换次数）、wait_sum（等待时间）
```

### 网络栈优化

```bash
# 🟡 中风险：网络栈调优
# 网卡多队列（高 PPS 场景）
# 查看当前队列数
ethtool -l eth0
# 设置为 CPU 核心数（最大不超过网卡支持）
sudo ethtool -L eth0 combined 16

# 开启 GRO/GSO（减少 CPU 中断）
sudo ethtool -K eth0 gro on gso on tso on

# 中断亲和性（避免所有中断集中在 CPU0）
# 使用 irqbalance 或手动设置
sudo systemctl enable --now irqbalance

# 或手动绑定（高性能场景）
for irq in $(grep eth0 /proc/interrupts | awk -F: '{print $1}'); do
  cpu=$((irq % $(nproc)))
  echo $cpu > /proc/irq/$irq/smp_affinity_list
done

# BPF 替代 iptables（Cilium kube-proxy replacement）
# 减少 conntrack 和 iptables 规则遍历开销
# 参考 Cilium 部署文档启用 kube-proxy-replacement

# 查看网络性能
ethtool -S eth0 | grep -E "rx_packets|tx_packets|rx_dropped|tx_dropped"
```

### 文件系统选择与优化

```bash
# 🔴 高风险：文件系统配置（需要格式化磁盘）
# 容器存储推荐：XFS（overlayfs 后端）或 ext4

# XFS 优化（推荐用于容器存储）
# 格式化
sudo mkfs.xfs -f -n ftype=1 /dev/nvme1n1

# 挂载选项
# /etc/fstab
# /dev/nvme1n1 /var/lib/containerd xfs defaults,noatime,nodiratime,logbufs=8,logbsize=256k,allocsize=1g 0 0

sudo mount -o remount,noatime,nodiratime /var/lib/containerd

# overlayfs 优化（containerd snapshotter）
# /etc/containerd/config.toml
# [plugins."io.containerd.snapshotter.v1.overlayfs"]
#   mount_options = ["index=off", "metacopy=on"]

# 查看当前文件系统
df -Th /var/lib/containerd
mount | grep containerd

# I/O 调度器（NVMe 使用 none/mq-deadline）
cat /sys/block/nvme1n1/queue/scheduler
echo "none" | sudo tee /sys/block/nvme1n1/queue/scheduler

# 预读优化
sudo blockdev --setra 256 /dev/nvme1n1  # NVMe 不需要大预读
sudo blockdev --setra 2048 /dev/sda     # HDD 需要大预读
```

## 运维操作

### 性能基线采集

```bash
# 🟢 低风险：性能基线采集
# CPU 性能
sysbench cpu --threads=$(nproc) run
# 记录 events per second

# 内存带宽
mbw -n 10 1024
# 记录 MB/s

# 磁盘 I/O
fio --name=randwrite --ioengine=libaio --direct=1 --bs=4k \
  --iodepth=64 --size=1G --numjobs=4 --runtime=30 \
  --directory=/var/lib/containerd --group_reporting

# 网络吞吐
iperf3 -s  # 服务端
iperf3 -c <server-ip> -t 30 -P 4  # 客户端

# 容器启动延迟
time crictl runp pod-config.json
time crictl create <pod-id> container-config.json pod-config.json
```

### 运行时监控

```bash
# 🟢 低风险：内核性能监控
# 上下文切换率
vmstat 1 5
# 关注 cs 列（context switches）

# 中断率
watch -n 1 'cat /proc/interrupts | head -3'

# 软中断（网络处理）
watch -n 1 'cat /proc/softirqs | grep NET'

# cgroup 压力（PSI - Pressure Stall Information）
cat /sys/fs/cgroup/cpu.pressure
cat /sys/fs/cgroup/memory.pressure
cat /sys/fs/cgroup/io.pressure
# some avg10=0.00 avg60=0.00 avg300=0.00 total=0
# 如果 avg10 > 10，表示资源压力大

# 容器级资源使用
kubectl top pods -A --sort-by=cpu | head -20
```

### 调优效果验证

```bash
# 🟢 低风险：验证调优效果
# 对比调优前后的 Pod 网络延迟
kubectl exec -it <pod> -- ping -c 100 <target-pod-ip> | tail -1
# 关注 avg 和 mdev

# 对比调优前后的 HTTP 延迟
kubectl exec -it <pod> -- wrk -t4 -c100 -d30s http://target-service:8080/

# 检查是否有 conntrack 表溢出
dmesg | grep "nf_conntrack: table full"
# 如果有，增大 nf_conntrack_max

# 检查是否有 TCP 重传
netstat -s | grep -i retrans
```

## 故障排查

### 常见内核性能问题

```bash
# 🟢 低风险：内核性能问题诊断
# 问题 1：Pod 网络延迟突增
# 检查 conntrack 表使用率
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max
# 如果 count/max > 80%，需要扩容或减少连接数

# 问题 2：容器 OOM Kill
dmesg | grep -i "oom\|killed process"
# 检查 cgroup 内存限制
cat /sys/fs/cgroup/<cgroup-path>/memory.max
cat /sys/fs/cgroup/<cgroup-path>/memory.peak

# 问题 3：CPU 节流（throttling）
cat /sys/fs/cgroup/<cgroup-path>/cpu.stat
# 关注 nr_throttled 和 throttled_time
# 如果 nr_throttled 持续增长，说明 CPU limit 过低

# 问题 4：磁盘 I/O 等待高
iostat -x 1 5
# 关注 %util 和 await
# 如果 %util > 90%，磁盘成为瓶颈

# 问题 5：大量 TIME_WAIT
ss -s
# 如果 TIME_WAIT > 100000，调整 tcp_tw_reuse 和 tcp_fin_timeout
```

## 最佳实践

### 调优原则

1. **先测量后调优**：使用 perf/bpftrace 定位瓶颈，不要盲目调参
2. **一次改一个参数**：每次只调整一个维度，观察效果后再调下一个
3. **记录基线**：调优前记录性能基线，调优后对比验证
4. **节点池差异化**：不同工作负载节点池使用不同调优配置
5. **内核版本**：生产环境使用 5.15+（cgroup v2 完善）或 6.1+（EEVDF 调度器）

### 容器特定建议

1. **CPU limit 不要过紧**：CFS 节流对延迟敏感服务影响大，建议 request = limit 或 limit = 2×request
2. **内存 limit 留余量**：Go/Java 运行时有额外内存开销，limit 应为实际使用的 1.3-1.5 倍
3. **使用 Guaranteed QoS**：延迟敏感服务设置 request = limit，获得 CPU 独占和 OOM 优先级
4. **NUMA 感知**：多 NUMA 节点服务器启用 topologyManager，避免跨 NUMA 内存访问
5. **与 [[容器运行时/containerd-CRI-O/01-containerd-production-operations|containerd]] 配合**：snapshotter 和 shim 配置也影响性能
6. **参考 [[系统基础/Linux/06-linux-performance-tuning|Linux 性能调优]] 了解更多通用调优**

## Related

- [[系统基础/Linux/06-linux-performance-tuning|Linux 性能调优]]
- [[系统基础/Linux/14-arm-architecture-k8s-optimization|ARM 架构优化]]
- [[系统基础/Linux/08-linux-container-fundamentals|Linux 容器基础]]
- [[容器运行时/containerd-CRI-O/01-containerd-production-operations|containerd 生产运维]]
- [[可观测性/ebpf-observability|eBPF 可观测性]]
- [[集群基础/节点管理|节点管理]]

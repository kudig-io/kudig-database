---
title: Linux Kernel Fundamentals for Kubernetes — Namespaces, Cgroups, eBPF
description: K8s 系统基础 — Linux 内核命名空间、Cgroups v2、eBPF、文件系统、网络栈、调度器与容器运行时关系
summary: 深入理解 Kubernetes 底层依赖的 Linux 内核机制，掌握容器隔离、资源控制与性能调优的内核原理
category: reference
tags:
- linux-kernel
- namespaces
- cgroups
- ebpf
- container-runtime
- scheduling
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: system-fundamentals
---
# Linux 内核基础 — Kubernetes 底层机制

> 理解 K8s 容器隔离、资源管理与性能调优的内核原理。

## 内核架构与容器关系

```
┌─────────────────────────────────────────────────────────────┐
│  用户空间                                                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ kubelet  │  │containerd│  │  runc    │  │ 应用进程  │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
│───────┼──────────────┼──────────────┼──────────────┼────────│
│  系统调用层 (syscall)                                       │
│───────┼──────────────┼──────────────┼──────────────┼────────│
│  内核空间                                                    │
│  ┌────┴──────────────┴──────────────┴──────────────┴────┐   │
│  │  Namespaces (隔离)                                    │   │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐  │   │
│  │  │ PID │ │ NET │ │ MNT │ │ UTS │ │ IPC │ │USER │  │   │
│  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘  │   │
│  ├───────────────────────────────────────────────────────┤   │
│  │  Cgroups v2 (资源控制)                                │   │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐          │   │
│  │  │ CPU │ │ MEM │ │ IO  │ │ PID │ │ NET │          │   │
│  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘          │   │
│  ├───────────────────────────────────────────────────────┤   │
│  │  eBPF (可编程内核)                                    │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐            │   │
│  │  │ 网络过滤 │ │ 安全追踪 │ │ 性能监控 │            │   │
│  │  └──────────┘ └──────────┘ └──────────┘            │   │
│  └───────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## Linux Namespaces（命名空间）

### 命名空间类型与容器映射

| Namespace | 隔离内容 | K8s 用途 | 内核版本 |
|-----------|----------|----------|----------|
| PID | 进程 ID | 容器内 PID 1 | 2.6.24 |
| NET | 网络栈 | Pod 独立网络 | 2.6.29 |
| MNT | 挂载点 | 容器文件系统 | 2.4.19 |
| UTS | 主机名 | Pod hostname | 2.6.19 |
| IPC | 进程间通信 | 共享内存隔离 | 2.6.19 |
| USER | UID/GID | 安全映射 | 3.8 |
| CGROUP | Cgroup 根 | 容器内不可见宿主 cgroup | 4.6 |
| TIME | 系统时钟 | 容器独立时间（实验） | 5.6 |

### 查看容器命名空间

```bash
# 查看容器进程的命名空间
PID=$(crictl inspect <container-id> | jq .info.pid)
ls -la /proc/$PID/ns/
# 输出示例:
# cgroup -> 'cgroup:[4026531835]'
# ipc -> 'ipc:[4026532456]'
# mnt -> 'mnt:[4026532454]'
# net -> 'net:[4026532457]'
# pid -> 'pid:[4026532455]'
# user -> 'user:[4026531837]'
# uts -> 'uts:[4026532453]'

# 进入容器命名空间（调试）
nsenter -t $PID -m -u -i -n -p -- /bin/sh

# 对比两个容器的网络命名空间
nsenter -t $PID1 -n ip addr show
nsenter -t $PID2 -n ip addr show
```

### Pod 内命名空间共享

```yaml
# Pod 内容器共享 NET/IPC/UTS 命名空间
apiVersion: v1
kind: Pod
metadata:
  name: shared-ns-example
spec:
  shareProcessNamespace: true  # 共享 PID namespace
  containers:
    - name: app
      image: myapp:v1
    - name: sidecar
      image: debug-tools:v1
      # sidecar 可以看到 app 进程（共享 PID ns）
```

## Cgroups v2（控制组）

### Cgroup 层级与 K8s 资源管理

```
/sys/fs/cgroup/
├── kubepods.slice/
│   ├── kubepods-burstable.slice/
│   │   ├── kubepods-burstable-pod<uid>.slice/
│   │   │   ├── cri-containerd-<id>.scope/  ← 容器
│   │   │   │   ├── cpu.max          # CPU 限制
│   │   │   │   ├── cpu.weight       # CPU 权重
│   │   │   │   ├── memory.max       # 内存上限
│   │   │   │   ├── memory.high      # 内存软限
│   │   │   │   ├── io.max           # IO 限制
│   │   │   │   └── pids.max         # 进程数限制
│   │   │   └── ...
│   │   └── ...
│   ├── kubepods-besteffort.slice/
│   │   └── ...
│   └── kubepods-guaranteed.slice/  ← 不存在（Guaranteed 直接在 kubepods.slice）
└── system.slice/
```

### QoS 与 Cgroup 映射

| QoS 等级 | 条件 | Cgroup 路径 | OOM Score |
|-----------|------|-------------|-----------|
| Guaranteed | requests == limits（所有容器） | kubepods.slice/pod<uid> | -997 |
| Burstable | 至少一个容器有 requests | kubepods-burstable.slice/ | 2~999 |
| BestEffort | 无任何 requests/limits | kubepods-besteffort.slice/ | 1000 |

### 资源控制参数

```bash
# CPU 限制（cpu.max: quota period）
# 1 CPU = 100000 100000
# 0.5 CPU = 50000 100000
cat /sys/fs/cgroup/kubepods.slice/pod<uid>/cpu.max
# 输出: 50000 100000

# 内存限制
cat /sys/fs/cgroup/kubepods.slice/pod<uid>/memory.max
# 输出: 536870912 (512Mi)

# 内存使用详情
cat /sys/fs/cgroup/kubepods.slice/pod<uid>/memory.stat
# anon 268435456      ← 匿名页（堆/栈）
# file 134217728      ← 文件页（页缓存）
# sock 8388608        ← Socket 缓冲

# IO 限制
cat /sys/fs/cgroup/kubepods.slice/pod<uid>/io.max
# 253:0 rbps=104857600 wbps=52428800 riops=1000 wiops=500
```

### K8s 资源请求到 Cgroup 的转换

```yaml
# Pod 定义
resources:
  requests:
    cpu: "500m"      # → cpu.weight = 501 (换算)
    memory: "256Mi"  # → memory.low = 268435456 (软限)
  limits:
    cpu: "1000m"     # → cpu.max = 100000 100000
    memory: "512Mi"  # → memory.max = 536870912 (硬限)
```

## eBPF（扩展伯克利包过滤器）

### eBPF 在 K8s 生态中的应用

| 项目 | 用途 | 挂载点 |
|------|------|--------|
| Cilium | CNI 网络/策略 | XDP/TC/socket |
| Falco | 运行时安全检测 | tracepoint/kprobe |
| Tetragon | 安全可观测+强制 | tracepoint |
| Pixie | 全链路可观测 | kprobe/uprobe/TC |
| Parca | 持续 Profiling | perf_event |
| bpftrace | 动态追踪 | 任意挂载点 |
| KubeArmor | 运行时安全策略 | LSM |

### eBPF 程序类型

```c
// XDP 程序（最早处理网络包，线速）
SEC("xdp")
int xdp_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;
    // 过滤逻辑...
    return XDP_PASS;
}

// TC 程序（流量控制，更灵活）
SEC("tc")
int tc_ingress(struct __sk_buff *skb) {
    // 网络策略执行
    return TC_ACT_OK;
}

// Tracepoint（内核事件追踪）
SEC("tracepoint/syscalls/sys_enter_execve")
int trace_execve(struct trace_event_raw_sys_enter *ctx) {
    // 记录进程执行（Falco/Tetragon 用）
    char comm[TASK_COMM_LEN];
    bpf_get_current_comm(&comm, sizeof(comm));
    // 发送到用户空间...
    return 0;
}

// Socket 程序（连接级控制）
SEC("cgroup/connect4")
int connect_filter(struct bpf_sock_addr *ctx) {
    // 网络策略：允许/拒绝连接
    if (ctx->user_port == bpf_htons(6443))
        return 1;  // 允许
    return 0;  // 拒绝
}
```

### bpftrace 常用诊断命令

```bash
# 追踪容器内系统调用
bpftrace -e 'tracepoint:raw_syscalls:sys_enter /pid == '$PID'/ { @[comm] = count(); }'

# 追踪 DNS 查询延迟
bpftrace -e '
kprobe:udp_sendmsg { @start[tid] = nsecs; }
kretprobe:udp_sendmsg /@start[tid]/ {
  @dns_latency = hist(nsecs - @start[tid]);
  delete(@start[tid]);
}'

# 追踪 TCP 重传
bpftrace -e 'tracepoint:tcp:tcp_retransmit_skb { @[comm] = count(); }'

# 追踪文件打开（安全审计）
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'

# 追踪调度延迟
bpftrace -e '
tracepoint:sched:sched_wakeup { @qtime[args->pid] = nsecs; }
tracepoint:sched:sched_switch /@qtime[args->next_pid]/ {
  @sched_lat = hist(nsecs - @qtime[args->next_pid]);
  delete(@qtime[args->next_pid]);
}'
```

## 内核网络栈与 K8s 网络

### 数据包路径（Pod 到 Pod）

```
Pod A (eth0)
    │
    ▼
veth pair (宿主机端)
    │
    ▼
Linux Bridge (cbr0) 或 eBPF (Cilium)
    │
    ├── 同节点 → veth pair → Pod B (eth0)
    │
    └── 跨节点 → 路由/隧道
         ├── VXLAN (Flannel)
         ├── WireGuard (加密)
         ├── eBPF (Cilium, 无隧道)
         └── IPIP (Calico)
              │
              ▼
         物理网卡 (eth0)
```

### 关键内核参数调优

```bash
# /etc/sysctl.d/99-kubernetes.conf

# 连接跟踪（Service/iptables 依赖）
net.netfilter.nf_conntrack_max = 1048576
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 120

# TCP 调优
net.ipv4.tcp_max_syn_backlog = 65535
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 15

# 文件描述符
fs.file-max = 2097152
fs.nr_open = 2097152

# inotify（kubelet 监控）
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192

# 网络命名空间
net.core.optmem_max = 65536

# ARP（大集群）
net.ipv4.neigh.default.gc_thresh1 = 4096
net.ipv4.neigh.default.gc_thresh2 = 8192
net.ipv4.neigh.default.gc_thresh3 = 16384
```

## 内核调度器与 CPU 管理

### CFS 调度器与 K8s CPU 管理

```bash
# 查看 CPU 分配
cat /sys/fs/cgroup/kubepods.slice/pod<uid>/cpuset.cpus.effective
# 输出: 2-3 (Guaranteed Pod 绑定 CPU 2,3)

# 查看调度统计
cat /sys/fs/cgroup/kubepods.slice/pod<uid>/cpu.stat
# usage_usec 123456789
# user_usec  100000000
# system_usec 23456789
# nr_periods 10000        ← 调度周期数
# nr_throttled 50         ← 被限流次数
# throttled_usec 500000   ← 限流时间
```

### CPU Manager 策略

```yaml
# kubelet 配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cpuManagerPolicy: static        # 静态 CPU 绑定
cpuManagerPolicyOptions:
  full-pcpus-only: "true"      # 只分配完整 CPU
topologyManagerPolicy: single-numa-node  # NUMA 感知
memoryManagerPolicy: Static     # 大页内存绑定
reservedSystemCPUs: "0,1"      # 系统保留 CPU
```

## 文件系统与存储

### OverlayFS（容器文件系统）

```bash
# 查看容器文件系统层
mount | grep overlay
# overlay on /var/lib/containerd/.../merged type overlay (
#   rw,relatime,
#   lowerdir=/var/lib/containerd/.../diff/layer1:/var/lib/containerd/.../diff/layer2,
#   upperdir=/var/lib/containerd/.../work/upper,
#   workdir=/var/lib/containerd/.../work/work)

# 层结构:
# lowerdir = 只读镜像层（多层叠加）
# upperdir = 容器可写层
# workdir  = OverlayFS 工作目录
# merged   = 容器看到的合并视图
```

### 内核 IO 调度器

```bash
# 查看当前 IO 调度器
cat /sys/block/nvme0n1/queue/scheduler
# none [mq-deadline] kyber bfq

# K8s 推荐（NVMe/SSD）
echo "none" > /sys/block/nvme0n1/queue/scheduler

# 查看 IO 统计
iostat -xz 1
# Device  r/s   w/s  rkB/s  wkB/s  await  %util
# nvme0n1 5000  3000 204800 102400 0.5    45%
```

## 内核版本与 K8s 兼容性

| 内核版本 | 关键特性 | 推荐 K8s 版本 |
|----------|----------|---------------|
| 5.4 LTS | eBPF CO-RE, Cgroup v2 基础 | 1.25+ |
| 5.10 LTS | BPF LSM, BTF | 1.26+ |
| 5.15 LTS | eBPF 性能优化, Landlock | 1.27+ |
| 6.1 LTS | BPF 内存优化, io_uring 成熟 | 1.28+ |
| 6.6 LTS | BPF Token, 调度器改进 | 1.29+ |
| 6.8+ | sched_ext (可编程调度器) | 1.30+ |

## 故障排查

| 问题 | 诊断命令 | 解决方案 |
|------|----------|----------|
| 容器 OOMKilled | `dmesg | grep -i oom` | 增加 memory limits |
| CPU Throttling | `cat cpu.stat` nr_throttled | 增加 CPU limits 或优化代码 |
| 网络丢包 | `netstat -s | grep drop` | 调整 conntrack/backlog |
| 文件描述符耗尽 | `cat /proc/sys/fs/file-nr` | 增加 fs.file-max |
| inotify 限制 | `dmesg | grep inotify` | 增加 max_user_watches |
| PID 耗尽 | `cat pids.current / pids.max` | 增加 pids.max 或排查泄漏 |
| 磁盘 IO 高 | `iostat -xz 1` | 检查 IO 调度器/换 NVMe |
| 内核死锁 | `dmesg | grep -i "rcu\|hung"` | 升级内核/检查驱动 |

## 最佳实践

| 实践 | 说明 |
|------|------|
| 使用 LTS 内核 | 5.15/6.1/6.6 优先 |
| 启用 Cgroup v2 | 统一层级，K8s 1.25+ 默认 |
| 调优 conntrack | 大集群必须增大 |
| 监控 CPU throttling | nr_throttled > 0 需关注 |
| 使用 eBPF 可观测 | 替代 iptables 性能更好 |
| NUMA 对齐 | 高性能工作负载启用 Topology Manager |
| 定期升级内核 | 安全补丁 + 性能改进 |
| 禁用 swap | K8s 要求（或启用 swap 支持 1.28+） |

## Related

- [[17-系统基础/index.md|系统基础]]
- [[17-系统基础/00-总览/01-production-readiness-operations-guide.md|生产就绪运维指南]]
- [[05-网络/05-eBPF/index.md|eBPF 网络]]
- [[14-容器运行时/03-containerd-CRI-O/index.md|容器运行时]]

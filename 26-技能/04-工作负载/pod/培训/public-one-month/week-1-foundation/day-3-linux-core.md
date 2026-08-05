---
title: 'Day 3: Linux 核心基础'
description: '**学习时间**: 4-5 小时 | **主题**: Linux 系统架构与进程管理'
summary: '**学习时间**: 4-5 小时 | **主题**: Linux 系统架构与进程管理'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- scheduler
- containerd
- docker
- job
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 3: Linux 核心基础 是什么'
- '如何 Day 3: Linux 核心基础'
trigger_keywords:
- Day
- '3:'
- Linux
- 核心基础
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 3: Linux 核心基础

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY3
title: Day 3 - Linux 核心基础
topic: linux
type: hands-on-guide
tags: [linux, namespace, cgroup, process, signal, system-call, container, hands-on, week-1]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Linux namespace 是什么"
  - "cgroup 怎么限制资源"
  - "容器隔离原理是什么"
  - "进程信号怎么用"
trigger_keywords:
  - Linux
  - namespace
  - PID namespace
  - NET namespace
  - MNT namespace
  - cgroup
  - control group
  - CPU限制
  - 内存限制
  - 进程
  - 信号
  - SIGTERM
  - SIGKILL
  - 系统调用
  - 容器原理
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - 系统基础
  - 容器运行时
related_topics:
  - linux
  - container
  - namespace
  - cgroup
related:
  - 生产运维/topic-learn/public-training/one-month/week-1-foundation/day-4-linux-network.md
  - 系统基础/01-linux-system-architecture.md
---

```

> **学习时间**: 4-5 小时 | **主题**: Linux 系统架构与进程管理

---

## 概述

本文是 [[kubernetes|Kubernetes]] 学习路径中 Linux 基础模块的第一部分，聚焦于 Linux 系统架构、进程管理和容器隔离原理（namespace + cgroup）。理解这些概念是掌握 K8s 的前提——K8s 节点运行在 Linux 上，容器的本质就是 Linux 内核提供的隔离能力，而 K8s 的 resources.limits 最终通过 cgroup 实现。本文将帮助你建立从 Linux 内核到容器到 K8s 的完整认知链条。

### 学习目标

- 理解 Linux 系统架构（内核空间/用户空间、系统调用）
- 掌握进程管理工具（ps、top、kill）和信号机制
- 深入理解 namespace 和 cgroup（容器隔离的底层原理）
- 掌握系统资源监控工具（free、df、ss、lsof）
- 理解僵尸进程和孤儿进程的产生原因与处理方法

---

## 核心概念详解

### Linux 系统架构

Linux 操作系统分为两个空间：**内核空间（Kernel Space）** 和 **用户空间（User Space）**。

内核空间运行着 Linux 内核，负责管理硬件资源（CPU、内存、磁盘、网络）并提供抽象接口。内核是唯一可以直接操作硬件的组件。用户空间运行着所有的用户进程（应用程序、Shell、Daemon 等）。用户进程通过**系统调用（System Call）** 请求内核服务。

系统调用是用户空间和内核空间之间的桥梁。常见的系统调用包括：
- `open()` / `read()` / `write()` / `close()`: 文件操作
- `fork()` / `exec()`: 创建和执行进程
- `clone()`: 创建轻量级进程（容器的底层机制）
- `socket()` / `bind()` / `listen()`: 网络操作
- `mount()` / `umount()`: 文件系统挂载

在 K8s 环境中，容器运行时（[[containerd|containerd]]）通过系统调用创建 namespace 和 cgroup 来实现容器隔离。kubelet 通过系统调用管理容器进程。

### 进程管理基础

进程是 Linux 中正在运行的程序实例。每个进程有唯一的 PID（Process ID），由 init 进程（PID 1，通常是 systemd）管理。进程之间有父子关系，形成进程树。

进程的状态包括：
- **Running (R)**: 正在运行或等待 CPU 调度
- **Sleeping (S)**: 可中断的等待状态（等待 I/O、信号等）
- **Uninterruptible Sleep (D)**: 不可中断的等待状态（通常是磁盘 I/O）
- **Stopped (T)**: 被暂停的进程
- **Zombie (Z)**: 已退出但父进程尚未回收的进程

在容器环境中，容器的主进程（PID 1，即 Dockerfile 中 CMD/ENTRYPOINT 指定的命令）特别重要。如果 PID 1 退出，容器就会停止。这也是为什么 Dockerfile 中应该使用前台运行模式（如 `nginx -g "daemon off;"`）而不是后台模式。

### 信号机制

信号是 Linux 中进程间通信的基本方式。每个信号有一个编号和名称：

| 信号 | 编号 | 含义 | K8s 关联 |
|------|------|------|---------|
| SIGHUP | 1 | 挂断（通常用于重新加载配置） | ConfigMap 更新通知 |
| SIGINT | 2 | 中断（Ctrl+C） | 交互式终止 |
| SIGTERM | 15 | 优雅终止 | Pod 终止流程的第一步 |
| SIGKILL | 9 | 强制终止（不可捕获） | Pod 终止超时后的最终步骤 |
| SIGUSR1 | 10 | 用户自定义信号 1 | 应用自定义处理 |
| SIGUSR2 | 12 | 用户自定义信号 2 | 应用自定义处理 |

K8s 的 Pod 终止流程与信号密切相关：当执行 `kubectl delete pod` 时，kubelet 先向容器发送 SIGTERM，等待 `terminationGracePeriodSeconds`（默认 30 秒），如果容器仍未退出，则发送 SIGKILL 强制终止。理解这个流程对于实现优雅关闭至关重要。

### Namespace 详解

Linux namespace 是容器隔离的核心技术。它为进程提供了一种"视图隔离"——让进程看到的是一组独立的系统资源，而不是真实的全局资源。

七种 namespace 类型：

| Namespace | 隔离内容 | 容器用途 | K8s 关联 |
|-----------|---------|---------|---------|
| PID | 进程 ID | 容器内进程从 PID 1 开始 | Pod 内的进程隔离 |
| NET | 网络栈 | 独立的 IP、端口、路由 | Pod 网络隔离（CNI） |
| IPC | 进程间通信 | 信号量、消息队列隔离 | Pod 内容器通信 |
| MNT | 文件系统挂载 | 独立的文件系统视图 | Volume 挂载 |
| UTS | 主机名和域名 | 容器可以有独立 hostname | Pod 的 hostname |
| USER | 用户和组 ID | 容器内 root 映射到宿主机非 root | securityContext.runAsUser |
| CGROUP | cgroup 根目录视图 | cgroup 视图隔离 | 容器内看不到宿主 cgroup |

### Cgroup 详解

cgroup（Control Group）是 Linux 内核提供的资源限制机制。它可以将进程分组，并对每个组施加资源限制。

cgroup 支持的子系统（Controller）：

| 子系统 | 限制内容 | K8s 对应 | 说明 |
|--------|---------|---------|------|
| cpu | CPU 使用时间 | resources.limits.cpu | CFS quota/period 机制 |
| memory | 内存使用量 | resources.limits.memory | 超限触发 OOMKilled |
| blkio | 块设备 I/O | 无直接对应 | 限制磁盘读写速率 |
| pids | 进程数量 | 无直接对应 | 防止 fork bomb |
| net_cls | 网络分类 | 无直接对应 | 配合 tc 做网络限速 |
| devices | 设备访问 | 无直接对应 | 控制可访问的设备 |

K8s 中 resources.requests 和 resources.limits 的实现：
- `requests.cpu`: 用于调度决策（Scheduler 选择资源充足的节点），通过 cgroup 的 `cpu.shares` 实现（相对权重）
- `limits.cpu`: 通过 cgroup 的 `cpu.cfs_quota_us` / `cpu.cfs_period_us` 实现绝对限制
- `limits.memory`: 通过 cgroup 的 `memory.limit_in_bytes` 实现，超限触发 OOM Killer

---

## 实战演练

### 任务 1: 进程管理命令练习 (45min)

```bash
# 查看进程树（理解父子关系）
pstree -p
# 预期输出（部分）:
# systemd(1)─┬─sshd(1234)───bash(5678)───pstree(9012)
#            ├─containerd(2345)─┬─containerd-shim(3456)───nginx(4567)
#            │                  └─containerd-shim(4567)───app(5678)
#            ├─kubelet(3456)
#            └─...

# ps 命令详解
ps aux
# 预期输出包含所有进程: USER PID %CPU %MEM VSZ RSS TTY STAT START TIME COMMAND

ps -ef
# 全格式: UID PID PPID C STIME TTY TIME CMD

# 按资源使用排序
ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%cpu | head -10
# 预期输出（按 CPU 使用降序）:
#   PID  PPID CMD                         %MEM %CPU
#  3456     1 /usr/bin/containerd          2.3  5.2
#  4567  3456 nginx: worker process        0.5  2.1
#  ...

ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%mem | head -10
# 预期输出（按内存使用降序）

# top 实时监控
top -c
# 在 top 交互界面中:
# P - 按 CPU 使用排序
# M - 按内存使用排序
# k - 输入 PID 杀死进程
# 1 - 显示每个 CPU 核心的使用率
# q - 退出

# htop（更友好的界面，如果已安装）
htop
# 支持鼠标操作、F9 杀进程、F5 树形视图

# 进程信号
kill -l
# 预期输出: 列出所有 64 个信号

# 优雅终止（SIGTERM）
kill -15 <PID>
# 或
kill <PID>  # 默认发送 SIGTERM

# 强制终止（SIGKILL，不可捕获）
kill -9 <PID>

# 重新加载配置（SIGHUP）
kill -HUP <PID>

# 后台进程管理
sleep 100 &
# [1] 12345  (输出 job 编号和 PID)

jobs
# [1]+  Running                 sleep 100 &

fg %1     # 切换到前台
bg %1     # 切换到后台
disown %1 # 脱离 Shell 管理

# 查看进程打开的文件
lsof -p <PID>
# 预期输出: 列出该进程打开的所有文件描述符
```

### 任务 2: 系统资源监控 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 内存信息
free -h
# 预期输出:
#               total        used        free      shared  buff/cache   available
# Mem:           15Gi       5.0Gi       3.0Gi       200Mi       7.0Gi        10Gi
# Swap:         2.0Gi          0B       2.0Gi

# /proc/meminfo 详细信息
cat /proc/meminfo | head -20
# MemTotal:       16384000 kB
# MemFree:         3072000 kB
# MemAvailable:   10240000 kB
# Buffers:          512000 kB
# Cached:          4096000 kB
# ...

# CPU 信息
lscpu
# Architecture:          x86_64
# CPU(s):                4
# Thread(s) per core:    2
# Core(s) per socket:    2
# Model name:            Intel(R) Xeon(R) CPU E5-2680 v4

cat /proc/cpuinfo | grep "model name" | head -1
# model name      : Intel(R) Xeon(R) CPU E5-2680 v4 @ 2.40GHz

# 磁盘信息
df -h
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/sda1        50G   20G   30G  40% /
# /dev/sdb1       200G   50G  150G  25% /data

du -sh /var/log
# 2.5G    /var/log

du -sh /var/* | sort -rh | head -10
# 按大小排序显示 /var 下各目录

# 系统负载
uptime
#  10:30:00 up 30 days,  5:00,  2 users,  load average: 0.5, 0.3, 0.2
# load average 三个数字分别是 1 分钟、5 分钟、15 分钟的平均负载
# 一般认为负载不应超过 CPU 核心数

cat /proc/loadavg
# 0.50 0.30 0.20 2/500 12345

# 网络连接
ss -tuln
# 监听端口列表:
# LISTEN  0  128  0.0.0.0:22     0.0.0.0:*
# LISTEN  0  128  0.0.0.0:80     0.0.0.0:*
# LISTEN  0  128  0.0.0.0:6443   0.0.0.0:*   (K8s API Server)
# LISTEN  0  128  0.0.0.0:10250  0.0.0.0:*   (kubelet)

ss -tunp
# 当前连接及关联进程:
# ESTAB  0  0  10.0.0.1:54321  10.0.0.2:6443  users:(("kubectl",pid=12345,fd=3))

# 查看占用特定端口的进程
lsof -i :80
# COMMAND   PID   USER   FD   TYPE DEVICE SIZE/OFF NODE NAME
# nginx   12345   root    6u  IPv4  ...      TCP  *:80 (LISTEN)

# 查看进程打开的所有文件
lsof -p <PID>

# 查看目录被哪些进程打开
lsof +D /var/log
```
### 任务 3: Namespace 实验 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看当前 Shell 进程的 namespace
ls -la /proc/$$/ns/
# 预期输出:
# lrwxrwxrwx 1 root root 0 Jan 15 10:00 cgroup -> 'cgroup:[4026531835]'
# lrwxrwxrwx 1 root root 0 Jan 15 10:00 ipc -> 'ipc:[4026531839]'
# lrwxrwxrwx 1 root root 0 Jan 15 10:00 mnt -> 'mnt:[4026531840]'
# lrwxrwxrwx 1 root root 0 Jan 15 10:00 net -> 'net:[4026531992]'
# lrwxrwxrwx 1 root root 0 Jan 15 10:00 pid -> 'pid:[4026531836]'
# lrwxrwxrwx 1 root root 0 Jan 15 10:00 user -> 'user:[4026531837]'
# lrwxrwxrwx 1 root root 0 Jan 15 10:00 uts -> 'uts:[4026531838]'

# 创建新的网络 namespace
sudo ip netns add test-ns
sudo ip netns list
# test-ns

# 在新 namespace 中查看网络（独立的网络栈）
sudo ip netns exec test-ns ip addr
# 1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN
#     inet 127.0.0.1/8 scope host lo

# 在新 namespace 中创建 veth pair 连接宿主机
sudo ip link add veth-host type veth peer name veth-ns
sudo ip link set veth-ns netns test-ns
sudo ip netns exec test-ns ip addr add 10.200.1.2/24 dev veth-ns
sudo ip netns exec test-ns ip link set veth-ns up
sudo ip addr add 10.200.1.1/24 dev veth-host
sudo ip link set veth-host up

# 测试连通性
sudo ip netns exec test-ns ping -c 3 10.200.1.1
# PING 10.200.1.1: 56 data bytes
# 64 bytes from 10.200.1.1: icmp_seq=0 ttl=64 time=0.1 ms
# --- 10.200.1.1 ping statistics ---
# 3 packets transmitted, 3 received, 0% packet loss

# 查看 Docker 容器的 namespace
docker run -d --name test-ns-container alpine sleep 3600
# a1b2c3d4e5f6...

CONTAINER_PID=$(docker inspect -f '{{.State.Pid}}' test-ns-container)
echo "Container PID: ${CONTAINER_PID}"

sudo ls -la /proc/$CONTAINER_PID/ns/
# 对比与宿主机 namespace 的区别（不同的 inode 编号）

# 在容器 namespace 中执行命令
sudo nsenter -t $CONTAINER_PID -n ip addr
# 查看容器的独立网络栈

sudo nsenter -t $CONTAINER_PID -p pstree -p
# 查看容器的进程树（从 PID 1 开始）

# 清理
sudo ip netns delete test-ns
docker rm -f test-ns-container  # ⚠️ 强制清理，可能杀运行中容器
```
### 任务 4: Cgroup 实验 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看 cgroup 挂载点
mount | grep cgroup
# cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec)

# 查看 cgroup 子系统
cat /proc/cgroups
# subsys_name    hierarchy   num_cgroups enabled
# cpuset         0           1           1
# cpu            0           1           1
# cpuacct        0           1           1
# blkio          0           1           1
# memory         0           1           1
# devices        0           1           1
# freezer        0           1           1
# ...

# 启动带资源限制的 Docker 容器
docker run -d --name cg-test --memory=100m --cpus=0.5 alpine sleep 3600
# 注意: --memory=100m 对应 K8s 的 resources.limits.memory: 100Mi
#       --cpus=0.5 对应 K8s 的 resources.limits.cpu: 500m

CONTAINER_ID=$(docker ps -q -f name=cg-test)
echo "Container ID: ${CONTAINER_ID}"

# 查看内存限制（cgroup v2）
cat /sys/fs/cgroup/docker/${CONTAINER_ID}/memory.max 2>/dev/null || \
cat /sys/fs/cgroup/memory/docker/${CONTAINER_ID}/memory.limit_in_bytes 2>/dev/null
# 预期输出: 104857600 (100MB)

# 查看 CPU 限制（cgroup v2）
cat /sys/fs/cgroup/docker/${CONTAINER_ID}/cpu.max 2>/dev/null || \
cat /sys/fs/cgroup/cpu/docker/${CONTAINER_ID}/cpu.cfs_quota_us 2>/dev/null
# cgroup v2: 50000 100000 (50% of 100ms period)
# cgroup v1: 50000 (quota) / 100000 (period) = 50%

# 查看容器实际内存使用
cat /sys/fs/cgroup/docker/${CONTAINER_ID}/memory.current 2>/dev/null || \
cat /sys/fs/cgroup/memory/docker/${CONTAINER_ID}/memory.usage_in_bytes 2>/dev/null

# 验证 CPU 限制生效（在容器内执行 CPU 密集任务）
docker exec cg-test sh -c 'while true; do :; done' &
# 在宿主机 top 中观察该进程的 CPU 使用率约为 50%

# 清理
kill %1 2>/dev/null
docker rm -f cg-test  # ⚠️ 强制清理，可能杀运行中容器
```
### 任务 5: 排障命令练习 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# strace - 跟踪系统调用（理解程序底层行为）
strace -p <PID> -e trace=open,read,write 2>&1 | head -20
# 预期输出: 显示该进程的 open/read/write 系统调用

# strace 查看 Docker 容器启动过程
strace -f -e trace=clone docker run --rm alpine echo hello 2>&1 | grep clone

# lsof - 查看打开的文件
lsof -p <PID>
# 预期输出: 列出进程打开的所有文件描述符

lsof +D /var/log
# 查看哪些进程在使用 /var/log 目录

# 查找高 CPU 进程
ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%cpu | head -5
# 预期输出:
#   PID  PPID CMD                         %MEM %CPU
#  3456     1 /usr/bin/containerd          2.3  5.2
#  4567  3456 nginx: worker process        0.5  2.1

# 查找高内存进程
ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%mem | head -5

# 查看进程的限制
cat /proc/<PID>/limits
# Limit                     Soft Limit           Hard Limit
# Max cpu time              unlimited            unlimited
# Max file size             unlimited            unlimited
# Max open files            65536                65536
# ...

# 查看进程的文件描述符
ls -la /proc/<PID>/fd/
# lrwx------ 1 root root 64 Jan 15 10:00 0 -> /dev/pts/0
# lrwx------ 1 root root 64 Jan 15 10:00 1 -> /dev/pts/0
# lrwx------ 1 root root 64 Jan 15 10:00 2 -> /dev/pts/0

# 查看僵尸进程
ps aux | awk '$8=="Z"'
# 如果有输出，说明存在僵尸进程

# 查看进程的环境变量
cat /proc/<PID>/environ | tr '\0' '\n' | head -20
```
---

## 配置示例

### Docker 资源限制与 K8s resources 对照

```yaml
# Docker 运行命令
# docker run -d \
#   --memory=256m \
#   --memory-reservation=128m \
#   --cpus=0.5 \
#   --pids-limit=100 \
#   nginx:alpine

# 对应的 K8s Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: resource-demo
spec:
  containers:
  - name: nginx
    image: nginx:alpine
    resources:
      requests:
        cpu: 250m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 256Mi
    # 等效 Docker 参数:
    # --cpus=0.5          → limits.cpu: 500m
    # --memory=256m       → limits.memory: 256Mi
    # --memory-reservation=128m → requests.memory: 128Mi
```

---

## 常见问题

### Q1: 什么是僵尸进程？如何产生？如何处理？

僵尸进程是子进程已退出但父进程尚未调用 `wait()` 回收其状态的进程。在 `ps` 中显示为状态 `Z`。僵尸进程不占用 CPU 和内存，但占用 PID 和进程表条目。如果大量僵尸进程积累，会导致无法创建新进程。处理方法：找到父进程并修复其 wait 逻辑，或者杀死父进程使僵尸进程被 init 进程回收。

### Q2: 容器中 PID 1 有什么特殊之处？

PID 1 是容器中第一个启动的进程（主进程），它有两个特殊职责：一是作为容器的生命周期标识（PID 1 退出则容器停止），二是需要回收孤儿进程（其他进程的子进程在父进程退出后会被 PID 1 接管）。如果 PID 1 进程不正确处理信号（如 Shell 脚本作为 PID 1），可能导致容器无法优雅关闭。

### Q3: cgroup v1 和 v2 有什么区别？

cgroup v2 是更新的版本，主要改进：统一的层级结构（v1 中每个控制器有独立的层级）、更好的资源分配一致性、支持线程化控制器。Docker 和 K8s 从较新版本开始默认使用 cgroup v2。在 v2 中，文件路径从 `/sys/fs/cgroup/<controller>/` 变为 `/sys/fs/cgroup/` 统一目录，文件名也有变化（如 `memory.limit_in_bytes` → `memory.max`）。

### Q4: namespace 和虚拟机的隔离有什么区别？

namespace 是进程级别的隔离，共享宿主机内核。虚拟机是硬件级别的隔离，运行独立的操作系统内核。namespace 隔离更轻量（启动快、资源少），但隔离性较弱（共享内核意味着内核漏洞影响所有容器）。虚拟机隔离性更强，但开销更大。K8s 通常在虚拟机之上运行容器，结合两者的优势。

### Q5: 如何查看进程属于哪个 cgroup？

使用 `cat /proc/<PID>/cgroup` 查看。在 cgroup v2 中输出如 `0::/docker/<container-id>`。在 K8s 环境中，kubelet 会为每个 Pod 创建一个 cgroup 目录（如 `/sys/fs/cgroup/kubepods/burstable/pod<pod-id>/`）。

### Q6: K8s 的 OOMKilled 是怎么触发的？

当容器使用的内存超过 `resources.limits.memory` 时，Linux 内核的 OOM Killer 会选择该容器进程并强制终止。在 `kubectl describe pod` 中会显示 `Last State: Terminated, Reason: OOMKilled`。解决方法：调大 limits.memory、优化应用内存使用、排查内存泄漏。

---

## 要点总结

| 概念 | 说明 | K8s 关联 |
|------|------|---------|
| namespace | 资源视图隔离（7种类型） | Pod 网络/PID/UTS 隔离 |
| cgroup | 资源使用限制（CPU/内存/IO） | resources.limits/requests |
| 进程信号 | 进程间通信机制 | Pod 终止流程（SIGTERM→SIGKILL） |
| 系统调用 | 用户空间与内核空间桥梁 | 容器运行时的底层操作 |
| 文件描述符 | 进程打开的文件/套接字 | lsof 排查端口占用问题 |

---

## 延伸阅读

- [Linux 系统架构](../../../../../../17-%E7%B3%BB%E7%BB%9F%E5%9F%BA%E7%A1%80/01-Linux/01-linux-system-architecture.md)
- [Linux 进程管理](../../[[17-系统基础/01-Linux/03-linux-process-management.md|02-linux-process-management]].md)
- [Linux 容器基础原理](../../../../../../17-系统基础/01-Linux/09-linux-container-fundamentals.md)
- [Linux 命令参考](../../../../../../17-系统基础/01-Linux/15-linux-commands-reference.md)

```

<!-- risk-assessed -->

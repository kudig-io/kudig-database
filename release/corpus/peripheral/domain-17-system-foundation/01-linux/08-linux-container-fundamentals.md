---
title: 08 - Linux 容器技术深度解析：生产环境容器运维专家指南
description: '# 08 - Linux 容器技术深度解析：生产环境容器运维专家指南'
summary: '容器技术是 [[Kubernetes|Kubernetes]] 和云原生生态的基石。理解容器的底层原理——Linux Namespaces、Cgroups、OverlayFS、Seccomp、Capabilities——对于排查容器问题、优化容器性能、实施容器安全策略至关重要。本文档从内核原理出发，深入解析每一项容器核心技术的实现机制，'
category: linux
tags:
- linux
- system
- kernel
- kubelet
- helm
- containerd
- cri-o
- docker
- falco
- ebpf
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
- Linux 容器技术深度解析：生产环境容器运维专家指南 是什么
- 如何 Linux 容器技术深度解析：生产环境容器运维专家指南
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 容器技术深度解析：生产环境容器运维专家指南
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- ebpf-basics
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



# 08 - Linux 容器技术深度解析：生产环境容器运维专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 概述 -->## 概述

容器技术是 [[Kubernetes|Kubernetes]] 和云原生生态的基石。理解容器的底层原理——Linux Namespaces、Cgroups、OverlayFS、Seccomp、Capabilities——对于排查容器问题、优化容器性能、实施容器安全策略至关重要。本文档从内核原理出发，深入解析每一项容器核心技术的实现机制，并通过手动创建容器的实践帮助读者建立直观理解。同时，本文档详细阐述了这些技术与 Kubernetes Pod 模型、容器运行时接口（CRI）、安全策略之间的关联，为构建企业级容器平台提供扎实的技术基础。

---

<!-- chunk: 核心概念详解 -->## 核心概念详解

## 容器 vs 虚拟机

```
┌─────────────────────────────────────────────────────────────────┐
│                     容器 vs 虚拟机架构对比                        │
│                                                                  │
│  虚拟机 (VM)                         容器 (Container)            │
│  ┌─────────────────────────┐        ┌─────────────────────────┐ │
│  │ App A  │ App B  │ App C │        │ App A  │ App B  │ App C │ │
│  ├─────────┼─────────┼───────┤        ├─────────┼─────────┼───────┤ │
│  │Bins/Libs│Bins/Libs│Bins/L│        │Bins/Libs│Bins/Libs│Bins/L│ │
│  ├─────────┼─────────┼───────┤        └────┬────┴────┬────┴──┬──┘ │
│  │  Guest  │  Guest  │ Guest │             │         │       │    │ │
│  │   OS    │   OS    │  OS   │        ┌────┴─────────┴───────┴──┐ │
│  ├─────────┴─────────┴───────┤        │    容器运行时             │ │
│  │       Hypervisor          │        │ (containerd/CRI-O)       │ │
│  ├───────────────────────────┤        ├──────────────────────────┤ │
│  │       Host OS             │        │    Host OS (Linux)       │ │
│  ├───────────────────────────┤        ├──────────────────────────┤ │
│  │       Hardware            │        │    Hardware              │ │
│  └───────────────────────────┘        └──────────────────────────┘ │
│                                                                  │
│  特点:                              特点:                        │
│  - 硬件级隔离                        - 进程级隔离                 │
│  - 独立内核                          - 共享宿主内核               │
│  - 启动慢 (分钟)                     - 启动快 (毫秒)              │
│  - 资源开销大                        - 资源开销小                 │
│  - 安全性高                          - 安全性中等                 │
│  - 适用于强隔离场景                   - 适用于微服务/云原生        │
└─────────────────────────────────────────────────────────────────┘
```

## 容器核心技术栈

```
┌─────────────────────────────────────────────────────────────────┐
│                     容器核心技术栈                                │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Namespaces (命名空间)                                     │  │
│  │  提供: PID/Network/Mount/UTS/IPC/User/Cgroup 隔离         │  │
│  │  实现: 每个容器有独立的进程树、网络栈、文件系统             │  │
│  └──────────────────────────────────────────────────────────┘  │
│                           +                                      │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Cgroups (控制组)                                          │  │
│  │  提供: CPU/Memory/IO/PIDs 资源限制和统计                   │  │
│  │  实现: 限制容器可使用的系统资源量                           │  │
│  └──────────────────────────────────────────────────────────┘  │
│                           +                                      │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  OverlayFS (联合文件系统)                                  │  │
│  │  提供: 分层文件系统，镜像层叠加                            │  │
│  │  实现: 镜像分层存储，容器写时复制                           │  │
│  └──────────────────────────────────────────────────────────┘  │
│                           +                                      │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  安全特性                                                  │  │
│  │  Capabilities: 权限细分                                    │  │
│  │  Seccomp: 系统调用过滤                                     │  │
│  │  SELinux/AppArmor: 强制访问控制                            │  │
│  │  User Namespaces: 用户映射                                 │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Namespaces 详解

Namespaces 是 Linux 内核提供的资源隔离机制，容器运行时通过创建不同的命名空间来实现进程的隔离视图。

## Namespace 类型详解

| 类型 | Flag | 隔离内容 | 内核版本 | 容器用途 |
|:---|:---|:---|:---|:---|
| **PID** | CLONE_NEWPID | 进程 ID 空间 | 2.6.24 | 容器内进程从 PID 1 开始 |
| **Network** | CLONE_NEWNET | 网络栈（接口、路由、iptables） | 2.6.29 | 独立网络配置 |
| **Mount** | CLONE_NEWNS | 文件系统挂载点 | 2.4.19 | 独立文件系统视图 |
| **UTS** | CLONE_NEWUTS | 主机名和域名 | 2.6.19 | 容器独立主机名 |
| **IPC** | CLONE_NEWIPC | System V IPC、POSIX 消息队列 | 2.6.19 | 进程间通信隔离 |
| **User** | CLONE_NEWUSER | 用户和组 ID | 3.8 | UID/GID 映射，rootless 容器 |
| **Cgroup** | CLONE_NEWCGROUP | Cgroup 根目录视图 | 4.6 | Cgroup 视图隔离 |
| **Time** | CLONE_NEWTIME | 系统时间 | 5.6 | 时间命名空间 |

## Namespace 操作

```bash
# 查看进程的所有 namespace
ls -la /proc/<pid>/ns/
# lrwxrwxrwx 1 root root 0 ... cgroup -> 'cgroup:[4026531835]'
# lrwxrwxrwx 1 root root 0 ... ipc -> 'ipc:[4026531839]'
# lrwxrwxrwx 1 root root 0 ... mnt -> 'mnt:[4026531840]'
# lrwxrwxrwx 1 root root 0 ... net -> 'net:[4026531992]'
# lrwxrwxrwx 1 root root 0 ... pid -> 'pid:[4026531836]'
# lrwxrwxrwx 1 root root 0 ... user -> 'user:[4026531837]'
# lrwxrwxrwx 1 root root 0 ... uts -> 'uts:[4026531838]'

# 查看当前进程的 namespace
ls -la /proc/self/ns/

# 比较 namespace ID
readlink /proc/1/ns/mnt
readlink /proc/<container_pid>/ns/mnt

# 使用 nsenter 进入容器 namespace
# 进入网络命名空间
nsenter --target <pid> --net ip addr show

# 进入所有命名空间（等同于在容器内执行）
nsenter --target <pid> --mount --uts --ipc --net --pid -- /bin/bash

# 使用 unshare 创建新的命名空间
# 新的 UTS 命名空间（独立主机名）
unshare --uts /bin/bash
hostname my-container
exit

# 新的 PID 命名空间
unshare --pid --fork --mount-proc /bin/bash
ps aux    # 只能看到新命名空间中的进程

# 新的网络命名空间
unshare --net /bin/bash
ip link   # 只看到 lo 接口

# 新的 Mount 命名空间
unshare --mount /bin/bash
mount --bind /tmp /mnt
# 只在当前命名空间中可见
```

## Kubernetes 中的 Namespace 使用

```
┌─────────────────────────────────────────────────────────────────┐
│                    Kubernetes Pod 的 Namespace                    │
│                                                                  │
│  Pod (Pause 容器/PID 1)                                         │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  共享 Namespace:                                          │  │
│  │  ├── Network (所有容器共享同一 IP)                         │  │
│  │  ├── UTS (共享主机名)                                     │  │
│  │  └── IPC (共享进程间通信)                                  │  │
│  │                                                            │  │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────┐             │  │
│  │  │Container 1│  │Container 2│  │Container 3│             │  │
│  │  │ nginx     │  │ sidecar   │  │ app       │             │  │
│  │  │ PID=隔离  │  │ PID=隔离  │  │ PID=隔离  │             │  │
│  │  │ Network=共享│ Network=共享│ Network=共享│             │  │
│  │  │ Mount=隔离 │ Mount=隔离  │ Mount=隔离  │             │  │
│  │  └───────────┘  └───────────┘  └───────────┘             │  │
│  │                                                            │  │
│  │  共享 Volume: /shared/data                                 │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  独立 Namespace (与宿主机隔离):                                   │
│  ├── PID (每个容器独立的进程树)                                   │
│  ├── Mount (每个容器独立的文件系统视图)                            │
│  ├── User (可选, UID 映射)                                       │
│  └── Cgroup (可选)                                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## Cgroups 详解

Cgroups (Control Groups) 实现 Linux 内核级别的资源限制、优先级分配和资源统计。Kubernetes 通过 cgroups 实现 Pod 的 resources.limits 和 resources.requests。

## Cgroups v2 控制器详解

| 控制器 | 功能 | 关键参数 | Kubernetes 对应 |
|:---|:---|:---|:---|
| **cpu** | CPU 时间分配 | cpu.max, cpu.weight, cpu.max.burst | resources.limits.cpu |
| **memory** | 内存使用限制 | memory.max, memory.high, memory.swap.max | resources.limits.memory |
| **io** | 块设备 I/O 限制 | io.max, io.weight | 无直接对应（可通过-device 限制） |
| **pids** | 进程数限制 | pids.max | pod 的 pidLimit |
| **cpuset** | CPU 亲和性 | cpuset.cpus, cpuset.mems | CPU 管理策略 |
| **hugetlb** | 大页内存 | hugetlb.<size>.max | 大页内存请求 |
| **rdma** | RDMA 资源 | rdma.max | RDMA 设备限制 |
| **misc** | 杂项设备 | misc.max | 其他设备限制 |

## Cgroups v2 实操

```bash
# 查看 cgroups 版本和挂载
mount | grep cgroup2
# cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec)

# 查看可用的控制器
cat /sys/fs/cgroup/cgroup.controllers
# cpuset cpu io memory hugetlb pids rdma misc

# 创建 cgroup 目录（自动创建 cgroup）
mkdir /sys/fs/cgroup/mycontainer

# 启用子控制器
echo "+cpu +memory +io +pids" > /sys/fs/cgroup/cgroup.subtree_control

# ===== CPU 限制 =====
# 设置 CPU 最大配额 (quota_us / period_us)
# 50000/100000 = 50% CPU
echo "50000 100000" > /sys/fs/cgroup/mycontainer/cpu.max

# 设置 CPU 权重 (1-10000, 默认 100)
echo "200" > /sys/fs/cgroup/mycontainer/cpu.weight

# 查看 CPU 统计
cat /sys/fs/cgroup/mycontainer/cpu.stat
# usage_usec 1234567
# user_usec 1000000
# system_usec 234567
# nr_periods 1000
# nr_throttled 50          ← 被限流次数
# throttled_usec 500000    ← 累计限流时间

# ===== 内存限制 =====
# 设置内存最大值 (字节)
echo "536870912" > /sys/fs/cgroup/mycontainer/memory.max    # 512MB

# 设置内存高水位（超过此值内核会积极回收）
echo "429496729" > /sys/fs/cgroup/mycontainer/memory.high   # ~410MB

# 设置 swap 限制
echo "268435456" > /sys/fs/cgroup/mycontainer/memory.swap.max  # 256MB swap

# 查看内存使用
cat /sys/fs/cgroup/mycontainer/memory.current     # 当前使用
cat /sys/fs/cgroup/mycontainer/memory.peak        # 历史峰值
cat /sys/fs/cgroup/mycontainer/memory.stat        # 详细统计
cat /sys/fs/cgroup/mycontainer/memory.events      # 事件通知

# ===== I/O 限制 =====
# 限制读取速率 (major:minor rbps wbps riops wiops)
echo "8:0 rbps=10485760 wbps=max riops=max wiops=max" > /sys/fs/cgroup/mycontainer/io.max
# 限制 /dev/sda (8:0) 读取速率为 10MB/s

# 设置 I/O 权重
echo "8:0 200" > /sys/fs/cgroup/mycontainer/io.weight

# ===== 进程数限制 =====
echo "100" > /sys/fs/cgroup/mycontainer/pids.max

# ===== 将进程加入 cgroup =====
echo $$ > /sys/fs/cgroup/mycontainer/cgroup.procs

# 查看进程列表
cat /sys/fs/cgroup/mycontainer/cgroup.procs
```

---

## OverlayFS 详解

OverlayFS 是 Linux 内核的联合文件系统，Docker 和 Kubernetes 使用它来实现镜像的分层存储和容器的写时复制（Copy-on-Write）。

## OverlayFS 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     OverlayFS 分层架构                            │
│                                                                  │
│                     merged (联合视图)                             │
│                     容器运行时看到的文件系统                        │
│                     ┌─────────────────────────────┐             │
│                     │  file1 (修改版) │ file3 (新) │             │
│                     └─────────────────────────────┘             │
│                              │                                   │
│              ┌───────────────┴───────────────┐                  │
│              │                               │                  │
│   ┌──────────┴─────────┐        ┌───────────┴──────────┐       │
│   │  upperdir (可写层)  │        │  lowerdir (只读层)    │       │
│   │  容器运行时修改      │        │  镜像层叠加            │       │
│   │  file1 (修改版)     │        │  ┌────────────────┐ │       │
│   │  file3 (新文件)     │        │  │ Layer 3 (顶层)  │ │       │
│   │                    │        │  │ file1 │ file2   │ │       │
│   │  whiteout 标记:    │        │  ├────────────────┤ │       │
│   │  .wh.file_old      │        │  │ Layer 2         │ │       │
│   │  (标记已删除文件)   │        │  │ file_old        │ │       │
│   └────────────────────┘        │  ├────────────────┤ │       │
│                                  │  │ Layer 1 (基础)  │ │       │
│   ┌────────────────────┐        │  │ /bin /lib /etc  │ │       │
│   │  workdir (工作目录)  │        │  └────────────────┘ │       │
│   │  OverlayFS 内部使用 │        │  ┌────────────────┐ │       │
│   └────────────────────┘        │  │ Layer 0 (基础)  │ │       │
│                                  │  └────────────────┘ │       │
│                                  └────────────────────┘        │
│                                                                  │
│  读操作: 从 merged 读 → 先查 upper → 再查 lower                  │
│  写操作: 写入 upper (COW) → lower 不变                           │
│  删操作: 在 upper 创建 whiteout 文件 → 隐藏 lower 中的文件        │
└─────────────────────────────────────────────────────────────────┘
```

## OverlayFS 实操

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
# Docker 的 overlay2 存储驱动
# 查看 Docker 存储驱动
docker info | grep "Storage Driver"

# Docker overlay2 目录结构
/var/lib/docker/overlay2/
├── <layer-id>/              # 镜像层
│   ├── diff/                # 层内容
│   ├── link                 # 层链接名
│   ├── lower                # 下层 ID 列表
│   └── merged/              # 挂载的联合视图
└── l/                       # 符号链接目录
    ├── XXX -> ../<id>/diff
    └── YYY -> ../<id>/diff

# 查看 Docker 容器的 overlay 挂载
mount | grep overlay
# overlay on /var/lib/docker/overlay2/<id>/merged type overlay
#   (rw,relatime,lowerdir=...,upperdir=...,workdir=...)

# 手动挂载 OverlayFS
mkdir -p /tmp/overlay/{lower1,lower2,upper,work,merged}

# 创建测试文件
echo "from lower1" > /tmp/overlay/lower1/file1.txt
echo "from lower2" > /tmp/overlay/lower2/file2.txt

# 挂载（lowerdir 支持多层，用:分隔）
mount -t overlay overlay \
  -o lowerdir=/tmp/overlay/lower2:/tmp/overlay/lower1,\
upperdir=/tmp/overlay/upper,\
workdir=/tmp/overlay/work \
  /tmp/overlay/merged

# 查看联合视图
ls /tmp/overlay/merged/
# file1.txt  file2.txt  (两个 lower 层的文件都可见)

# 修改文件 (COW - 写时复制)
echo "modified" > /tmp/overlay/merged/file1.txt

# 检查: upper 层包含修改版
cat /tmp/overlay/upper/file1.txt    # "modified"
# 检查: lower 层不变
cat /tmp/overlay/lower1/file1.txt   # "from lower1"

# 删除文件 (whiteout)
rm /tmp/overlay/merged/file2.txt
# upper 层出现 whiteout 标记
ls -la /tmp/overlay/upper/
# c--------- 1 root root 0, 0 ... file2.txt  (字符设备, 0/0)

# 创建新文件
echo "new file" > /tmp/overlay/merged/file3.txt
cat /tmp/overlay/upper/file3.txt    # 新文件在 upper 层

# 清理
umount /tmp/overlay/merged
rm -rf /tmp/overlay  # ⚠️ 删除系统/数据文件
```

---

## 容器安全特性

## Linux Capabilities

Linux Capabilities 将传统的 root 权限细分为约 40 种独立的能力，容器运行时默认只保留必要的 capabilities。

```bash
# 查看所有 capabilities
capsh --print
cat /proc/self/status | grep Cap

# 查看进程 capabilities
getpcaps <pid>
cat /proc/<pid>/status | grep Cap
# CapEff: 0000003fffffffff  (有效 capabilities 位图)

# 解码 capabilities 位图
capsh --decode=0000003fffffffff

# 常见 capabilities
# CAP_NET_BIND_SERVICE    - 绑定 1024 以下端口
# CAP_NET_ADMIN           - 网络管理 (修改路由表、防火墙等)
# CAP_NET_RAW             - 使用原始套接字 (ping 等)
# CAP_SYS_ADMIN           - 系统管理 (mount, hostname 等) ⚠️ 非常强大
# CAP_SYS_PTRACE          - 进程跟踪 (strace, gdb)
# CAP_SYS_CHROOT          - chroot
# CAP_DAC_OVERRIDE        - 绕过文件权限检查
# CAP_FOWNER              - 绕过文件所有者检查
# CAP_KILL                - 发送信号给其他用户进程
# CAP_SETUID              - 改变用户 ID
# CAP_SETGID              - 改变组 ID
# CAP_MKNOD               - 创建设备文件
# CAP_AUDIT_WRITE         - 写审计日志

# 设置文件 capabilities
setcap cap_net_bind_service=+ep /usr/bin/python3    # 允许绑定低端口
getcap /usr/bin/python3

# 删除文件 capabilities
setcap -r /usr/bin/python3

# Docker 默认保留的 capabilities:
# CAP_NET_BIND_SERVICE
# CAP_CHOWN
# CAP_DAC_OVERRIDE
# CAP_FOWNER
# CAP_FSETID
# CAP_KILL
# CAP_SETGID
# CAP_SETUID
# CAP_SETPCAP
# CAP_NET_RAW
# CAP_SYS_CHROOT
# CAP_MKNOD
# CAP_AUDIT_WRITE
# CAP_SETFCAP
```

## Seccomp (Secure Computing Mode)

Seccomp 限制进程可以使用的系统调用，是容器安全的重要防线。

```bash
# 查看 Seccomp 状态
cat /proc/<pid>/status | grep Seccomp
# 0 = SECCOMP_MODE_DISABLED  (禁用)
# 1 = SECCOMP_MODE_STRICT    (严格: 只允许 read/write/exit/sigreturn)
# 2 = SECCOMP_MODE_FILTER    (过滤: 使用 BPF 程序自定义)

# Docker 默认的 Seccomp 配置文件
# /usr/share/docker/seccomp/default.json  (Docker)
# 禁止约 44 个危险系统调用

# 被 Docker 默认禁止的系统调用 (部分):
# acct              - 进程记账
# add_key           - 内核密钥管理
# bpf               - eBPF 程序加载
# clock_adjtime     - 时钟调整
# clock_settime     - 设置时钟
# create_module     - 创建内核模块
# delete_module     - 删除内核模块
# get_kernel_syms   - 获取内核符号
# init_module       - 加载内核模块
# io_setup          - 异步 I/O (可能耗尽资源)
# kcmp              - 内核比较
# kexec_file_load   - kexec 加载
# kexec_load        - kexec 加载
# keyctl            - 内核密钥操作
# lock              - 锁定内存
# mount             - 挂载文件系统
# nfsservctl        - NFS 服务控制
# open_by_handle_at - 通过句柄打开文件
# perf_event_open   - 性能事件
# pivot_root        - 改变根文件系统
# query_module      - 查询模块
# quotactl          - 磁盘配额控制
# reboot            - 重启系统
# swapon/swapoff    - swap 管理
# sysfs             - 系统文件系统
# umount            - 卸载文件系统

# 使用自定义 Seccomp 配置运行容器
docker run --security-opt seccomp=/path/to/seccomp.json nginx

# 完全禁用 Seccomp (仅调试用!)
docker run --security-opt seccomp=unconfined nginx

# Kubernetes 中使用 Seccomp
# 在 Pod spec 中:
# securityContext:
#   seccompProfile:
#     type: RuntimeDefault     # 使用容器运行时的默认配置
#     type: Localhost          # 使用节点上的自定义配置文件
#     localhostProfile: profiles/audit.json
```

---

<!-- chunk: 常用命令参考 -->## 常用命令参考

## 容器调试命令

```bash
# 查看容器进程信息
docker inspect --format '{{.State.Pid}}' <container>       # Docker PID
crictl inspect <container> | jq .info.pid                   # CRI-O/containerd PID

# 进入容器命名空间
nsenter --target <pid> --mount --uts --ipc --net --pid /bin/bash

# 查看容器的 cgroup
cat /proc/<pid>/cgroup
ls /sys/fs/cgroup/system.slice/docker-<id>.scope/           # systemd
ls /sys/fs/cgroup/docker/<id>/                               # cgroupfs

# 查看容器的 namespace
ls -la /proc/<pid>/ns/

# 查看容器的 capabilities
cat /proc/<pid>/status | grep Cap

# 查看容器的 overlay 挂载
cat /proc/<pid>/mountinfo | grep overlay

# 查看容器的网络
nsenter --target <pid> --net ip addr
nsenter --target <pid> --net ip route
nsenter --target <pid> --net iptables -t nat -L -n
```

---

<!-- chunk: 性能调优 -->## 性能调优

## 容器性能优化

```bash
# 1. 镜像大小优化
# 使用多阶段构建
# 使用更小的基础镜像 (Alpine, Distroless)

# 2. 存储驱动选择
# overlay2 (推荐): 性能好，稳定
# devicemapper: 旧版本兼容
# btrfs/zfs: 特定场景

# 3. 日志驱动优化
# 限制日志大小
dockerd --log-driver=json-file --log-opt max-size=10m --log-opt max-file=3

# 4. cgroup 资源限制调优
# 在 Kubernetes 中通过 resources.limits 设置
# 注意 CPU throttling 问题
```

---

<!-- chunk: 安全加固 -->## 安全加固

## 容器安全最佳实践

```yaml
# Kubernetes Pod 安全最佳实践
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  hostNetwork: false              # 不使用宿主机网络
  hostPID: false                  # 不使用宿主机 PID
  hostIPC: false                  # 不使用宿主机 IPC
  securityContext:
    runAsNonRoot: true            # 禁止 root 运行
    runAsUser: 1000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: app:latest
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
    resources:
      limits:
        cpu: "1"
        memory: "512Mi"
      requests:
        cpu: "100m"
        memory: "128Mi"
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /var/cache
  volumes:
  - name: tmp
    emptyDir:
      medium: Memory              # tmpfs 用于临时文件
  - name: cache
    emptyDir: {}
```

---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

## 容器运行时接口 (CRI)

Kubernetes 通过 CRI (Container Runtime Interface) 与容器运行时交互，支持 containerd、CRI-O 等：

```
┌─────────────────────────────────────────────────────────────────┐
│                    Kubernetes 容器运行时架构                      │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                     kubelet                               │  │
│  │                                                           │  │
│  │  ┌──────────────────────────────────────────────────┐    │  │
│  │  │           CRI (gRPC API)                          │    │  │
│  │  │  RuntimeService / ImageService                    │    │  │
│  │  └───────────────┬──────────────────────────────────┘    │  │
│  └──────────────────┼───────────────────────────────────────┘  │
│                      │                                           │
│         ┌────────────┼────────────┐                              │
│         │            │            │                              │
│         ▼            ▼            ▼                              │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐                    │
│  │containerd │ │  CRI-O    │ │  Docker   │                    │
│  │ (推荐)    │ │           │ │ (已弃用)  │                    │
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘                    │
│        │             │             │                            │
│        ▼             ▼             ▼                            │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐                    │
│  │   runc    │ │   runc    │ │ containerd│                    │
│  │ (OCI)     │ │ (OCI)     │ │ + runc    │                    │
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘                    │
│        │             │             │                            │
│        ▼             ▼             ▼                            │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Linux Kernel                                │  │
│  │   Namespaces + Cgroups + OverlayFS + Seccomp + Capabilities│
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

1. **使用非 root 运行容器**: `runAsNonRoot: true`
2. **只读根文件系统**: `readOnlyRootFilesystem: true`
3. **最小化 capabilities**: `drop: ["ALL"]`，仅添加必要的
4. **使用默认 seccomp**: `seccompProfile: type: RuntimeDefault`
5. **限制资源**: 始终设置 resources.limits
6. **使用多阶段构建**: 减小镜像大小和攻击面
7. **镜像安全扫描**: 使用 Trivy/Clair 持续扫描
8. **启用 Pod Security Standards**: 使用 restricted 级别

---

<!-- chunk: 故障排查 -->## 故障排查

## 容器常见问题

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

```bash
# 容器无法启动
docker logs <container>              # 查看日志
crictl logs <container>
kubectl logs <pod>
kubectl describe pod <pod>           # 查看 Events

# 权限被拒绝
# 检查 SELinux
ausearch -m avc -ts recent
# 检查 capabilities
cat /proc/<pid>/status | grep Cap
# 检查文件权限
ls -laZ /path

# 容器 OOMKilled
kubectl describe pod <pod> | grep -A5 "Last State"
# 检查 cgroup 内存使用
cat /sys/fs/cgroup/.../memory.current
cat /sys/fs/cgroup/.../memory.max

# overlay2 磁盘空间不足
docker system df                     # 查看 Docker 磁盘使用
docker system prune -a               # 清理未使用的镜像和容器  # ⚠️ 强制清理，可能杀运行中容器
```

---

## 手动创建容器

理解容器底层原理的最佳方式是手动创建一个容器。以下步骤展示了容器运行时的核心操作：

```bash
#!/bin/bash
# 手动创建容器 - 演示容器核心技术

# 1. 准备 rootfs
mkdir -p /tmp/mycontainer/rootfs
cd /tmp/mycontainer/rootfs

# 2. 下载并解压 Alpine 基础镜像
docker export $(docker create alpine:latest) | tar -xf -

# 3. 使用 unshare 创建隔离环境
unshare --pid --fork --mount --uts --ipc --net \
  --mount-proc=/tmp/mycontainer/rootfs/proc \
  chroot /tmp/mycontainer/rootfs /bin/sh

# 现在你在一个"容器"中了:
# - 独立的 PID 空间 (ps aux 只看到自己)
# - 独立的 mount 空间
# - 独立的 hostname
# - 独立的 IPC
# - 独立的网络 (只有 lo 接口)
```

## 使用 runc 创建 OCI 标准容器

```bash
# 1. 创建 bundle 目录
mkdir -p /tmp/mycontainer/rootfs

# 2. 准备 rootfs
docker export $(docker create alpine:latest) | tar -C /tmp/mycontainer/rootfs -xf -

# 3. 生成 OCI 配置
cd /tmp/mycontainer
runc spec

# 4. 编辑 config.json (可选)
# 修改 process.args, 挂载点, namespace 等

# 5. 运行容器
runc run mycontainer

# 6. 在另一个终端查看
runc list
runc exec mycontainer ps aux
```

## 容器运行时对比

| 特性 | runc | crun | containerd | CRI-O |
|:---|:---|:---|:---|:---|
| **语言** | Go | C | Go | Go |
| **角色** | OCI 运行时 | OCI 运行时 | 高级运行时 | 高级运行时 |
| **性能** | 标准 | 更快 | N/A | N/A |
| **rootless** | 支持 | 支持 | 支持 | 支持 |
| **K8s 使用** | 底层运行时 | 底层运行时 | 直接使用 | 直接使用 |

## 容器镜像安全扫描

```bash
# Trivy - 镜像漏洞扫描
trivy image nginx:latest
trivy image --severity HIGH,CRITICAL nginx:latest
trivy image --ignore-unfixed nginx:latest

# 在 CI/CD 中集成
trivy image --exit-code 1 --severity CRITICAL myapp:latest

# 扫描 Kubernetes 集群中的所有镜像
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{range .spec.containers[*]}{"\t"}{.image}{"\n"}{end}{end}' | \
  while read image; do
    trivy image "$image" 2>/dev/null | grep -E "Total|HIGH|CRITICAL"
  done
```

## 容器运行时安全监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```bash
# Falco - 运行时安全监控
# 安装 Falco
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm install falco falcosecurity/falco -n falco --create-namespace

# 查看 Falco 告警
kubectl logs -n falco -l app=falco -f

# 自定义规则
# /etc/falco/falco_rules.local.yaml
- rule: Terminal Shell in Container
  desc: A shell was spawned in a container
  condition: >
    spawned_process and container and
    proc.name in (bash, sh, zsh, fish) and
    not proc.pname in (docker-entrypoi)
  output: "Shell spawned in container (user=%user.name container=%container.name shell=%proc.name parent=%proc.pname cmdline=%proc.cmdline)"
  priority: WARNING
  tags: [container, shell]
```

## rootless 容器

rootless 容器是容器安全的重要发展方向，它允许非 root 用户运行容器，即使容器被攻破，攻击者也只能获得普通用户权限，无法影响宿主机系统。

```bash
# Podman - rootless 容器运行
# 安装
yum install -y podman                   # RHEL
apt install -y podman                   # Ubuntu

# 以普通用户运行容器
podman run -d --name nginx nginx:latest
podman ps
podman logs nginx

# rootless 容器原理:
# 1. User Namespace: 容器内的 root (UID 0) 映射为宿主机的普通用户
# 2. Subordinate UID/GID: /etc/subuid 和 /etc/subgid 定义映射范围
# 3. SLIRP 网络模拟: 不需要 root 权限创建网络设备

# 查看用户映射
cat /etc/subuid
cat /etc/subgid

# Kubernetes 对 rootless 的支持
# 通过 User Namespace (KEP-127, Kubernetes 1.25+)
# hostUsers: false 在 Pod spec 中启用
```

---

<!-- chunk: 相关文档 -->## 相关文档

- [01-linux-system-architecture](./01-linux-system-architecture.md) - 系统架构
- [07-linux-security-hardening](./07-linux-security-hardening.md) - 安全加固
- [02-linux-process-management](./02-linux-process-management.md) - 进程管理

---

**维护者**: Allen Galler (allengaller@gmail.com) | **许可证**: MIT

## See Also

- 06-linux-performance-tuning
- 07-linux-security-hardening
- 09-linux-operations-basics
- 99-linux-commands-reference

```
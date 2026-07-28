---
title: 02 - Linux 进程管理与系统监控：生产环境运维专家实践
description: '# 02 - Linux 进程管理与系统监控：生产环境运维专家实践'
summary: '进程管理是 Linux 系统运维的核心技能之一。在 [[kubernetes|Kubernetes]] 环境中，每个容器本质上就是一个或一组被 Linux 内核隔离和限制的进程。理解进程的创建、调度、信号处理、资源限制机制，对于排查容器异常（如 OOMKilled、CrashLoopBackOff、僵尸进程）至关重要。本文档从内核原理到运维实践，'
category: linux
tags:
- linux
- system
- kernel
- kubelet
- scheduler
- docker
tier: supporting
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
- Linux 进程管理与系统监控：生产环境运维专家实践 是什么
- 如何 Linux 进程管理与系统监控：生产环境运维专家实践
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 进程管理与系统监控：生产环境运维专家实践
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
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
  path: ../系统基础/topic-cheat-sheet/linux.md
  label: '速查卡: linux'
lifecycle: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 02 - Linux 进程管理与系统监控：生产环境运维专家实践

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 概述 -->## 概述

进程管理是 Linux 系统运维的核心技能之一。在 [[kubernetes|Kubernetes]] 环境中，每个容器本质上就是一个或一组被 Linux 内核隔离和限制的进程。理解进程的创建、调度、信号处理、资源限制机制，对于排查容器异常（如 OOMKilled、CrashLoopBackOff、僵尸进程）至关重要。本文档从内核原理到运维实践，全面深入地讲解 Linux 进程管理的各个方面，包括进程状态机、信号机制、cgroups 资源控制、OOM Killer 工作原理，以及与 Kubernetes Pod 生命周期管理的紧密关联。

---

<!-- chunk: 核心概念详解 -->## 核心概念详解

## 进程与线程

Linux 内核采用"一切皆进程"的设计哲学，线程在内核视角中被称为"轻量级进程"（Lightweight Process, LWP）。每个进程或线程都有一个唯一的 PID（Process ID），由内核统一分配。

```
┌─────────────────────────────────────────────────────────────────┐
│                       用户空间视角                               │
│                                                                  │
│   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐     │
│   │   进程 A      │    │   进程 B      │    │   进程 C      │     │
│   │  PID=1000     │    │  PID=2000     │    │  PID=3000     │     │
│   │  独立地址空间  │    │  独立地址空间  │    │  独立地址空间  │     │
│   │               │    │  ┌────────┐  │    │               │     │
│   │               │    │  │线程B-1 │  │    │               │     │
│   │               │    │  │TID=2001│  │    │               │     │
│   │               │    │  └────────┘  │    │               │     │
│   │               │    │  ┌────────┐  │    │               │     │
│   │               │    │  │线程B-2 │  │    │               │     │
│   │               │    │  │TID=2002│  │    │               │     │
│   │               │    │  └────────┘  │    │               │     │
│   └──────────────┘    └──────────────┘    └──────────────┘     │
│                                                                  │
├─────────────────────────────────────────────────────────────────┤
│                       内核视角                                   │
│                                                                  │
│   task_struct (进程描述符)                                        │
│   ├── PID / TGID (Thread Group ID)                               │
│   ├── 状态 (R/S/D/T/Z)                                          │
│   ├── 优先级 (nice / RT priority)                                │
│   ├── 地址空间 (mm_struct)                                       │
│   ├── 打开文件 (files_struct)                                    │
│   ├── 信号处理 (signal_struct)                                   │
│   └── cgroup 归属                                                │
└─────────────────────────────────────────────────────────────────┘
```

进程与线程的关键概念对比：

| 概念 | 说明 | 特点 | 内核表示 |
|:---|:---|:---|:---|
| **进程** | 程序执行实例 | 独立地址空间、独立资源 | task_struct + mm_struct |
| **线程** | 进程内执行单元 | 共享地址空间和资源 | task_struct（共享 mm_struct） |
| **PID** | 进程标识符 | 命名空间内唯一 | task_struct->pid |
| **PPID** | 父进程 ID | 创建者进程 | task_struct->parent |
| **PGID** | 进程组 ID | 作业控制用 | task_struct->group_leader |
| **SID** | 会话 ID | 终端会话 | task_struct->session |
| **TID** | 线程标识符 | 系统全局唯一 | task_struct->pid |

## 进程类型

| 类型 | 说明 | 示例 | 特征 |
|:---|:---|:---|:---|
| **前台进程** | 占用终端 | 交互式命令 | 与终端绑定 |
| **后台进程** | 不占用终端 | `command &` | 不接收终端输入 |
| **守护进程** | 系统后台服务 | sshd, nginx, dockerd | 脱离终端、PID 为 1 的子进程 |
| **僵尸进程** | 已终止未回收 | Z 状态 | 占用 PID 和 task_struct |
| **孤儿进程** | 父进程已终止 | 被 init/systemd 收养 | PPID 变为 1 |
| **内核线程** | 内核空间运行 | kworker, ksoftirqd | PID 用方括号标记 |

---

## 进程生命周期

Linux 进程从创建到销毁经历一系列状态转换。理解这些状态对于排查容器中进程挂起、僵尸进程等问题至关重要。

```
                        fork() / clone()
                            │
                            ▼
                     ┌──────────────┐
                     │   就绪 (R)    │  ◄─── 被调度器选中
                     │  TASK_RUNNING │       (在运行队列中等待)
                     └──────┬───────┘
                            │
                     ┌──────┴───────┐
                     │              │
                     ▼              │ (时间片耗尽)
              ┌──────────────┐     │
              │   运行 (R)    │     │
              │  TASK_RUNNING │     │
              └──────┬───────┘     │
                     │              │
          ┌──────────┼──────────┐   │
          │          │          │   │
          ▼          ▼          │   │
   ┌────────────┐ ┌────────────┐│   │
   │ 可中断睡眠  │ │不可中断睡眠 ││   │
   │  (S)       │ │  (D)       ││   │
   │ TASK_      │ │ TASK_      ││   │
   │ INTERRUPT  │ │ UNINTERRUPT││   │
   └──────┬─────┘ └──────┬─────┘│   │
          │               │      │   │
          │ 信号/I/O完成  │ I/O   │   │
          └───────┬───────┘ 完成  │   │
                  │               │   │
                  ▼               │   │
           ┌────────────┐        │   │
           │ 停止 (T)    │        │   │
           │ TASK_STOPPED│        │   │
           │ TASK_TRACED │        │   │
           └──────┬─────┘        │   │
                  │ SIGCONT      │   │
                  └──────┬───────┘   │
                         │           │
                         ▼           │
                  ┌────────────┐    │
                  │  退出      │    │
                  │  exit()    │    │
                  └──────┬─────┘    │
                         │          │
                         ▼          │
                  ┌────────────┐    │
                  │ 僵尸 (Z)    │    │
                  │ EXIT_ZOMBIE │    │
                  └──────┬─────┘    │
                         │ wait()   │
                         ▼          │
                  ┌────────────┐    │
                  │  终止/回收  │    │
                  └────────────┘    │
                                    │
                  ◄─────────────────┘
```

进程状态详解：

| 状态码 | 名称 | 内核常量 | 说明 | 典型场景 |
|:---:|:---|:---|:---|:---|
| **R** | Running | TASK_RUNNING | 运行中或就绪（在运行队列中） | CPU 密集型计算 |
| **S** | Sleeping | TASK_INTERRUPTIBLE | 可中断睡眠，等待事件 | 等待 I/O、等待锁 |
| **D** | Disk Sleep | TASK_UNINTERRUPTIBLE | 不可中断睡眠（不能被信号唤醒） | 等待磁盘 I/O 完成 |
| **T** | Stopped | TASK_STOPPED | 已停止 | 被 SIGSTOP/SIGTSTP 暂停 |
| **Z** | Zombie | EXIT_ZOMBIE | 僵尸进程，已退出但父进程未回收 | 父进程未调用 wait() |
| **I** | Idle | TASK_IDLE | 空闲内核线程 | 内核空闲时 |

---

## 进程创建：fork() 与 clone()

Linux 进程通过 `fork()` 系统调用创建，新进程是父进程的完整副本。`clone()` 系统调用提供了更细粒度的控制，容器运行时正是利用 `clone()` 配合不同的 flags 来创建隔离的进程。

```bash
# 查看系统调用
strace -e trace=clone,clone3,fork,vfork <command>

# fork 后的写时复制 (COW) 机制
# 父子进程共享相同的物理内存页
# 只有当某一方尝试写入时，内核才会复制该页
```

```
┌──────────────────┐     fork()     ┌──────────────────┐
│   父进程          │ ─────────────► │   子进程          │
│   PID=1000        │               │   PID=1001        │
│   PPID=1          │               │   PPID=1000       │
│                   │               │                   │
│   虚拟地址空间     │   COW 共享     │   虚拟地址空间     │
│   ┌───────────┐  │ ◄────────────► │   ┌───────────┐  │
│   │ 代码段 .text │  │               │   │ 代码段 .text │  │
│   │ 数据段 .data │  │               │   │ 数据段 .data │  │
│   │ 堆 heap     │  │               │   │ 堆 heap     │  │
│   │ 栈 stack    │  │               │   │ 栈 stack    │  │
│   └───────────┘  │               │   └───────────┘  │
└──────────────────┘               └──────────────────┘
```

---

## 信号与进程控制

信号（Signal）是 Linux 进程间通信和异步事件处理的核心机制。理解信号对于排查容器中的进程终止行为（如 Kubernetes 发送 SIGTERM 优雅终止）至关重要。

## 信号架构

```
┌───────────────┐    kill()    ┌──────────────────────────┐
│  发送进程      │ ──────────► │      内核信号队列         │
│               │              │  ┌────┬────┬────┬────┐   │
│               │              │  │ 1  │ 2  │...│ 31 │   │
│               │              │  └────┴────┴────┴────┘   │
└───────────────┘              └─────────────┬────────────┘
                                             │
                                             ▼
                                    ┌──────────────────────┐
                                    │    目标进程            │
                                    │                       │
                                    │  1. 检查信号屏蔽字     │
                                    │  2. 查找信号处理函数   │
                                    │  3. 执行默认动作/     │
                                    │     自定义处理函数     │
                                    └──────────────────────┘
```

## 常用信号详解

| 信号 | 编号 | 名称 | 说明 | 默认动作 | 是否可捕获 | Kubernetes 用途 |
|:---|:---:|:---|:---|:---|:---:|:---|
| **SIGHUP** | 1 | 终端挂起 | 终端关闭时发送 | 终止 | 是 | 重新加载配置 |
| **SIGINT** | 2 | 中断 | Ctrl+C | 终止 | 是 | 交互式中断 |
| **SIGQUIT** | 3 | 退出 | Ctrl+\ | 终止+core dump | 是 | 调试分析 |
| **SIGKILL** | 9 | 强制终止 | 不可拦截的终止 | 终止 | **否** | 强制删除 Pod |
| **SIGTERM** | 15 | 终止请求 | 优雅终止信号 | 终止 | 是 | Pod 优雅终止 |
| **SIGSTOP** | 19 | 停止 | 不可拦截的停止 | 停止 | **否** | 暂停进程 |
| **SIGCONT** | 18 | 继续 | 恢复执行 | 继续执行 | 是 | 恢复进程 |
| **SIGUSR1** | 10 | 用户自定义 1 | 应用自定义用途 | 终止 | 是 | Nginx 重开日志 |
| **SIGUSR2** | 12 | 用户自定义 2 | 应用自定义用途 | 终止 | 是 | 应用热重载 |
| **SIGCHLD** | 17 | 子进程状态变化 | 子进程终止/停止 | 忽略 | 是 | init 系统核心 |

## 信号在 Kubernetes 中的应用

Kubernetes 在终止 Pod 时遵循以下信号流程：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
1. kubectl delete pod / Pod 演进删除
   │
   ▼
2. kubelet 发送 SIGTERM 给 PID 1 (容器主进程)
   │
   ├── 进程捕获 SIGTERM，执行优雅关闭
   │   ├── 停止接受新连接
   │   ├── 完成进行中的请求
   │   ├── 保存状态/关闭数据库连接
   │   └── 进程正常退出 (exit code 0)
   │
   └── 等待 terminationGracePeriodSeconds (默认 30s)
       │
       ▼
3. 超时后发送 SIGKILL (信号 9)
   │
   ▼
4. 进程被强制终止 (exit code 137 = 128 + 9)
```
```bash
# 查看进程等待的信号
cat /proc/<pid>/status | grep -i sig

# 信号屏蔽字解读
# SigBlk: 被屏蔽的信号
# SigIgn: 被忽略的信号
# SigCgt: 已注册处理函数的信号

# 发送信号给进程
kill -SIGTERM <pid>
kill -15 <pid>

# 强制终止（最后手段）
kill -SIGKILL <pid>
kill -9 <pid>

# 按名称发送信号
pkill -SIGTERM -f "nginx"
killall -HUP nginx
```

---

## 进程优先级与调度

## CFS 调度器

Linux 默认使用完全公平调度器（Completely Fair Scheduler, CFS），它使用红黑树维护进程的虚拟运行时间，确保所有进程公平地获得 CPU 时间。

```
┌─────────────────────────────────────────────────────────────────┐
│                    CFS 调度器                                    │
│                                                                  │
│   红黑树 (按 vruntime 排序)                                      │
│                                                                  │
│                    ┌────────┐                                    │
│                    │vruntime│                                    │
│                    │  最小   │ ← 下一个被调度                     │
│                    └───┬────┘                                    │
│                   ╱         ╲                                    │
│              ┌───┴───┐   ┌───┴───┐                               │
│              │ 左子树 │   │ 右子树 │                               │
│              │vruntime│   │vruntime│                               │
│              └────────┘   └────────┘                               │
│                                                                  │
│   nice 值影响权重:                                                │
│   nice -20 → weight 8871 (最高)                                   │
│   nice   0 → weight 1024 (默认)                                   │
│   nice  19 → weight   15 (最低)                                   │
│                                                                  │
│   vruntime 增长速度 = 真实时间 × (1024 / weight)                  │
│   nice 值越小 → weight 越大 → vruntime 增长越慢 → 获得更多CPU     │
└─────────────────────────────────────────────────────────────────┘
```

## 优先级调整

```bash
# nice 值范围: -20 (最高) 到 19 (最低)
# 默认: 0
# 需要 root 才能设置负值

# 启动时设置优先级
nice -n 10 command          # 以 nice 10 启动
nice -n -5 command          # 以 nice -5 启动（需要 root）

# 调整运行中进程的优先级
renice 10 -p <pid>          # 调整指定进程
renice -n 5 -u username     # 调整用户所有进程
renice -n -5 -p <pid>       # 提高优先级（需要 root）

# 查看进程 nice 值
ps -eo pid,nice,comm | grep nginx

# 实时优先级（实时调度策略）
chrt -f 50 command          # SCHED_FIFO, 优先级 50
chrt -r 50 command          # SCHED_RR, 优先级 50
chrt -p <pid>               # 查看调度策略
```

| nice 值 | 优先级 | 权重 | 说明 | 场景 |
|:---:|:---:|:---:|:---|:---|
| -20 | 最高 | 8871 | 实时性要求高 | 关键服务进程 |
| -10 | 高 | 305 | 较高优先级 | 数据库进程 |
| 0 | 默认 | 1024 | 普通进程 | Web 服务器 |
| 10 | 低 | 110 | 较低优先级 | 后台批处理 |
| 19 | 最低 | 15 | 最低优先级 | 日志分析、备份 |

---

## cgroups 资源控制

cgroups（Control Groups）是 Linux 内核提供的资源限制机制，也是 Kubernetes 实现 Pod 资源管理的底层基础。Kubernetes 中的 `resources.limits` 和 `resources.requests` 最终都通过 cgroups 来实施。

## cgroups v1 vs v2

```
cgroups v1 (传统)                      cgroups v2 (现代)
┌──────────────────────────┐          ┌──────────────────────────┐
│ 多个独立的层级树           │          │ 单一统一的层级树           │
│                           │          │                           │
│ /sys/fs/cgroup/cpu/       │          │ /sys/fs/cgroup/           │
│ /sys/fs/cgroup/memory/    │          │   ├── cgroup.controllers  │
│ /sys/fs/cgroup/blkio/     │          │   ├── cgroup.procs        │
│ /sys/fs/cgroup/devices/   │          │   ├── cpu.max             │
│ /sys/fs/cgroup/pids/      │          │   ├── memory.max          │
│ ...                       │          │   ├── io.max              │
│                           │          │   ├── pids.max            │
│ 每个子系统独立管理          │          │   └── ...                 │
│ 进程可出现在多个 cgroup     │          │                           │
│                           │          │ 统一管理所有资源           │
│ 逐步淘汰中                 │          │ Kubernetes 默认使用       │
└──────────────────────────┘          └──────────────────────────┘
```

## cgroups v2 操作示例

```bash
# 查看 cgroups 版本
mount | grep cgroup

# 查看当前 cgroup 树
ls /sys/fs/cgroup/

# 创建自定义 cgroup
mkdir /sys/fs/cgroup/myapp

# 查看可用的控制器
cat /sys/fs/cgroup/cgroup.controllers

# 启用控制器
echo "+cpu +memory +io +pids" > /sys/fs/cgroup/cgroup.subtree_control

# 设置 CPU 限制 (20000 微秒配额 / 100000 微秒周期 = 20% CPU)
echo "20000 100000" > /sys/fs/cgroup/myapp/cpu.max

# 设置 CPU 权重 (默认 100，范围 1-10000)
echo "200" > /sys/fs/cgroup/myapp/cpu.weight

# 设置内存限制 (512MB)
echo "536870912" > /sys/fs/cgroup/myapp/memory.max

# 设置内存+swap 限制 (1GB)
echo "1073741824" > /sys/fs/cgroup/myapp/memory.swap.max

# 设置 I/O 限制 (读取 10MB/s)
echo "8:0 rbps=10485760 wiops=max rios=max wbps=max" > /sys/fs/cgroup/myapp/io.max

# 设置进程数限制
echo "100" > /sys/fs/cgroup/myapp/pids.max

# 将进程加入 cgroup
echo <pid> > /sys/fs/cgroup/myapp/cgroup.procs

# 查看资源使用统计
cat /sys/fs/cgroup/myapp/cpu.stat
cat /sys/fs/cgroup/myapp/memory.current
cat /sys/fs/cgroup/myapp/memory.peak
cat /sys/fs/cgroup/myapp/io.stat
```

## Kubernetes 与 cgroups 的对应关系

| Kubernetes 字段 | cgroups v2 参数 | 说明 |
|:---|:---|:---|
| `resources.limits.cpu: "2"` | `cpu.max: "200000 100000"` | 限制最多使用 2 核 CPU |
| `resources.requests.cpu: "1"` | `cpu.weight` (影响权重) | 调度依据，影响 CPU 分配比例 |
| `resources.limits.memory: "512Mi"` | `memory.max: "536870912"` | 内存硬限制 |
| `resources.requests.memory: "256Mi"` | 用于调度决策 | 调度依据，不影响 cgroup |

---

## OOM Killer 机制

OOM Killer（Out-Of-Memory Killer）是 Linux 内核在内存不足时选择并终止进程的机制。在 Kubernetes 环境中，Pod 因 OOM 被终止（exit code 137）是最常见的问题之一。

## OOM Killer 工作流程

```
系统内存不足
    │
    ▼
┌──────────────────┐
│ 内存分配失败       │
│ (物理内存+swap    │
│  不足以满足请求)   │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐     否    ┌──────────────┐
│ 是否可回收内存?    │ ────────► │ 唤醒 kswapd   │
│ (缓存、缓冲区等)   │          │ 回收页面缓存   │
└────────┬─────────┘          └──────────────┘
         │ 是
         ▼
┌──────────────────┐
│ 仍不足?           │
│ 检查 vm.panic_on_ │
│ oom 设置          │
└────────┬─────────┘
         │
    ┌────┴────┐
    │         │
    ▼         ▼
 panic=1   panic=0
 系统崩溃   触发 OOM Killer
             │
             ▼
┌──────────────────────────────┐
│       OOM Score 计算          │
│                               │
│  oom_score = f(              │
│    内存使用量,                │
│    CPU 使用量,                │
│    nice 值,                   │
│    运行时间,                  │
│    oom_score_adj              │
│  )                            │
│                               │
│  分数越高 → 越可能被终止       │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│  选择 oom_score 最高的进程    │
│  发送 SIGKILL (信号 9)        │
│  释放其占用的所有内存          │
└──────────────────────────────┘
```

## OOM 相关参数

```bash
# 查看 OOM 分数
cat /proc/<pid>/oom_score

# 调整 OOM 分数 (-1000 到 1000)
# -1000 = 永不被 OOM Killer 终止
# 1000 = 优先被 OOM Killer 终止
echo -1000 > /proc/<pid>/oom_score_adj

# 查看当前调整值
cat /proc/<pid>/oom_score_adj

# 内核参数
# vm.overcommit_memory:
#   0 = 启发式 (默认)
#   1 = 总是允许
#   2 = 严格模式 (commit_ratio)
sysctl vm.overcommit_memory
sysctl vm.overcommit_ratio

# OOM 时是否 panic
sysctl vm.panic_on_oom

# 禁止特定进程被 OOM
# 在 Kubernetes 中，qosClass=Guaranteed 的 Pod 有更低的 oom_score_adj
# qosClass=BestEffort 的 Pod 有最高的 oom_score_adj (1000)
```

## Kubernetes OOM 等级对应

| Pod QoS 类 | oom_score_adj | 被终止优先级 | 说明 |
|:---|:---:|:---|:---|
| **Guaranteed** | -997 | 最低 | requests == limits（CPU 和内存都设置了且相等） |
| **Burstable** | 根据 memory limit 计算 | 中等 | 设置了部分 requests/limits |
| **BestEffort** | 1000 | 最高 | 未设置任何 requests/limits |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看容器进程的 OOM 分数
# 找到容器进程 PID
docker inspect --format '{{.State.Pid}}' <container_id>
# 或
crictl inspect <container_id> | jq .info.pid

# 查看 OOM 分数
cat /proc/<pid>/oom_score
cat /proc/<pid>/oom_score_adj

# 查看系统 OOM 日志
dmesg | grep -i "out of memory|oom-killer|killed process"
journalctl -k | grep -i "oom"
```
---

<!-- chunk: 常用命令参考 -->## 常用命令参考

## 进程查看

```bash
# ps 命令 - 进程快照
ps aux                          # BSD 风格，显示所有进程
ps -ef                          # System V 风格
ps -eo pid,ppid,user,%cpu,%mem,stat,cmd --sort=-%cpu   # 自定义输出，按 CPU 排序
ps -eo pid,ppid,rss,vsz,comm --sort=-rss                # 按内存排序
ps -ejH                         # 显示进程树（缩进）
ps axjf                         # 显示进程树（ASCII）
ps -u username                  # 显示指定用户的进程
ps -p <pid> -o pid,ppid,cmd     # 显示指定进程信息

# top 命令 - 实时监控
top                             # 默认视图
top -u username                 # 按用户过滤
top -p <pid1>,<pid2>            # 监控指定进程
top -bn1 | head -20             # 批处理模式，适合脚本
# 快捷键: 1-显示各CPU, P-按CPU排序, M-按内存排序, k-杀死进程

# htop 命令 - 增强版（推荐）
htop                            # 交互式进程查看
htop -u username                # 按用户过滤
htop -p <pid>                   # 监控指定进程
htop -t                         # 树状显示

# pgrep/pkill - 按模式查找/终止
pgrep nginx                     # 查找 nginx 进程 PID
pgrep -u root                   # 查找 root 用户的进程
pgrep -f "java -jar"            # 按完整命令行匹配
pgrep -l nginx                  # 显示进程名
pgrep -a nginx                  # 显示完整命令行

# pstree - 进程树
pstree                          # 显示进程树
pstree -p                       # 显示 PID
pstree -u username              # 显示指定用户
pstree -s <pid>                 # 显示进程的父进程链
```

## 进程终止

```bash
# kill - 发送信号
kill <pid>                      # SIGTERM (15)
kill -9 <pid>                   # SIGKILL - 强制终止
kill -HUP <pid>                 # SIGHUP - 重载配置
kill -USR1 <pid>                # SIGUSR1 - 自定义

# pkill - 按名称终止
pkill nginx                     # 终止所有 nginx 进程
pkill -u username               # 终止用户所有进程
pkill -f "java -jar app.jar"    # 按命令行匹配

# killall - 按名称终止
killall nginx                   # 终止所有 nginx 进程
killall -9 nginx                # 强制终止
killall -HUP nginx              # 发送 SIGHUP

# 列出所有信号
kill -l
```

## 进程分析

```bash
# 查看进程文件描述符
ls -la /proc/<pid>/fd           # 列出所有 FD
lsof -p <pid>                   # 详细列表
lsof -p <pid> | wc -l           # FD 数量

# 查看进程内存映射
cat /proc/<pid>/maps            # 内存映射
pmap <pid>                      # 格式化输出
pmap -x <pid>                   # 详细信息

# 查看进程限制
cat /proc/<pid>/limits          # 资源限制

# 查看进程环境变量
cat /proc/<pid>/environ | tr '\0' '\n'

# 查看进程工作目录
ls -la /proc/<pid>/cwd

# 查看进程打开的文件
lsof -p <pid>

# 查看占用端口的进程
lsof -i :80
fuser 80/tcp
ss -tlnp | grep :80

# 系统调用追踪
strace -p <pid>                 # 追踪正在运行的进程
strace -c command               # 统计系统调用次数
strace -e trace=network command # 只追踪网络相关调用
strace -o output.txt command    # 输出到文件

# 库调用追踪
ltrace -p <pid>
ltrace -c command

# 性能分析
perf top                        # 实时热点分析
perf stat command               # 统计计数
perf record -g command          # 记录调用栈
perf report                     # 分析记录数据
```

---

<!-- chunk: 性能调优 -->## 性能调优

## 进程相关性能参数

```bash
# /etc/sysctl.d/99-process.conf

# 进程最大数
kernel.pid_max = 4194303

# 线程最大数
kernel.threads-max = 4194303

# 文件描述符限制
fs.file-max = 2097152

# 每用户最大进程数
# 在 /etc/security/limits.conf 中设置
# * soft nproc 65536
# * hard nproc 65536

# 调度器优化
# 减少调度迁移开销
kernel.sched_migration_cost_ns = 5000000
kernel.sched_autogroup_enabled = 0

# NUMA 优化
kernel.numa_balancing = 0       # 禁用自动 NUMA 平衡（数据库建议禁用）
```

## ulimit 配置

```bash
# 查看当前限制
ulimit -a

# 临时修改
ulimit -n 65536                 # 文件描述符
ulimit -u 65536                 # 用户进程数
ulimit -m unlimited             # 内存

# 永久配置 /etc/security/limits.conf
*       soft    nofile    65536
*       hard    nofile    65536
*       soft    nproc     65536
*       hard    nproc     65536
root    soft    nofile    65536
root    hard    nofile    65536

# systemd 服务级限制
# /etc/systemd/system/myapp.service
[Service]
LimitNOFILE=65536
LimitNPROC=65536
LimitMEMLOCK=infinity
```

---

<!-- chunk: 安全加固 -->## 安全加固

## 进程安全相关配置

```bash
# 内核安全参数
kernel.dmesg_restrict = 1       # 限制非特权用户查看 dmesg
kernel.kptr_restrict = 2        # 限制内核地址暴露
kernel.yama.ptrace_scope = 2    # 限制 ptrace 调试

# 进程审计
# 审计所有进程执行
auditctl -a always,exit -F arch=b64 -S execve -k process_exec
auditctl -a always,exit -F arch=b32 -S execve -k process_exec

# 查看审计日志
ausearch -k process_exec | tail -20

# seccomp - 系统调用过滤
# 查看进程 seccomp 状态
cat /proc/<pid>/status | grep Seccomp
# 0 = 禁用, 1 = 严格, 2 = 过滤

# capabilities - 权限细分
# 查看进程 capabilities
cat /proc/<pid>/status | grep Cap
getpcaps <pid>

# 查看文件 capabilities
getcap /usr/bin/ping
```

---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

## 容器进程管理

在 Kubernetes 中，每个容器都有自己的 PID 命名空间。容器的 entrypoint 进程通常成为该命名空间中的 PID 1，这与 systemd 在主机上作为 PID 1 的角色类似。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入容器查看进程
kubectl exec -it <pod> -- ps aux

# 进入容器的网络命名空间
# 方法 1: 通过 docker
docker inspect --format '{{.State.Pid}}' <container_id>
nsenter --target <pid> --net /bin/bash

# 方法 2: 通过 crictl
crictl inspect <container_id> | jq .info.pid
nsenter --target <pid> --net /bin/bash

# 方法 3: 直接通过 /proc
PID=$(cat /proc/<pid>/task/<tid>/children 2>/dev/null || echo "N/A")
```
## Pod 生命周期与信号

Kubernetes 通过信号控制 Pod 的生命周期：

1. **启动**: kubelet 调用 CRI 运行时创建容器，容器 entrypoint 成为 PID 1
2. **运行**: 容器进程在 cgroup 限制下运行，受 OOM Killer 监控
3. **终止**: kubelet 发送 SIGTERM，等待 terminationGracePeriodSeconds，超时后 SIGKILL
4. **状态**: Pod 的 ContainerStatus 中会记录 LastTerminationState，包括 OOM 和退出码

常见退出码含义：

| 退出码 | 含义 | 原因 |
|:---:|:---|:---|
| 0 | 正常退出 | 进程完成工作后退出 |
| 1 | 应用错误 | 应用程序错误 |
| 137 | SIGKILL (128+9) | OOM Killed 或被强制终止 |
| 139 | 段错误 (128+11) | SIGSEGV，内存访问违规 |
| 143 | SIGTERM (128+15) | 正常终止信号 |
| 255 | 退出码超出范围 | 容器异常退出 |

---

<!-- chunk: 最佳实践 -->## 最佳实践

## 进程管理最佳实践

1. **PID 1 进程必须是信号转发器**: 容器中 PID 1 进程必须正确处理 SIGTERM 信号。如果使用 shell 脚本作为 entrypoint，确保使用 `exec` 替换进程

```dockerfile
# 错误: shell 作为 PID 1，不会转发 SIGTERM
ENTRYPOINT ["sh", "-c", "java -jar app.jar"]

# 正确: 应用直接作为 PID 1
ENTRYPOINT ["java", "-jar", "app.jar"]

# 正确: 使用 tini/dumb-init 作为 init 进程
ENTRYPOINT ["tini", "--", "java", "-jar", "app.jar"]
```

2. **设置合理的资源限制**: 始终为容器设置 resources.limits 和 resources.requests

```yaml
resources:
  requests:
    cpu: "100m"
    memory: "128Mi"
  limits:
    cpu: "500m"
    memory: "512Mi"
```

3. **避免僵尸进程**: 确保 PID 1 进程能正确回收子进程，使用 tini 或 dumb-init

4. **配置 preStop hook**: 在容器终止前执行清理操作

```yaml
lifecycle:
  preStop:
    exec:
      command: ["/bin/sh", "-c", "sleep 5"]
```

5. **监控 OOM 事件**: 设置 Event 监控，及时发现 OOMKilled

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get events --field-selector reason=OOMKilling
```
---

<!-- chunk: 故障排查 -->## 故障排查

## 常见进程问题诊断

```bash
# 僵尸进程处理
# 1. 查找僵尸进程
ps aux | awk '$8=="Z"'
ps -eo pid,ppid,stat,cmd | grep "Z"

# 2. 查看僵尸进程数量
ps aux | awk '$8=="Z"' | wc -l

# 3. 找到僵尸进程的父进程
ps -o ppid= -p <zombie_pid>

# 4. 终止父进程（如果无法修复）
kill -SIGTERM <parent_pid>

# 进程假死 (D 状态)
# 1. 查找 D 状态进程
ps aux | awk '$8=="D"'
ps -eo pid,stat,cmd | grep " D"

# 2. 查看进程等待的资源
cat /proc/<pid>/wchan

# 3. 如果 D 状态持续很长时间，通常意味着存储有问题
# 检查磁盘健康状况
smartctl -a /dev/sda
iostat -xz 1
```

## 容器进程故障排查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Pod 处于 CrashLoopBackOff
kubectl describe pod <pod>          # 查看 Events 和 Last State
kubectl logs <pod> --previous       # 查看上一次容器的日志

# OOMKilled 诊断
kubectl describe pod <pod> | grep -A5 "Last State"
# 查看 cgroup 内存使用
cat /sys/fs/cgroup/kubepods/.../memory.current
cat /sys/fs/cgroup/kubepods/.../memory.max

# 进程在容器内不可见
kubectl exec -it <pod> -- ps aux
kubectl exec -it <pod> -- ls /proc

# 在宿主机上查看容器进程
docker top <container_id>
crictl ps
```
---

<!-- chunk: 相关文档 -->## 相关文档

- [01-linux-system-architecture](./01-linux-system-architecture.md) - 系统架构
- [06-linux-performance-tuning](./06-linux-performance-tuning.md) - 性能调优
- [08-linux-container-fundamentals](./08-linux-container-fundamentals.md) - 容器基础

---

**维护者**: Allen Galler (allengaller@gmail.com) | **许可证**: MIT

## See Also

- 99-linux-commands-reference
- 01-linux-system-architecture
- 03-linux-filesystem-deep-dive
- 04-linux-networking-configuration

```

<!-- risk-assessed -->

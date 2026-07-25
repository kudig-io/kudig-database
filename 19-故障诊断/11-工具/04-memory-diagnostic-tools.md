---
title: K8s 内存诊断工具集
description: 'valgrind/mtrace 内存泄漏检测、ASan 容器集成、pmap/smaps 进程内存分析、/proc/meminfo 解读、OOM Killer 日志分析、cgroup 内存限制排查、内存碎片化诊断'
summary: 'valgrind/mtrace 内存泄漏检测、ASan 容器集成、pmap/smaps 进程内存分析、/proc/meminfo 解读、OOM Killer 日志分析、cgroup 内存限制排查、内存碎片化诊断'
category: troubleshooting-diagnostics
tags:
- memory-diagnostic
- valgrind
- asan
- oom-killer
- cgroup-memory
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- K8s 内存诊断工具 是什么
- 如何排查 OOM Killer
- 如何检测内存泄漏
trigger_keywords:
- 内存泄漏
- OOM Killer
- valgrind
- ASan
- pmap
- cgroup 内存
prerequisites:
- kubectl-basics
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


# K8s 内存诊断工具集

## 概述

Kubernetes 容器环境中的内存问题分为三类：应用内存泄漏、OOM Killer 终止、cgroup 限制导致的异常。本文档覆盖从系统级到应用级的完整内存诊断工具链。

```
内存问题诊断路径:

容器 OOMKilled?
  ├─ 是 → OOM Killer 日志分析（第5节）
  │        → cgroup 内存限制排查（第6节）
  │        → 应用内存泄漏检测（第1-2节）
  └─ 否 → 内存使用异常增长?
           ├─ 是 → pmap/smaps 分析（第3节）
           │        → valgrind/ASan 检测（第1-2节）
           └─ 否 → 系统内存压力?
                    → /proc/meminfo 分析（第4节）
                    → 内存碎片化诊断（第7节）
```

## 1. valgrind/mtrace 内存泄漏检测

### 1.1 valgrind 基础使用

```bash
# valgrind 是最强大的内存调试工具，支持 C/C++ 程序
# 在 K8s 中使用需要注入调试容器或修改镜像

# 在容器内安装 valgrind
# Debian/Ubuntu:
apt-get update && apt-get install -y valgrind

# Alpine:
apk add valgrind

# 基本内存泄漏检测
valgrind --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --verbose \
  --log-file=/tmp/valgrind.log \
  ./my-application

# 只检测明确的内存泄漏（忽略 still reachable）
valgrind --leak-check=full \
  --show-leak-kinds=definite,indirect,possible \
  --errors-for-leak-kinds=definite,indirect \
  ./my-application
```

### 1.2 valgrind 输出解读

```
==12345== LEAK SUMMARY:
==12345==    definitely lost: 1,024 bytes in 2 blocks   ← 明确泄漏，必须修复
==12345==    indirectly lost: 512 bytes in 4 blocks     ← 间接泄漏（结构体内的指针）
==12345==      possibly lost: 256 bytes in 1 blocks     ← 可能泄漏，需确认
==12345==    still reachable: 0 bytes in 0 blocks       ← 仍可达，通常不是问题
==12345==         suppressed: 0 bytes in 0 blocks       ← 被抑制的错误

# 泄漏类型说明:
# definitely lost:   没有任何指针指向这块内存
# indirectly lost:   指向这块内存的指针本身也泄漏了
# possibly lost:     有指针指向这块内存的中间位置（不是起始位置）
# still reachable:   程序结束时仍有指针指向，但未释放（通常可忽略）
```

### 1.3 mtrace 轻量级检测

```bash
# mtrace 是 glibc 内置的内存追踪工具，比 valgrind 轻量
# 适合生产环境快速检测

# 在代码中启用 mtrace
# #include <mcheck.h>
# mtrace();  // 在 main() 开头调用

# 设置 mtrace 输出文件
export MALLOC_TRACE=/tmp/mtrace.log

# 运行程序
./my-application

# 分析 mtrace 日志
mtrace ./my-application /tmp/mtrace.log
```

### 1.4 K8s 中使用 valgrind

```yaml
# 方法 1: 使用 ephemeral 容器注入 valgrind
apiVersion: v1
kind: Pod
metadata:
  name: my-c-app
spec:
  containers:
  - name: app
    image: my-c-app:latest
    command: ["./my-app"]
    # 必须禁用 ASLR 以便 valgrind 正常工作
    securityContext:
      capabilities:
        add: ["SYS_PTRACE"]
---
# 注入调试容器
# kubectl debug -it pod/my-c-app --image=ubuntu:22.04 --target=app
# apt-get install valgrind
# valgrind --leak-check=full --pid=<pid>

# 方法 2: 构建包含 valgrind 的调试镜像
# Dockerfile.debug:
# FROM my-c-app:latest
# RUN apt-get update && apt-get install -y valgrind
# ENTRYPOINT ["valgrind", "--leak-check=full", "--log-file=/tmp/valgrind.log", "./my-app"]
```

## 2. AddressSanitizer (ASan) 容器集成

### 2.1 ASan 概述

```bash
# ASan (AddressSanitizer) 是编译器级别的内存错误检测工具
# 比 valgrind 快 2-5 倍，但需要重新编译

# 支持的错误类型:
# - 堆缓冲区溢出 (heap-buffer-overflow)
# - 栈缓冲区溢出 (stack-buffer-overflow)
# - 全局缓冲区溢出 (global-buffer-overflow)
# - 使用后释放 (use-after-free)
# - 使用超出作用域的栈变量 (use-after-scope)
# - 双重释放 (double-free)
# - 内存泄漏 (leak)
```

### 2.2 编译时启用 ASan

```bash
# C/C++ 编译时启用 ASan
# GCC/Clang:
gcc -fsanitize=address -fno-omit-frame-pointer -g -O1 my-app.c -o my-app
clang -fsanitize=address -fno-omit-frame-pointer -g -O1 my-app.c -o my-app

# Go (1.18+):
# Go 默认使用自己的内存安全检查，ASan 支持有限
# 但可以通过 CGO 启用
CGO_ENABLED=1 go build -asan -o my-app

# Rust:
RUSTFLAGS="-Z sanitizer=address" cargo build --target x86_64-unknown-linux-gnu
```

### 2.3 K8s 中使用 ASan

```yaml
# 构建 ASan 调试镜像
# Dockerfile.asan:
# FROM ubuntu:22.04
# RUN apt-get update && apt-get install -y build-essential
# COPY my-app.c /app/
# RUN gcc -fsanitize=address -fno-omit-frame-pointer -g -O1 /app/my-app.c -o /app/my-app
# ENTRYPOINT ["/app/my-app"]

apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-asan
spec:
  template:
    spec:
      containers:
      - name: app
        image: my-app:asan-debug
        env:
        # ASan 运行时选项
        - name: ASAN_OPTIONS
          value: >-
            detect_leaks=1
            halt_on_error=0
            log_path=/tmp/asan.log
            max_malloc_fill_size=4096
            quarantine_size_mb=64
        - name: LSAN_OPTIONS
          value: >-
            log_path=/tmp/lsan.log
            suppressions=/tmp/lsan-suppressions.txt
        volumeMounts:
        - name: asan-logs
          mountPath: /tmp
        securityContext:
          # ASan 需要以下权限
          capabilities:
            add: ["SYS_PTRACE"]
          # 禁用 seccomp（ASan 使用的信号可能被阻止）
          seccompProfile:
            type: Unconfined
      volumes:
      - name: asan-logs
        emptyDir:
          sizeLimit: 500Mi
```

### 2.4 ASan 日志分析

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ASan 日志格式
# ==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000010
# READ of size 4 at 0x602000000010 thread T0
#     #0 0x4a3c2f in process_data my-app.c:42
#     #1 0x4a3e1a in main my-app.c:58

# 拷贝 ASan 日志
kubectl cp my-namespace/my-app-asan-xxx:/tmp/asan.log.12345 ./asan.log

# 常见错误模式:
# 1. heap-buffer-overflow: 数组越界访问
# 2. use-after-free: 使用已释放的内存
# 3. double-free: 同一块内存释放两次
# 4. stack-buffer-overflow: 栈上缓冲区溢出
# 5. memory-leak: 函数退出时未释放分配的内存
```
## 3. pmap/smaps 进程内存分析

### 3.1 pmap 基础

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# pmap 显示进程的内存映射（Memory Map）
# 用于分析进程的内存分布和碎片情况

# 查看进程内存映射
kubectl exec -it pod/my-app -- pmap -x 1

# 输出示例:
# Address           Kbytes     RSS   Dirty Mode  Mapping
# 0000000000400000     892     424       0 r-x-- my-app
# 00000000006de000      12      12      12 rw--- my-app
# 00000000006e1000     136      36      36 rw---   [ anon ]
# 00007f1234567000    1024     512       0 r-x-- libc-2.31.so
# ...
# total kB         1234567  567890   12345

# 关键指标:
# Kbytes: 虚拟内存大小
# RSS:    实际使用的物理内存（Resident Set Size）
# Dirty:  被修改的内存页（需要写回磁盘的）
```
### 3.2 smaps 详细分析

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# smaps 提供比 pmap 更详细的内存信息
# 每个内存区域都有详细的统计

# 查看 smaps 摘要
kubectl exec -it pod/my-app -- cat /proc/1/smaps_rollup

# 输出示例:
# Rss:              567890 kB    ← 总物理内存
# Pss:              456789 kB    ← 按比例分摊的物理内存（共享内存按比例分配）
# Shared_Clean:     123456 kB    ← 共享的干净页
# Shared_Dirty:      12345 kB    ← 共享的脏页
# Private_Clean:    234567 kB    ← 私有的干净页
# Private_Dirty:    197522 kB    ← 私有的脏页
# Referenced:       500000 kB    ← 被访问过的页
# Anonymous:        400000 kB    ← 匿名页（堆/栈）
# LazyFree:               0 kB    ← 延迟释放的页
# AnonHugePages:     50000 kB    ← 透明大页
# ShmemPmdMapped:         0 kB    ← 共享内存大页映射
# Shared_Hugetlb:         0 kB    ← 共享大页
# Private_Hugetlb:        0 kB    ← 私有大页
# Swap:                   0 kB    ← 交换出去的内存
# SwapPss:                0 kB    ← 按比例分摊的交换内存
# Locked:                 0 kB    ← 锁定的内存

# 查看详细的 smaps（每个内存区域）
kubectl exec -it pod/my-app -- cat /proc/1/smaps
```
### 3.3 smaps 分析脚本

```bash
#!/bin/bash
# analyze-memory.sh - 分析进程内存分布
# 用法: ./analyze-memory.sh <pid>

PID=$1

echo "=== 进程 $PID 内存概览 ==="
pmap -x $PID | tail -1

echo ""
echo "=== 按类型统计 ==="
cat /proc/$PID/smaps | awk '
/^[0-9a-f]/ {
    region = $NF
    if (region == "") region = "[anon]"
}
/^Rss:/ { rss[region] += $2 }
/^Pss:/ { pss[region] += $2 }
/^Private_Dirty:/ { pd[region] += $2 }
END {
    printf "%-30s %10s %10s %10s\n", "Region", "RSS(kB)", "PSS(kB)", "Priv_Dirty(kB)"
    printf "%-30s %10s %10s %10s\n", "------", "--------", "--------", "-------------"
    for (r in rss) {
        printf "%-30s %10d %10d %10d\n", r, rss[r], pss[r], pd[r]
    }
}' | sort -t' ' -k2 -rn

echo ""
echo "=== 堆内存统计 ==="
cat /proc/$PID/smaps | awk '
/^\[heap\]/ { in_heap = 1 }
in_heap && /^Rss:/ { heap_rss = $2 }
in_heap && /^Size:/ { heap_size = $2 }
in_heap && /^Private_Dirty:/ { heap_dirty = $2 }
/^[0-9a-f]/ && !/^\[heap\]/ { in_heap = 0 }
END {
    printf "Heap Size:  %d kB\n", heap_size
    printf "Heap RSS:   %d kB\n", heap_rss
    printf "Heap Dirty: %d kB\n", heap_dirty
}'

echo ""
echo "=== 共享库内存 ==="
cat /proc/$PID/smaps | awk '
/\.so/ { in_lib = 1; lib = $NF }
in_lib && /^Pss:/ { pss[lib] += $2 }
!/\.so/ { in_lib = 0 }
END {
    for (l in pss) {
        printf "%10d kB  %s\n", pss[l], l
    }
}' | sort -rn | head -10
```

## 4. /proc/meminfo 解读

### 4.1 关键字段说明

```bash
# 查看节点内存信息
cat /proc/meminfo

# 关键字段解读:
# MemTotal:       16384000 kB    ← 总物理内存
# MemFree:          512000 kB    ← 完全空闲的内存
# MemAvailable:    8192000 kB    ← 可用内存（含可回收的缓存）
# Buffers:          128000 kB    ← 块设备缓冲区
# Cached:          4096000 kB    ← 页缓存（文件缓存）
# SwapCached:            0 kB    ← 被交换出后又读入的缓存
# Active:          6144000 kB    ← 最近使用的内存
# Inactive:        2048000 kB    ← 最近未使用的内存
# Active(anon):    3072000 kB    ← 活跃的匿名页（堆/栈）
# Inactive(anon):   512000 kB    ← 非活跃的匿名页
# Active(file):    3072000 kB    ← 活跃的文件页
# Inactive(file):  1536000 kB    ← 非活跃的文件页
# Unevictable:           0 kB    ← 不可回收的内存（mlock）
# Mlocked:               0 kB    ← 被锁定的内存
# SwapTotal:             0 kB    ← 交换空间总量
# SwapFree:              0 kB    ← 交换空间剩余
# Dirty:              8192 kB    ← 等待写回磁盘的脏页
# Writeback:             0 kB    ← 正在写回磁盘的页
# AnonPages:       3584000 kB    ← 匿名页（进程堆/栈）
# Mapped:           512000 kB    ← mmap 映射的内存
# Shmem:            256000 kB    ← 共享内存（tmpfs/shmem）
# Slab:             384000 kB    ← 内核 slab 分配器
# SReclaimable:     256000 kB    ← 可回收的 slab
# SUnreclaim:       128000 kB    ← 不可回收的 slab
# KernelStack:       32000 kB    ← 内核栈
# PageTables:        16000 kB    ← 页表
# NFS_Unstable:          0 kB    ← NFS 不稳定的页
# Bounce:                0 kB    ← 弹跳缓冲区
# WritebackTmp:          0 kB    ← 临时写回缓冲
# CommitLimit:     8192000 kB    ← 内存提交限制
# Committed_AS:    6144000 kB    ← 已提交的内存
# VmallocTotal:  34359738367 kB  ← vmalloc 总空间
# VmallocUsed:       16384 kB    ← vmalloc 已使用
# HugePages_Total:       0       ← 大页总数
# HugePages_Free:        0       ← 大页剩余
# Hugepagesize:       2048 kB    ← 大页大小
```

### 4.2 内存使用分析公式

```bash
# 实际可用内存（最准确的指标）
可用内存 = MemAvailable

# 如果 MemAvailable 不可用（旧内核），使用以下公式:
可用内存 ≈ MemFree + Buffers + Cached - (Dirty + Writeback)

# 进程实际使用的物理内存
进程 RSS = sum(/proc/<pid>/smaps 中的 Rss)

# 系统内存压力指标
内存压力 = 1 - (MemAvailable / MemTotal)
# < 0.5: 正常
# 0.5-0.8: 警告
# > 0.8: 严重

# 页缓存命中率（间接指标）
缓存占比 = Cached / MemTotal
# > 0.3: 正常（缓存充足）
# < 0.1: 内存紧张（缓存不足）
```

## 5. OOM Killer 日志分析

### 5.1 OOM Killer 触发机制

```
OOM Killer 触发条件:

1. 进程申请内存时，系统内存不足
2. 内核尝试回收内存（页缓存、slab、swap）后仍然不足
3. OOM Killer 选择一个进程终止

OOM 评分机制:
  /proc/<pid>/oom_score: 0-1000（越高越容易被杀）
  /proc/<pid>/oom_score_adj: -1000 到 1000（手动调整）

  评分因素:
    - 进程 RSS 大小（主要因素）
    - 进程运行时间（新进程评分更高）
    - oom_score_adj 调整值
    - 是否为 root 进程（root 进程评分略低）

K8s 容器 OOM:
  - 容器内存超过 limits 时触发 cgroup OOM
  - cgroup OOM 优先级高于全局 OOM
  - 被 cgroup OOM 终止的进程状态为 OOMKilled
```

### 5.2 OOM Killer 日志查看

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看内核 OOM 日志
# 方法 1: dmesg
dmesg | grep -i "oom\|out of memory\|killed process"

# 方法 2: journalctl
journalctl -k | grep -i "oom\|out of memory\|killed process"

# 方法 3: /var/log/messages
grep -i "oom\|out of memory\|killed process" /var/log/messages

# K8s 中查看 Pod 的 OOM 事件
kubectl get events --field-selector reason=OOMKilling -n my-namespace

# 查看 Pod 的终止状态
kubectl get pod my-pod -o jsonpath='{.status.containerStatuses[0].lastState.terminated}'

# 查看 Pod 的 OOM 详情
kubectl describe pod my-pod | grep -A 10 "Last State"
```
### 5.3 OOM 日志解读

```
# 典型的 OOM Killer 日志:
[12345.678901] my-app invoked oom-killer: gfp_mask=0x100cca, order=0, oom_score_adj=0
[12345.678902] CPU: 3 PID: 12345 Comm: my-app Not tainted 5.15.0 #1
[12345.678903] Hardware name: VMware, Inc. VMware Virtual Platform/440BX
[12345.678904] Call trace:
[12345.678905]  dump_stack+0x68/0x84
[12345.678906]  dump_header+0x50/0x1e0
[12345.678907]  oom_kill_process+0x200/0x3c0
[12345.678908]  out_of_memory+0x108/0x2c0
[12345.678909]  __alloc_pages+0x8b4/0x940
[12345.678910]  ...
[12345.678911] memory: usage 512000kB, limit 512000kB, failcnt 1234     ← cgroup 内存限制
[12345.678912] memory+swap: usage 512000kB, limit 9007199254740988kB, failcnt 0
[12345.678913] oom-kill:constraint=CONSTRAINT_MEMCG,nodemask=(null),task=my-app,pid=12345,uid=1000
[12345.678914] Memory cgroup out of memory: Killed process 12345 (my-app)    ← 实际被杀的进程

# 关键信息:
# 1. 内存使用达到限制 (usage == limit)
# 2. 被杀的进程是 my-app (pid=12345)
# 3. 触发原因是 cgroup 内存限制 (CONSTRAINT_MEMCG)
```

### 5.4 OOM 预防策略

```bash
# 1. 合理设置容器内存限制
# 不要设置过低的 limits（避免频繁 OOM）
# 不要设置过高的 limits（避免资源浪费）
# 推荐: limits = 1.5 * 正常使用量

# 2. 设置 oom_score_adj（K8s 自动处理）
# Guaranteed QoS: oom_score_adj = -997
# Burstable QoS: oom_score_adj = 根据 request 计算
# BestEffort QoS: oom_score_adj = 1000

# 3. 使用 Pod Disruption Budget 保护关键服务
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
spec:
  minAvailable: 1
  selector:
    matchLabels:
      app: my-app

# 4. 启用 Pod 重启策略
# restartPolicy: Always（默认）
# 确保 OOM 后 Pod 自动重启
```

## 6. cgroup 内存限制排查

### 6.1 cgroup 内存信息查看

```bash
# cgroup v1 路径
# 查看容器内存限制
cat /sys/fs/cgroup/memory/memory.limit_in_bytes

# 查看容器内存使用
cat /sys/fs/cgroup/memory/memory.usage_in_bytes

# 查看容器内存统计
cat /sys/fs/cgroup/memory/memory.stat

# 查看容器内存峰值
cat /sys/fs/cgroup/memory/memory.max_usage_in_bytes

# 查看 OOM 计数器
cat /sys/fs/cgroup/memory/memory.failcnt

# cgroup v2 路径（K8s 1.25+ 推荐）
cat /sys/fs/cgroup/memory.max
cat /sys/fs/cgroup/memory.current
cat /sys/fs/cgroup/memory.stat
cat /sys/fs/cgroup/memory.peak
cat /sys/fs/cgroup/memory.events
```

### 6.2 K8s 中查看 cgroup 信息

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 方法 1: 通过 kubectl exec
kubectl exec -it pod/my-app -- cat /sys/fs/cgroup/memory/memory.limit_in_bytes
kubectl exec -it pod/my-app -- cat /sys/fs/cgroup/memory/memory.usage_in_bytes

# 方法 2: 通过 cAdvisor/metrics
kubectl get --raw /api/v1/nodes/my-node/proxy/metrics/cadvisor | grep container_memory

# 方法 3: 通过 describe
kubectl describe pod my-app | grep -A 5 "Limits"

# 方法 4: 查看 cgroup 挂载信息
kubectl exec -it pod/my-app -- mount | grep cgroup
kubectl exec -it pod/my-app -- cat /proc/1/cgroup
```
### 6.3 cgroup 内存统计解读

```bash
# memory.stat 字段说明:
# cache                - 页缓存
# rss                  - 匿名内存（堆/栈）
# mapped_file          - mmap 映射的文件
# pgpgin               - 从磁盘读入的页数
# pgpgout              - 写出到磁盘的页数
# pgfault              - 缺页次数
# pgmajfault           - 主要缺页次数（需要磁盘 I/O）
# inactive_anon        - 非活跃匿名页
# active_anon          - 活跃匿名页
# inactive_file        - 非活跃文件页
# active_file          - 活跃文件页
# unevictable          - 不可回收的内存
# hierarchical_memory_limit - cgroup 内存限制
# hierarchical_memsw_limit  - cgroup 内存+swap 限制

# 计算容器内存使用率
LIMIT=$(cat /sys/fs/cgroup/memory/memory.limit_in_bytes)
USAGE=$(cat /sys/fs/cgroup/memory/memory.usage_in_bytes)
echo "内存使用率: $(echo "scale=2; $USAGE * 100 / $LIMIT" | bc)%"
```

## 7. 内存碎片化诊断

### 7.1 内存碎片化指标

```bash
# 查看内存碎片化状态
cat /proc/buddyinfo

# 输出示例:
# Node 0, zone      DMA      1      1      0      1      1      1      0      0      1      1      3
# Node 0, zone    DMA32   1234    567    234    123     56     23     12      5      2      1      0
# Node 0, zone   Normal  12345   5678   2345   1234    567    234    123     56     23     12      5

# 每列代表 2^N 个连续页:
# 列 0: 2^0 = 1 页 (4KB)
# 列 1: 2^1 = 2 页 (8KB)
# 列 2: 2^2 = 4 页 (16KB)
# ...
# 列 10: 2^10 = 1024 页 (4MB)

# 碎片化评估:
# 如果大量内存集中在小 order（0-3），说明碎片化严重
# 如果大 order（8-10）数量充足，说明内存连续性好
```

### 7.2 碎片化指标计算

```bash
# 计算碎片化指数
# 碎片化指数 = sum(每个order的空闲页 * 2^order) / 总空闲页
# 指数越高，碎片化越轻

cat /proc/buddyinfo | awk '
{
    zone = $4
    total = 0
    weighted = 0
    for (i = 5; i <= NF; i++) {
        order = i - 5
        free = $i
        total += free
        weighted += free * (2 ^ order)
    }
    if (total > 0) {
        fragmentation = 1 - (total / weighted)
        printf "%-10s 碎片化指数: %.4f (0=完全碎片化, 1=完全连续)\n", zone, fragmentation
    }
}'

# 查看 compaction 统计
cat /proc/vmstat | grep compact

# 查看大页分配失败次数
cat /proc/vmstat | grep thp_fault_alloc
cat /proc/vmstat | grep thp_fault_fallback
```

### 7.3 碎片化缓解措施

```bash
# 1. 手动触发内存整理（需要 root）
echo 1 > /proc/sys/vm/compact_memory

# 2. 调整内核参数
# vm.min_free_kbytes: 最小空闲内存（影响碎片整理时机）
echo 65536 > /proc/sys/vm/min_free_kbytes

# vm.vfs_cache_pressure: 控制 slab 缓存回收倾向
# 默认 100，增大则更积极回收 slab
echo 150 > /proc/sys/vm/vfs_cache_pressure

# vm.overcommit_memory: 内存超分配策略
# 0: 启发式超分配（默认）
# 1: 总是允许超分配
# 2: 不允许超分配（推荐生产环境）
echo 2 > /proc/sys/vm/overcommit_memory

# vm.overcommit_ratio: 超分配比例（仅 overcommit_memory=2 时生效）
echo 80 > /proc/sys/vm/overcommit_ratio

# 3. 启用透明大页（THP）
echo always > /sys/kernel/mm/transparent_hugepage/enabled
echo always > /sys/kernel/mm/transparent_hugepage/defrag

# 4. 使用 hugetlbfs 大页（需要预分配）
# 分配 1024 个 2MB 大页
echo 1024 > /proc/sys/vm/nr_hugepages

# 查看大页状态
grep -i huge /proc/meminfo
```

---

## Related

- 故障诊断/01-resource-troubleshooting/
- [[19-故障诊断/11-工具/03-ebpf-diagnostic-tools|eBPF 诊断工具]]
- [[17-系统基础/05-速查卡/linux.md|Linux 速查卡]]

## See Also

- [Linux Kernel Documentation: OOM Killer](https://www.kernel.org/doc/html/latest/admin-guide/mm/oom.html)
- [Valgrind Documentation](https://valgrind.org/docs/manual/)
- [AddressSanitizer Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)


<!-- risk-assessed -->

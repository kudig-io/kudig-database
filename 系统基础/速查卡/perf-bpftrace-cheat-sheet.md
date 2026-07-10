---
title: perf/bpftrace 速查卡
description: 'perf 常用命令、bpftrace 单行脚本集、容器内 perf 使用、K8s 节点性能分析工作流、火焰图生成'
summary: 'perf 常用命令、bpftrace 单行脚本集、容器内 perf 使用、K8s 节点性能分析工作流、火焰图生成'
category: system-foundation
tags:
- perf
- bpftrace
- flame-graph
- performance-profiling
- ebpf
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
- perf 命令 是什么
- 如何使用 bpftrace 分析性能
- 如何生成火焰图
trigger_keywords:
- perf
- bpftrace
- 火焰图
- flame graph
- CPU profiling
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


# perf/bpftrace 速查卡

## 概述

perf 和 bpftrace 是 Linux 性能分析的两大核心工具。perf 提供 CPU 级硬件事件采样，bpftrace 提供动态追踪能力。两者结合可以覆盖从硬件到应用的全栈性能分析。

```
工具选择决策树:

问题类型？
  ├─ CPU 热点 / 火焰图 → perf record + FlameGraph
  ├─ 函数调用延迟 → perf trace / bpftrace
  ├─ 磁盘 I/O 延迟分布 → bpftrace (biolatency)
  ├─ 网络延迟分布 → bpftrace (tcp latency)
  ├─ 调度延迟 → perf sched / bpftrace
  ├─ 锁竞争 → perf lock / bpftrace
  └─ 系统调用追踪 → perf trace / bpftrace
```

## 1. perf 常用命令

### 1.1 perf stat - 硬件计数器统计

```bash
# 统计程序的硬件事件
perf stat ./my-app

# 统计指定事件
perf stat -e cycles,instructions,cache-misses,cache-references ./my-app

# 统计运行中的进程（按 PID）
perf stat -p 12345 sleep 10

# 统计整个系统（10 秒）
perf stat -a sleep 10

# 统计指定 CPU
perf stat -C 0,1 sleep 10

# 输出 JSON 格式
perf stat -j -a sleep 10

# 常用事件:
# cycles              - CPU 周期数
# instructions        - 指令数
# cache-references    - 缓存访问次数
# cache-misses        - 缓存未命中次数
# branch-instructions - 分支指令数
# branch-misses       - 分支预测失败数
# context-switches    - 上下文切换次数
# cpu-migrations      - CPU 迁移次数
# page-faults         - 缺页次数
```

### 1.2 perf record / perf report - CPU 采样

```bash
# 采样记录（默认按 CPU 周期采样）
perf record -g ./my-app

# 采样运行中的进程
perf record -g -p 12345 sleep 30

# 采样整个系统（30 秒）
perf record -g -a sleep 30

# 指定采样频率（每秒 99 次，避免与时钟同步）
perf record -g -F 99 -a sleep 30

# 指定采样事件
perf record -g -e cycles -a sleep 30

# 采样指定 CPU
perf record -g -C 0,1 sleep 30

# 采样并立即生成报告
perf record -g -a sleep 30 && perf report

# 查看采样报告（交互式）
perf report

# 查看采样报告（文本模式）
perf report --stdio

# 查看特定符号的调用链
perf report --sort=dso,symbol
```

### 1.3 perf top - 实时热点

```bash
# 实时显示系统热点函数
perf top

# 实时显示指定进程的热点
perf top -p 12345

# 指定采样事件
perf top -e cache-misses

# 指定 CPU
perf top -C 0
```

### 1.4 perf trace - 系统调用追踪

```bash
# 追踪系统调用（类似 strace，但开销更低）
perf trace ./my-app

# 追踪运行中的进程
perf trace -p 12345

# 追踪指定系统调用
perf trace -e read,write,open,close ./my-app

# 追踪整个系统
perf trace -a sleep 10

# 统计系统调用延迟
perf trace -s ./my-app

# 追踪并显示调用栈
perf trace --call-graph dwarf ./my-app

# 追踪并显示时间戳
perf trace -T ./my-app
```

### 1.5 perf sched - 调度分析

```bash
# 记录调度事件
perf sched record sleep 10

# 查看调度延迟统计
perf sched latency

# 查看调度时间线
perf sched timehist

# 查看调度映射（哪个任务在哪个 CPU 上运行）
perf sched map

# 查看调度统计
perf sched stat
```

### 1.6 perf lock - 锁分析

```bash
# 记录锁事件
perf lock record sleep 10

# 查看锁竞争统计
perf lock report

# 查看锁等待延迟
perf lock report --sort=wait_total
```

### 1.7 perf mem - 内存访问分析

```bash
# 记录内存访问事件
perf mem record ./my-app

# 查看内存访问统计
perf mem report

# 查看内存访问延迟分布
perf mem report --sort=mem
```

## 2. bpftrace 单行脚本集

### 2.1 CPU 分析

```bash
# 进程 CPU 使用分布（按 PID）
bpftrace -e 'profile:hz:99 { @[pid, comm] = count(); }'

# CPU 上下文切换追踪
bpftrace -e 'tracepoint:sched:sched_switch { @[comm] = count(); }'

# 追踪 CPU 迁移
bpftrace -e 'tracepoint:sched:sched_migrate_task { printf("%s pid=%d %d->%d\n", comm, pid, args->orig_cpu, args->dest_cpu); }'

# 追踪抢占
bpftrace -e 'tracepoint:sched:sched_wakeup { @[comm] = count(); }'

# CPU 频率变化追踪
bpftrace -e 'tracepoint:power:cpu_frequency { printf("cpu%d: %d MHz\n", args->cpu, args->state / 1000); }'

# 追踪 idle 状态
bpftrace -e 'tracepoint:power:cpu_idle { printf("cpu%d: %s\n", args->cpu, args->state == 4294967295 ? "active" : "idle"); }'
```

### 2.2 磁盘 I/O 分析

```bash
# 磁盘 I/O 延迟分布
bpftrace -e 'tracepoint:block:block_rq_complete { @usecs = hist((nsecs - args->alloc_time) / 1000); }'

# 磁盘 I/O 延迟分布（使用 biolatency 思路）
bpftrace -e '
tracepoint:block:block_rq_issue { @start[args->dev, args->sector] = nsecs; }
tracepoint:block:block_rq_complete {
    $start = @start[args->dev, args->sector];
    if ($start) {
        @usecs = hist((nsecs - $start) / 1000);
        delete(@start[args->dev, args->sector]);
    }
}'

# 磁盘 I/O 大小分布
bpftrace -e 'tracepoint:block:block_rq_issue { @bytes = hist(args->bytes); }'

# 追踪特定进程的磁盘 I/O
bpftrace -e 'tracepoint:block:block_rq_issue /pid == 12345/ { printf("%s %d bytes sector=%lld\n", comm, args->bytes, args->sector); }'

# 磁盘 I/O 队列深度
bpftrace -e '
tracepoint:block:block_rq_issue { @qdepth[args->dev] = count(); }
tracepoint:block:block_rq_complete { @qdepth[args->dev]--; }
'
```

### 2.3 网络分析

```bash
# TCP 连接建立追踪
bpftrace -e 'kprobe:tcp_connect { printf("%s -> %s\n", comm, ntop(((struct sock *)arg0)->__sk_common.skc_daddr)); }'

# TCP 连接延迟分布
bpftrace -e '
kprobe:tcp_connect { @start[tid] = nsecs; }
kretprobe:tcp_connect /@start[tid]/ {
    @usecs = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
}'

# TCP 发送字节数统计
bpftrace -e 'kprobe:tcp_sendmsg { @bytes[comm] = sum(arg2); }'

# TCP 接收字节数统计
bpftrace -e 'kprobe:tcp_recvmsg { @bytes[comm] = count(); }'

# TCP 重传统计
bpftrace -e 'tracepoint:tcp:tcp_retransmit_skb { @[comm, pid] = count(); }'

# UDP 发送统计
bpftrace -e 'kprobe:udp_sendmsg { @bytes[comm] = sum(arg2); }'

# DNS 查询追踪
bpftrace -e 'kprobe:udp_sendmsg /arg2 > 0/ { @dns[comm] = count(); }'
```

### 2.4 调度器分析

```bash
# 调度延迟分布（从唤醒到运行）
bpftrace -e '
tracepoint:sched:sched_wakeup { @qtime[args->pid] = nsecs; }
tracepoint:sched:sched_switch /@qtime[args->next_pid]/ {
    @usecs = hist((nsecs - @qtime[args->next_pid]) / 1000);
    delete(@qtime[args->next_pid]);
}'

# 进程运行时间分布
bpftrace -e '
tracepoint:sched:sched_switch {
    if (args->prev_state == 0) {  # Running
        @runtime[comm] = hist((nsecs - @switch_time[pid]) / 1000);
    }
    @switch_time[args->next_pid] = nsecs;
}'

# 追踪 CFS 带宽控制
bpftrace -e 'tracepoint:sched:sched_process_throttle { printf("%s throttled\n", comm); }'

# 追踪 CPU 亲和性设置
bpftrace -e 'tracepoint:sched:sched_setaffinity { printf("%s pid=%d\n", comm, pid); }'
```

### 2.5 文件系统分析

```bash
# 文件读取延迟分布
bpftrace -e '
tracepoint:syscalls:sys_enter_read /pid == 12345/ { @start[tid] = nsecs; }
tracepoint:syscalls:sys_exit_read /@start[tid]/ {
    @usecs = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
}'

# 文件打开追踪
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'

# 文件系统操作统计
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[probe] = count(); }'
```

### 2.6 内存分析

```bash
# 内存分配追踪（kmalloc）
bpftrace -e 'tracepoint:kmem:kmalloc { @bytes[comm] = sum(args->bytes_alloc); }'

# 内存释放追踪
bpftrace -e 'tracepoint:kmem:kfree { @bytes[comm] = sum(args->bytes_alloc); }'

# 页面分配延迟
bpftrace -e '
kprobe:__alloc_pages_nodemask { @start[tid] = nsecs; }
kretprobe:__alloc_pages_nodemask /@start[tid]/ {
    @usecs = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
}'

# OOM 事件追踪
bpftrace -e 'tracepoint:oom:oom_score_adj_update { printf("%s pid=%d adj=%d\n", comm, pid, args->oom_score_adj); }'

# 内存回收追踪
bpftrace -e 'tracepoint:vmscan:mm_shrink_slab_start { printf("%s: %ld bytes\n", comm, args->shrinks); }'
```

## 3. 容器内 perf 使用

### 3.1 权限配置

```yaml
# 方法 1: 使用 privileged 容器（不推荐生产环境）
apiVersion: v1
kind: Pod
metadata:
  name: perf-debug
spec:
  containers:
  - name: perf
    image: ubuntu:22.04
    securityContext:
      privileged: true
    command: ["sleep", "infinity"]

# 方法 2: 使用 SYS_PTRACE 和 PERFMON 能力（推荐）
apiVersion: v1
kind: Pod
metadata:
  name: perf-debug
spec:
  containers:
  - name: perf
    image: ubuntu:22.04
    securityContext:
      capabilities:
        add: ["SYS_PTRACE", "PERFMON"]
    command: ["sleep", "infinity"]

# 方法 3: 使用 ephemeral 容器注入
# kubectl debug -it pod/my-app --image=ubuntu:22.04 --target=my-app
```

### 3.2 容器内 perf 命令

```bash
# 安装 perf
apt-get update && apt-get install -y linux-tools-common linux-tools-$(uname -r)

# 采样容器内的进程
perf record -g -p 1 -- sleep 30

# 采样整个容器（如果使用 PID 命名空间共享）
perf record -g -a -- sleep 30

# 生成火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

### 3.3 容器内 bpftrace

```bash
# bpftrace 需要内核头文件和特权
# 在 ephemeral 容器内使用:

# 安装 bpftrace
apt-get update && apt-get install -y bpftrace

# 追踪容器内的系统调用
bpftrace -e 'tracepoint:syscalls:sys_enter_* /pid == 1/ { @[probe] = count(); }'

# 追踪容器内的网络连接
bpftrace -e 'kprobe:tcp_connect { printf("%s pid=%d\n", comm, pid); }'
```

## 4. K8s 节点性能分析工作流

### 4.1 节点级性能分析

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 使用 perf top 快速定位热点
# 通过 kubectl debug 进入节点
kubectl debug node/worker-1 -it --image=ubuntu:22.04

# 安装 perf
apt-get update && apt-get install -y linux-tools-common linux-tools-generic

# 查看实时热点
perf top

# Step 2: 采样记录（30 秒）
perf record -g -F 99 -a sleep 30

# Step 3: 生成火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > node-flamegraph.svg

# Step 4: 分析报告
perf report --stdio
```
### 4.2 容器级性能分析

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 确定目标容器的 PID
# 方法 A: 通过 crictl（节点上）
crictl ps | grep my-app
crictl inspect <container-id> | grep pid

# 方法 B: 通过 kubectl（Pod 内）
kubectl exec -it pod/my-app -- cat /proc/1/status | grep Pid

# Step 2: 对目标进程采样
perf record -g -F 99 -p <pid> sleep 30

# Step 3: 生成火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > container-flamegraph.svg

# Step 4: 分析
perf report --stdio
```
### 4.3 自动化性能分析脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# k8s-perf-analysis.sh - K8s 节点/容器性能分析
# 用法: ./k8s-perf-analysis.sh [node|container] <target>

MODE=$1
TARGET=$2
DURATION=${3:-30}
OUTPUT_DIR="/tmp/perf-$(date +%Y%m%d-%H%M%S)"

mkdir -p $OUTPUT_DIR

case $MODE in
  node)
    echo "=== 采样节点: $TARGET (${DURATION}s) ==="
    perf record -g -F 99 -a -o $OUTPUT_DIR/perf.data sleep $DURATION
    
    echo "=== 生成火焰图 ==="
    perf script -i $OUTPUT_DIR/perf.data | \
      stackcollapse-perf.pl | \
      flamegraph.pl > $OUTPUT_DIR/flamegraph.svg
    
    echo "=== 生成报告 ==="
    perf report -i $OUTPUT_DIR/perf.data --stdio > $OUTPUT_DIR/report.txt
    ;;
    
  container)
    # 获取容器 PID
    PID=$(crictl inspect $TARGET 2>/dev/null | jq '.info.pid')
    if [ -z "$PID" ]; then
      echo "容器未找到，尝试作为 Pod 名称..."
      PID=$(kubectl get pod $TARGET -o jsonpath='{.status.containerStatuses[0].containerID}' 2>/dev/null)
      if [ -z "$PID" ]; then
        echo "错误: 无法找到目标容器"
        exit 1
      fi
    fi
    
    echo "=== 采样容器 PID: $PID (${DURATION}s) ==="
    perf record -g -F 99 -p $PID -o $OUTPUT_DIR/perf.data sleep $DURATION
    
    echo "=== 生成火焰图 ==="
    perf script -i $OUTPUT_DIR/perf.data | \
      stackcollapse-perf.pl | \
      flamegraph.pl > $OUTPUT_DIR/flamegraph.svg
    ;;
    
  *)
    echo "用法: $0 [node|container] <target> [duration]"
    exit 1
    ;;
esac

echo "=== 输出目录: $OUTPUT_DIR ==="
ls -la $OUTPUT_DIR/
```
## 5. 火焰图生成

### 5.1 FlameGraph 工具安装

```bash
# 克隆 FlameGraph 工具集
git clone https://github.com/brendangregg/FlameGraph.git
cd FlameGraph

# 主要工具:
# stackcollapse-perf.pl  - 将 perf script 输出折叠成一行一个栈
# stackcollapse-bpftrace.pl - 将 bpftrace 输出折叠
# flamegraph.pl          - 生成 SVG 火焰图
# difffolded.pl          - 比较两个火焰图
```

### 5.2 CPU 火焰图

```bash
# 采样
perf record -g -F 99 -a sleep 30

# 生成火焰图（完整流程）
perf script | \
  ./FlameGraph/stackcollapse-perf.pl | \
  ./FlameGraph/flamegraph.pl --title="CPU Flame Graph" \
    --subtitle="30s sample" \
    --width=1200 > cpu-flamegraph.svg

# 生成 CPU 火焰图（按进程分组）
perf script | \
  ./FlameGraph/stackcollapse-perf.pl --pid | \
  ./FlameGraph/flamegraph.pl --title="CPU Flame Graph by PID" > cpu-by-pid.svg

# 生成差分火焰图（对比两次采样）
perf script | ./FlameGraph/stackcollapse-perf.pl > before.folded
# ... 做一些改变 ...
perf script | ./FlameGraph/stackcollapse-perf.pl > after.folded
./FlameGraph/difffolded.pl before.folded after.folded | \
  ./FlameGraph/flamegraph.pl > diff-flamegraph.svg
```

### 5.3 off-CPU 火焰图

```bash
# off-CPU 火焰图显示进程不在 CPU 上运行的时间分布
# 使用 bpftrace 采集 off-CPU 时间

# 采集 off-CPU 栈
bpftrace -e '
tracepoint:sched:sched_switch {
    @offcpu[pid, comm, kstack] = nsecs;
}
tracepoint:sched:sched_wakeup /@offcpu[args->pid]/ {
    $offcpu = @offcpu[args->pid];
    @usecs[kstack(args->pid), comm(args->pid)] = hist((nsecs - $offcpu) / 1000);
    delete(@offcpu[args->pid]);
}' > offcpu.stacks

# 转换为火焰图格式
cat offcpu.stacks | ./FlameGraph/stackcollapse-bpftrace.pl | \
  ./FlameGraph/flamegraph.pl --title="off-CPU Flame Graph" --color=io > offcpu-flamegraph.svg
```

### 5.4 内存分配火焰图

```bash
# 使用 perf 采集内存分配事件
perf record -g -e 'kmem:kmalloc' -a sleep 30

# 生成火焰图
perf script | \
  ./FlameGraph/stackcollapse-perf.pl | \
  ./FlameGraph/flamegraph.pl --title="Memory Allocation Flame Graph" > alloc-flamegraph.svg

# 使用 bpftrace 采集更详细的内存分配
bpftrace -e '
tracepoint:kmem:kmalloc {
    @bytes[kstack] = sum(args->bytes_alloc);
}' > alloc.stacks

cat alloc.stacks | ./FlameGraph/stackcollapse-bpftrace.pl | \
  ./FlameGraph/flamegraph.pl --title="Memory Allocation (bytes)" > alloc-bytes-flamegraph.svg
```

### 5.5 火焰图解读

```
火焰图解读指南:

X 轴: 采样比例（宽度 = 该函数及其子函数的采样占比）
Y 轴: 调用栈深度（底部是根函数，顶部是叶子函数）

关键指标:
  宽度: 越宽 = 该函数消耗的 CPU 时间越多
  颜色: 默认随机色，区分不同函数
        红色: 通常是内核函数
        橙色: 用户态函数

常见模式:
  1. 金字塔形: 正常的调用栈分布
  2. 倒金字塔: 热点函数被大量调用
  3. 平顶: 某个函数是主要瓶颈
  4. 多峰: 多个热点路径

优化建议:
  - 最宽的叶子函数是优化重点
  - 关注用户态 vs 内核态的比例
  - 对比优化前后的火焰图（差分火焰图）
```

## 6. perf/bpftrace 常见问题

### 6.1 权限问题

```bash
# 错误: Permission denied
# 解决方案 1: 使用 root 权限
sudo perf record ...

# 解决方案 2: 调整 kernel.perf_event_paranoid
# 0: 允许所有用户使用 perf
# 1: 允许非 root 用户使用（默认）
# 2: 仅允许 root 用户使用
# 3: 完全禁用 perf
sudo sysctl kernel.perf_event_paranoid=0

# 解决方案 3: K8s 中使用 SYS_PTRACE 和 PERFMON 能力
# 参考 3.1 节
```

### 6.2 符号解析问题

```bash
# 错误: 无法解析符号（显示为地址）
# 解决方案 1: 确保二进制包含调试信息
gcc -g -O2 my-app.c -o my-app

# 解决方案 2: 使用 --dsos 指定符号文件
perf report --dsos=my-app

# 解决方案 3: 使用 perf annotate 查看汇编
perf annotate my_function

# 解决方案 4: 对于 JIT 程序（Java/Node.js），需要使用 perf-map-agent
# Java:
java -XX:+PreserveFramePointer ...
create-java-perf-map.sh <pid>

# Node.js:
node --perf-basic-prof ...
```

### 6.3 bpftrace 内核版本兼容性

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# bpftrace 需要内核 >= 4.9（基本功能）
# 推荐内核 >= 5.4（完整功能）

# 检查内核版本
uname -r

# 检查 bpftrace 支持的特性
bpftrace --info

# 如果 bpftrace 版本过低，使用容器版本
docker run --privileged -it quay.io/iovisor/bpftrace:latest
```
---

## Related

- [[系统基础/速查卡/linux.md|Linux 速查卡]]
- [[故障诊断/工具/03-ebpf-diagnostic-tools|eBPF 诊断工具]]
- [[故障诊断/工具/04-memory-diagnostic-tools|内存诊断工具]]

## See Also

- [Brendan Gregg: perf Examples](https://www.brendangregg.com/perf.html)
- [Brendan Gregg: bpftrace One-Liners](https://www.brendangregg.com/BPF/bpftrace-one-liners.html)
- [FlameGraph GitHub](https://github.com/brendangregg/FlameGraph)
- [bpftrace Reference Guide](https://github.com/bpftrace/bpftrace/blob/master/docs/reference_guide.md)


<!-- risk-assessed -->

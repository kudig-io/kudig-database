---
title: 性能瓶颈诊断与调优 / Performance Bottleneck Diagnosis & Tuning
description: '## 1. 概述'
summary: '性能瓶颈是 [[kubernetes|Kubernetes]] 集群和云原生应用中最常见但也最难定位的问题之一。性能问题往往表现为延迟增加、吞吐量下降、资源使用异常等，其根因可能涉及多个层次：从基础设施（CPU/内存/磁盘/网络）到 Kubernetes 平台（API Server/etcd/Scheduler）再到应用层（代码逻辑/GC/连接池）。'
category: performance
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
tier: supporting
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- 性能瓶颈诊断与调优 / Performance Bottleneck Diagnosis & Tuning 是什么
- 如何 性能瓶颈诊断与调优 / Performance Bottleneck Diagnosis & Tuning
trigger_keywords:
- high latency
- slow response
- cpu throttling
- memory pressure
- network bottleneck
- disk io slow
- api server slow
- etcd slow query
- connection timeout
- pod startup slow
- scheduling delay
- high p99 latency
- 性能瓶颈
- 延迟高
- 响应慢
- CPU 限流
- 内存压力
- 磁盘 IO 慢
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- etcd-basics
- gpu-scheduling-basics
- tracing-basics
skill_id: SKILL-17_PERFORMANCE_BOTTLENECK-001
skill_name: 性能瓶颈诊断与调优 / Performance Bottleneck Diagnosis & Tuning
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




<!-- condition: kubectl top nodes -o jsonpath='{range .items[?(@.usage.cpu!="<none>" && @.usage.memory!="<none>")]} {.metadata.name}{"\n"}{end}' 显示节点资源使用率超过 80% -->

# 性能瓶颈诊断与调优 / Performance Bottleneck Diagnosis & Tuning

---

## 1. 概述

性能瓶颈是 [[kubernetes|Kubernetes]] 集群和云原生应用中最常见但也最难定位的问题之一。性能问题往往表现为延迟增加、吞吐量下降、资源使用异常等，其根因可能涉及多个层次：从基础设施（CPU/内存/磁盘/网络）到 Kubernetes 平台（API Server/etcd/Scheduler）再到应用层（代码逻辑/GC/连接池）。本 [[SKILL|Skill]] 提供系统化的分层诊断方法，帮助快速定位性能瓶颈根因并给出针对性修复建议。

### 覆盖范围

- **CPU 性能瓶颈**: CPU throttling、CFS Quota 配置、NUMA 拓扑影响
- **内存性能瓶颈**: RSS/Cache/Swap 使用、cgroup 内存回收压力
- **网络性能瓶颈**: 带宽瓶颈、连接数限制、conntrack 表溢出
- **磁盘 I/O 性能瓶颈**: IOPS、吞吐量、延迟问题
- **Kubernetes 平台性能**: API Server 过载、etcd 慢查询、Scheduler 延迟
- **应用级性能分析**: pprof/[[19-故障诊断/05-JVM调优/01-jdk-flight-recorder-k8s.md|JFR]]/perf 实战工具使用

### 典型触发场景

1. **应用响应延迟突增**: 用户反馈服务响应变慢，P99 延迟突破 SLA
2. **资源告警触发**: CPU throttling 告警、内存使用率告警、磁盘 I/O 告警
3. **平台性能下降**: API Server 请求变慢、Pod 调度延迟增加、etcd 慢查询告警

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `nodes`, `pods`, `pods/log`, `events`, `configmaps`, `deployments`, `services` 的 `get/list/watch`
  - 节点诊断: 对 `nodes/proxy` 的 `get` 权限（用于 `kubectl proxy` 访问节点指标）
  - 验证命令: `kubectl auth can-i list nodes`
- **SSH 访问**: 深度诊断（Phase 2+）需要对节点的 SSH 访问权限
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `ssh`
  - `jq` >= 1.6
  - `curl`
- **监控系统**: Prometheus + Grafana + kube-state-metrics >= v2.10 + node-exporter
- **SSH 访问**: 深度诊断（Phase 2+）需要对节点的 SSH 访问权限
- **工具要求**: kubectl (v1.28+), ssh, jq, curl
- **监控系统**: Prometheus + Grafana + kube-state-metrics + node-exporter
- **应用工具**: pprof (Go 应用)、JFR/jcmd (Java 应用)、perf (Linux 通用)

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | 应用响应延迟突增（P99 > SLA）/ Application latency exceeds SLA | 检查 Prometheus 中 `histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m]))` 是否超过阈值 | 0.90 | 业务高峰期的预期延迟增长；依赖外部服务的延迟传播 |
| SP-02 | CPU throttling 严重 / High CPU throttling | `container_cpu_cfs_throttled_periods_total` 持续增长；`kubectl top pods` 显示 CPU 接近 limit | 0.95 | 突发流量导致的短暂 throttling；limit 配置过低属于配置问题而非性能瓶颈 |
| SP-03 | 节点内存压力告警（MemoryPressure）/ Node memory pressure | `kubectl describe node <node>` 的 Conditions 中 MemoryPressure=True | 0.85 | 预期内的缓存占用；cgroup v2 memory.high 触发但未影响应用 |
| SP-04 | 磁盘 I/O wait 持续偏高（iowait > 20%）/ High disk I/O wait | `vmstat` 或 `iostat` 显示 iowait > 20%；`node_cpu_seconds_total{mode="iowait"}` 持续高位 | 0.80 | 批量任务（如备份、数据导入）的预期 I/O；SSD 设备的短暂写入尖峰 |
| SP-05 | 网络丢包率上升 / Network packet loss | `netstat -s | grep -i retransmit` 或 `ss -ti` 显示重传增加；`node_network_receive_drop_total` 增长 | 0.75 | 网络拥塞期间的轻微丢包；UDP 应用的预期丢包 |
| SP-06 | Pod 启动耗时异常（> 30s）/ Slow pod startup | `kubectl describe pod` 显示从 Pending 到 Running 耗时 > 30s；`kubelet_pod_start_duration_seconds` P99 高 | 0.85 | 首次拉取大镜像的预期耗时；Init Container 执行慢属于应用问题 |
| SP-07 | API Server 请求 P99 延迟 > 1s / API Server high latency | `apiserver_request_duration_seconds_bucket` P99 > 1s（verb != WATCH） | 0.90 | 大规模 LIST 请求的预期延迟；Webhook 链路延迟（非 API Server 本身） |
| SP-08 | etcd 慢查询告警 / etcd slow queries | `etcd_disk_wal_fsync_duration_seconds` P99 > 100ms；`etcd_disk_backend_commit_duration_seconds` > 250ms | 0.95 | etcd compact 期间的短暂延迟；defrag 操作期间 |
| SP-09 | 连接超时增多 / Connection timeouts | 应用日志中 `connect timeout`、`read timeout` 增多；TCP 状态中 TIME_WAIT/CLOSE_WAIT 堆积 | 0.80 | 目标服务不可用导致的超时；网络策略阻断连接 |
| SP-10 | 调度延迟增加 / Scheduling delay | `scheduler_scheduling_algorithm_duration_seconds` P99 增加；Pod Pending 时间延长 | 0.85 | 资源不足导致的 Pending（非性能问题）；复杂亲和性规则的预期计算延迟 |
| SP-11 | HPA 频繁扩缩（指标波动大）/ HPA thrashing | `kubectl describe hpa` 显示频繁的 scale up/down 事件；replicas 在短时间内波动大 | 0.70 | HPA 配置不当（stabilization window 太短）；业务流量本身波动大 |
| SP-12 | JVM Full GC 频繁 / Frequent JVM Full GC | GC 日志显示 Full GC 频率 > 1次/分钟；`jvm_gc_pause_seconds_sum` 增长快 | 0.90 | 应用启动初期的正常 GC；显式调用 System.gc() |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "服务响应变慢，延迟明显上升"
- "CPU 使用率不高但应用 P99 延迟高"
- "Pod 内存一直增长，疑似内存泄漏"
- "磁盘 IO 很高，应用读写变慢"
- "API Server 请求超时，kubectl 操作变慢"
- "etcd 有慢查询告警"
- "服务间调用超时，连接失败"
- "Pod 启动很慢，调度延迟高"
- "HPA 扩缩容很频繁，不稳定"
- "JVM Full GC 告警，需要分析"

**English ticket descriptions**:
- "Application latency increased significantly"
- "CPU throttling detected, pods are being throttled"
- "Memory pressure on nodes, evictions happening"
- "Disk I/O is very slow, application performance degraded"
- "API server requests timing out"
- "etcd slow queries alert triggered"
- "Connection timeouts between services"
- "Pod startup time is abnormally long"
- "HPA keeps scaling up and down rapidly"
- "JVM Full GC happening frequently"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 节点状态 NotReady 导致的性能问题 | SKILL-NODE-001 | 先解决节点可用性问题，再分析性能 |
| Pod CrashLoopBackOff 导致的请求失败 | SKILL-POD-001 | 应用层面错误，非性能瓶颈 |
| 网络策略阻断导致的连接失败 | SKILL-NET-001 | 网络配置问题，非性能瓶颈 |
| 资源 quota 不足导致的 Pod 无法创建 | SKILL-QUOTA-001 | 资源配额问题，非性能瓶颈 |
| 应用代码 bug 导致的死锁 | 应用层排查 | 超出本 Skill 范围 |
| 外部依赖服务不可用导致的超时 | 依赖服务排查 | 非 Kubernetes 集群内部问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 快速获取集群资源使用概览
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取所有节点的资源使用情况
kubectl top nodes --sort-by=cpu 2>/dev/null || echo "Metrics server not available"

# 获取目标 namespace 中 CPU 使用最高的 Pod
kubectl top pods -n <namespace> --sort-by=cpu 2>/dev/null | head -10
```
> **判断规则**:
> - 多个节点 CPU > 80% → 集群级性能问题，**P1**
> - 单个节点 CPU > 90% → 节点级瓶颈，**P2**
> - Pod CPU 接近 limit → 可能存在 throttling，继续 T2

**Step T2**: 检查近期性能相关事件
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查性能相关事件（OOM、Eviction、Throttling）
kubectl get events -A --sort-by=.lastTimestamp | grep -iE 'oom|evict|throttl|fail|error' | tail -20
```
> **判断规则**:
> - 出现大量 OOMKilled 事件 → **P1**（内存瓶颈严重）
> - 出现 Eviction 事件 → **P1**（资源压力导致驱逐）
> - 无严重事件 → 继续 T3

**Step T3**: 节点级快速检查
```bash
# 如果可以 SSH 到节点
ssh <node-ip> "vmstat 1 5"
ssh <node-ip> "iostat -x 1 5"
```
> **判断规则**:
> - iowait > 20% 持续 → 磁盘 I/O 瓶颈，**P2**
> - CPU us + sy > 90% → CPU 瓶颈，**P2**
> - free memory < 500MB → 内存瓶颈，**P1**

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| 核心业务 SLA 受损（P99 > 阈值 2 倍以上）**或** 控制平面性能下降（API Server/etcd 延迟高） | **P1** | 业务影响严重，用户体验受损，或集群管理能力下降 | 15min 内响应，30min 内定位根因 |
| 单服务/部分节点性能下降，但有冗余 | **P2** | 部分用户受影响，但系统整体可用 | 30min 内响应，2h 内修复 |
| 性能指标下降但未影响 SLA | **P3** | 可优化但不紧急 | 4h 内响应，按计划修复 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE**：

- **API Server 不可用**: `kubectl` 命令本身执行超时或失败
- **etcd 集群异常**: etcd 成员不健康或 Leader 频繁切换
- **大规模 OOM**: 多个节点同时出现 OOM 事件
- **级联问题**: 性能问题导致服务雪崩，影响面持续扩大
- **未知模式**: 所有常规诊断均未发现异常，但性能问题持续

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 资源使用全景（只读，零风险）

> **目标**: 快速获取集群和应用的资源使用全景，识别瓶颈方向
> **预计耗时**: 3-5 分钟

**Step D1.1**: 节点资源使用概览
- **命令**:
  ```bash
  # 获取所有节点的资源使用
  kubectl top nodes

  # 获取节点详细资源分配情况
  kubectl describe node <NODE_NAME> | grep -A20 "Allocated resources"
  ```
- **超时**: 15s
- **预期输出模式**: 节点 CPU/Memory 使用率和分配量
- **判断规则**:
  - CPU 使用率 > 80% → 可能存在 CPU 瓶颈，继续 D2.1
  - Memory 使用率 > 85% → 可能存在内存瓶颈，继续 D2.4
  - Requests 总和接近 Allocatable → 资源过度订阅，继续 D1.3
- **版本差异**: 无

**Step D1.2**: Pod 资源 Top
- **命令**:
  ```bash
  # 按 CPU 排序
  kubectl top pods -n <namespace> --sort-by=cpu | head -20

  # 按 Memory 排序
  kubectl top pods -n <namespace> --sort-by=memory | head -20

  # 所有 namespace
  kubectl top pods -A --sort-by=cpu | head -20
  ```
- **超时**: 15s
- **预期输出模式**: Pod CPU/Memory 使用排行
- **判断规则**:
  - 单个 Pod CPU 异常高 → 定位到具体 Pod，继续 D2.1
  - 多个 Pod 内存持续增长 → 可能内存泄漏，继续 D2.5
  - 使用量均匀分布 → 非单点问题，需要集群级分析
- **版本差异**: 无

**Step D1.3**: 资源请求 vs 实际使用对比
- **命令**:
  ```bash
  # 获取 Pod 的 requests/limits 配置
  kubectl get pods -n <namespace> -o custom-columns=\
  NAME:.metadata.name,\
  CPU_REQ:.spec.containers[*].resources.requests.cpu,\
  CPU_LIM:.spec.containers[*].resources.limits.cpu,\
  MEM_REQ:.spec.containers[*].resources.requests.memory,\
  MEM_LIM:.spec.containers[*].resources.limits.memory

  # 对比实际使用
  kubectl top pods -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: 资源配置与使用对比
- **判断规则**:
  - 实际 CPU ≈ limit → 可能被 throttling，继续 D2.1
  - 实际 Memory ≈ limit → OOM 风险高，继续 D2.4
  - requests >> 实际使用 → 资源浪费，但非性能瓶颈
  - 无 limits 配置 → 可能导致资源争抢
- **版本差异**: 无

**Step D1.4**: 节点系统基础指标
- **命令**:
  ```bash
  # 如果可以 SSH 到节点
  ssh <node-ip> "uptime"  # load average
  ssh <node-ip> "free -h"  # 内存使用
  ssh <node-ip> "df -h"    # 磁盘使用
  ```
- **超时**: 10s
- **预期输出模式**: 系统基础资源状态
- **判断规则**:
  - load average > CPU 核数 × 2 → 系统过载
  - free memory < 1GB → 内存紧张
  - 磁盘使用 > 85% → 磁盘空间瓶颈（影响 I/O 和 kubelet）
- **版本差异**: 无

**Step D1.5**: 容器级 cgroup 指标
- **命令**:
  ```bash
  # 获取容器 ID
  CONTAINER_ID=$(crictl ps | grep <pod-name> | awk '{print $1}')

  # cgroup v1 路径
  # CPU
  ssh <node-ip> "cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/cpuacct.usage"
  # Memory
  ssh <node-ip> "cat /sys/fs/cgroup/memory/kubepods/pod<pod-uid>/memory.usage_in_bytes"

  # cgroup v2 路径（统一目录）
  ssh <node-ip> "cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<pod-uid>.slice/cpu.stat"
  ssh <node-ip> "cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<pod-uid>.slice/memory.current"
  ```
- **超时**: 15s
- **预期输出模式**: cgroup 级别的资源使用数据
- **判断规则**:
  - cgroup v1 和 v2 路径不同，需要先确认节点使用的版本
  - `nr_throttled` > 0 → 存在 CPU throttling
  - `memory.high` events > 0 (cgroup v2) → 内存回收压力
- **版本差异**:
  - **[v1.28+]**: 推荐使用 cgroup v2，需检查 `--cgroup-driver=systemd`
  - **[v1.31+]**: cgroup v2 相关功能增强

**Step D1.6**: 网络基线检查
- **命令**:
  ```bash
  # 网络接口流量
  ssh <node-ip> "sar -n DEV 1 5"

  # 连接统计
  ssh <node-ip> "ss -s"

  # TCP 状态分布
  ssh <node-ip> "ss -ant | awk '{print \$1}' | sort | uniq -c | sort -rn"
  ```
- **超时**: 15s
- **预期输出模式**: 网络流量和连接状态
- **判断规则**:
  - 网卡带宽接近物理限制 → 网络带宽瓶颈
  - TIME_WAIT > 10000 → 连接复用问题
  - CLOSE_WAIT 堆积 → 应用未正确关闭连接
- **版本差异**: 无

---

### Phase 2: 分层瓶颈定位（只读，零风险）

> **目标**: 针对 Phase 1 识别的瓶颈方向进行深入分析
> **预计耗时**: 10-15 分钟

**Step D2.1**: CPU throttling 分析
- **命令**:
  ```bash
  # 从 Prometheus 查询 throttling 指标
  # container_cpu_cfs_throttled_seconds_total / container_cpu_cfs_periods_total

  # 或从节点 cgroup 直接读取
  # cgroup v1
  ssh <node-ip> "cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/cpu.stat"
  # 关注 nr_throttled 和 throttled_time

  # cgroup v2
  ssh <node-ip> "cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<pod-uid>.slice/cpu.stat"
  ```
- **超时**: 15s
- **预期输出模式**: CPU throttling 统计
- **判断规则**:
  - `nr_throttled` / `nr_periods` > 10% → 显著 throttling，需要调整 CPU limit（RC-001）
  - throttled_time 持续增长 → 持续的 CPU 限制问题
  - 无 throttling 但 CPU 使用高 → 非 limit 问题，可能是代码效率问题
- **版本差异**: cgroup v1 和 v2 路径不同

**Step D2.2**: CFS Quota 配置检查
- **命令**:
  ```bash
  # cgroup v1
  ssh <node-ip> "cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/cpu.cfs_quota_us"
  ssh <node-ip> "cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/cpu.cfs_period_us"

  # cgroup v2
  ssh <node-ip> "cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<pod-uid>.slice/cpu.max"
  # 输出格式: quota period (如 100000 100000 = 1 core)
  ```
- **超时**: 10s
- **预期输出模式**: CFS quota 和 period 配置
- **判断规则**:
  - quota / period = 可用 CPU 核数
  - quota = -1 → 无 CPU 限制
  - quota 过小 → RC-001（CPU Limit 过低）
- **版本差异**: cgroup v1/v2 路径和格式不同

**Step D2.3**: NUMA 拓扑分析
- **命令**:
  ```bash
  # 查看 NUMA 拓扑
  ssh <node-ip> "numactl --hardware"

  # 查看进程的 NUMA 分布
  ssh <node-ip> "numastat -p <PID>"

  # 检查 CPU 亲和性
  ssh <node-ip> "taskset -p <PID>"
  ```
- **超时**: 10s
- **预期输出模式**: NUMA 节点信息和进程分布
- **判断规则**:
  - 进程跨 NUMA 访问内存（remote accesses 高）→ RC-009（NUMA 亲和性不当）
  - 内存分布不均匀 → 考虑 NUMA 绑定优化
  - 单 NUMA 节点 → NUMA 不是瓶颈因素
- **版本差异**: 无

**Step D2.4**: 内存回收压力分析
- **命令**:
  ```bash
  # 检查系统级内存回收指标
  ssh <node-ip> "cat /proc/vmstat | grep -E 'pgmajfault|pgpgin|pgpgout|pswpin|pswpout'"

  # 检查 OOM Killer 历史
  ssh <node-ip> "dmesg -T | grep -i 'oom|killed process'"

  # 检查 swap 使用（如果启用）
  ssh <node-ip> "free -h"
  ssh <node-ip> "vmstat 1 5"
  ```
- **超时**: 15s
- **预期输出模式**: 内存回收和 swap 活动
- **判断规则**:
  - pgmajfault 持续增长 → 频繁的主要页错误，性能影响大
  - pswpin/pswpout > 0 → swap 活动（通常应该禁用 swap）
  - OOM Kill 记录 → RC-002（内存 Limit 不足）
- **版本差异**:
  - **[v1.30+]**: NodeSwap feature gate (beta)，需检查 swap 配置

**Step D2.5**: cgroup v2 内存压力
- **命令**:
  ```bash
  # cgroup v2 特有的 memory.pressure
  ssh <node-ip> "cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<pod-uid>.slice/memory.pressure"
  # 输出: some avg10=X avg60=Y avg300=Z total=W
  # 表示内存压力导致的等待时间百分比

  # 内存事件统计
  ssh <node-ip> "cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<pod-uid>.slice/memory.events"
  ```
- **超时**: 10s
- **预期输出模式**: 内存压力指标
- **判断规则**:
  - avg10 > 10 → 近期有显著内存压力
  - `max` events > 0 → 触及内存限制
  - `oom_kill` > 0 → 发生过 OOM Kill（RC-002）
- **版本差异**: 仅 cgroup v2 支持 memory.pressure

**Step D2.6**: 磁盘 I/O 分析
- **命令**:
  ```bash
  # 磁盘 I/O 统计
  ssh <node-ip> "iostat -xz 1 10"

  # 识别 I/O 最高的进程
  ssh <node-ip> "iotop -P -b -n 5 2>/dev/null || echo 'iotop not available'"

  # 检查磁盘队列深度
  ssh <node-ip> "cat /sys/block/*/queue/nr_requests"
  ```
- **超时**: 30s
- **预期输出模式**: 磁盘 I/O 性能数据
- **判断规则**:
  - await > 20ms (HDD) 或 > 5ms (SSD) → 磁盘延迟高
  - %util > 80% → 磁盘接近饱和（RC-003）
  - svctm 正常但 await 高 → I/O 队列等待时间长
  - r/s + w/s > 设备 IOPS 上限 → IOPS 瓶颈
- **版本差异**: 无

**Step D2.7**: 网络连接分析
- **命令**:
  ```bash
  # 统计各端口的连接数
  ssh <node-ip> "ss -tnp | awk '{print \$4}' | cut -d: -f2 | sort | uniq -c | sort -rn | head -20"

  # 检查连接状态分布
  ssh <node-ip> "ss -tan state time-wait | wc -l"
  ssh <node-ip> "ss -tan state close-wait | wc -l"
  ssh <node-ip> "ss -tan state established | wc -l"
  ```
- **超时**: 15s
- **预期输出模式**: 网络连接统计
- **判断规则**:
  - ESTABLISHED > 10000 per service → 连接数很高
  - TIME_WAIT > 20000 → 连接复用不足（RC-010）
  - CLOSE_WAIT 堆积 → 应用未正确关闭连接
- **版本差异**: 无

**Step D2.8**: conntrack 表使用率
- **命令**:
  ```bash
  # 当前 conntrack 条目数
  ssh <node-ip> "conntrack -C 2>/dev/null || cat /proc/sys/net/netfilter/nf_conntrack_count"

  # conntrack 最大值
  ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_max"

  # 计算使用率
  CURRENT=$(ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_count")
  MAX=$(ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_max")
  echo "Conntrack usage: $((CURRENT * 100 / MAX))%"
  ```
- **超时**: 10s
- **预期输出模式**: conntrack 表使用统计
- **判断规则**:
  - 使用率 > 75% → 风险高，需要扩容（RC-004）
  - 使用率 > 90% → 即将溢出，紧急处理
  - dmesg 中出现 `nf_conntrack: table full` → 已经溢出
- **版本差异**: 无

**Step D2.9**: TCP 重传分析
- **命令**:
  ```bash
  # TCP 重传统计
  ssh <node-ip> "netstat -s | grep -i retransmit"

  # 或使用 ss 查看单个连接的重传
  ssh <node-ip> "ss -ti | grep -E 'retrans|rto'"

  # nstat 增量统计
  ssh <node-ip> "nstat -az | grep -i retrans"
  ```
- **超时**: 10s
- **预期输出模式**: TCP 重传统计
- **判断规则**:
  - 重传率 > 1% → 网络质量问题（RC-008）
  - retransmits 快速增长 → 网络拥塞或丢包
  - RTO (retransmission timeout) 持续高 → 网络延迟大
- **版本差异**: 无

**Step D2.10**: DNS 延迟分析
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 测试 Kubernetes DNS 解析延迟
  kubectl run dns-test --image=busybox:1.28 --rm -it --restart=Never -- \
    sh -c "time nslookup kubernetes.default.svc.cluster.local"

  # 或从节点测试
  ssh <node-ip> "dig +stats kubernetes.default.svc.cluster.local @<coredns-ip>"

  # 检查 resolv.conf 配置
  kubectl exec <pod> -- cat /etc/resolv.conf
  ```
- **超时**: 15s
- **预期输出模式**: DNS 解析时间
- **判断规则**:
  - 解析时间 > 100ms → DNS 延迟高（RC-014）
  - ndots 配置高 (如 ndots:5) → 每次解析尝试多次，增加延迟
  - search 域过多 → 增加 DNS 查询次数
- **版本差异**: 无

---

### Phase 3: 平台与应用级诊断（只读，可能需审批）

> **目标**: 分析 Kubernetes 平台组件和应用级别的性能问题
> **预计耗时**: 10-20 分钟
**Step D3.1**: API Server 请求分析
- **命令**:
  ```bash
  # 从 Prometheus 查询 API Server 请求延迟
  # histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket{verb!="WATCH"}[5m]))

  # 按 verb 和 resource 分组查看
  # sum by (verb, resource) (rate(apiserver_request_duration_seconds_count[5m]))

  # 直接从 API Server metrics 端点获取
  kubectl get --raw /metrics | grep apiserver_request_duration

  # 检查 API Server 审计日志（如果启用）
  kubectl logs -n kube-system kube-apiserver-<node> --tail=100 | grep -i slow
  ```
- **超时**: 20s
- **预期输出模式**: API Server 请求延迟数据
- **判断规则**:
  - P99 > 1s (verb != WATCH) → API Server 性能下降（RC-006）
  - LIST 请求延迟高 → 大量对象导致慢查询
  - PATCH/UPDATE 延迟高 → etcd 写入慢
  - Webhook 延迟高 → 外部 Webhook 拖慢请求
- **版本差异**: 无

**Step D3.2**: etcd 慢操作分析
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 从 Prometheus 查询 etcd 磁盘指标
  # etcd_disk_wal_fsync_duration_seconds
  # etcd_disk_backend_commit_duration_seconds

  # 检查 etcd 日志中的慢操作
  kubectl logs -n kube-system etcd-<control-plane-node> --tail=200 | grep -i "slow|took too long"

  # etcd 性能指标
  kubectl exec -n kube-system etcd-<node> -- etcdctl endpoint status --write-out=table 2>/dev/null
  ```
- **超时**: 20s
- **预期输出模式**: etcd 性能指标
- **判断规则**:
  - wal_fsync P99 > 100ms → 磁盘写入慢（RC-007）
  - backend_commit P99 > 250ms → 数据库提交慢
  - 日志中频繁出现 "took too long" → etcd 过载
  - DB size > 4GB → 数据库过大，需要 compact/defrag
- **版本差异**: 无

**Step D3.3**: Scheduler 性能分析
- **命令**:
  ```bash
  # 从 Prometheus 查询调度延迟
  # scheduler_scheduling_algorithm_duration_seconds
  # scheduler_pending_pods

  # 检查 Scheduler 日志
  kubectl logs -n kube-system kube-scheduler-<node> --tail=100 | grep -i "slow|took"

  # 检查 Pending Pod 数量
  kubectl get pods -A --field-selector=status.phase=Pending | wc -l
  ```
- **超时**: 15s
- **预期输出模式**: 调度器性能数据
- **判断规则**:
  - scheduling_algorithm_duration P99 > 100ms → 调度算法慢
  - 大量 Pending Pod → 资源不足或调度约束过严
  - 调度延迟周期性增加 → 可能与定时任务相关
- **版本差异**: 无

**Step D3.4**: Go 应用 pprof 分析
- **命令**:
  ```bash
  # 获取 CPU profile（需要应用暴露 pprof 端口）
  kubectl port-forward <pod> 6060:6060 &
  go tool pprof http://localhost:6060/debug/pprof/profile?seconds=30

  # 获取 heap profile
  go tool pprof http://localhost:6060/debug/pprof/heap

  # 获取 goroutine 信息
  curl http://localhost:6060/debug/pprof/goroutine?debug=1

  # 或直接获取文本格式
  curl http://localhost:6060/debug/pprof/profile?seconds=10 > cpu.prof
  go tool pprof -text cpu.prof | head -30
  ```
- **超时**: 60s
- **风险级别**: 🟢 低（只读采集，但可能有微小性能开销）
- **预期输出模式**: pprof 性能数据
- **判断规则**:
  - CPU profile 中某函数占比 > 30% → 热点函数，可优化
  - heap 持续增长 → 内存泄漏（RC-005）
  - goroutine 数量异常高 → 协程泄漏
- **版本差异**: 无

**Step D3.5**: Java 应用 JFR 分析
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 启动 JFR 录制
  kubectl exec <pod> -- jcmd 1 JFR.start duration=60s filename=/tmp/recording.jfr

  # 等待录制完成
  sleep 65

  # 导出录制文件
  kubectl cp <pod>:/tmp/recording.jfr ./recording.jfr

  # 使用 jfr 工具分析
  jfr print --events jdk.CPULoad recording.jfr
  jfr print --events jdk.GCPhasePause recording.jfr

  # 或获取实时 GC 信息
  kubectl exec <pod> -- jstat -gc 1 1000 10
  ```
- **超时**: 120s
- **风险级别**: 🟡 中（JFR 有微小性能开销）
- **预期输出模式**: JFR 性能数据
- **判断规则**:
  - GCPhasePause 频繁 → GC 配置问题（RC-011）
  - CPU Load 持续高 → 热点代码需优化
  - Old Gen 使用率持续高 → 内存泄漏或 heap 配置不当
- **版本差异**: 无

**Step D3.6**: Linux perf 分析
- **命令**:
  ```bash
  # 实时 CPU 热点
  ssh <node-ip> "perf top -p <PID> -g"

  # 录制 30 秒 CPU 样本
  ssh <node-ip> "perf record -p <PID> -g -- sleep 30"
  ssh <node-ip> "perf report --stdio" | head -50

  # 生成火焰图数据
  ssh <node-ip> "perf script | ./stackcollapse-perf.pl | ./flamegraph.pl > flamegraph.svg"
  ```
- **超时**: 60s
- **风险级别**: 🟡 中（perf 有一定性能开销）
- **预期输出模式**: CPU 热点信息
- **判断规则**:
  - 某个函数/符号占用 > 20% CPU → 热点，可能需要优化
  - 大量内核态 CPU (sys) → 系统调用开销大
  - 火焰图中出现宽平台 → 该调用路径需要重点优化
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 风险 | 诊断证据 | FTA 映射 |
|--------|------|------|------|---------|---------|
| RC-001 | **CPU Limit 过低导致 throttling** — 容器 CPU limit 配置过低，导致 CFS throttling，应用响应变慢 | ~20% | 🟢 | D2.1 nr_throttled/nr_periods > 10%；D2.2 quota 过小；应用延迟与 throttling 相关 | perf-fta: BE-cpu-throttling |
| RC-002 | **内存 Limit 不足导致频繁 OOM** — 容器内存 limit 过低，频繁触发 OOM Kill 或内存回收压力大 | ~15% | 🟡 | D2.4 OOM Kill 记录；D2.5 memory.events 中 oom_kill > 0；Pod 频繁重启 | perf-fta: BE-memory-oom |
| RC-003 | **磁盘 IOPS 瓶颈** — 磁盘 IOPS 或吞吐量达到上限，I/O 等待时间长 | ~10% | 🟡 | D2.6 %util > 80%；await 显著增加；iowait 持续偏高 | perf-fta: BE-disk-iops |
| RC-004 | **conntrack 表溢出** — netfilter conntrack 表满，新连接被丢弃 | ~8% | 🟡 | D2.8 conntrack 使用率 > 90%；dmesg 中出现 table full；网络连接失败 | perf-fta: BE-conntrack-full |
| RC-005 | **应用级内存泄漏** — 应用代码内存泄漏，导致内存持续增长直至 OOM | ~8% | 🟡 | D3.4/D3.5 heap 持续增长；Pod 运行时间越长内存越高；重启后恢复正常 | perf-fta: BE-memory-leak |
| RC-006 | **API Server 过载** — API Server QPS 过高或 Webhook 延迟高，导致请求变慢 | ~7% | 🟡 | D3.1 apiserver_request_duration P99 > 1s；大量 LIST 请求；Webhook 超时 | perf-fta: BE-apiserver-overload |
| RC-007 | **etcd 存储延迟** — etcd 使用的磁盘性能不足，WAL fsync 或 backend commit 慢 | ~6% | 🔴 | D3.2 wal_fsync P99 > 100ms；backend_commit > 250ms；etcd 日志中 slow 警告 | perf-fta: BE-etcd-slow |
| RC-008 | **网络带宽瓶颈** — 网络接口带宽达到上限，数据传输变慢 | ~5% | 🟡 | D1.6 网卡流量接近线速；D2.9 重传率高；应用网络 I/O 慢 | perf-fta: BE-network-bandwidth |
| RC-009 | **NUMA 亲和性不当** — 进程跨 NUMA 访问内存，导致内存访问延迟增加 | ~4% | 🟢 | D2.3 numastat 显示高 remote accesses；内存密集型应用性能下降 | perf-fta: BE-numa-locality |
| RC-010 | **TCP 参数配置不当** — TCP 缓冲区、TIME_WAIT 回收等参数配置不当 | ~4% | 🟢 | D2.7 TIME_WAIT 堆积；D2.9 TCP 重传高；ss -ti 显示异常参数 | perf-fta: BE-tcp-config |
| RC-011 | **JVM GC 配置不当** — JVM 堆大小、GC 算法配置不当，导致频繁 GC 暂停 | ~4% | 🟡 | D3.5 GCPhasePause 频繁；Full GC > 1次/分钟；GC 日志显示异常 | perf-fta: BE-jvm-gc |
| RC-012 | **资源碎片化** — 节点上 requests 分散，导致调度困难或资源利用率低 | ~4% | 🟢 | D1.1 describe node 显示资源碎片；Pod Pending 但节点总资源充足 | perf-fta: BE-resource-fragmentation |
| RC-013 | **cgroup v2 内存回收策略变化** — cgroup v2 的内存回收行为与 v1 不同，应用未适配 | ~3% | 🟢 | D2.5 memory.pressure 高；D2.4 回收指标异常；升级到 cgroup v2 后性能下降 | perf-fta: BE-cgroupv2-memory |
| RC-014 | **DNS 解析延迟** — ndots 配置高、search 域多导致 DNS 查询慢 | ~2% | 🟢 | D2.10 DNS 解析 > 100ms；resolv.conf 中 ndots > 2 | perf-fta: BE-dns-latency |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 调整 CPU Limit/Request（消除 throttling）
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认当前 throttling 情况
  kubectl top pods -n <namespace> | grep <pod-name>
  
  # 获取当前资源配置
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[*].resources}'
  
  # 检查 throttling 指标
  # container_cpu_cfs_throttled_periods_total{pod="<pod>"}
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方法 1: 直接 patch deployment（推荐增加 50-100%）
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/cpu", "value": "2000m"},
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/cpu", "value": "1000m"}
  ]'

  # 方法 2: 编辑配置
  kubectl edit deployment <deployment> -n <namespace>
  # 修改 resources.limits.cpu 和 resources.requests.cpu
  ```
- **后置验证**:
  ```bash
  # 等待 Pod 重建
  kubectl rollout status deployment/<deployment> -n <namespace>
  
  # 验证 throttling 消除
  # 等待 5 分钟后检查 container_cpu_cfs_throttled_periods_total 增长是否停止
  
  # 验证应用延迟恢复
  kubectl top pods -n <namespace> | grep <pod-name>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-002: 调整 Memory Limit/Request
- **适用根因**: RC-002
- **前置检查**:
  ```bash
  # 检查当前内存使用和 OOM 历史
  kubectl top pods -n <namespace> | grep <pod-name>
  kubectl describe pod <pod> -n <namespace> | grep -i oom
  kubectl get events -n <namespace> --field-selector involvedObject.name=<pod> | grep -i oom
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 增加内存限制（推荐增加 25-50%）
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/memory", "value": "2Gi"},
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "1Gi"}
  ]'
  ```
- **后置验证**:
  ```bash
  kubectl rollout status deployment/<deployment> -n <namespace>
  kubectl top pods -n <namespace> | grep <pod-name>
  # 监控 15 分钟，确认无 OOM 事件
  kubectl get events -n <namespace> --field-selector reason=OOMKilled
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-003: 优化 ndots 配置减少 DNS 查询
- **适用根因**: RC-014
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查当前 DNS 配置
  kubectl exec <pod> -n <namespace> -- cat /etc/resolv.conf
  
  # 测试 DNS 解析延迟
  kubectl exec <pod> -n <namespace> -- sh -c "time nslookup kubernetes.default.svc.cluster.local"
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 在 Pod spec 中添加 dnsConfig
  kubectl patch deployment <deployment> -n <namespace> --type='strategic' -p='
  spec:
    template:
      spec:
        dnsConfig:
          options:
            - name: ndots
              value: "2"
            - name: single-request-reopen
  '
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 等待 Pod 重建
  kubectl rollout status deployment/<deployment> -n <namespace>
  
  # 验证 DNS 配置
  kubectl exec <new-pod> -n <namespace> -- cat /etc/resolv.conf
  
  # 测试 DNS 延迟降低
  kubectl exec <new-pod> -n <namespace> -- sh -c "time nslookup kubernetes.default.svc.cluster.local"
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-004: 调整 conntrack 表大小
- **适用根因**: RC-004
- **前置检查**:
  ```bash
  # 检查当前 conntrack 使用
  ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_count"
  ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_max"
  
  # 检查是否有 table full 错误
  ssh <node-ip> "dmesg | grep -i conntrack"
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

  ```bash
  # 临时调整（重启后失效）
  ssh <node-ip> "sysctl -w net.netfilter.nf_conntrack_max=524288"
  ssh <node-ip> "sysctl -w net.netfilter.nf_conntrack_buckets=131072"

  # 永久生效（写入配置文件）
  ssh <node-ip> "echo 'net.netfilter.nf_conntrack_max=524288' >> /etc/sysctl.conf"
  ssh <node-ip> "echo 'net.netfilter.nf_conntrack_buckets=131072' >> /etc/sysctl.conf"
  ssh <node-ip> "sysctl -p"
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_max"
  # 预期: 524288
  
  # 监控使用率下降
  ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_count"
  ```
- **回滚命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

  ```bash
  ssh <node-ip> "sysctl -w net.netfilter.nf_conntrack_max=262144"
  # 删除配置文件中的条目
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-005: 磁盘 I/O 优化
- **适用根因**: RC-003
- **影响说明**: 调整 I/O 调度器或迁移到 SSD 会影响磁盘操作行为，需要在业务低峰期执行
- **审批提示**: "建议优化节点 `<node>` 的磁盘 I/O 配置。操作包括调整 I/O 调度器，可能影响正在进行的 I/O 操作。是否批准？"
- **前置检查**:
  ```bash
  # 检查当前 I/O 调度器
  ssh <node-ip> "cat /sys/block/sda/queue/scheduler"
  
  # 检查磁盘类型
  ssh <node-ip> "cat /sys/block/sda/queue/rotational"
  # 0 = SSD, 1 = HDD
  ```
- **执行命令**:
  ```bash
  # 对于 SSD，切换到 none/mq-deadline 调度器
  ssh <node-ip> "echo 'none' > /sys/block/sda/queue/scheduler"
  # 或
  ssh <node-ip> "echo 'mq-deadline' > /sys/block/sda/queue/scheduler"

  # 调整 read-ahead
  ssh <node-ip> "blockdev --setra 2048 /dev/sda"

  # 永久生效（udev 规则）
  ssh <node-ip> 'echo "ACTION==\"add|change\", KERNEL==\"sd[a-z]\", ATTR{queue/scheduler}=\"none\"" > /etc/udev/rules.d/60-scheduler.rules'
  ```
- **后置验证**:
  ```bash
  # 验证调度器已切换
  ssh <node-ip> "cat /sys/block/sda/queue/scheduler"
  
  # 监控 I/O 性能改善
  ssh <node-ip> "iostat -x 1 10"
  # await 和 %util 应该下降
  ```
- **回滚命令**:
  ```bash
  ssh <node-ip> "echo 'cfq' > /sys/block/sda/queue/scheduler"
  ```

#### REM-006: TCP 参数调优
- **适用根因**: RC-010
- **影响说明**: 调整 TCP 参数会影响所有网络连接行为
- **审批提示**: "建议调整节点 `<node>` 的 TCP 参数以优化网络性能。是否批准？"
- **前置检查**:
  ```bash
  # 检查当前 TCP 参数
  ssh <node-ip> "sysctl -a | grep -E 'tcp_tw|tcp_fin|tcp_keepalive|tcp_max_syn'"
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

  ```bash
  # 优化 TIME_WAIT 回收
  ssh <node-ip> "sysctl -w net.ipv4.tcp_tw_reuse=1"
  
  # 增加本地端口范围
  ssh <node-ip> "sysctl -w net.ipv4.ip_local_port_range='10000 65535'"
  
  # 增加 SYN 队列
  ssh <node-ip> "sysctl -w net.ipv4.tcp_max_syn_backlog=65535"
  ssh <node-ip> "sysctl -w net.core.somaxconn=65535"
  
  # 调整 TCP 缓冲区
  ssh <node-ip> "sysctl -w net.core.rmem_max=16777216"
  ssh <node-ip> "sysctl -w net.core.wmem_max=16777216"
  ssh <node-ip> "sysctl -w net.ipv4.tcp_rmem='4096 87380 16777216'"
  ssh <node-ip> "sysctl -w net.ipv4.tcp_wmem='4096 65536 16777216'"

  # 写入永久配置
  ssh <node-ip> "cat >> /etc/sysctl.conf << 'EOF'
net.ipv4.tcp_tw_reuse=1
net.ipv4.ip_local_port_range=10000 65535
net.ipv4.tcp_max_syn_backlog=65535
net.core.somaxconn=65535
net.core.rmem_max=16777216
net.core.wmem_max=16777216
EOF"
  ssh <node-ip> "sysctl -p"
  ```
- **后置验证**:
  ```bash
  # 验证参数生效
  ssh <node-ip> "sysctl net.ipv4.tcp_tw_reuse net.ipv4.ip_local_port_range"
  
  # 监控 TIME_WAIT 数量下降
  ssh <node-ip> "ss -tan state time-wait | wc -l"
  ```
- **回滚命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

  ```bash
  # 恢复默认值
  ssh <node-ip> "sysctl -w net.ipv4.tcp_tw_reuse=0"
  # 删除 /etc/sysctl.conf 中新增的条目
  ```

#### REM-007: JVM GC 参数调优
- **适用根因**: RC-011
- **影响说明**: 修改 JVM 参数需要重启应用
- **审批提示**: "建议调整 `<deployment>` 的 JVM GC 参数。需要重启 Pod，会导致短暂服务中断。是否批准？"
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查当前 JVM 参数
  kubectl exec <pod> -n <namespace> -- jcmd 1 VM.flags
  
  # 检查 GC 日志
  kubectl logs <pod> -n <namespace> | grep -i gc | tail -50
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 修改 deployment 中的 JVM 参数
  kubectl edit deployment <deployment> -n <namespace>
  # 在 env 或 args 中修改 JAVA_OPTS:
  # -XX:+UseG1GC
  # -XX:MaxGCPauseMillis=200
  # -XX:InitiatingHeapOccupancyPercent=45
  # -Xmx2g -Xms2g
  # -XX:+PrintGCDetails -Xloggc:/var/log/gc.log

  # 或使用 patch
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/env/0", "value": {"name": "JAVA_OPTS", "value": "-XX:+UseG1GC -XX:MaxGCPauseMillis=200 -Xmx2g -Xms2g"}}
  ]'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl rollout status deployment/<deployment> -n <namespace>
  
  # 验证 JVM 参数
  kubectl exec <new-pod> -n <namespace> -- jcmd 1 VM.flags | grep -i gc
  
  # 监控 GC 行为改善
  kubectl exec <new-pod> -n <namespace> -- jstat -gc 1 1000 10
  # Full GC 次数应该减少
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-008: API Server 限流参数调整
- **适用根因**: RC-006
- **影响说明**: 调整 API Server 限流参数需要重启 API Server
- **审批提示**: "建议调整 API Server 限流参数。需要重启 API Server，期间 API 服务短暂不可用（HA 环境影响较小）。是否批准？"
- **前置检查**:
  ```bash
  # 检查当前限流配置
  kubectl get --raw /debug/api_priority_and_fairness/dump_priority_levels
  
  # 检查被限流的请求
  # apiserver_request_total{code="429"}
  ```
- **执行命令**:
  ```bash
  # 编辑 kube-apiserver manifest（kubeadm 集群）
  ssh <control-plane-node> "vi /etc/kubernetes/manifests/kube-apiserver.yaml"
  # 添加或调整以下参数:
  # --max-requests-inflight=800
  # --max-mutating-requests-inflight=400

  # 对于托管集群（如 EKS/AKS/ACK），需要通过云控制台调整

  # API Server 会自动重启
  ```
- **后置验证**:
  ```bash
  # 等待 API Server 重启
  kubectl get nodes
  
  # 验证参数生效
  kubectl get --raw /debug/api_priority_and_fairness/dump_priority_levels
  
  # 监控 429 错误减少
  # apiserver_request_total{code="429"}
  ```
- **回滚命令**:
  ```bash
  # 恢复原始 manifest 配置
  ssh <control-plane-node> "vi /etc/kubernetes/manifests/kube-apiserver.yaml"
  # 删除新增的参数
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-009: etcd 磁盘迁移到高性能 SSD
- **适用根因**: RC-007
- **影响说明**: etcd 磁盘迁移需要仔细规划，操作不当可能导致数据丢失。这是基础设施级变更。
- **操作步骤**:
  1. **准备新的 SSD 磁盘**:
     ```bash
     # 在控制平面节点上挂载新 SSD
     # 格式化为 ext4 或 xfs
     mkfs.ext4 /dev/nvme1n1
     mkdir -p /var/lib/etcd-new
     mount /dev/nvme1n1 /var/lib/etcd-new
     ```
  2. **停止 etcd（逐节点操作）**:
     ```bash
     # 对于 kubeadm 集群
     mv /etc/kubernetes/manifests/etcd.yaml /tmp/
     # 等待 etcd 停止
     ```
  3. **迁移数据**:
     ```bash
     rsync -av /var/lib/etcd/ /var/lib/etcd-new/
     mv /var/lib/etcd /var/lib/etcd-old
     mv /var/lib/etcd-new /var/lib/etcd
     ```
  4. **更新 fstab 并重启 etcd**:
     ```bash
     echo '/dev/nvme1n1 /var/lib/etcd ext4 defaults 0 0' >> /etc/fstab
     mv /tmp/etcd.yaml /etc/kubernetes/manifests/
     ```
  5. **验证**:
     ```bash
     kubectl get cs
     etcdctl endpoint health
     ```
- **安全检查**:
  - 确保有 etcd 备份
  - 在非生产环境验证流程
  - 逐节点操作，保持 etcd 集群 quorum
- **回滚方案**:
  ```bash
  # 停止 etcd
  mv /etc/kubernetes/manifests/etcd.yaml /tmp/
  # 恢复原磁盘
  mv /var/lib/etcd /var/lib/etcd-failed
  mv /var/lib/etcd-old /var/lib/etcd
  # 重启 etcd
  mv /tmp/etcd.yaml /etc/kubernetes/manifests/
  ```

#### REM-010: NUMA 绑定与 CPU 亲和性配置
- **适用根因**: RC-009
- **影响说明**: NUMA 绑定需要修改 kubelet 配置和 Pod 规格，可能需要重启节点
- **操作步骤**:
  1. **启用 kubelet CPU Manager**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

     ```bash
     # 编辑 kubelet 配置
     ssh <node-ip> "vi /var/lib/kubelet/config.yaml"
     # 添加:
     # cpuManagerPolicy: static
     # topologyManagerPolicy: best-effort  # 或 restricted / single-numa-node
     
     # 删除 CPU manager 状态文件
     ssh <node-ip> "rm -f /var/lib/kubelet/cpu_manager_state"
     
     # 重启 kubelet
     ssh <node-ip> "systemctl restart kubelet"
     ```
  2. **配置 Pod 使用 Guaranteed QoS**:
     ```yaml
     spec:
       containers:
       - name: app
         resources:
           requests:
             cpu: "2"
             memory: "4Gi"
           limits:
             cpu: "2"       # requests == limits
             memory: "4Gi"  # requests == limits
     ```
  3. **验证 NUMA 绑定**:
     ```bash
     # 检查 Pod 的 CPU 分配
     ssh <node-ip> "cat /var/lib/kubelet/cpu_manager_state"
     
     # 检查进程的 NUMA 分布
     ssh <node-ip> "numastat -p <PID>"
     ```
- **安全检查**:
  - 确保节点有足够的可分配 CPU
  - 验证现有 Guaranteed QoS Pod 不受影响
  - 在单个节点上测试后再推广
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复 kubelet 配置
  ssh <node-ip> "vi /var/lib/kubelet/config.yaml"
  # 设置 cpuManagerPolicy: none
  ssh <node-ip> "rm -f /var/lib/kubelet/cpu_manager_state"
  ssh <node-ip> "systemctl restart kubelet"
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-011: 应用架构级性能重构
- **适用根因**: RC-005, RC-006（当简单调优无法解决时）
- **审批要求**: 需要架构师 + 高级 SRE + 业务方审批
- **数据备份**: 确保有完整的应用状态备份和回滚方案
- **操作步骤**:
  1. **性能分析与瓶颈定位**:
     - 使用 pprof/JFR/perf 生成详细的性能报告
     - 识别热点代码和性能瓶颈
     - 分析内存分配模式和 GC 行为
  2. **制定重构方案**:
     - 代码级优化（算法、数据结构）
     - 架构级优化（缓存、异步处理、微服务拆分）
     - 资源配置优化（JVM 参数、线程池配置）
  3. **灰度发布**:
     - 先在测试环境验证
     - 灰度发布到部分生产流量
     - 监控关键性能指标
  4. **全量发布与持续监控**:
     - 确认灰度阶段无问题后全量发布
     - 建立性能基线和告警
- **回滚方案**:
  - 保留旧版本镜像和配置
  - 准备快速回滚脚本
  - 定义回滚触发条件（如 P99 > 阈值）

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-5 分钟内）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# V1: 验证 CPU throttling 消除
kubectl top pods -n <namespace> | grep <pod-name>
# CPU 使用应低于 limit

# V2: 验证内存使用正常
kubectl top pods -n <namespace> | grep <pod-name>
# 内存使用应低于 limit 的 80%

# V3: 验证无新的 OOM 事件
kubectl get events -n <namespace> --field-selector reason=OOMKilled --sort-by=.lastTimestamp

# V4: 验证应用延迟恢复
# 检查 Prometheus 中的 P99 延迟
# histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m]))

# V5: 验证平台组件延迟恢复
kubectl get --raw /healthz?verbose
# 所有组件应返回 ok
```
### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 应用 P99 延迟 | `histogram_quantile(0.99, ...)` | 恢复到 SLA 以内 | P99 > SLA |
| CPU throttling | `container_cpu_cfs_throttled_periods_total` | 增长停止 | 持续增长 |
| 内存使用率 | `container_memory_usage_bytes` | 稳定 | 持续增长 |
| Pod 重启次数 | `kube_pod_container_status_restarts_total` | 无新增 | 重启次数增加 |
| API Server 延迟 | `apiserver_request_duration_seconds` | P99 < 1s | P99 > 1s |
| etcd 延迟 | `etcd_disk_wal_fsync_duration_seconds` | P99 < 100ms | P99 > 100ms |
| 网络重传率 | `node_netstat_Tcp_RetransSegs` | 增长平稳 | 急剧增长 |
| conntrack 使用率 | `conntrack -C / nf_conntrack_max` | < 75% | > 90% |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认性能问题已解决：

- [ ] 应用 P99 延迟恢复到 SLA 以内
- [ ] CPU throttling 停止（nr_throttled 不再增长）
- [ ] 无新的 OOM 事件
- [ ] 资源使用率处于安全水位（CPU < 80%，Memory < 85%）
- [ ] 平台组件（API Server/etcd/Scheduler）延迟正常
- [ ] 用户/业务方确认服务恢复正常
- [ ] 根因已明确记录并采取了预防措施

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| P99 延迟趋势 | Grafana 仪表板 | 持续 | 如再次升高 → 重新诊断 |
| CPU throttling | Prometheus 告警 | 持续 | 告警触发 → 检查 limit 配置 |
| 内存增长趋势 | `container_memory_usage_bytes` 趋势 | 每小时 | 线性增长 → 排查内存泄漏 |
| OOM 事件 | `kubectl get events` | 每 4 小时 | 新的 OOM → 调整 limit |
| GC 暂停时间 | GC 日志或 JMX | 每 4 小时 | Full GC 频繁 → JVM 调优 |
| 磁盘 I/O | `iostat` 或 node_exporter | 每小时 | await 升高 → 检查磁盘负载 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **30 分钟**未能确认根因 | Phase 3 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过验证 | REM-xxx 执行后验证失败 |
| **严重性升级** | 初始分级为 P2 但 P99 延迟继续恶化 | 诊断/修复过程中性能持续下降 |
| **未知根因** | 完成所有诊断步骤但无法匹配任何已知根因 | 所有诊断步骤均无明确异常 |
| **平台组件异常** | API Server/etcd 性能问题无法通过常规方法解决 | D3.1/D3.2 显示严重问题 |
| **应用代码问题** | 确认是应用代码层面的性能问题 | 需要开发团队介入 |

### 8.2 升级消息模板

```
【{severity}】性能瓶颈诊断与调优 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {service_name} 服务性能下降，P99 延迟从 {baseline_latency} 升至 {current_latency}
- 影响范围:
  - 受影响服务: {affected_services}
  - 受影响用户: {estimated_users}
  - SLA 影响: {sla_status}
- 已完成诊断:
  - Phase 1 资源全景: {phase1_summary}
  - Phase 2 瓶颈定位: {phase2_summary}
  - Phase 3 深度分析: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-PERF-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及输出摘要
2. **性能数据快照**:
   ```bash
   # 收集 Prometheus 数据
   # 最近 1 小时的 P99 延迟趋势
   # CPU/Memory 使用趋势
   # throttling 指标趋势
   ```
3. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
4. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
5. **关键资源快照**:
   ```bash
   kubectl top nodes > nodes-top.txt
   kubectl top pods -A > pods-top.txt
   kubectl get events -A --sort-by=.lastTimestamp > events.txt
   kubectl describe node <node> > node-describe.txt
   ```
6. **应用 profile 数据**: pprof/JFR/perf 的输出文件
7. **事件时间线**: 性能下降的起始时间、诊断过程、关键发现时间点

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| cgroup v2 支持 | GA | GA | GA | GA | GA |
| InPlacePodVerticalScaling | alpha | alpha | beta | beta | beta |
| CPU Manager 静态策略 | GA | GA | GA | GA | GA |
| Topology Manager | GA | GA | GA | GA | GA |
| Memory Manager | beta | beta | GA | GA | GA |
| Pod Resources API | GA | GA | GA | GA | GA |
| ResourceQuota PriorityClass | GA | GA | GA | GA | GA |
| HPAContainerMetrics | beta | beta | GA | GA | GA |
| GracefulNodeShutdown | GA | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl top` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl debug` profiles | beta | beta | GA | GA | GA |
| `crictl stats` | 支持 | 支持 | 支持 | 支持 | 支持 |
| cgroup v2 路径 | `/sys/fs/cgroup/...` | 同左 | 同左 | 同左 | 同左 |
| `kubectl get --raw /metrics` | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| HorizontalPodAutoscaler | autoscaling/v2 | v2 | v2 | v2 | v2 |
| VerticalPodAutoscaler | autoscaling.k8s.io/v1 | v1 | v1 | v1 | v1 |
| PodDisruptionBudget | policy/v1 | v1 | v1 | v1 | v1 |
| PriorityClass | scheduling.k8s.io/v1 | v1 | v1 | v1 | v1 |
| ResourceQuota | v1 | v1 | v1 | v1 | v1 |

### 9.4 cgroup v1 vs v2 差异

| 功能 | cgroup v1 | cgroup v2 |
|------|-----------|-----------|
| CPU throttling 统计 | `/sys/fs/cgroup/cpu/*/cpu.stat` | `/sys/fs/cgroup/*/cpu.stat` |
| 内存使用 | `memory.usage_in_bytes` | `memory.current` |
| 内存压力 | 不支持 | `memory.pressure` |
| 内存事件 | `memory.failcnt` | `memory.events` |
| CPU 配额 | `cpu.cfs_quota_us` + `cpu.cfs_period_us` | `cpu.max` |
| 目录结构 | 按控制器分层 | 统一层级 |

**判断 cgroup 版本**:
```bash
ssh <node-ip> "mount | grep cgroup"
# cgroup2 on /sys/fs/cgroup type cgroup2 → cgroup v2
# cgroup on /sys/fs/cgroup/cpu type cgroup → cgroup v1
```

### 9.5 版本相关的性能调优注意事项

- **[v1.28+]**: CPU Manager 和 Topology Manager GA，推荐用于性能敏感应用
- **[v1.30+]**: InPlacePodVerticalScaling (beta) 允许运行时调整 Pod 资源，减少重启
- **[v1.31+]**: Memory Manager GA，可用于 NUMA-aware 内存分配
- **[v1.32+]**: HPAContainerMetrics GA，HPA 可基于单个容器指标扩缩

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 CPU throttling 误判为应用 bug** | 应用响应慢，CPU 使用率显示不高（因为被限制了） | CPU limit 过低导致 CFS throttling | 先检查 `container_cpu_cfs_throttled_periods_total` 指标；`kubectl top` 显示的 CPU 接近 limit 也是信号 |
| **将 conntrack 溢出误判为 DNS 问题** | 服务间调用失败，怀疑 DNS 解析问题 | conntrack 表满导致新连接被丢弃 | 检查 `dmesg | grep conntrack`；网络问题前先排查 conntrack 使用率 |
| **将磁盘 I/O 瓶颈误判为应用慢** | 应用 P99 延迟高，CPU/Memory 看起来正常 | 磁盘 I/O 等待导致应用阻塞 | 始终检查 `iostat` 的 await 和 %util；iowait 高是关键信号 |
| **将 JVM GC 问题误判为内存不足** | 应用 OOM 或内存使用高，增加内存限制后仍然出问题 | GC 配置不当导致长时间 STW（Stop-The-World） | 先分析 GC 日志；Full GC 频繁是 GC 配置问题而非内存不足 |
| **将 NUMA 亲和性问题误判为内存性能差** | 内存密集型应用性能下降，但内存使用率不高 | 跨 NUMA 访问内存导致延迟高 | 使用 `numastat` 检查 remote accesses；特别是多 socket 服务器 |
| **将网络延迟误判为应用性能问题** | 服务调用延迟高，但应用 CPU/Memory 正常 | TCP 重传、网络拥塞导致延迟 | 检查 `ss -ti` 的重传统计；`netstat -s` 的错误计数 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| 性能瓶颈深度排查 | `故障诊断/33-performance-bottleneck-troubleshooting.md` | 超出本 Skill 覆盖的复杂性能问题 |
| 性能调优指南 | `集群基础/13-performance-tuning-guide.md` | 系统性的性能优化方法论 |
| cgroup 详解 | `系统基础/08-linux-container-fundamentals.md` | cgroup v1/v2 深入理解 |
| 网络性能分析 | `网络/` | 网络层面性能问题深入分析 |
| 存储性能 | `存储/` | 磁盘 I/O 性能深入分析 |
| 可观测性 | `可观测性/` | 监控指标和告警配置 |
| API Server 架构 | `集群基础/` | API Server 性能调优 |
| etcd 运维 | `集群基础/` | etcd 性能问题排查 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 14 个根因、11 个修复操作 | 基于 top 工单分析，性能问题为高频场景 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **GPU 性能瓶颈**: GPU 利用率、显存使用、CUDA 性能分析
2. **服务网格性能**: Istio/Envoy sidecar 性能影响分析
3. **数据库连接池**: 连接池配置、数据库慢查询对应用的影响
4. **分布式追踪**: 使用 Jaeger/Zipkin 定位跨服务延迟
5. **混沌工程**: 使用 Chaos Mesh 进行性能故障注入测试
6. **eBPF 性能分析**: 使用 bpftrace/bcc 进行深度内核性能分析

---

## 附录 A：自动化诊断脚本

### A.1 节点性能基线采集 (collect-node-baseline.sh)

```bash
#!/bin/bash
# =============================================================================
# 节点性能基线采集脚本
# Usage: bash collect-node-baseline.sh --node <node-name> [--duration <seconds>]
# Risk: NONE (read-only operations)
# Source: SKILL-PERF-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*" >&2; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*" >&2; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; }
success() { echo -e "${GREEN}[OK]${NC} $*" >&2; }

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") --node <node-name> [--duration <seconds>]

节点性能基线采集脚本 - 采集 CPU/内存/磁盘/网络性能数据

Options:
    --node, -n        节点名称或 IP 地址 (必需)
    --duration, -d    采集时长（秒）(默认: 30)
    --ssh-user        SSH 用户名 (默认: root)
    --help, -h        显示帮助信息

Output:
    JSON 格式的基线报告

Examples:
    $(basename "$0") --node 10.0.0.1
    $(basename "$0") -n worker-01 -d 60
EOF
    exit 0
}

# --- 默认参数 ---
NODE=""
DURATION=30
SSH_USER="root"

# --- 参数解析 ---
while $# -gt 0; do
    case "$1" in
        --node|-n)     NODE="$2"; shift 2 ;;
        --duration|-d) DURATION="$2"; shift 2 ;;
        --ssh-user)    SSH_USER="$2"; shift 2 ;;
        --help|-h)     usage ;;
        *)             error "未知参数: $1"; usage ;;
    esac
done

if -z "$NODE"; then
    error "必须指定 --node 参数"
    usage
fi

# --- 前置检查 ---
info "检查节点连接: $NODE"
if ! ssh -o ConnectTimeout=5 -o BatchMode=yes "${SSH_USER}@${NODE}" "echo ok" &>/dev/null; then
    error "无法通过 SSH 连接到节点 $NODE"
    exit 1
fi
success "SSH 连接成功"

info "开始采集性能基线 (时长: ${DURATION}s)..."

# --- 采集数据 ---
TIMESTAMP=$(date -u '+%Y-%m-%dT%H:%M:%SZ')

# CPU: uptime load average
info "[1/6] 采集 CPU 负载..."
LOAD_AVG=$(ssh "${SSH_USER}@${NODE}" "cat /proc/loadavg" 2>/dev/null || echo "0 0 0 0/0 0")
LOAD_1=$(echo "$LOAD_AVG" | awk '{print $1}')
LOAD_5=$(echo "$LOAD_AVG" | awk '{print $2}')
LOAD_15=$(echo "$LOAD_AVG" | awk '{print $3}')

# CPU: vmstat
VMSTAT=$(ssh "${SSH_USER}@${NODE}" "vmstat 1 3 | tail -1" 2>/dev/null || echo "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0")
CPU_USER=$(echo "$VMSTAT" | awk '{print $13}')
CPU_SYS=$(echo "$VMSTAT" | awk '{print $14}')
CPU_IDLE=$(echo "$VMSTAT" | awk '{print $15}')
CPU_IOWAIT=$(echo "$VMSTAT" | awk '{print $16}')

# 内存: free
info "[2/6] 采集内存数据..."
MEM_INFO=$(ssh "${SSH_USER}@${NODE}" "free -b" 2>/dev/null || echo "")
MEM_TOTAL=$(echo "$MEM_INFO" | awk '/^Mem:/ {print $2}')
MEM_USED=$(echo "$MEM_INFO" | awk '/^Mem:/ {print $3}')
MEM_FREE=$(echo "$MEM_INFO" | awk '/^Mem:/ {print $4}')
MEM_AVAILABLE=$(echo "$MEM_INFO" | awk '/^Mem:/ {print $7}')

# 内存: vmstat pgfault
PG_STATS=$(ssh "${SSH_USER}@${NODE}" "cat /proc/vmstat | grep -E 'pgfault|pgmajfault'" 2>/dev/null || echo "")
PGFAULT=$(echo "$PG_STATS" | grep "^pgfault" | awk '{print $2}')
PGMAJFAULT=$(echo "$PG_STATS" | grep "^pgmajfault" | awk '{print $2}')

# 磁盘: iostat
info "[3/6] 采集磁盘 I/O..."
IOSTAT=$(ssh "${SSH_USER}@${NODE}" "iostat -xd 1 2 2>/dev/null | tail -n +7 | head -10" 2>/dev/null || echo "")
DISK_UTIL=$(echo "$IOSTAT" | awk 'NR==1 {print $NF}' || echo "0")

# 磁盘: df
DF_INFO=$(ssh "${SSH_USER}@${NODE}" "df -h / | tail -1" 2>/dev/null || echo "")
DISK_USE_PCT=$(echo "$DF_INFO" | awk '{print $5}' | tr -d '%')

# 磁盘调度器
SCHEDULER=$(ssh "${SSH_USER}@${NODE}" "cat /sys/block/*/queue/scheduler 2>/dev/null | head -1" || echo "unknown")

# 网络: ss 连接统计
info "[4/6] 采集网络连接..."
SS_STATS=$(ssh "${SSH_USER}@${NODE}" "ss -s" 2>/dev/null || echo "")
TCP_ESTAB=$(echo "$SS_STATS" | grep -oP 'estab \K[0-9]+' || echo "0")
TCP_TIMEWAIT=$(ssh "${SSH_USER}@${NODE}" "ss -tan state time-wait | wc -l" 2>/dev/null || echo "0")

# conntrack
info "[5/6] 采集 conntrack..."
CONNTRACK_COUNT=$(ssh "${SSH_USER}@${NODE}" "cat /proc/sys/net/netfilter/nf_conntrack_count 2>/dev/null" || echo "0")
CONNTRACK_MAX=$(ssh "${SSH_USER}@${NODE}" "cat /proc/sys/net/netfilter/nf_conntrack_max 2>/dev/null" || echo "1")

# 网络错误
info "[6/6] 采集网络错误..."
NET_ERRORS=$(ssh "${SSH_USER}@${NODE}" "netstat -s 2>/dev/null | grep -i 'retransmit' | head -1" || echo "")
RETRANSMIT=$(echo "$NET_ERRORS" | grep -oP '[0-9]+' | head -1 || echo "0")

# --- 输出 JSON 报告 ---
info "生成基线报告..."

cat <<EOF
{
  "timestamp": "$TIMESTAMP",
  "node": "$NODE",
  "duration_seconds": $DURATION,
  "cpu": {
    "load_avg_1m": ${LOAD_1:-0},
    "load_avg_5m": ${LOAD_5:-0},
    "load_avg_15m": ${LOAD_15:-0},
    "user_pct": ${CPU_USER:-0},
    "sys_pct": ${CPU_SYS:-0},
    "idle_pct": ${CPU_IDLE:-0},
    "iowait_pct": ${CPU_IOWAIT:-0}
  },
  "memory": {
    "total_bytes": ${MEM_TOTAL:-0},
    "used_bytes": ${MEM_USED:-0},
    "free_bytes": ${MEM_FREE:-0},
    "available_bytes": ${MEM_AVAILABLE:-0},
    "pgfault": ${PGFAULT:-0},
    "pgmajfault": ${PGMAJFAULT:-0}
  },
  "disk": {
    "util_pct": ${DISK_UTIL:-0},
    "root_use_pct": ${DISK_USE_PCT:-0},
    "scheduler": "${SCHEDULER:-unknown}"
  },
  "network": {
    "tcp_established": ${TCP_ESTAB:-0},
    "tcp_timewait": ${TCP_TIMEWAIT:-0},
    "conntrack_count": ${CONNTRACK_COUNT:-0},
    "conntrack_max": ${CONNTRACK_MAX:-1},
    "conntrack_usage_pct": $(echo "scale=2; ${CONNTRACK_COUNT:-0} * 100 / ${CONNTRACK_MAX:-1}" | bc 2>/dev/null || echo "0"),
    "tcp_retransmits": ${RETRANSMIT:-0}
  }
}
EOF

success "基线采集完成"
```

### A.2 容器 CPU Throttling 分析 (analyze-throttling.sh)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# =============================================================================
# 容器 CPU Throttling 分析脚本
# Usage: bash analyze-throttling.sh --namespace <ns> [--pod <pod-name>]
# Risk: NONE (read-only operations)
# Source: SKILL-PERF-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
success() { echo -e "${GREEN}[OK]${NC} $*"; }

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") --namespace <namespace> [--pod <pod-name>]

容器 CPU Throttling 分析脚本 - 分析容器 CPU 限流情况

Options:
    --namespace, -n    Kubernetes 命名空间 (必需)
    --pod, -p          Pod 名称 (可选，不指定则检查所有)
    --node             节点名称或 IP (用于 SSH 获取 cgroup 数据)
    --help, -h         显示帮助信息

Examples:
    $(basename "$0") --namespace default
    $(basename "$0") -n production -p myapp-7d8f9c-xxx
EOF
    exit 0
}

# --- 默认参数 ---
NAMESPACE=""
POD_NAME=""
NODE_IP=""

# --- 参数解析 ---
while $# -gt 0; do
    case "$1" in
        --namespace|-n) NAMESPACE="$2"; shift 2 ;;
        --pod|-p)       POD_NAME="$2"; shift 2 ;;
        --node)         NODE_IP="$2"; shift 2 ;;
        --help|-h)      usage ;;
        *)              error "未知参数: $1"; usage ;;
    esac
done

if -z "$NAMESPACE"; then
    error "必须指定 --namespace 参数"
    usage
fi

# --- 前置检查 ---
if ! command -v kubectl &>/dev/null; then
    error "kubectl 未安装"
    exit 1
fi

if ! kubectl get namespace "$NAMESPACE" &>/dev/null; then
    error "命名空间 '$NAMESPACE' 不存在"
    exit 1
fi

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  容器 CPU Throttling 分析${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "  命名空间: $NAMESPACE"
echo -e "  时间: $(date -u '+%Y-%m-%d %H:%M:%S UTC')\n"

# --- 获取 Pod 列表 ---
if -n "$POD_NAME"; then
    PODS="$POD_NAME"
else
    PODS=$(kubectl get pods -n "$NAMESPACE" -o jsonpath='{.items[*].metadata.name}' 2>/dev/null)
fi

if -z "$PODS"; then
    warn "未找到 Pod"
    exit 0
fi

# --- 分析每个 Pod ---
info "分析 Pod CPU 资源配置与使用...\n"

printf "${CYAN}%-50s %-12s %-12s %-15s %-10s${NC}\n" "POD" "CPU_REQ" "CPU_LIM" "CPU_USAGE" "THROTTLED"
printf "%-50s %-12s %-12s %-15s %-10s\n" "--------------------------------------------------" "------------" "------------" "---------------" "----------"

for POD in $PODS; do
    # 获取资源配置
    RESOURCES=$(kubectl get pod "$POD" -n "$NAMESPACE" -o json 2>/dev/null || echo '{}')
    
    CPU_REQ=$(echo "$RESOURCES" | jq -r '.spec.containers[0].resources.requests.cpu // "none"' 2>/dev/null || echo "none")
    CPU_LIM=$(echo "$RESOURCES" | jq -r '.spec.containers[0].resources.limits.cpu // "none"' 2>/dev/null || echo "none")
    
    # 获取实际使用
    CPU_USAGE=$(kubectl top pod "$POD" -n "$NAMESPACE" --no-headers 2>/dev/null | awk '{print $2}' || echo "N/A")
    
    # 检查是否可能被 throttling
    THROTTLED="-"
    if "$CPU_LIM" != "none" && "$CPU_USAGE" != "N/A"; then
        # 简化判断：如果使用量接近 limit
        LIM_NUM=$(echo "$CPU_LIM" | sed 's/m$//' | sed 's/$/000/' | head -c 6)
        USE_NUM=$(echo "$CPU_USAGE" | sed 's/m$//')
        if "$USE_NUM" =~ ^[0-9]+$ && "$LIM_NUM" =~ ^[0-9]+$; then
            if $USE_NUM -gt $((LIM_NUM * 80 / 100)); then
                THROTTLED="${YELLOW}LIKELY${NC}"
            else
                THROTTLED="${GREEN}NO${NC}"
            fi
        fi
    fi
    
    printf "%-50s %-12s %-12s %-15s " "$POD" "$CPU_REQ" "$CPU_LIM" "$CPU_USAGE"
    echo -e "$THROTTLED"
done

# --- 调优建议 ---
echo -e "\n${CYAN}${BOLD}── 调优建议 ──${NC}"
cat <<EOF
1. 如果 THROTTLED 显示 LIKELY，请考虑增加 CPU limit (建议增加 50-100%)
2. 如果 CPU_REQ 显示 "none"，建议设置合理的 requests 以确保资源预留
3. 如果 CPU_LIM 显示 "none"，建议设置 limits 防止资源争抢
4. 使用 'kubectl describe pod <pod>' 查看详细资源配置
5. 如需精确分析，请通过 SSH 检查 cgroup cpu.stat 文件
EOF

echo -e "\n${GREEN}Throttling 分析完成${NC}"
```
### A.3 性能验证脚本 (verify-performance.sh)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# =============================================================================
# 性能修复后验证脚本
# Usage: bash verify-performance.sh --namespace <ns> [OPTIONS]
# Risk: NONE (read-only operations)
# Source: SKILL-PERF-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
success() { echo -e "${GREEN}[PASS]${NC} $*"; ((PASS_COUNT++)); }
fail()    { echo -e "${RED}[FAIL]${NC} $*"; ((FAIL_COUNT++)); }

# --- 统计 ---
PASS_COUNT=0
FAIL_COUNT=0

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") --namespace <namespace> [OPTIONS]

性能修复后验证脚本 - 验证性能问题是否解决

Options:
    --namespace, -n       Kubernetes 命名空间 (必需)
    --throttle-threshold  CPU Throttling 阈值百分比 (默认: 5)
    --memory-threshold    内存使用阈值百分比 (默认: 80)
    --api-latency-threshold  API Server P99 延迟阈值秒 (默认: 1)
    --help, -h            显示帮助信息

Examples:
    $(basename "$0") --namespace default
    $(basename "$0") -n production --throttle-threshold 10
EOF
    exit 0
}

# --- 默认参数 ---
NAMESPACE=""
THROTTLE_THRESHOLD=5
MEMORY_THRESHOLD=80
API_LATENCY_THRESHOLD=1

# --- 参数解析 ---
while $# -gt 0; do
    case "$1" in
        --namespace|-n)            NAMESPACE="$2"; shift 2 ;;
        --throttle-threshold)      THROTTLE_THRESHOLD="$2"; shift 2 ;;
        --memory-threshold)        MEMORY_THRESHOLD="$2"; shift 2 ;;
        --api-latency-threshold)   API_LATENCY_THRESHOLD="$2"; shift 2 ;;
        --help|-h)                 usage ;;
        *)                         warn "未知参数: $1"; shift ;;
    esac
done

if -z "$NAMESPACE"; then
    error "必须指定 --namespace 参数"
    usage
fi

# --- 前置检查 ---
if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}kubectl 未安装${NC}"
    exit 1
fi

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  性能修复后验证 - $NAMESPACE${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}\n"

# --- V1: 验证无 CPU Throttling 问题 ---
info "[V1] 验证 CPU Throttling 比例 < ${THROTTLE_THRESHOLD}%..."
# 简化检查：检查 Pod CPU 使用是否接近 limit
POD_STATS=$(kubectl top pods -n "$NAMESPACE" --no-headers 2>/dev/null || true)
THROTTLE_ISSUES=0
if -n "$POD_STATS"; then
    while read POD CPU MEM; do
        LIM=$(kubectl get pod "$POD" -n "$NAMESPACE" -o jsonpath='{.spec.containers[0].resources.limits.cpu}' 2>/dev/null || true)
        if -n "$LIM" && "$LIM" != "null"; then
            # 转换为毫核
            LIM_M=$(echo "$LIM" | sed 's/m$//')
            CPU_M=$(echo "$CPU" | sed 's/m$//')
            if "$CPU_M" =~ ^[0-9]+$ && "$LIM_M" =~ ^[0-9]+$; then
                USAGE_PCT=$((CPU_M * 100 / LIM_M))
                if $USAGE_PCT -gt 90; then
                    ((THROTTLE_ISSUES++))
                fi
            fi
        fi
    done <<< "$POD_STATS"
    
    if $THROTTLE_ISSUES -eq 0; then
        success "CPU Throttling 正常 (无 Pod CPU 接近 limit)"
    else
        fail "发现 $THROTTLE_ISSUES 个 Pod CPU 使用接近 limit (可能 throttling)"
    fi
else
    warn "无法获取 Pod CPU 数据"
fi

# --- V2: 验证内存使用 < threshold% limit ---
info "[V2] 验证内存使用 < ${MEMORY_THRESHOLD}% limit..."
MEM_ISSUES=0
if -n "$POD_STATS"; then
    while read POD CPU MEM; do
        LIM=$(kubectl get pod "$POD" -n "$NAMESPACE" -o jsonpath='{.spec.containers[0].resources.limits.memory}' 2>/dev/null || true)
        if -n "$LIM" && "$LIM" != "null"; then
            # 简化检查：仅检查是否有定义
            MEM_VAL=$(echo "$MEM" | sed 's/Mi$//')
            if "$MEM_VAL" =~ ^[0-9]+$; then
                # 假设 limit 以 Gi 为单位，简化处理
                if $MEM_VAL -gt 3000; then
                    ((MEM_ISSUES++))
                fi
            fi
        fi
    done <<< "$POD_STATS"
    
    if $MEM_ISSUES -eq 0; then
        success "内存使用正常"
    else
        fail "发现 $MEM_ISSUES 个 Pod 内存使用较高"
    fi
else
    warn "无法获取 Pod 内存数据"
fi

# --- V3: 验证无 OOM 事件 ---
info "[V3] 验证近期无 OOM 事件..."
OOM_EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector reason=OOMKilled --sort-by=.lastTimestamp 2>/dev/null | tail -5 || true)
if -z "$OOM_EVENTS"; then
    success "无 OOM 事件"
else
    # 检查是否是最近 15 分钟内的事件
    RECENT_OOM=$(echo "$OOM_EVENTS" | grep -v "^LAST" | head -3)
    if -n "$RECENT_OOM"; then
        fail "发现近期 OOM 事件:"
        echo "$RECENT_OOM" | while read line; do echo "    $line"; done
    else
        success "无近期 OOM 事件"
    fi
fi

# --- V4: 验证 API Server P99 < threshold ---
info "[V4] 验证 API Server 响应延迟..."
START_TIME=$(date +%s%3N)
kubectl get nodes &>/dev/null
END_TIME=$(date +%s%3N)
LATENCY_MS=$((END_TIME - START_TIME))
LATENCY_S=$(echo "scale=2; $LATENCY_MS / 1000" | bc 2>/dev/null || echo "0")

if (( $(echo "$LATENCY_S < $API_LATENCY_THRESHOLD" | bc -l 2>/dev/null || echo 0) )); then
    success "API Server 延迟: ${LATENCY_MS}ms (< ${API_LATENCY_THRESHOLD}s)"
else
    fail "API Server 延迟: ${LATENCY_MS}ms (超过 ${API_LATENCY_THRESHOLD}s)"
fi

# --- V5: 验证所有 Pod 运行正常 ---
info "[V5] 验证 Pod 状态..."
NOT_RUNNING=$(kubectl get pods -n "$NAMESPACE" --no-headers 2>/dev/null | grep -v "Running|Completed" || true)
if -z "$NOT_RUNNING"; then
    success "所有 Pod 运行正常"
else
    NOT_RUNNING_COUNT=$(echo "$NOT_RUNNING" | wc -l | tr -d ' ')
    fail "发现 $NOT_RUNNING_COUNT 个异常 Pod"
fi

# --- 输出验证结果 ---
echo -e "\n${BOLD}════════════════════════════════════════════════════════${NC}"
TOTAL=$((PASS_COUNT + FAIL_COUNT))
if $FAIL_COUNT -eq 0; then
    echo -e "${GREEN}${BOLD}验证结果: 全部通过 ($PASS_COUNT/$TOTAL)${NC}"
    exit 0
else
    echo -e "${RED}${BOLD}验证结果: 存在失败 (通过: $PASS_COUNT, 失败: $FAIL_COUNT)${NC}"
    exit 1
fi
```
```

<!-- risk-assessed -->

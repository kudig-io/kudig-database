---
title: 资源超额订阅 (Overcommit) 与 QoS 驱逐联动
summary: 系统化解析 Kubernetes 资源 overcommit 模型、可压缩/不可压缩资源差异、QoS 与节点驱逐的联动机制。
category: 设计原则
tags:
- overcommit
- qos
- resource-management
- eviction
- cpu-management
- memory-pressure
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 容量规划
estimated_read_time: 25min
intent_queries:
- Kubernetes overcommit 是什么
- QoS 三类如何判定
- CPU 和内存 overcommit 有何不同
- 节点驱逐如何与 QoS 联动
trigger_keywords:
- overcommit
- 超额订阅
- 超卖
- QoS
- 驱逐
- 压缩
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
>
> 特别提醒：**内存（Memory）是不可压缩资源，overcommit 内存不会"慢一点"，而是直接 OOM Kill 或触发节点驱逐**。生产环境中激进的内存超卖（requests 之和远超节点 Allocatable）是 Pod 被杀、雪崩式驱逐、P99 抖动最常见的根因。任何 overcommit 策略上线前，必须先用 `kubectl describe node` 核对 requests/limits 累加值，并准备好 Prometheus 对 `container_cpu_cfs_throttled_periods_total`、`container_memory_working_set_bytes`、`kubelet_evicted_*` 的告警面板。

# 资源超额订阅 (Overcommit) 与 QoS 驱逐联动

**Overcommit（超额订阅 / 超卖）** 是 Kubernetes 提升资源利用率的核心机制：它允许节点上所有 Pod 的 `requests` 之和**超过**节点实际可分配容量（allocatable）。这一切之所以"安全"，是因为 K8s 在底层用两套机制兜底——**QoS 分类** 决定"谁先让步"，**节点驱逐（Eviction）** 决定"何时、按什么顺序让步"。

本文是**串联性专题**。知识库中 [[概念/resource-management.md|资源管理]]、[[概念/pod-overhead.md|Pod Overhead]]、[[系统基础/知识字典/scheduling/koordinator.md|Koordinator]]、[[系统基础/知识字典/scheduling/resource-bin-packing.md|Resource Bin Packing]] 各自讲了 overcommit 的一个侧面，但缺少把"**QoS → 可压缩/不可压缩资源 → 节点驱逐阈值 → Bin-Packing/overcommit ratio**"这条因果链一次打通的文档。本文即做这个串联。

---

## 一、概述

### 1.1 什么是 Overcommit

Overcommit 的本质是**对"请求量（requests）"和"实际使用量（usage）"之间的鸿沟做投机**。绝大多数工作负载的 requests 是按峰值（P99）规划的，而平均使用量远低于峰值——这部分"已声明但未使用"的资源，如果不被复用，就白白浪费了。

```
┌──────────────────────────────────────────────────────────────┐
│  节点 CPU Capacity = 32 核                                    │
│                                                              │
│  不 overcommit（保守）：                                       │
│    Σ requests ≤ 32  → 最多放 N 个 Pod，实际平均只用 12 核      │
│    利用率 ~37%，节点采购成本浪费                                │
│                                                              │
│  overcommit 3x（激进）：                                       │
│    Σ requests ≤ 96（但调度上限仍是 32 核的实际吞吐）            │
│    放 3N 个 Pod，平均使用 ~28 核                               │
│    利用率 ~87%，但峰值重叠时 CPU throttle / 内存驱逐风险升高    │
└──────────────────────────────────────────────────────────────┘
```

### 1.2 核心权衡：利用率 vs 稳定性

| 维度 | 低 overcommit | 高 overcommit |
|------|---------------|---------------|
| 资源利用率 | 低（30-50%） | 高（70-90%） |
| 节点成本 | 高（需要更多节点） | 低（同样硬件放更多 Pod） |
| CPU throttle 风险 | 低 | 高（延迟毛刺） |
| 内存驱逐/OOM 风险 | 低 | 高（业务 Pod 被杀） |
| 尾延迟（P99/P999） | 稳定 | 抖动剧烈 |
| 适合负载 | 在线核心服务 | 批处理 / 可重试任务 |

**一句话**：overcommit 用"稳定性余量"换"成本"。换多少，取决于负载类型与 QoS 分级能力。

### 1.3 为什么 K8s 能"安全地"overcommit

原生 K8s 并不会主动计算"overcommit ratio"，它通过三件事把风险约束在可控范围内：

1. **可压缩 / 不可压缩资源二分法**——CPU 超限只会 throttle（变慢），内存超限会 OOM（被杀）。这让 CPU overcommit 天然比内存 overcommit 安全。
2. **QoS 三级分类**——Pod 创建时按 requests/limits 配置被分到 Guaranteed / Burstable / BestEffort，决定 OOM 与驱逐的先后顺序。
3. **kubelet 节点驱逐（Node-pressure Eviction）**——节点内存/磁盘逼近耗尽时，kubelet 主动按 QoS 顺序杀 Pod，避免整机 OOM 导致 kubelet/容器运行时自身被杀。

理解这三者的联动，是设计任何 overcommit 策略的前提。

---

## 二、资源模型基础

### 2.1 Requests vs Limits

这是 overcommit 的起点。两个字段含义完全不同，混用是新手最常见的错误。

| 字段 | 含义 | 调度阶段 | 运行时（cgroup） |
|------|------|----------|------------------|
| **requests** | 调度依据 / 最低保证 | kube-scheduler **按 requests 做 bin-packing**，累计不得超过节点 Allocatable | CPU `cpu.shares`/`cpu.weight`；内存无强制（仅记账） |
| **limits** | 运行时硬上限 | **不参与调度** | CPU `cpu.cfs_quota_us`（限流）；内存 `memory.max`（OOM） |

**关键推论**：

- `Σ requests` 决定能调度多少 Pod（密度 / overcommit ratio 的分母）。
- `Σ limits` 可以远超节点实际容量——这是合法的，因为 limits 只是"突发上限"，调度器不关心。但一旦多个 Pod 同时突到 limits，瞬时压力就来了。
- **overcommit 本质上是"放大 requests 与 limits 之间的差距，同时压低 requests"**。

```yaml
# 一个典型 Burstable Pod：requests 保守，limits 宽松
resources:
  requests:
    cpu: 250m      # 调度时只占 0.25 核 → 节点能塞更多 Pod
    memory: 256Mi
  limits:
    cpu: "2"       # 运行时最多可用 2 核（突发）
    memory: 1Gi    # 内存硬上限，超过 OOMKilled
# QoS = Burstable（requests != limits）
```

### 2.2 可压缩资源（Compressible Resources）

可压缩资源**可以被"无限分割/延迟"而不杀进程**——超限时系统变慢，但进程不死。

| 资源 | 超限行为 | 对应 cgroup 文件 |
|------|----------|------------------|
| **CPU** | CFS 限流（throttle），进程继续运行但时间片被剥夺 | `cpu.cfs_quota_us` / `cpu.max` |
| blkio IO（部分） | I/O 带宽限制，请求排队 | `io.max` |

**CPU overcommit 为什么"安全"**：假设节点 32 核，放了 `Σ requests = 80 核` 的 Pod（2.5x overcommit）。只要大家平均使用加起来 < 32 核，就互不干扰；偶尔峰值重叠，CFS 公平排队，最坏结果就是"某几个 Pod 这一秒慢了 50%"——没有数据丢失，没有进程死亡。这就是为什么生产中 CPU overcommit 2-5x 很常见。

### 2.3 不可压缩资源（Incompressible Resources）

不可压缩资源**无法被延迟或分割**——节点耗尽时只能"杀掉某个进程"来腾空间。

| 资源 | 耗尽行为 |
|------|----------|
| **Memory** | 容器超 limits → cgroup OOM Kill；节点内存不足 → kubelet 主动驱逐 |
| **ephemeral-storage** | 写满 → 磁盘压力驱逐 |
| **inode** | inode 耗尽 → 无法创建文件 → 驱逐 |
| PID | 进程数耗尽 → `pid.available` 驱逐（1.28+） |

**Memory overcommit 为什么"危险"**：内存没法"等一下"。假设节点 64Gi，放了 `Σ requests = 90Gi`（1.4x overcommit），如果多个 Pod 同时把内存涨到各自的 limits（每个 limits 通常 = 或 > requests），实际使用很容易冲过 64Gi。此时内核 OOM Killer 或 kubelet 必须立刻杀进程，否则整机崩溃。被杀的可能是你的核心业务 Pod。**所以生产中内存 overcommit ratio 几乎总是 ≤ 1.x，很多团队干脆不允许内存超卖。**

### 2.4 二者的根本差异（一张图记住）

```
                CPU（可压缩）              Memory（不可压缩）
              ┌───────────────┐          ┌───────────────┐
   超限信号 → │   throttle    │          │   OOM Kill    │
              │  （时间片减少） │          │  （进程被杀）  │
              └───────┬───────┘          └───────┬───────┘
                      │                          │
           Pod 继续运行，只是慢          Pod 死亡，需要重启
                      │                          │
        overcommit 安全上限：高          overcommit 安全上限：低（≤1.x）
           常见 2-5x，激进 10x+            常见 1.0-1.2x
```

这是整篇文档的逻辑基石：**后面所有的 QoS、驱逐、ratio 策略，本质都是在回答"当可压缩资源紧张/不可压缩资源耗尽时，牺牲谁"。**

---

## 三、QoS 三类与判定算法

QoS（Quality of Service）是 K8s 对 Pod 的**资源优先级分类**。它在 Pod 创建时由 kubelet 根据 requests/limits 静态推算，运行期不可更改。QoS 决定两件事：OOM 优先级（`oom_score_adj`）与节点驱逐顺序。

### 3.1 三类 QoS 速查

| QoS 类 | 判定条件（所有容器） | OOM 优先级 | 驱逐顺序 |
|--------|---------------------|-----------|----------|
| **Guaranteed** | 每个容器 cpu/memory 的 requests == limits（都设了） | 最低（`oom_score_adj = -997`） | **最后**被驱逐 |
| **Burstable** | 不满足 Guaranteed，且至少一个容器有 request 或 limit | 中等（动态，100~1000 之间） | 中间（按超用量排序） |
| **BestEffort** | 所有容器都**没有**设置任何 requests/limits | 最高（`oom_score_adj = 1000`） | **最先**被驱逐 |

### 3.2 精确判定算法（决策树）

以下逻辑对应 kubelet 源码 `pkg/kubelet/qos/policy.go` 的 `GetPodQOS`，是面试与排障的关键：

```
对 Pod 中【每一个容器】逐个检查：

1. 收集该容器的 cpu/memory requests 与 limits
2. 判定该容器是否"满足 Guaranteed 条件"：
     - cpu 的 request == limit（且都存在）
     - memory 的 request == limit（且都存在）
     注意：只设 limits 没设 requests 时，K8s 默认 requests = limits，
           这种也算满足（隐式相等）

然后对整个 Pod：
┌─────────────────────────────────────────────────────┐
│  IF  【所有容器】都满足 Guaranteed 条件              │
│      → return Guaranteed                            │
│                                                     │
│  ELSE IF  【所有容器】都没有任何 requests 和 limits  │
│           （含扩展资源也都没有）                      │
│      → return BestEffort                            │
│                                                     │
│  ELSE                                               │
│      → return Burstable                             │
└─────────────────────────────────────────────────────┘
```

**关键细节**（容易踩坑）：

- Guaranteed 要求 **CPU 和 Memory 都设**，且 requests==limits。只设 CPU 不设 Memory，或 CPU 设了 Memory 没设，都不算 Guaranteed。
- Guaranteed 是**逐容器**判定——Pod 内只要有一个容器不满足，整个 Pod 就降级为 Burstable（包括 pause 容器不计入，但 sidecar/init 容器计入）。
- 只设 limits 不设 requests：K8s 会**自动令 requests = limits**，此时仍可能拿到 Guaranteed。
- 扩展资源（如 `nvidia.com/gpu`）**不影响** QoS 判定，QoS 只看 cpu/memory。

### 3.3 三类示例对照

```yaml
# ✅ Guaranteed：requests == limits，CPU + Memory 都设
apiVersion: v1
kind: Pod
metadata: { name: guaranteed }
spec:
  containers:
  - name: app
    image: app:1.0
    resources:
      requests: { cpu: "1", memory: "1Gi" }
      limits:   { cpu: "1", memory: "1Gi" }   # 必须 == requests
---
# ✅ Guaranteed（隐式）：只设 limits，requests 自动等于 limits
resources:
  limits: { cpu: "1", memory: "1Gi" }
  # requests 字段省略 → K8s 默认 requests = limits → Guaranteed
---
# ⚠️ Burstable：requests < limits
resources:
  requests: { cpu: "250m", memory: "256Mi" }
  limits:   { cpu: "1",    memory: "1Gi" }
---
# ❌ Burstable（不是 Guaranteed）：CPU 相等但 Memory 没设
resources:
  requests: { cpu: "1" }
  limits:   { cpu: "1" }   # memory 缺失 → 降级为 Burstable
---
# 🔴 BestEffort：什么都没设（生产慎用，随时被驱逐）
resources: {}              # 完全省略
```

### 3.4 QoS 决定 OOM 分数与驱逐优先级

QoS 的真正威力体现在两个数字上：

| QoS | `oom_score_adj` | 含义 |
|-----|-----------------|------|
| Guaranteed | **-997** | 几乎不会被内核 OOM Killer 选中（分数越低越安全） |
| Burstable | `1000 - 1000 × memoryRequest / nodeMemory`，范围 [1, 1000) | request 越大越安全；request 越小越接近 BestEffort |
| BestEffort | **1000** | 最高优先级被杀（内核 OOM Killer 首选） |

> Linux OOM Killer 选择 `oom_score` 最高的进程杀。`oom_score_adj` 是人为偏置——K8s 用它把"业务重要性"硬编码进内核决策。Guaranteed 的 -997 让它几乎免疫节点级 OOM，而 BestEffort 的 1000 让它成为首当其冲的牺牲品。

```bash
# 🟢 低风险：只读，查看 Pod 的 QoS 类
kubectl get pod <pod-name> -o jsonpath='{.status.qosClass}'; echo
# 输出: Guaranteed / Burstable / BestEffort
```

---

## 四、Overcommit Ratio 概念

### 4.1 定义

```
overcommit_ratio = Σ(所有 Pod 的 requests) / 节点 Allocatable
```

- ratio = 1.0：保守，requests 之和刚好等于可分配量（实际很少这么设）。
- ratio > 1.0：超卖。调度器仍然允许，因为 Allocatable 是按 requests 累加判定的吗？——**不**，这里有个关键认知：**原生 K8s 调度器不允许 Σ requests 超过 Allocatable**。也就是说，单纯靠 requests 做 overcommit，ratio 永远 ≤ 1.0。

那么 overcommit > 1.0 是怎么实现的？两条路：

1. **压低 requests（把 requests 设得远低于实际平均使用）**——让 requests 不再反映真实需求，从而在 requests 层面"塞进"更多 Pod。此时 `Σ requests ≤ Allocatable`，但 `Σ 实际使用` 可以接近甚至超过 capacity。这是最常见的"overcommit"含义。
2. **超卖调度器（如 Koordinator）**：在节点上动态上报一个"超卖后的可分配量"（> 物理 Allocatable），让调度器按这个膨胀值继续调度。原生 K8s 不支持，需要扩展。

### 4.2 Allocatable 的计算（重要细节）

```
Allocatable = Capacity
            - system-reserved      （给 OS 进程）
            - kube-reserved        （给 kubelet/容器运行时等 K8s 组件）
            - eviction-hard        （硬驱逐阈值，也算预留！）
```

**易错点**：`eviction-hard` 的阈值（如 `memory.available < 100Mi`）会从 Capacity 中**减去**，作为不可用的预留。所以即便你不设 system-reserved/kube-reserved，只要设了 eviction-hard，Allocatable 就会变小。这直接影响能调度多少 Pod。

```bash
# 🟢 低风险：查看节点 Capacity vs Allocatable
kubectl describe node <node-name> | grep -A6 "Capacity:"
# Capacity:     cpu 32, memory 128Gi
# Allocatable:  cpu 31950m, memory 122Gi   ← 差额是各项 reserved
```

### 4.3 常见 ratio 建议

| 资源 | 建议 overcommit ratio | 说明 |
|------|----------------------|------|
| CPU | **2x - 5x** | 安全（可压缩）；延迟敏感服务降到 1-1.5x |
| Memory | **≤ 1.1x** | 保守；很多团队坚持 1.0x（不允许超卖） |
| ephemeral-storage | ≤ 1.0x | 磁盘满会触发驱逐，不建议超卖 |
| GPU | **1.0x**（或 GPU Sharing 多路复用） | 不可压缩且昂贵，通常独占 |

> 这里的"ratio"是工程经验值，指 `Σ requests / Allocatable`。CPU 高 ratio 安全的原因是 requests 普遍远小于 limits，且平均使用 << requests；内存则相反，使用量容易逼近 limits。

---

## 五、可压缩资源 Overcommit（CPU）

### 5.1 CFS Quota 限流机制

Linux CFS（Completely Fair Scheduler）通过两个 cgroup 参数控制 CPU 上限：

- `cpu.cfs_period_us`：调度周期，默认 **100000us（100ms）**。
- `cpu.cfs_quota_us`：周期内可用 CPU 时间。`limits.cpu = 1` → `quota = 100000`（每个周期 100ms，恰好 1 核）。

```
limits.cpu = 2      →  cpu.cfs_quota_us = 200000  (每 100ms 可用 200ms = 2 核)
limits.cpu = 500m   →  cpu.cfs_quota_us = 50000   (每 100ms 可用 50ms = 0.5 核)
limits.cpu = 250m   →  cpu.cfs_quota_us = 25000   (每 100ms 可用 25ms)
```

当容器在一个周期内用完 quota，CFS 把它"冻结"到下个周期——这就是 **throttle**。

```
周期 = 100ms，quota = 50ms（500m）

时间轴：
|←── 周期1 ──→|←── 周期2 ──→|←── 周期3 ──→|
| 运行 50ms   | | 运行 50ms   | | 运行 50ms   | |
|▓▓▓▓▓░░░░░░░░|▓▓▓▓▓░░░░░░░░|▓▓▓▓▓░░░░░░░░|
       ↑throttle     ↑throttle      ↑throttle
       (剩 50ms 被冻结，等待下个周期)
```

### 5.2 CPU Throttle 的危害

throttle 表现为**周期性的延迟毛刺**：进程"能跑"但被强行暂停，对延迟敏感服务（API 网关、数据库连接池、实时推理）极其致命。典型现象：

- P99/P999 延迟周期性飙升，但 CPU 使用率（平均）看起来不高。
- GC（如 JVM Full GC）本应在 200ms 完成，因 throttle 拖到 2s。
- 健康检查间歇性超时 → Pod 被标记不健康 → 被删除重建 → 雪上加霜。

### 5.3 关键监控指标

| Prometheus 指标 | 含义 |
|----------------|------|
| `container_cpu_cfs_periods_total` | 累计调度周期数 |
| `container_cpu_cfs_throttled_periods_total` | **被 throttle 的周期数** |
| `container_cpu_cfs_throttled_seconds_total` | 累计被冻结的秒数 |

推荐告警：`throttled_periods_total / periods_total > 0.1`（超过 10% 的周期被限流）即报警。

```bash
# 🟢 低风险：查看某容器是否被 CPU 限流（节点上执行，需访问 cgroup）
cat /sys/fs/cgroup/cpu/kubepods/burstable/<poduid>/cpu.stat
# 输出包含:
# nr_periods 12345
# nr_throttled 678        ← 非 0 说明发生过限流
# throttled_time 987654321
```

### 5.4 为什么 CPU Overcommit "安全但要注意延迟"

| 情形 | 结果 |
|------|------|
| `Σ 实际使用 << CPU capacity` | 完全无感，overcommit 收益最大 |
| 峰值重叠，瞬时 `Σ 使用 > capacity` | CFS 公平排队，所有 Pod 等比例变慢，无人死亡 |
| 某 Pod 设了很低的 `limits.cpu` | 该 Pod **自己**被 throttle（与其他 Pod 无关） |

**实践要点**：对延迟敏感服务，常见做法是**只设 requests 不设 limits**——让它在节点空闲时尽情使用 CPU，避免 limits 带来的自我限流。代价是失去对"邻居"的保护（你成了潜在的"吵闹邻居"），所以必须配合 Guaranteed + CPU Manager static（独占核）使用。

---

## 六、不可压缩资源 Overcommit（Memory）

### 6.1 内存超卖 → 节点 MemoryPressure

当 `Σ Pod 内存使用` 逼近节点物理上限时，kubelet 把节点 condition 设为 `MemoryPressure=True`，并触发驱逐。这与 CPU 的 throttle 完全不同——**内存没法"等"，必须立刻有人死**。

```
节点 Memory = 64Gi
  Allocatable = 60Gi（reserved 4Gi）

情况 A（未超卖）：Σ requests = 58Gi，Σ limits = 58Gi
  → 每个 Pod 最多用 58Gi 中的自己那份，总量受控
  → 安全

情况 B（超卖）：Σ requests = 58Gi，但每个 Pod limits = 2Gi，共 50 个 Pod
  → requests 之和 58Gi ≤ Allocatable ✓（能调度）
  → 但 Σ limits = 100Gi >> 64Gi
  → 若多个 Pod 同时涨到 limits，实际使用冲过 64Gi
  → kubelet eviction / 内核 OOM → 杀 Pod
```

### 6.2 OOM Kill 机制（容器级）

当容器内存超过 `limits.memory`，cgroup 内存子系统直接 OOM Kill 该容器进程（不是 Pod 内的其他容器）。容器被重启（受 `restartPolicy` 控制）。这是**容器自己的事**，与节点无关。

### 6.3 节点级 OOM / 驱逐（危险区）

当**节点整体**内存不足（不是单个容器超 limit），内核 OOM Killer 会扫描所有进程，按 `oom_score`（受 `oom_score_adj` 偏置）选择牺牲品。K8s 优先牺牲 BestEffort Pod。但**这通常是最后手段**——更早一步，kubelet 会主动驱逐（见第七节），避免整机 OOM 把 kubelet 自己也搞死。

### 6.4 working_set vs usage（关键认知）

OOM 判断用的不是"已分配内存"，而是 **working set**：

```
working_set_bytes ≈ memory.usage_in_bytes - total_inactive_file
```

- `usage_in_bytes`：cgroup 总内存（含已回收的 page cache）。
- `total_inactive_file`：可立即回收的文件缓存（读过的文件页）。
- `working_set`：真正"活跃、不易回收"的内存——**OOM 和驱逐看的就是这个**。

> 这就是为什么 `kubectl top pod` 显示内存 900Mi，但 Pod 被 OOMKilled 时 limits 是 1Gi——因为某个瞬间 working_set（去掉可回收 cache）冲过了 1Gi。监控内存压力，**永远看 `container_memory_working_set_bytes`，而不是 `container_memory_usage_bytes`**。

```bash
# 🟢 低风险：查看 Pod 实际 working set（需 metrics-server）
kubectl top pod <pod-name> --containers
# NAME           CPU    MEMORY(working set)
# app           120m   920Mi   ← 这个值逼近 limits.memory 就危险
```

### 6.5 内存 Overcommit 的真实风险

- **业务 Pod 被杀**：核心服务可能因为邻居暴涨而连带被驱逐（尤其 Burstable）。
- **雪崩（thundering herd）**：一批 Pod 同时被驱逐 → 重新调度到其他节点 → 其他节点也压力大 → 连锁驱逐。
- **不可预测性**：CPU throttle 是"慢一点"（可观测、可接受），内存 OOM 是"突然死"（业务中断、连接断开、数据不一致风险）。

---

## 七、与节点驱逐阈值的联动

这是整篇文档的串联核心：**overcommit 越激进，驱逐越频繁；QoS 决定驱逐时牺牲谁。**

### 7.1 kubelet 驱逐信号（Eviction Signals）

kubelet 周期性采样以下信号，与阈值比较：

| Signal | 描述 | 默认硬阈值 |
|--------|------|-----------|
| `memory.available` | 节点可用内存 | `< 100Mi` |
| `nodefs.available` | 根文件系统可用空间 | `< 10%` |
| `nodefs.inodesFree` | 根文件系统可用 inode | `< 5%` |
| `imagefs.available` | 镜像存储可用空间 | `< 15%` |
| `imagefs.inodesFree` | 镜像存储可用 inode | `< 5%` |
| `pid.available` | 可用进程数（1.28+） | `< 1000` |

更完整的信号列表、计算方式与配置示例见 [[集群基础/控制平面/33-kubelet-eviction-thresholds.md|kubelet 驱逐阈值]]。

### 7.2 Soft Eviction vs Hard Eviction

| 维度 | Hard Eviction（硬驱逐） | Soft Eviction（软驱逐） |
|------|------------------------|------------------------|
| 配置 | `evictionHard` | `evictionSoft` + `evictionSoftGracePeriod` |
| 宽限期 | **无**，立即驱逐 | 有 grace period（如 1m），给 Pod 优雅退出时间 |
| 触发 | 信号一过阈值立刻杀 | 信号持续超过阈值且过 grace period 才杀 |
| 适用 | 守底线（防整机崩溃） | 生产提前清理，避免硬驱逐 |
| 风险 | 可能中断业务 | 较低 |

```yaml
# /var/lib/kubelet/config.yaml 生产推荐
evictionHard:
  memory.available: "500Mi"      # 比 100Mi 更保守，留余量
  nodefs.available: "10%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "1Gi"        # 提前预警
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m"
  nodefs.available: "30s"
evictionPressureTransitionPeriod: "30s"
```

### 7.3 驱逐顺序（QoS 决定）

kubelet 按以下顺序挑选被驱逐的 Pod：

```
1. BestEffort Pod —— 全部驱逐
       ↓（还不够）
2. Burstable Pod 中，实际使用 > requests 的，按"超出比例"从大到小排
       ↓（还不够）
3. Guaranteed Pod —— 最后手段，同 QoS 内按使用量排
```

**Burstable 的排序逻辑很关键**：驱逐不是"谁用得多谁死"，而是"**谁超出自己的 request 最多谁死**"。`rank = (usage - request) / request`。一个 request=256Mi 但用了 2Gi 的 Burstable Pod，比一个 request=2Gi 用了 3Gi 的 Pod 更先被驱逐（前者超出 7x，后者 1.5x）。

```
┌─────────────────────────────────────────────────────────┐
│  节点 memory.available < 100Mi（触发 hard eviction）     │
├─────────────────────────────────────────────────────────┤
│  1) 先杀所有 BestEffort Pod                              │
│       be-1, be-2, be-3 → 释放 X Mi                       │
│                                                         │
│  2) 还不够？遍历 Burstable，按 (usage-request)/request 排 │
│       burst-A: req 256Mi, used 2Gi  → 超 7x  ← 先杀      │
│       burst-B: req 1Gi,  used 1.5Gi → 超 0.5x            │
│                                                         │
│  3) 仍不够？杀 Guaranteed（极少到这步，通常意味着节点爆了）│
└─────────────────────────────────────────────────────────┘
```

### 7.4 Overcommit 与驱逐频率的正相关

```
overcommit ratio ↑  →  节点资源利用率 ↑  →  触及 eviction 阈值的概率 ↑
                                          →  驱逐事件频率 ↑
                                          →  业务抖动 ↑
```

**实践含义**：监控 `kubelet_evicted_*`（如 `kubelet_pod_evict_total`）指标。如果一个节点频繁驱逐，几乎可以断定 overcommit 过激——要么调高 requests（让调度少放 Pod），要么扩容节点，要么引入超卖调度器精细化分配。

### 7.5 与 system-reserved / kube-reserved 的关系

这两项 reserved **直接缩小 Allocatable**，相当于"主动降低 overcommit 上限"：

```
Capacity = 64Gi
- system-reserved (OS):       2Gi
- kube-reserved (K8s 组件):    1Gi
- eviction-hard (memory):    0.5Gi
= Allocatable = 60.5Gi

→ 调度器最多放 Σ requests = 60.5Gi 的 Pod
→ 留出 3.5Gi 余量给 OS/K8s 组件，降低驱逐概率
```

reserved 越大 → Allocatable 越小 → 能超卖的越少 → 越安全。这是 overcommit 与稳定性的"旋钮"。建议按节点规格阶梯配置（见 [[集群基础/控制平面/33-kubelet-eviction-thresholds.md|kubelet 驱逐阈值]] 中的建议表）。

---

## 八、Bin-Packing 与调度碎片

### 8.1 调度器按 requests 做装箱

kube-scheduler 的 `NodeResourcesFit` 插件在 Score 阶段有三种策略（详见 [[系统基础/知识字典/scheduling/resource-bin-packing.md|Resource Bin Packing]]）：

| 策略 | 行为 | 与 overcommit 的关系 |
|------|------|---------------------|
| `LeastAllocated`（默认） | 优先选最空闲的节点 | 反装箱，碎片多，密度低 |
| `MostAllocated` | 优先选最满的节点 | **装箱**，碎片少，密度高，overcommit 倾向集中 |
| `RequestedToCapacityRatio` | 自定义利用率-分数曲线 | 精细控制装箱力度 |

### 8.2 Overcommit 让装箱更"满"

把 requests 压低（overcommit 的常见做法）后，同样大小的节点能塞更多 Pod，`MostAllocated` 评分让调度器优先填满节点而非摊平——结果是：

- 节点数量减少（成本下降）。
- 空闲节点更容易被 Cluster Autoscaler / Karpenter 回收。
- **但**每个节点的瞬时压力更大（多个 Pod 峰值重叠概率升高）。

### 8.3 装箱的风险：limits 累加远超实际

装箱只看 requests，但运行时 Pod 可以用到 limits。一个节点上 30 个 Burstable Pod，`Σ requests = 8 核`（装箱通过），`Σ limits = 60 核`（远超 32 核 capacity）。一旦业务高峰，实际使用可能冲到 40 核——CPU throttle 全面爆发，内存可能触及驱逐。**装箱省的钱，可能被 throttle 导致的业务降级（如超时重试、SLA 罚款）赔回去。**

### 8.4 碎片（Fragmentation）

装箱的反面是碎片：节点上 CPU 用满但内存还剩很多（或反之），导致新 Pod 调度不上去，明明总资源够却不得不扩容节点。`MostAllocated` 能缓解碎片，但不能根治。重调度（Descheduler）和混部（Koordinator）是更彻底的解法（见第九节）。

---

## 九、超卖调度器与重调度（生态）

原生 K8s 的 overcommit 是"被动"的——它不会主动计算使用率来动态调整调度上限。生态项目填补了这个空白。

### 9.1 Koordinator：基于真实负载的混部与超卖

[[系统基础/知识字典/scheduling/koordinator.md|Koordinator]]（阿里开源，CNCF Sandbox）核心能力：

- **动态资源超卖（Dynamic Overcommitment）**：根据节点真实使用率，动态上报一个"超卖后的可分配量"（如把空闲的 30% CPU 上报为可调度），让调度器按膨胀值继续放 Pod。
- **在线/离线混部（Colocation）**：把延迟敏感的在线服务与可压缩的离线批处理放同一节点，离线任务"填"在线服务的资源谷，离线被严格隔离（CPU 抢占、内存优先级）。
- **CPU Burst / CFS Burst**：允许 Pod 短时突破 limits，缓解 throttle。
- **干扰感知（Descheduling by interference）**：检测到在线服务被离线干扰时，主动迁移离线任务。

> Koordinator 的价值在于把 overcommit 从"盲目压低 requests"升级为"按真实负载动态分配"——既提升利用率，又通过 QoS 隔离保住在线服务的稳定性。

### 9.2 Descheduler：按使用率重排

Descheduler 周期性扫描集群，驱逐"低效"Pod 让它们重新调度，缓解装箱造成的失衡：

- `LowNodeUtilization`：把高负载节点的 Pod 迁到低负载节点，均衡压力。
- `HighNodeUtilization`：把分散的 Pod 集中，腾出空节点供回收。
- `RemovePodsViolatingTopologySpreadConstraint`：修复拓扑分布违规。

与 overcommit 的关系：overcommit 让装箱激进，Descheduler 负责"事后纠偏"——两者配合能在高密度的同时维持可用性。

### 9.3 原生 K8s 的局限 vs 扩展方案

| 能力 | 原生 K8s | Koordinator / Descheduler |
|------|---------|---------------------------|
| 调度上限依据 | 静态 requests | 动态真实负载 |
| 混部（在线+离线） | 无原生支持 | Koordinator 一等公民 |
| CPU 突发 | 仅靠 limits（会 throttle） | CPU Burst 临时突破 |
| 使用率重平衡 | 无 | Descheduler |
| 干扰检测 | 无 | Koordinator interference |

---

## 十、生产实践

### 10.1 按 QoS 分级策略表

| 负载类型 | 推荐 QoS | requests 设置 | limits 设置 | overcommit 倾向 |
|---------|----------|--------------|-------------|----------------|
| 在线核心服务（API、网关） | **Guaranteed** | requests = limits（按 P99） | = requests | **1.0-1.5x**（保守） |
| 数据库 / 状态服务 | **Guaranteed** | requests = limits（充足） | = requests | **1.0x**（不超卖） |
| 一般微服务 | Burstable | requests 按 P95，limits 按 P99 | > requests | **2-3x** |
| 批处理 / CI | Burstable / BestEffort | requests 低 | 宽松或不设 | **3-5x** |
| 可重试 Job（Spark、AI 训练） | BestEffort | 不设 | 不设 | **激进超卖** |

### 10.2 延迟敏感服务：Guaranteed + CPU Manager Static

对 P99 敏感的服务，最佳实践：

1. QoS 设为 **Guaranteed**（requests == limits，整数核如 `cpu: 4`）。
2. 节点启用 **CPU Manager static 策略**，让 Pod **独占 CPU 核**（不再被其他 Pod 抢占，无 context switch 开销）。
3. 配合 `system-reserved` 给系统留核，避免 OS 抢占业务核。

```yaml
# kubelet 配置启用 CPU Manager static
# /var/lib/kubelet/config.yaml
cpuManagerPolicy: static          # 默认 none
reservedSystemCPUs: "0,1"         # 留 2 核给系统，独占核从 CPU 2 开始
systemReserved:
  cpu: "500m"
  memory: "1Gi"
```

独占核的 Pod 拿到的 CPU 是**物理隔离**的，彻底消除 throttle 与邻居干扰，是 overcommit 场景下保住 SLA 的最硬手段。

### 10.3 批处理：BestEffort / Burstable overcommit

可重试、对延迟不敏感的批处理任务，可以激进 overcommit：

- 不设 requests/limits（BestEffort），或设很低 requests（Burstable）。
- 配合 `priorityClass` 设低优先级，资源紧张时优先被驱逐（符合预期）。
- 用 Koordinator 等混部方案，"借"在线服务的资源谷。

### 10.4 内存 Overcommit 红线

- **内存 overcommit 不超过 1.1x**（即 `Σ requests ≤ 1.1 × Allocatable` 由调度器层面的 requests 控制；实际靠压低 requests 实现）。
- 始终设 `limits.memory`，避免单容器无限涨内存拖垮节点。
- 给 JVM/Go 等运行时留足 off-heap 余量（Metaspace、direct buffer、goroutine 栈）。
- 监控 `container_memory_working_set_bytes`，告警阈值设在 `limits.memory × 0.8`。

### 10.5 必备监控项

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| `container_cpu_cfs_throttled_periods_total / container_cpu_cfs_periods_total` | CPU 限流比例 | > 10% |
| `container_memory_working_set_bytes / container_memory_limit_bytes` | 内存使用率 | > 80% |
| `kubelet_evicted_*` / `kubelet_pod_evict_total` | 驱逐事件 | 任何非零告警 |
| `kube_node_status_condition{condition="MemoryPressure"}` | 节点内存压力 | status=true |
| `node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes` | 节点可用内存比 | < 10% critical |

### 10.6 运维命令速查

```bash
# 🟢 低风险：查看节点 Allocatable 与 requests/limits 累加（最常用的 overcommit 体检）
kubectl describe node <node-name> | grep -A20 "Allocated resources"
# 输出示例:
#   Allocated resources:
#     (Total limits may be over 100 percent, i.e., overcommitted.)
#     Resource           Requests     Limits
#     --------           --------     ------
#     cpu                25650m (80%) 64000m (200%)   ← limits 200% = 2x overcommit
#     memory             90Gi (70%)   180Gi (140%)    ← 内存 limits 1.4x，偏高
```

```bash
# 🟢 低风险：查看所有 Pod 的 QoS 类
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.qosClass}{"\n"}{end}'
```

```bash
# 🟢 低风险：统计某节点上 Pod 的 QoS 分布与 requests 累加
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> \
  -o jsonpath='{range .items[*]}{.status.qosClass}{"\n"}{end}' | sort | uniq -c
```

```bash
# 🟢 低风险：查看 Pod 被驱逐的原因（Evicted 状态）
kubectl get pod <pod-name> -o jsonpath='{.status.reason}{"\t"}{.status.message}'; echo
# 典型输出: Evicted  The node was low on resource: memory.
```

```bash
# 🟢 低风险：查看节点是否处于压力状态
kubectl describe node <node-name> | grep -A8 Conditions
# MemoryPressure / DiskPressure / PIDPressure / Ready
```

```bash
# 🟡 中风险：调整某 Deployment 的 requests/limits（会触发滚动更新）
kubectl set resources deployment/<name> \
  --requests=cpu=500m,memory=512Mi \
  --limits=cpu=2,memory=1Gi
```

```bash
# 🔴 高风险：手动驱逐节点上所有 Pod（drain，会中断业务，慎用）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
# 完成后恢复调度:
kubectl uncordon <node-name>
```

### 10.7 常见误区

| 误区 | 正确理解 |
|------|----------|
| requests 是实际使用量 | requests 是调度依据与最低保证，与实际使用无关 |
| 内存可以像 CPU 一样 overcommit | 内存不可压缩，超卖只会带来 OOM 与驱逐 |
| limits 越大越安全 | limits 累加远超 capacity 时，瞬时压力全靠 throttle/OOM 兜底 |
| 设了 limits 就不会被驱逐 | limits 只防容器级 OOM；节点级驱逐看整体使用，Guaranteed 也可能被牺牲 |
| QoS 可以运行时改 | QoS 在 Pod 创建时静态确定，改 requests/limits 需重建 Pod |
| CPU throttle 无害 | 对延迟敏感服务，throttle 是 P99 抖动的头号杀手 |
| 不设 limits 更安全 | 不设 limits.cpu 可减少自我限流，但不设 limits.memory 等于让容器无限涨内存，极其危险 |

---

## 十一、决策流程图：资源紧张时谁先死

把全文逻辑浓缩成一张决策图，排障与容量规划时对照：

```
            节点资源紧张
                 │
        ┌────────┴────────┐
        ▼                 ▼
   CPU 紧张            内存/磁盘紧张
   (可压缩)            (不可压缩)
        │                 │
        ▼                 ▼
   CFS throttle      kubelet 检测
   (按 shares 公平)   eviction signal
   无人死亡                │
                     ┌─────┴─────┐
                     ▼           ▼
               soft 阈值     hard 阈值
               (grace period) (立即)
                     │           │
                     └─────┬─────┘
                           ▼
                  按 QoS 选牺牲品:
                  1) BestEffort 全部
                  2) Burstable 按 (usage-request)/request 降序
                  3) Guaranteed 最后
                           │
                           ▼
                   cgroup OOM Kill / Pod 驱逐
```

---

## 十二、面试要点

1. **Q: 为什么 CPU 可以激进 overcommit 而内存不行？**
   A: CPU 是可压缩资源，超限时 CFS 限流（throttle），进程继续运行只是变慢，无数据丢失；内存是不可压缩资源，节点耗尽时必须杀进程释放空间（OOM Kill 或 kubelet 驱逐），导致业务中断。因此 CPU overcommit 2-5x 常见，内存通常 ≤1.1x。

2. **Q: QoS 三类如何精确判定？**
   A: Guaranteed 要求所有容器的 cpu 和 memory 都设 requests==limits（含隐式相等）；BestEffort 要求所有容器不设任何 requests/limits；其余全是 Burstable。QoS 在 Pod 创建时静态确定，运行期不可改。

3. **Q: 节点驱逐时，Burstable Pod 之间的顺序如何决定？**
   A: 按"超出 request 的比例"降序：`rank = (usage - request) / request`。超出比例越大越先被驱逐，而非"用得最多"的先死。

4. **Q: OOM 时看 usage 还是 working_set？**
   A: 看 working_set_bytes（≈ usage - inactive_file）。working_set 反映"活跃、不易回收"的内存，OOM 与驱逐都基于它。监控内存压力必须用 working_set，不能看 usage。

5. **Q: 如何在 overcommit 场景下保住延迟敏感服务的 SLA？**
   A: 三板斧：① QoS 设为 Guaranteed（requests==limits）；② 节点启用 CPU Manager static 策略独占 CPU 核，消除邻居干扰与 throttle；③ 配合 system-reserved 给系统留核，并监控 container_cpu_cfs_throttled_periods_total。

---

## 相关文档

- [[概念/resource-management.md|资源管理]] — requests/limits/QoS 的基础概念
- [[概念/pod-overhead.md|Pod Overhead]] — 运行时额外开销如何计入 overcommit 计算
- [[系统基础/知识字典/scheduling/qos.md|QoS]] — QoS 三类速查
- [[集群基础/控制平面/33-kubelet-eviction-thresholds.md|kubelet 驱逐阈值]] — 驱逐信号、soft/hard、预留配置完整手册
- [[系统基础/知识字典/scheduling/koordinator.md|Koordinator]] — 基于真实负载的混部与动态超卖
- [[系统基础/知识字典/scheduling/resource-bin-packing.md|Resource Bin Packing]] — MostAllocated / RequestedToCapacityRatio 装箱策略
- [[集群基础/设计原则/01-design-principles-foundations.md|设计原则基础]] — K8s 设计哲学总纲

<!-- risk-assessed -->

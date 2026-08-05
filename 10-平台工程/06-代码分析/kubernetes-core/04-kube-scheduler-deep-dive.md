---
title: kube-scheduler 源码深度剖析
description: 基于 kubernetes-1.36.2 源码的调度器双循环、三级队列、调度框架扩展点、Assume 乐观绑定与抢占机制完整剖析
summary: 剖析 ScheduleOne 调度主循环、activeQ/backoffQ/unschedulablePods 三级队列、Filter/Score 扩展点执行链、numFeasibleNodesToFind 采样优化与 Preemption 抢占评估，全部函数附实测行号。
category: source-analysis
tags:
- k8s
- source-code
- scheduler
- scheduling-framework
- preemption
- queue
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- kube-scheduler 调度流程源码分析
- 调度框架 Filter Score 扩展点执行顺序
- Pod Pending 调度队列 backoff 机制
- 调度器抢占 Preemption 源码
trigger_keywords:
- kube-scheduler
- ScheduleOne
- schedulingCycle
- activeQ
- backoffQ
- PostFilter
- preemption
- assume
related_domains:
- 集群基础
- 工作负载
- AI基础设施
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# kube-scheduler 源码深度剖析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/`
> 概念层配套阅读：[[01-集群基础/03-控制平面/20-kube-scheduler-deep-dive.md|控制平面：Scheduler Deep Dive]]

## 概述

调度器只做一件事：为 `spec.nodeName` 为空的 Pod 选一个节点，然后发一个 Binding。源码围绕三个支柱组织：

1. **调度队列**（`pkg/scheduler/backend/queue/`）：activeQ / backoffQ / unschedulablePods 三级结构
2. **调度框架**（`pkg/scheduler/framework/`）：PreFilter→Filter→Score→Bind 等十余个扩展点，所有内置策略都是插件
3. **双循环**：调度循环（串行、逐 Pod）与绑定循环（并行、异步）通过 Assume 乐观机制解耦

---

## 一、调度主循环 ScheduleOne

```go
// pkg/scheduler/schedule_one.go:67
func (sched *Scheduler) ScheduleOne(ctx context.Context) {
    podInfo, err := sched.NextPod(logger)          // 从 activeQ 弹出（阻塞）
    fwk, err := sched.frameworkForPod(pod)         // 按 schedulerName 选 Profile
    // ── 调度周期（串行）──
    scheduleResult, assumedPodInfo, status := sched.schedulingCycle(...)  // :175
    // ── 绑定周期（异步 goroutine，不阻塞下一个 Pod 的调度）──
    go func() {
        status := sched.bindingCycle(...)          // :397
    }()
}
```

### 1.1 schedulingCycle 内部

```go
// schedule_one.go:175 起（关键步骤）
scheduleResult, err = sched.SchedulePod(ctx, fwk, state, pod)
    ├── findNodesThatFitPod    // :628  PreFilter + Filter（并行 16 goroutine 逐节点）
    │      └── numFeasibleNodesToFind  // :864  大集群采样：只找够 N 个可行节点即停
    ├── prioritizeNodes        // PreScore + Score（并行打分 + 权重归一）
    └── selectHost             // 最高分中随机挑一（打散热点）
// 失败 → RunPostFilterPlugins → 抢占（见第四节）
// 成功 → sched.assume(...)    // :1108  先在本地缓存假定 Pod 已在节点上
```

**采样优化**（`numFeasibleNodesToFind`, :864）：节点数 >100 时按 `percentageOfNodesToScore`（自适应默认：`50 - numNodes/125`，下限 5%）只评估部分节点。这解释了大集群中「明明有更优节点却没被选中」——那个节点可能根本没进入本轮评估窗口。

### 1.2 Assume 与双循环解耦

`assume`（:1108）把 Pod 写入 scheduler cache 并标记 assumed，**调度循环立即继续处理下一个 Pod**，绑定（API 调用，慢）由绑定循环异步完成：

```
调度循环(串行): Pod-A assume ─→ Pod-B assume ─→ Pod-C ...
                    │               │
绑定循环(并行):     └ bind A (API)  └ bind B (API)
绑定失败 → ForgetPod（回滚缓存）→ Pod 重回队列
```

后续 Pod 的 Filter 基于「含 assumed Pod 的缓存视图」计算——保证资源不被双重分配，同时调度吞吐不受 API 延迟拖累。

---

## 二、三级调度队列

```
pkg/scheduler/backend/queue/（实测目录）
├── scheduling_queue.go     # PriorityQueue 门面
├── active_queue.go         # activeQ: 堆结构，QueueSort 插件定序（默认优先级降序）
├── backoff_queue.go        # backoffQ: 按 backoff 到期时间的堆（初始 1s，×2 递增，上限 10s）
├── unschedulable_pods.go   # 不可调度 Pod 集合（map）
└── nominator.go            # 抢占提名（nominatedNodeName 的内存视图）
```

Pod 状态迁移：

```
新 Pod ──→ activeQ ──ScheduleOne 失败──→ unschedulablePods
              ▲                              │
              │                    相关集群事件(QueueingHint 判定可能变可调度)
              │                              ▼
              └────backoff 到期──── backoffQ ┘
```

关键机制：

- **QueueingHint**：Pod 因「节点资源不足」失败后，只有「新节点加入/某 Pod 删除」等相关事件才会把它捞回 backoffQ，避免无效重试空转 CPU
- **Pending 排障顺序**：先看 Pod event 里最后一次失败的插件与原因（`Insufficient cpu`、`node(s) had untolerated taint`），再判断它卡在 unschedulablePods（等事件）还是 backoffQ（等退避到期）；指标 `scheduler_pending_pods{queue=...}` 直接给出三个队列的水位

---

## 三、调度框架：扩展点与插件

### 3.1 扩展点执行链（framework/runtime/framework.go，实测行号）

| 扩展点 | 入口 | 语义 |
|--------|------|------|
| PreFilter | `RunPreFilterPlugins` :922 | 预计算（如 Pod 资源请求汇总），可直接判不可调度 |
| Filter | `RunFilterPlugins` :1093 | 逐节点硬性淘汰（谓词） |
| PostFilter | 失败后触发 | 抢占入口（DefaultPreemption） |
| PreScore/Score | `RunScorePlugins` :1339 | 打分（0-100）× 插件权重 |
| Reserve/Unreserve | bindingCycle 内 | 资源预留与回滚（Volume 绑定用） |
| Permit | bindingCycle 内 | 可 Wait（gang scheduling 的挂点） |
| PreBind/Bind/PostBind | bindingCycle 内 | 默认 Bind 即 POST pods/binding 子资源 |

### 3.2 内置插件一览（pkg/scheduler/framework/plugins/，实测目录）

`noderesources`（资源匹配与打分策略 LeastAllocated/MostAllocated）、`interpodaffinity`、`podtopologyspread`、`tainttoleration`、`nodeaffinity`、`nodevolumelimits`、`imagelocality`、`schedulinggates`、`dynamicresources`（DRA，GPU 等设备调度）、`gangscheduling` 等。

两个值得注意的方向：

- **dynamicresources**：DRA 调度落点，AI 场景 GPU/NPU 拓扑调度的上游实现，关联 [[15-AI基础设施/README.md|AI基础设施域]] 与 [[01-集群基础/03-控制平面/32-dynamic-resource-allocation.md|DRA 深度解析]]
- **gangscheduling**：上游原生 gang 调度（此前由 Volcano 等外部调度器承担），基于 Permit 扩展点的 Wait 语义实现「凑齐才放行」

自定义调度行为的正确姿势是 KubeSchedulerConfiguration 的 Profile 覆盖插件启停与权重，而非 fork 调度器。

---

## 四、抢占（Preemption）

Filter 全军覆没后，PostFilter 触发抢占评估：

```go
// framework/plugins/defaultpreemption/default_preemption.go:135
func (pl *DefaultPreemption) PostFilter(...) (*fwk.PostFilterResult, *fwk.Status)
    └── // framework/preemption/preemption.go:103
        func (ev *Evaluator) Preempt(...) {
            // 1. 候选节点：驱逐若干低优先级 Pod 后能容纳抢占者的节点
            // 2. 择优：违反 PDB 最少 > 被逐 Pod 最高优先级最低 > 数量最少 ...
            // 3. 执行：API 删除牺牲者，向抢占者写 status.nominatedNodeName
        }
```

要点：

- 抢占者**不直接调度**到目标节点，只是获得提名（nominatedNodeName），等牺牲者优雅退出后重新走正常调度——期间若出现更优节点，提名可被推翻
- 抢占尽量尊重 PDB 但**不保证**（PDB 是首选排序条件而非硬约束），这是「配了 PDB 仍被抢占」的源码依据
- 关联 [[12-可靠性/README.md|可靠性域]] 的容量规划与优先级体系设计

---

## 五、生产排障速查

| 症状 | 源码定位 | 检查手段 |
|------|---------|---------|
| Pod 长期 Pending | Filter 失败聚合 (Diagnosis, schedule_one.go:628) | `kubectl describe pod` 看各插件淘汰计数 |
| Pending 且无 event 更新 | unschedulablePods 等待相关事件 | `scheduler_pending_pods` 指标分队列观察 |
| 调度慢、吞吐低 | 打分节点过多 / 插件耗时 | `scheduler_framework_extension_point_duration_seconds` |
| 大集群调度结果"不最优" | 采样 (numFeasibleNodesToFind :864) | 调 `percentageOfNodesToScore`（牺牲吞吐换精度） |
| 低优 Pod 频繁被杀 | Preempt 评估 (preemption.go:103) | 审视 PriorityClass 体系与 PDB |
| 绑定失败循环 | bindingCycle (:397) → ForgetPod | 看 apiserver 端 binding 请求错误（配额/节点删除） |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/01-source-tree-architecture.md|01 - 源码整体架构与目录结构]]
- [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 - 声明式 API 与 Informer 机制源码剖析]]（调度器如何感知 Pod/Node 变化）
- [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 - 组件交互关系与数据流向]]
- [[10-平台工程/06-代码分析/cluster-create/23-scheduler.md|代码分析：kube-scheduler 调度详解]]（kubeadm 部署视角）
- [[01-集群基础/03-控制平面/20-kube-scheduler-deep-dive.md|控制平面：Scheduler Deep Dive]]
- [[15-AI基础设施/README.md|AI基础设施域]]（GPU/批调度延伸）
